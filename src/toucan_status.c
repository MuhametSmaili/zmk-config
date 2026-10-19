// SPDX-License-Identifier: MIT
#define DT_DRV_COMPAT zmk_behavior_toucan_status

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <drivers/behavior.h>
#include <zmk/battery.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/toucan_status.h>
#include <zmk/split/bluetooth/service.h>

BUILD_ASSERT(sizeof(((struct zmk_split_run_behavior_data *)0)->position) == 1,
             "Check Toucan status frame ID if the ZMK BLE position field changes");
BUILD_ASSERT(TOUCAN_STATUS_STATE_TAG <= UINT8_MAX,
             "Toucan status frame ID must survive the 8-bit BLE wire format");
BUILD_ASSERT(sizeof(DEVICE_DT_NAME(DT_DRV_INST(0))) <= ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN,
             "Toucan status behavior name must fit the BLE device-name field");

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || IS_ENABLED(CONFIG_ZMK_TOUCAN_SCREEN)
static struct k_spinlock status_lock;
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/endpoints.h>
#include <zmk/split/central.h>
#include <zmk/split/transport/central.h>

static uint8_t batteries[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
static uint32_t battery_valid;

// This integration is BLE-only. Read the enabled transport's current source IDs.
static uint32_t connected_sources(void) {
    STRUCT_SECTION_FOREACH(zmk_split_transport_central, transport) {
        if (transport->api->get_available_source_ids && transport->api->get_status &&
            transport->api->get_status().enabled) {
            uint8_t sources[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
            int count = transport->api->get_available_source_ids(sources);
            uint32_t connected = 0;
            for (int i = 0; i < count && i < ARRAY_SIZE(sources); i++) {
                if (sources[i] < ARRAY_SIZE(batteries)) {
                    connected |= BIT(sources[i]);
                }
            }
            return connected;
        }
    }
    return 0;
}

static int battery_event(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev && ev->source < ARRAY_SIZE(batteries) && ev->state_of_charge <= 100) {
        uint32_t connected = connected_sources();
        k_spinlock_key_t key = k_spin_lock(&status_lock);
        batteries[ev->source] = ev->state_of_charge;
        // ZMK also emits a synthetic 0% on disconnect, not a battery reading.
        battery_valid = (battery_valid | BIT(ev->source)) & connected;
        k_spin_unlock(&status_lock, key);
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(toucan_status_battery, battery_event);
ZMK_SUBSCRIPTION(toucan_status_battery, zmk_peripheral_battery_state_changed);

static uint8_t peer_battery(uint8_t source) {
    uint32_t connected = connected_sources();
    k_spinlock_key_t key = k_spin_lock(&status_lock);
    battery_valid &= connected;
    uint8_t level = source < ARRAY_SIZE(batteries) && (battery_valid & BIT(source))
                        ? batteries[source] : TOUCAN_BATTERY_UNKNOWN;
    k_spin_unlock(&status_lock, key);
    return level;
}

static uint8_t output_flags(void) {
    // ZMK selects only a ready endpoint, not merely USB power or a preference.
    switch (zmk_endpoint_get_selected().transport) {
    case ZMK_TRANSPORT_USB:
        return TOUCAN_STATUS_USB;
    case ZMK_TRANSPORT_BLE:
        return TOUCAN_STATUS_CONNECTED;
    default:
        return 0;
    }
}

void toucan_status_get(struct toucan_status *state) {
    *state = (struct toucan_status){
        .battery_left = zmk_battery_state_of_charge(),
        .battery_right = peer_battery(0),
        .flags = output_flags(),
        .valid = true,
    };
}

#if IS_ENABLED(CONFIG_ZMK_TOUCAN_STATUS_RELAY)
BUILD_ASSERT(ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT == 2,
             "Toucan2 screen relay expects exactly two split halves");
// Reuse the encrypted split behavior channel. The queue symbol is an internal
// ZMK dependency; checking it is best-effort, not a capacity reservation.
extern struct k_msgq zmk_split_central_split_run_msgq;
static struct k_work_q relay_queue;
K_THREAD_STACK_DEFINE(relay_stack, 2048);
static struct {
    uint8_t battery, flags;
    int64_t sent_at;
    bool sent;
} peers[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
static uint8_t next_source;

static void relay_work(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(relay, relay_work);

static void relay_work(struct k_work *work) {
    uint32_t connected = connected_sources();
    for (int i = 0; i < ARRAY_SIZE(peers); i++) {
        if (!(connected & BIT(i))) {
            peers[i].sent = false;
        }
    }
    if (k_msgq_num_used_get(&zmk_split_central_split_run_msgq)) {
        goto done;
    }

    uint8_t flags = output_flags();
    int64_t now = k_uptime_get();
    for (int i = 0; i < ARRAY_SIZE(peers); i++) {
        uint8_t source = (next_source + i) % ARRAY_SIZE(peers);
        if (!(connected & BIT(source))) {
            continue;
        }
        // Each recipient gets the OTHER slot's battery. The left display reads
        // its own sensor for L, so pairing order cannot exchange the readings.
        uint8_t battery = peer_battery(toucan_status_other_source(source));
        if (peers[source].sent && battery == peers[source].battery &&
            flags == peers[source].flags && now - peers[source].sent_at < 5000) {
            continue;
        }
        struct zmk_behavior_binding binding = {
            .behavior_dev = DEVICE_DT_NAME(DT_DRV_INST(0)),
            .param1 = battery,
            .param2 = flags,
        };
        struct zmk_behavior_binding_event event = {
            .timestamp = now, .position = TOUCAN_STATUS_STATE_TAG,
        };
        if (zmk_split_central_invoke_behavior(source, &binding, event, true) == 0) {
            peers[source].battery = battery;
            peers[source].flags = flags;
            peers[source].sent_at = now;
            peers[source].sent = true;
        }
        next_source = (source + 1) % ARRAY_SIZE(peers);
        break;
    }
done:
    k_work_reschedule_for_queue(&relay_queue, &relay, K_MSEC(250));
}

static int relay_init(void) {
    k_work_queue_start(&relay_queue, relay_stack, K_THREAD_STACK_SIZEOF(relay_stack), 10, NULL);
    k_work_reschedule_for_queue(&relay_queue, &relay, K_SECONDS(1));
    return 0;
}
SYS_INIT(relay_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

#elif IS_ENABLED(CONFIG_ZMK_TOUCAN_SCREEN)
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
static struct toucan_status received;
static int64_t received_at;

static int link_event(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);
    if (ev && !ev->connected) {
        k_spinlock_key_t key = k_spin_lock(&status_lock);
        received.valid = false;
        k_spin_unlock(&status_lock, key);
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(toucan_status_link, link_event);
ZMK_SUBSCRIPTION(toucan_status_link, zmk_split_peripheral_status_changed);

void toucan_status_get(struct toucan_status *state) {
    bool connected = zmk_split_bt_peripheral_is_connected();
    k_spinlock_key_t key = k_spin_lock(&status_lock);
    received.valid &= connected;
    *state = received;
    state->valid &= k_uptime_get() - received_at < 15000;
    k_spin_unlock(&status_lock, key);
    if (!state->valid) {
        state->battery_right = TOUCAN_BATTERY_UNKNOWN;
        state->flags = 0;
    }
    state->battery_left = zmk_battery_state_of_charge();
}
#endif

static int status_received(struct zmk_behavior_binding *binding,
                            struct zmk_behavior_binding_event event) {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) && IS_ENABLED(CONFIG_ZMK_TOUCAN_SCREEN)
    k_spinlock_key_t key = k_spin_lock(&status_lock);
    if (event.position == TOUCAN_STATUS_STATE_TAG) {
        if (!toucan_status_unpack(binding->param1, binding->param2, &received)) {
            k_spin_unlock(&status_lock, key);
            return -EINVAL;
        }
        received_at = k_uptime_get();
        received.valid = true;
    }
    // Old name frames are ignored and cannot extend the status deadline.
    k_spin_unlock(&status_lock, key);
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api status_api = {
    .binding_pressed = status_received,
    .binding_released = status_received,
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &status_api);

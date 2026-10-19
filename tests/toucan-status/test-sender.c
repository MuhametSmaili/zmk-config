// Exercise production sender scheduling and real BLE field widths.
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 1
#define CONFIG_ZMK_TOUCAN_STATUS_RELAY 1
#define CONFIG_ZMK_TOUCAN_SCREEN 0
#include "mock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zmk/split/bluetooth/service.h>
int64_t now = 1000;
bool link_connected = true, powered, host_ble_connected = true;
int selected_transport = ZMK_TRANSPORT_USB;
uint8_t local_battery = 99, current_profile = 2, current_layer = 5;
int current_wpm = 65;
const char *current_name = "TPM";
struct k_msgq zmk_split_central_split_run_msgq;
static bool connections[2] = {true, true};
static bool transport_enabled = true;
static int send_error;
static int sources(uint8_t *out) {
    int count = 0;
    for (int i = 0; i < 2; i++) if (connections[i]) out[count++] = i;
    return count;
}
static struct zmk_split_transport_status transport_status(void) {
    return (struct zmk_split_transport_status){.enabled = transport_enabled};
}
static const struct zmk_split_transport_central_api transport_api = {
    .get_available_source_ids = sources, .get_status = transport_status,
};
struct zmk_split_transport_central mock_transport = {.api = &transport_api};
static struct {
    int count;
    uint8_t source;
    int64_t sent_at[2];
    struct zmk_split_run_behavior_payload wire;
} capture;
int zmk_split_central_invoke_behavior(uint8_t source, struct zmk_behavior_binding *b,
                                      struct zmk_behavior_binding_event e, bool pressed) {
    assert(pressed && !strcmp(b->behavior_dev, "tstatus"));
    if (send_error) return send_error;
    capture.count++;
    capture.source = source;
    capture.sent_at[source] = e.timestamp;
    capture.wire = (struct zmk_split_run_behavior_payload){
        .data = {.position = e.position, .state = pressed,
                 .param1 = b->param1, .param2 = b->param2},
    };
    return 0;
}
#include "../../src/toucan_status.c"

static void poll_relay(void) {
    rescheduled_delay = -1;
    relay_work(NULL);
    assert(rescheduled_delay == 250);
}
static void tick(void) { now += 250; poll_relay(); }
static void battery_reading(uint8_t source, uint8_t level) {
    struct zmk_peripheral_battery_state_changed event = {source, level};
    battery_event(&event);
}
static void expect_frame(int count, uint8_t source, uint8_t battery, uint8_t flags) {
    assert(capture.count == count && capture.source == source);
    assert(capture.wire.data.position == 0xa1);
    assert(capture.wire.data.param1 == battery && "No profile/layer/WPM bytes on the wire");
    assert(capture.wire.data.param2 == flags);
}
int main(void) {
    assert(relay_init() == 0 && rescheduled_delay == 1000);
    battery_reading(0, 73);
    battery_reading(1, 41);
    poll_relay();
    expect_frame(1, 0, 41, TOUCAN_STATUS_USB);
    tick();
    expect_frame(2, 1, 73, TOUCAN_STATUS_USB);
    // No name frames, or traffic for layer/profile changes, even during typing.
    for (int i = 0; i < 8; i++) {
        current_layer = i; current_profile = i % 5; current_name = i % 2 ? "NAV" : "TPM";
        tick();
        assert(capture.count == 2);
    }
    // Battery changes coalesce behind occupied command queues.
    zmk_split_central_split_run_msgq.used = 1;
    battery_reading(1, 40); tick();
    battery_reading(1, 39); tick();
    assert(capture.count == 2);
    zmk_split_central_split_run_msgq.used = 0;
    tick(); expect_frame(3, 0, 39, TOUCAN_STATUS_USB);
    selected_transport = ZMK_TRANSPORT_BLE;
    tick(); expect_frame(4, 1, 73, TOUCAN_STATUS_CONNECTED);
    tick(); expect_frame(5, 0, 39, TOUCAN_STATUS_CONNECTED);
    // USB power or a non-selected BLE profile must not masquerade as host readiness.
    selected_transport = ZMK_TRANSPORT_NONE;
    powered = true;
    tick(); expect_frame(6, 1, 73, 0);
    tick(); expect_frame(7, 0, 39, 0);
    tick(); assert(capture.count == 7);
    // Each recipient's heartbeat becomes due at 5000 ms, not 4999 ms.
    now = capture.sent_at[1] + 4999;
    poll_relay(); assert(capture.count == 7);
    now++;
    poll_relay(); expect_frame(8, 1, 73, 0);
    now = capture.sent_at[0] + 4999;
    poll_relay(); assert(capture.count == 8);
    now++;
    poll_relay(); expect_frame(9, 0, 39, 0);
    // Disconnected peer is unknown, not 0%. Do not send to it.
    connections[1] = false;
    battery_reading(1, 0);
    tick(); expect_frame(10, 0, 255, 0);
    connections[1] = true;
    tick(); expect_frame(11, 1, 73, 0);
    assert(peer_battery(1) == 255);
    battery_reading(1, 0); // A real connected 0% is valid.
    tick(); expect_frame(12, 0, 0, 0);
    battery_reading(1, 150); // Invalid readings do not replace it.
    tick(); assert(capture.count == 12);
    // Enqueue failure must not mark pending data delivered.
    battery_reading(1, 12);
    send_error = -EAGAIN;
    tick(); assert(capture.count == 12);
    send_error = 0;
    tick(); expect_frame(13, 0, 12, 0);
    transport_enabled = false;
    tick(); assert(capture.count == 13);
    transport_enabled = true;
    tick(); expect_frame(14, 1, 73, 0);
    tick(); expect_frame(15, 0, 12, 0);
    puts("PASS sender: battery/output only, reciprocal slots, coalescing, retries, cadence/heartbeat boundaries and zero/unknown");
}

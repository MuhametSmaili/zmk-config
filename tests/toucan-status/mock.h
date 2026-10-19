#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#define IS_ENABLED(x) (x)
#define BIT(n) (1u << (n))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define BUILD_ASSERT(c, m) _Static_assert(c, m)
#define ZMK_BEHAVIOR_OPAQUE 0
#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT 2
#define ZMK_KEYMAP_LAYERS_LEN 6
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#define ZMK_TRANSPORT_NONE 0
#define ZMK_TRANSPORT_USB 1
#define ZMK_TRANSPORT_BLE 2
#define ZMK_SENSOR_EVENT_MAX_CHANNELS 1
struct zmk_sensor_channel_data { int32_t value; };
#define DT_DRV_INST(n) n
#define DEVICE_DT_NAME(n) "tstatus"
#define BEHAVIOR_DT_INST_DEFINE(...)
#define SYS_INIT(...)
#define K_SECONDS(n) ((n) * 1000)
#define K_MSEC(n) (n)
struct k_spinlock { int unused; };
typedef int k_spinlock_key_t;
static inline int k_spin_lock(struct k_spinlock *l) { return 0; }
static inline void k_spin_unlock(struct k_spinlock *l, int key) {}
struct k_work { int unused; };
struct k_work_delayable { int unused; };
struct k_work_q { int unused; };
struct k_msgq { int used; };
#define K_THREAD_STACK_DEFINE(name, n) static char name[n]
#define K_THREAD_STACK_SIZEOF(name) sizeof(name)
#define K_WORK_DELAYABLE_DEFINE(name, cb) static struct k_work_delayable name
static inline void k_work_queue_start(struct k_work_q *q, void *s, int n, int p, void *x) {}
static int rescheduled_delay;
static inline void k_work_reschedule_for_queue(struct k_work_q *q, struct k_work_delayable *w, int t) {
    rescheduled_delay = t;
}
static inline int k_msgq_num_used_get(struct k_msgq *q) { return q->used; }
extern int64_t now;
static inline int64_t k_uptime_get(void) { return now; }
extern bool link_connected;
extern bool powered;
extern int selected_transport;
extern bool host_ble_connected;
extern uint8_t local_battery;
extern uint8_t current_profile;
extern uint8_t current_layer;
extern int current_wpm;
extern const char *current_name;
static inline bool zmk_split_bt_peripheral_is_connected(void) { return link_connected; }
static inline uint8_t zmk_battery_state_of_charge(void) { return local_battery; }
static inline bool zmk_usb_is_powered(void) { return powered; }
static inline int zmk_ble_active_profile_index(void) { return current_profile; }
static inline bool zmk_ble_active_profile_is_connected(void) { return host_ble_connected; }
static inline bool zmk_ble_active_profile_is_open(void) { return false; }
static inline int zmk_keymap_highest_layer_active(void) { return current_layer; }
static inline int zmk_keymap_layer_index_to_id(int i) { return i; }
static inline const char *zmk_keymap_layer_name(int i) { return current_name; }
static inline int zmk_wpm_get_state(void) { return current_wpm; }
struct zmk_endpoint_instance { int transport; };
static inline struct zmk_endpoint_instance zmk_endpoint_get_selected(void) {
    return (struct zmk_endpoint_instance){.transport = selected_transport};
}
struct zmk_behavior_binding { const char *behavior_dev; uint32_t param1, param2; };
struct zmk_behavior_binding_event { uint32_t position; int64_t timestamp; };
struct behavior_driver_api {
    int (*binding_pressed)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    int (*binding_released)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
};
struct zmk_peripheral_battery_state_changed { uint8_t source, state_of_charge; };
struct zmk_split_peripheral_status_changed { bool connected; };
typedef void zmk_event_t;
static inline const struct zmk_peripheral_battery_state_changed *as_zmk_peripheral_battery_state_changed(const zmk_event_t *e) { return e; }
static inline const struct zmk_split_peripheral_status_changed *as_zmk_split_peripheral_status_changed(const zmk_event_t *e) { return e; }
static int (*link_handler)(const zmk_event_t *);
#define ZMK_LISTENER(name, cb) static int (*name)(const zmk_event_t *) = cb
#define SUB_zmk_peripheral_battery_state_changed(name)
#define SUB_zmk_split_peripheral_status_changed(name) \
    static void __attribute__((constructor)) register_##name(void) { link_handler = name; }
#define ZMK_SUBSCRIPTION(name, event) SUB_##event(name)
struct zmk_split_transport_status { bool enabled; };
struct zmk_split_transport_central_api {
    int (*get_available_source_ids)(uint8_t *);
    struct zmk_split_transport_status (*get_status)(void);
};
struct zmk_split_transport_central { const struct zmk_split_transport_central_api *api; };
extern struct zmk_split_transport_central mock_transport;
#define STRUCT_SECTION_FOREACH(type, var) for (struct type *var = &mock_transport; var; var = NULL)
int zmk_split_central_invoke_behavior(uint8_t, struct zmk_behavior_binding *, struct zmk_behavior_binding_event, bool);

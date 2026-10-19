// Production receiver through the actual ZMK 20-byte BLE payload.
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 0
#define CONFIG_ZMK_TOUCAN_STATUS_RELAY 0
#define CONFIG_ZMK_TOUCAN_SCREEN 1
#include "mock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zmk/split/bluetooth/service.h>
int64_t now;
bool link_connected = true;
uint8_t local_battery = 91;
#include "../../src/toucan_status.c"

static int deliver(uint32_t tag, uint32_t data, uint32_t flags) {
    struct zmk_split_run_behavior_payload tx = {
        .data = {.position = tag, .state = 1, .param1 = data, .param2 = flags},
        .behavior_dev = "tstatus",
    };
    uint8_t bytes[sizeof(tx)];
    memcpy(bytes, &tx, sizeof(tx));
    struct zmk_split_run_behavior_payload rx;
    memcpy(&rx, bytes, sizeof(rx));
    struct zmk_behavior_binding binding = {.param1 = rx.data.param1, .param2 = rx.data.param2};
    struct zmk_behavior_binding_event event = {.position = rx.data.position};
    return status_api.binding_pressed(&binding, event);
}
int main(void) {
    _Static_assert(sizeof(struct zmk_split_run_behavior_payload) == 20, "Check actual BLE wire size");
    _Static_assert(sizeof(((struct zmk_split_run_behavior_payload *)0)->data.position) == 1,
                   "Test must exercise actual 8-bit narrowing");
    struct toucan_status state;
    toucan_status_get(&state);
    assert(!state.valid && state.battery_left == 91 && state.battery_right == 255);
    // Historical regression: a 32-bit tag is truncated by the real wire field.
    assert(deliver(0x54535301u, 43, 1) == 0);
    toucan_status_get(&state);
    assert(!state.valid);
    // Uptime zero is a valid receipt time, not an uninitialized sentinel.
    assert(deliver(0xa1, 43, 1) == 0);
    toucan_status_get(&state);
    assert(state.valid && state.battery_right == 43 && state.flags == 1);
    // Old profile/layer/WPM bytes and bonded flag do not enter visible state.
    for (unsigned battery = 0; battery <= 255; battery++) {
        for (unsigned flags = 0; flags < 8; flags++) {
            struct toucan_status previous = state;
            int result = deliver(0xa1, 0xff050200u | battery, flags);
            toucan_status_get(&state);
            if (battery <= 100 || battery == 255) {
                assert(result == 0 && state.valid);
                assert(state.battery_left == 91 && state.battery_right == battery);
                assert(state.flags == (flags & 3));
            } else {
                assert(result == -EINVAL);
                assert(!memcmp(&state, &previous, sizeof(state)));
            }
        }
    }
    assert(deliver(0xa1, 0, 0) == 0);
    now = 14999;
    assert(deliver(0xa1, 70, 8) == -EINVAL);
    for (unsigned layer = 0; layer < 32; layer++) {
        assert(deliver(0xc0 | layer, 0x41424344, 0x45464748) == 0);
    }
    toucan_status_get(&state);
    assert(state.valid && state.battery_right == 0 && state.flags == 0);
    now = 15000;
    toucan_status_get(&state);
    assert(!state.valid && state.battery_right == 255 && state.flags == 0);
    assert(deliver(0xa1, 100, 2) == 0);
    struct zmk_split_peripheral_status_changed link = {.connected = false};
    assert(link_handler);
    link_handler(&link);
    link.connected = true;
    link_handler(&link); // Reconnect between UI polls must not revive old data.
    toucan_status_get(&state);
    assert(!state.valid && state.battery_left == 91);
    assert(deliver(0xa1, 100, 2) == 0);
    toucan_status_get(&state);
    assert(state.valid && state.battery_right == 100 && state.flags == 2);
    link_connected = false;
    toucan_status_get(&state);
    assert(!state.valid);
    link_connected = true;
    toucan_status_get(&state);
    assert(!state.valid);
    puts("PASS receiver/wire: legacy compatibility, malformed frames, 0%, 100%, unknown, expiry and reconnect");
}

#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 1
#define CONFIG_ZMK_TOUCAN_STATUS_RELAY 0
#define CONFIG_ZMK_TOUCAN_SCREEN 1
#include "mock.h"
#include <assert.h>
#include <stdio.h>
uint8_t local_battery = 73;
int selected_transport = ZMK_TRANSPORT_USB;
static bool connected = true;
static int sources(uint8_t *out) { out[0] = 0; return connected ? 1 : 0; }
static struct zmk_split_transport_status transport_status(void) {
    return (struct zmk_split_transport_status){.enabled = true};
}
static const struct zmk_split_transport_central_api api = {sources, transport_status};
struct zmk_split_transport_central mock_transport = {&api};
#include "../../src/toucan_status.c"

int main(void) {
    struct toucan_status state;
    toucan_status_get(&state);
    assert(state.valid && state.battery_left == 73 && state.battery_right == 255);
    assert(state.flags == TOUCAN_STATUS_USB);
    struct zmk_peripheral_battery_state_changed event = {0, 41};
    battery_event(&event);
    selected_transport = ZMK_TRANSPORT_BLE;
    toucan_status_get(&state);
    assert(state.valid && state.battery_right == 41 && state.flags == TOUCAN_STATUS_CONNECTED);
    connected = false;
    event.state_of_charge = 0;
    battery_event(&event);
    selected_transport = ZMK_TRANSPORT_NONE;
    toucan_status_get(&state);
    assert(state.valid && state.battery_right == 255 && state.flags == 0);
    connected = true;
    toucan_status_get(&state);
    assert(state.battery_right == 255);
    battery_event(&event);
    toucan_status_get(&state);
    assert(state.battery_right == 0);
    puts("PASS direct: local/remote battery and selected USB/BLE/no-host endpoint without relay");
}

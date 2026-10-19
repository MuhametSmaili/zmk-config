#pragma once

#include <stdbool.h>
#include <stdint.h>

// Preserve the existing state frame's byte-sized BLE position tag.
#define TOUCAN_STATUS_STATE_TAG 0xa1u
#define TOUCAN_BATTERY_UNKNOWN 255u
#define TOUCAN_STATUS_USB 1u
#define TOUCAN_STATUS_CONNECTED 2u

struct toucan_status {
    uint8_t battery_left;
    uint8_t battery_right;
    uint8_t flags;
    bool valid;
};

// Only the low battery byte and USB/BLE bits remain meaningful. Accept old
// profile/layer/WPM bytes and the old bonded bit without displaying them.
static inline bool toucan_status_unpack(uint32_t data, uint32_t flags,
                                       struct toucan_status *state) {
    uint8_t battery = data & 0xff;
    const uint32_t legacy_bonded = 4u;
    if ((battery > 100 && battery != TOUCAN_BATTERY_UNKNOWN) ||
        (flags & ~(TOUCAN_STATUS_USB | TOUCAN_STATUS_CONNECTED | legacy_bonded))) {
        return false;
    }
    state->battery_right = battery;
    state->flags = flags & (TOUCAN_STATUS_USB | TOUCAN_STATUS_CONNECTED);
    return true;
}

void toucan_status_get(struct toucan_status *state);

static inline uint8_t toucan_status_other_source(uint8_t source) {
    return source ^ 1u;
}

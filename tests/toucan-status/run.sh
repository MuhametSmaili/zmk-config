#!/usr/bin/env bash
# Host integration tests using the actual ZMK BLE packet header from a workspace.
set -euo pipefail
repo="$(cd "$(dirname "$0")/../.." && pwd)"
headers="${1:?Usage: tests/toucan-status/run.sh /path/to/zmk/app/include}"
headers="$(cd "$headers" && pwd)"
test -f "$headers/zmk/split/bluetooth/service.h"
work="$(mktemp -d "${TMPDIR:-/tmp}/toucan-status.XXXXXX")"
trap 'rm -rf "$work"' EXIT

# Stub platform APIs, but never substitute ZMK's real BLE wire definition.
for header in zephyr/kernel.h zephyr/sys/iterable_sections.h drivers/behavior.h; do
    mkdir -p "$work/$(dirname "$header")"
    printf '#include "mock.h"\n' > "$work/$header"
done
for header in battery.h ble.h endpoints.h event_manager.h keymap.h \
    events/battery_state_changed.h events/split_peripheral_status_changed.h \
    events/sensor_event.h sensors.h split/bluetooth/peripheral.h \
    split/central.h split/transport/central.h; do
    mkdir -p "$work/zmk/$(dirname "$header")"
    printf '#include "mock.h"\n' > "$work/zmk/$header"
done

for source in "$repo"/tests/toucan-status/test-*.c; do
    name="$(basename "$source" .c)"
    "${CC:-cc}" -std=c11 -g -Wall -Wextra -Werror \
        -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable \
        -Wno-sign-compare -fsanitize=address,undefined \
        -I"$repo/tests/toucan-status" -I"$work" -I"$repo/include" -I"$headers" \
        "$source" -o "$work/$name"
    "$work/$name"
done

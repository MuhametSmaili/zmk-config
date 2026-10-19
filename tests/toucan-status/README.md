# Toucan status regression tests

These checks compile the production status code with platform mocks and the **real ZMK BLE packet header**. They preserve the regression for the earlier 32-bit/8-bit frame-tag mismatch.

## Host tests

With a ZMK workspace and a C compiler that supports AddressSanitizer/UndefinedBehaviorSanitizer:

```sh
tests/toucan-status/run.sh /path/to/zmk/app/include
```

Coverage: direct and dongle roles, reciprocal battery mapping, selected host endpoint, queue occupancy, enqueue failure, coalescing, heartbeat, disconnect/reconnect, unknown versus real 0%, malformed packets and legacy frames. Layer/profile-only changes must not cause relay traffic. The runner generates temporary platform header shims and removes them on exit.

The mock RTOS does not exercise thread concurrency or real BLE delivery. Tests drive the existing sender work callback and receiver behavior; they do not replace hardware validation.

## Native screen test

Use the same LVGL checkout as the firmware, not another release:

```sh
cmake -S tests/toucan-status -B build/toucan-status-preview \
  -DLVGL_SOURCE_DIR=/path/to/modules/lib/gui/lvgl
cmake --build build/toucan-status-preview -j 8
build/toucan-status-preview/screen-preview build/toucan-status-preview
build/toucan-status-preview/screen-preview build/toucan-status-preview full
```

The preview executes the production renderer with LVGL's 1-bit format and a 16KB heap. It checks label content, clipping, every connection state, 0%/100%, unchanged-state redraw suppression and removal of old pixels after text changes. It writes four grayscale PGM images for inspection. The default mode matches firmware partial refresh; the second run checks full refresh. Builds with `NDEBUG` are rejected so the checks cannot silently disappear.

The ZMK build container lacks a native C++ driver. For its C-only LVGL configuration, add `-DCMAKE_CXX_COMPILER=/usr/bin/gcc` to CMake. Normal desktop toolchains do not need that workaround. Run previews with a timeout when diagnosing LVGL allocation or configuration failures.

## Compiled RGB integration

After building all four Toucan roles, place each role's `.config` and `zephyr.dts` in `BUILD_ARTIFACT_DIRECTORY/<role>/`. Use the devicetree parser from the same Zephyr workspace:

```sh
PYTHONPATH="$ZEPHYR_BASE/scripts/dts/python-devicetree/src" \
  python3 tests/toucan-status/check-rgb.py BUILD_ARTIFACT_DIRECTORY
```

This checks RGB is enabled only on the dongle, the shared peripheral-battery/USB settings and XIAO LED aliases, and the battery/connection behaviors and FNC combos on all four roles. The common commands must exist on the LED-disabled halves because the upstream behaviors use global split delivery.

No test flashes hardware or changes dependencies.

# Toucan2

[Return to the README](../README.md).

Toucan2 uses the shared 34-key keymap on a 36-key board.
The two outer thumb keys are inactive.
The left half has a nice!view screen. The right half has a TPS43 trackpad.

The configuration uses [my hardware fork](https://github.com/MuhametSmaili/zmk-keyboard-toucan2).
The firmware names remain `toucan_*`.

## Firmware and connection modes

All Toucan targets use `xiao_ble//zmk`.
Local builds save firmware in `build/local/`.

| File | Device and role |
| --- | --- |
| `toucan_dongle.uf2` | XIAO BLE dongle, central, RGB feedback, USB ZMK Studio |
| `toucan_left_peripheral.uf2` | Left half with screen, connected to the dongle |
| `toucan_left_central.uf2` | Left half with screen, central, USB ZMK Studio |
| `toucan_right.uf2` | Right half with trackpad, peripheral in either mode |

For dongle mode, flash the dongle, left-peripheral, and right files to their matching devices.
For dongleless mode, flash the left-central and right files to their matching halves.

Only the dongle uses `rgbled_adapter`. The halves do not drive RGB LEDs.
A dongle serves one keyboard at a time.

For routine updates, use matching firmware for all devices in the selected mode.
If the central device remains the same, a Bluetooth settings reset is not necessary.

If ZMK Studio contains an older custom keymap, restore the firmware default keymap to use the new bindings and layers.
Hardware behavior still needs verification after flashing.

### Change connection mode

The central device connects to the host and coordinates the halves.
A change between dongle and dongleless modes changes the Bluetooth bonds between devices.

**CAUTION: Save any custom settings before a reset.**
The settings-reset firmware erases saved Bluetooth bonds and settings.

1. Flash `settings_reset_xiao_ble.uf2` to the dongle and both halves.
2. Flash the matching normal firmware to each device in the selected mode.
3. For a Bluetooth host connection, remove the old host pairing.
4. Pair the new central device with the host.

The integration uses [ZMK's dongle guide](https://zmk.dev/docs/hardware-integration/dongle) and [split-pointing integration](https://zmk.dev/docs/hardware-integration/pointing).
The local left-peripheral shield keeps the fork's pin assignments without its left-central-only battery defaults.

## Trackpad

The trackpad supports tap-to-click, hold-to-drag, two-finger right-click and scrolling, pinch zoom, and three-finger swipes.
Zoom and swipe shortcuts use the fork's macOS defaults. The keyboard OS selector does not change these gestures.

Use two-finger scrolling on any layer.
`SYM` and `NAV` do not convert pointer movement to scrolling.
Finger contact alone does not send a held left click. Normal pointer movement must not select text.

In dongle mode, the right half sends trackpad input to split-input ID `0` on the dongle.
The dongle runs the shared keymap and gesture processors. It has no local trackpad or keyboard GPIO scan.

Both connection modes use the same trackpad settings.

### Automatic mouse layer (TPM)

After a 200 ms pause in typing, trackpad movement or button activity activates `TPM`.
The layer ends after 1 second without trackpad activity.
Finger contact alone does not activate it.

`TPM` changes only the left-hand mouse keys.
The right hand and all thumbs keep their underlying bindings.
Normal typing keys and all thumbs cancel `TPM` immediately. Mouse-action keys and modifiers do not cancel it.

| Base positions | Action on `TPM` |
| --- | --- |
| `W` / `E` / `R` | Back / middle / forward mouse buttons |
| `F` / `S` | Left / right mouse button |
| `A` / `D` / `G` | Scroll left / down / right |
| `Z` / `X` / `C` / `V` | OS-aware modifiers, as on `MSE` |

The click order differs from manual `MSE`: `TPM` uses `F` for left click, but `MSE` uses `S`.
Active `SYM`, `NAV`, `FNC`, or manual `MSE` prevents automatic activation.
Other keyboards do not have `TPM`.

## Screen and connection status

The 144×168 screen shows left and right battery percentages plus host connection status.
It has no layer names, profile numbers, gauges, WPM, or animations.
`nice_view` supplies the driver. `toucan_display` supplies the screen content.

The battery labels look like `L: 73%` and `R: 41%`.
`--` means that a reading is unavailable. `0%` means a real empty battery.
The screen keeps its local left battery reading even without remote status.

| Host label | Meaning |
| --- | --- |
| `HOST USB` | The central selected a ready USB keyboard connection. USB power alone is not enough. |
| `HOST BLE` | The central selected a connected Bluetooth host. |
| `NO HOST` | Status is available, but the central has no ready host connection. |
| `STATUS ?` | The screen has no fresh central status. This label does not prove that typing is disconnected. |

In dongleless mode, the left central reads status directly.
In dongle mode, a relay sends the other battery reading and host status over the encrypted split link.

The left battery comes from the screen half. The right battery comes from the other recipient, regardless of pairing order.

### Relay details

The relay sends changed battery or host status, with a five-second refresh for each connected recipient.
It sends at most one frame per 250 ms. It skips an occupied BLE command queue.
Layer and profile changes do not cause screen traffic.

The receiver invalidates remote status on disconnect or after 15 seconds without a valid state frame.

Both halves receive status because pairing slots do not identify the screen. The screenless right half ignores it.

The receiver accepts older state frames but ignores extra layer information and obsolete layer-name frames.

The queue guard uses an internal ZMK transport symbol and does not reserve queue capacity.
A future ZMK update can require an integration change.

## Dongle RGB feedback

On `FNC`, press a combo at these base positions:

| Combo | Feedback |
| --- | --- |
| `E` + `I` | Battery status |
| `R` + `U` | Connection status |

The dongle flashes its built-in RGB LED. USB-priority feedback uses cyan.
Layer-color indication is disabled.
Battery flashes use pairing-slot order, not necessarily left/right order.

RGB supplements the screen. The RGB widget treats 0% as missing, but the screen distinguishes a real 0% from an unknown reading.
In dongleless mode, these combos have no LED output. The screen remains available.

All three dongle-mode devices need the shared indicator behaviors, even though the halves do not drive LEDs.

## Power and tuning

The halves use the shared seven-minute deep-sleep timeout, 30-second idle timeout, and +8 dBm radio power.
The USB-powered dongle does not enter automatic deep sleep.

WPM and idle display blanking remain disabled. No measured battery-life improvement is claimed.

The screen task runs every 250 ms and redraws only changed labels.
The Sharp driver also performs VCOM maintenance every 33 ms.
The screen task interval does not describe all hardware wakeups.

**CAUTION: Do not change the VCOM interval without the exact panel requirements.**
The driver requires this maintenance.

The trackpad scroll divisor is `40` in [`toucan-pointing.dtsi`](../config/includes/toucan-pointing.dtsi).
For slower scrolling, increase the divisor.
For faster scrolling, decrease it.
This setting does not change keyboard scrolling, pointer speed, gestures, or `TPM` timing.

## Source files and tests

| File | Purpose |
| --- | --- |
| [`config/hardware_layouts/toucan.dtsi`](../config/hardware_layouts/toucan.dtsi) | Five-column layout adapter and matching Studio layout |
| [`config/includes/toucan-mouse.dtsi`](../config/includes/toucan-mouse.dtsi) | `TPM` bindings, timeout, and typing guard |
| [`config/includes/toucan-pointing.dtsi`](../config/includes/toucan-pointing.dtsi) | Input routing, contact filter, gestures, and scroll tuning |
| [`config/rgbled_adapter.conf`](../config/rgbled_adapter.conf) | Shared RGB settings |
| [`boards/shields/toucan_display/screen.c`](../boards/shields/toucan_display/screen.c) | Screen labels |
| [`src/toucan_status.c`](../src/toucan_status.c) | Battery and host status relay |
| [`include/zmk/toucan_status.h`](../include/zmk/toucan_status.h) | Status and wire format |
| [`tests/toucan-status/README.md`](../tests/toucan-status/README.md) | Host, LVGL, and RGB regression checks |

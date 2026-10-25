# Controls

[Return to the README](../README.md).

All key positions in this guide refer to the letters on `BASE`.
A combo uses two or more keys together.
The [layout image](../tools/keymap-drawer/cradio.svg) shows the full keymap.

## Layers and thumbs

| Thumb, left to right | Tap | Hold |
| --- | --- | --- |
| First | Space | `SYM` |
| Second | Tab | Ctrl |
| Third | Enter | Shift |
| Fourth | Backspace | `NAV` |

Hold the Space and Backspace thumbs together to activate `FNC`.
Home-row keys provide modifiers on a hold and their normal keys on a tap.
[`hold-taps.dtsi`](../config/includes/hold-taps.dtsi) contains the timing settings.

## OS selection and shortcuts

The default mode is macOS. OS-aware shortcuts use Cmd on macOS and Ctrl on Linux/Windows where applicable.
Word movement, line movement, and deletion also adapt to the selected OS.
`FNC` uses the same modifier order as `BASE`.

On `FNC`, press the appropriate combo:

| Base positions | OS mode |
| --- | --- |
| `Q` + `W` + `E` | macOS |
| `I` + `O` + `P` | Linux |

The screenshot key at `V` on `FNC` sends `Cmd+Shift+4` on macOS and `PrintScreen` on Linux/Windows.

On `NAV`, the comma key sends `Ctrl+Alt+Shift+Space` for my Hammerspoon leader shortcut.
That shortcut requires a matching host configuration. It is not a ZMK leader mode.

## System controls

Normal taps on `F1`–`F12` send function keys.
For a system action, hold its key on `FNC` for the specified duration:

| Key | Hold | Action |
| --- | --- | --- |
| `F1`–`F4` | 3 seconds | Select Bluetooth profile 0–3 |
| `F5` | 500 ms | Select Bluetooth output |
| `F6` | 500 ms | Select USB output |
| `F7` | 3 seconds | Unlock ZMK Studio |
| `F10` | 3 seconds | Clear the current Bluetooth profile |
| `F11` or `F12` | 3 seconds | Enter the bootloader |
| Media previous (`X`) or next (`.`) | 3 seconds | Reset |
| `F11` + `F12` (`G` + `H`) | 3 seconds | Soft Off |

The media keys send previous/next on a tap.
Soft Off powers off without a key release. A short Soft Off combo does nothing.
Volume up and down repeat on a hold. Mute has a separate key at `M` on `FNC`.

The `F2` hold selects Bluetooth profile 1, labeled BT 2 in the drawing, and sets Linux mode.
Other profile selections do not change OS mode.
`OUTPUT_HOLD_MS` in [`hold-taps.dtsi`](../config/includes/hold-taps.dtsi) sets the output-selection duration.

## Combos

These combos work on `BASE`, unless the table lists another layer:

| Base positions | Action | Layer |
| --- | --- | --- |
| `D` + `F` | Delete | `BASE` |
| `X` + `C` | Copy | `BASE` |
| `C` + `V` | Paste | `BASE` |
| `X` + `V` | Cut | `BASE` |
| `J` + `K`, or `Q` + `W` | Escape | `BASE` |
| `A` + `S` + `D` | Enter | `BASE` |
| `D` + `F` + `J` + `K` | Caps Word | `BASE` |
| `D` + `K` | Sticky Shift | `BASE` |
| `U` + `I` + `O` | Scroll up | `BASE`, `MSE` |
| `M` + `,` + `.` | Scroll down | `BASE`, `MSE` |
| `E` + `I` | Battery indicator | `FNC` |
| `R` + `U` | Connection indicator | `FNC` |

Clipboard combos use the selected OS mode.
Indicator output depends on the hardware. Toucan's [dongle RGB feedback](toucan.md#dongle-rgb-feedback) has additional limitations.
[`combos.dtsi`](../config/includes/combos.dtsi) defines the timing for each combo.

### Case modes

| Base positions | Mode |
| --- | --- |
| `V` + `M` | `snake_case` |
| `F` + `J` | `kebab-case` |
| `R` + `U` | `camelCase` |

On `BASE`:

1. Pause for at least 150 ms after the previous key.
2. Press both combo keys within 45 ms.
3. Hold both keys until 400 ms after the first press.
4. Release both keys before you type.

If you release either key before the hold completes, the case mode does not activate.
A recognized short combo still consumes both letters. The hold prevents accidental mode changes, not lost letters from accidental combos.

`CASE_HOLD_MS` in [`hold-taps.dtsi`](../config/includes/hold-taps.dtsi) sets the hold duration.

## Mouse controls

On `FNC`, press the key at `C` to toggle `MSE`.
On `MSE`, press the Backspace thumb or the `Q` + `W` combo to exit.
`S` is left click. `F` is right click.

The bottom-left keys provide modifiers without a tap/hold delay:

| Base position | macOS | Linux/Windows |
| --- | --- | --- |
| `Z` | Ctrl | GUI (Super/Windows) |
| `X` | Option | Alt |
| `C` | Cmd | Ctrl |
| `V` | Shift | Shift |

Hold a modifier during a click or pointer movement.
The right bottom row remains available for the scroll-down combo.
Toucan's automatic `TPM` layer has a [different click order](toucan.md#automatic-mouse-layer-tpm).

## Modules

[`config/west.yml`](../config/west.yml) lists the dependencies and revisions.

| Module | Purpose |
| --- | --- |
| [zmk-case-mode](https://github.com/MSmaili/zmk-case-mode) | Case modes for identifiers |
| [oskey](https://github.com/mentaldesk/oskey) | OS-aware modifiers and shortcuts |
| [zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget) | Battery and connection indicators |
| [zmk-keyboard-toucan2](https://github.com/MuhametSmaili/zmk-keyboard-toucan2) | Toucan hardware definitions |
| [zmk_driver_azoteq](https://github.com/beekeeb/zmk_driver_azoteq) | TPS43 trackpad driver |
| [zmk-input-zoom](https://github.com/beekeeb/zmk-input-zoom) | Trackpad zoom processor |

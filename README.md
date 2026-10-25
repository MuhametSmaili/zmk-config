# 34-Key ZMK Layout

| [Ferris Sweep](https://github.com/davidphilipbarr/Sweep) | [Urchin](https://github.com/duckyb/urchin) | [Forager](https://github.com/carrefinho/forager) |
| --- | --- | --- |
| <img width="250" height="200" alt="Ferris Sweep" src="https://github.com/user-attachments/assets/b81f97ac-0166-47d5-a392-54801568a16a" /> | <img width="250" height="200" alt="Urchin" src="https://github.com/user-attachments/assets/3eb89138-079e-4ab2-a906-678a655aec41" /> | <img width="250" height="200" alt="Forager keyboard" src="https://github.com/user-attachments/assets/5efa6680-a84d-45ea-9c31-fbd49ce2de95" /> |

This is my personal [ZMK](https://zmk.dev/) keymap for Sweep, Urchin, Forager, and Toucan2.
All four share the same 34-key layout. Toucan2 has 36 physical keys, but its two outer thumb keys are inactive.

I use QWERTY with home-row modifiers, symbols close to the home row, and Vim-style navigation.
The shortcuts adapt between macOS and Linux, so I can use the same keymap on both.

## Layout

![Shared 34-key layout preview](./tools/keymap-drawer/cradio.svg)

| Layer | Purpose |
| --- | --- |
| `BASE` | QWERTY and home-row modifiers |
| `SYM` | Symbols and punctuation |
| `NAV` | Numbers, text navigation, and editing |
| `FNC` | Function keys, media, and system controls |
| `MSE` | Mouse movement, scrolling, clicks, and modifiers |

The four active thumbs provide Space/`SYM`, Tab/Ctrl, Enter/Shift, and Backspace/`NAV`.
Numbers occupy the top row on `NAV`. Common programming symbols sit near the home row on `SYM`.

Hold both layer thumbs to activate `FNC`.
Normal taps send function keys. Holds select outputs, Bluetooth profiles, or system actions.

Combos provide clipboard shortcuts, Escape, Caps Word, and case modes such as `snake_case`.
The default OS mode is macOS.

The [controls guide](docs/controls.md) lists combos, hold durations, OS selection, and mouse controls.
Toucan2 also has a trackpad and a screen with both battery percentages and host status.
The [Toucan guide](docs/toucan.md) covers its setup and controls.

## Build and flash

GitHub Actions builds all targets in [`build.yaml`](build.yaml).
Local builds use the same file, but build only the selected keyboard.

1. Start Docker.
2. Run the commands from the repository root.

For a build without a dongle, run:

```sh
make build KEYBOARD=urchin
```

For a dongle build, run:

```sh
make build KEYBOARD=urchin DONGLE=1
```

Use `sweep`, `urchin`, `forager`, or `toucan` as the keyboard name.
Local builds save firmware in `build/local/`.

| Mode | Dongle firmware | Left-half firmware | Right-half firmware |
| --- | --- | --- | --- |
| With a dongle | `<keyboard>_dongle.uf2` | `<keyboard>_left_peripheral.uf2` | `<keyboard>_right.uf2` |
| Without a dongle | — | `<keyboard>_left_central.uf2` | `<keyboard>_right.uf2` |

Flash the matching file to each device.
The central device connects the keyboard to the host: either the dongle or the left half.
A dongle serves one keyboard at a time.

Before you change Toucan's connection mode, read the [pairing instructions](docs/toucan.md#change-connection-mode).
A settings reset erases saved Bluetooth bonds and settings.

To regenerate a layout image, run:

```sh
make draw KEYBOARD=urchin
```

The command saves drawings in `tools/keymap-drawer/`.
The first run builds a pinned Docker image for keymap-drawer.
`make help` lists the available commands.

## Change the keymap

Shared settings live in [`config/includes/`](config/includes/).
Changes there apply to all four keyboards.

| File | Contents |
| --- | --- |
| `base.dtsi` | Layers, key bindings, and system macros |
| `hold-taps.dtsi` | Hold-tap behavior and timing |
| `oskey.dtsi` | OS-aware modifiers and shortcuts |
| `combos.dtsi` | Combo keys and timing |
| `mouse.dtsi` | Keyboard mouse settings |

Sweep uses the internal shield name `cradio`, which also appears in its drawing filename.
The [controls guide](docs/controls.md#modules) lists the external modules.

## Credits

- [urob/zmk-config](https://github.com/urob/zmk-config) — home-row modifier and layout ideas.
- [caksoylar/zmk-config](https://github.com/caksoylar/zmk-config) — layout structure and keymap-drawer integration.

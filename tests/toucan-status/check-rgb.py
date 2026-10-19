#!/usr/bin/env python3
"""Check compiled Toucan RGB integration using Zephyr's real devicetree parser.

PYTHONPATH=$ZEPHYR_BASE/scripts/dts/python-devicetree/src \
  python3 tests/toucan-status/check-rgb.py BUILD_ARTIFACT_DIRECTORY

The directory must contain .config and zephyr.dts for all four Toucan roles.
No firmware is built or flashed by this check.
"""
import sys
import unittest
from pathlib import Path

from devicetree import dtlib

ROLES = ("toucan_dongle", "toucan_left_peripheral", "toucan_left_central", "toucan_right")


class ToucanRGB(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.configs = {}
        cls.trees = {}
        for role in ROLES:
            directory = ARTIFACTS / role
            cls.configs[role] = dict(
                line.split("=", 1) for line in (directory / ".config").read_text().splitlines()
                if line.startswith("CONFIG_") and "=" in line
            )
            cls.trees[role] = dtlib.DT(str(directory / "zephyr.dts"))

    def test_dongle_has_shared_rgb_feedback(self):
        config = self.configs["toucan_dongle"]
        for key, value in {
            "CONFIG_RGBLED_WIDGET": "y",
            "CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS": "y",
            "CONFIG_RGBLED_WIDGET_CONN_SHOW_USB": "y",
            "CONFIG_RGBLED_WIDGET_CONN_COLOR_USB": "6",
            "CONFIG_RGBLED_WIDGET_INTERVAL_MS": "500",
            "CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH": "50",
            "CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL": "20",
        }.items():
            with self.subTest(setting=key):
                self.assertEqual(config.get(key), value)
        for key in ("CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE", "CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS"):
            with self.subTest(setting=key):
                self.assertNotEqual(config.get(key), "y")
        tree = self.trees["toucan_dongle"]
        for color, label in (("red", "led0"), ("green", "led1"), ("blue", "led2")):
            with self.subTest(color=color):
                self.assertIs(tree.alias2node.get(f"led-{color}"), tree.label2node[label])
        status = tree.get_node("/leds").props.get("status")
        self.assertIn(status.to_string() if status else "okay", ("okay", "ok"))

    def test_halves_do_not_drive_rgb(self):
        for role in ROLES[1:]:
            with self.subTest(role=role):
                self.assertNotEqual(self.configs[role].get("CONFIG_RGBLED_WIDGET"), "y")

    def test_all_roles_accept_shared_indicator_commands(self):
        for role in ROLES:
            tree = self.trees[role]
            with self.subTest(role=role):
                self.assertEqual(self.configs[role].get("CONFIG_ZMK_BEHAVIOR_RGBLED_WIDGET"), "y")
                for name, behavior, positions in (
                    ("indbat", "ind_bat", [2, 7]),
                    ("indcon", "ind_con", [3, 6]),
                ):
                    combo = tree.get_node(f"/combos/combo_{name}")
                    self.assertEqual(combo.props["layers"].to_nums(), [3])  # FNC only
                    self.assertEqual(combo.props["key-positions"].to_nums(), positions)
                    self.assertIs(combo.props["bindings"].to_node(), tree.label2node[behavior])


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: check-rgb.py BUILD_ARTIFACT_DIRECTORY")
    ARTIFACTS = Path(sys.argv[1])
    unittest.main(argv=[sys.argv[0]], verbosity=2)

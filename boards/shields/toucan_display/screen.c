// SPDX-License-Identifier: MIT
// Only battery percentages and host readiness belong on this screen.
#include <lvgl.h>
#include <zmk/toucan_status.h>

static lv_obj_t *left_battery;
static lv_obj_t *right_battery;
static lv_obj_t *host_status;
static struct toucan_status previous;
static bool first = true;

static void set_battery(lv_obj_t *label, const char *side, uint8_t level) {
    if (level <= 100) {
        lv_label_set_text_fmt(label, "%s: %u%%", side, (unsigned)level);
    } else {
        lv_label_set_text_fmt(label, "%s: --", side);
    }
}

static void refresh(lv_timer_t *timer) {
    struct toucan_status state;
    toucan_status_get(&state);
    if (first || state.battery_left != previous.battery_left) {
        set_battery(left_battery, "L", state.battery_left);
    }
    if (first || state.battery_right != previous.battery_right) {
        set_battery(right_battery, "R", state.battery_right);
    }
    if (first || state.valid != previous.valid || state.flags != previous.flags) {
        const char *host = !state.valid ? "STATUS ?" :
                           state.flags & TOUCAN_STATUS_USB ? "HOST USB" :
                           state.flags & TOUCAN_STATUS_CONNECTED ? "HOST BLE" : "NO HOST";
        lv_label_set_text(host_status, host);
    }
    previous = state;
    first = false;
}

static lv_obj_t *label_at(lv_obj_t *screen, int y, const lv_font_t *font) {
    lv_obj_t *label = lv_label_create(screen);
    lv_obj_set_pos(label, 8, y);
    lv_obj_set_width(label, 128);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    left_battery = label_at(screen, 28, &lv_font_montserrat_24);
    right_battery = label_at(screen, 72, &lv_font_montserrat_24);
    // Pixel font keeps the small host text crisp on the 1-bit panel.
    host_status = label_at(screen, 128, &lv_font_unscii_16);
    lv_timer_create(refresh, 250, NULL);
    refresh(NULL);
    return screen;
}

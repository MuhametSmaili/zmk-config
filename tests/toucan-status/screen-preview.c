#include <lvgl.h>
#ifdef NDEBUG
#error "Screen regression tests require active assertions; do not use NDEBUG"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zmk/toucan_status.h>
#define WIDTH 144
#define HEIGHT 168
static uint8_t draw_buffer[8 + WIDTH * HEIGHT / 8];
static uint8_t pixels[WIDTH * HEIGHT];
static unsigned flush_count;
static struct toucan_status sample = {73, 41, TOUCAN_STATUS_USB, true};
void toucan_status_get(struct toucan_status *state) { *state = sample; }
lv_obj_t *zmk_display_status_screen(void);

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
    int width = area->x2 - area->x1 + 1;
    uint32_t stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_I1);
    data += 8; // LVGL's two-entry I1 palette precedes the pixel bits.
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = 0; x < width; x++) {
            pixels[y * WIDTH + area->x1 + x] = data[x / 8] & (0x80u >> (x % 8)) ? 255 : 0;
        }
        data += stride;
    }
    flush_count++;
    lv_display_flush_ready(display);
}
static void step(void) {
    lv_tick_inc(250);
    lv_timer_handler();
    lv_tick_inc(33);
    lv_timer_handler();
}
static void expect_labels(lv_obj_t *screen, const char *left, const char *right, const char *host) {
    const char *expected[] = {left, right, host};
    assert(lv_obj_get_child_count(screen) == 3);
    lv_obj_update_layout(screen);
    for (int i = 0; i < 3; i++) {
        lv_obj_t *label = lv_obj_get_child(screen, i);
        assert(!strcmp(lv_label_get_text(label), expected[i]));
        lv_point_t size;
        lv_text_get_size(&size, expected[i], lv_obj_get_style_text_font(label, 0),
                         0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        assert(size.x <= lv_obj_get_width(label));
        assert(lv_obj_get_x(label) >= 0 && lv_obj_get_y(label) >= 0);
        assert(lv_obj_get_x(label) + lv_obj_get_width(label) <= WIDTH);
        assert(lv_obj_get_y(label) + lv_obj_get_height(label) <= HEIGHT);
    }
}
static void save(const char *directory, const char *name) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.pgm", directory, name);
    FILE *file = fopen(path, "wb");
    assert(file);
    fprintf(file, "P5\n144 168\n255\n");
    size_t written = fwrite(pixels, 1, sizeof(pixels), file);
    assert(written == sizeof(pixels));
    int result = fclose(file);
    assert(result == 0);
    unsigned white = 0;
    for (unsigned i = 0; i < sizeof(pixels); i++) white += pixels[i] > 127;
    assert(white > 100 && white < 6000);
    printf("%s: %u white pixels\n", name, white);
}
static void log_message(lv_log_level_t level, const char *message) {
    (void)level;
    fputs(message, stderr);
}
int main(int argc, char **argv) {
    assert(argc == 2 || (argc == 3 && !strcmp(argv[2], "full")));
    lv_init();
    lv_log_register_print_cb(log_message);
    lv_display_t *display = lv_display_create(WIDTH, HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
    // Firmware uses partial refresh with a full-size draw buffer. Also check full refresh.
    lv_display_render_mode_t mode = argc == 3 ? LV_DISPLAY_RENDER_MODE_FULL : LV_DISPLAY_RENDER_MODE_PARTIAL;
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof(draw_buffer), mode);
    lv_display_set_flush_cb(display, flush);
    lv_obj_t *screen = zmk_display_status_screen();
    lv_screen_load(screen);
    step();
    expect_labels(screen, "L: 73%", "R: 41%", "HOST USB");
    save(argv[1], "screen-usb");
    uint8_t initial_pixels[sizeof(pixels)];
    memcpy(initial_pixels, pixels, sizeof(pixels));
    unsigned before = flush_count;
    for (int i = 0; i < 10; i++) step();
    assert(flush_count == before && "Identical status must not redraw");
    sample.battery_left = 100; sample.battery_right = 0; sample.flags = TOUCAN_STATUS_CONNECTED;
    step();
    expect_labels(screen, "L: 100%", "R: 0%", "HOST BLE");
    save(argv[1], "screen-ble");
    sample.flags = 0; sample.battery_right = TOUCAN_BATTERY_UNKNOWN;
    step();
    expect_labels(screen, "L: 100%", "R: --", "NO HOST");
    save(argv[1], "screen-no-host");
    sample.valid = false;
    step();
    expect_labels(screen, "L: 100%", "R: --", "STATUS ?");
    save(argv[1], "screen-unknown");
    sample.battery_right = 100; sample.valid = true; sample.flags = TOUCAN_STATUS_USB;
    step();
    expect_labels(screen, "L: 100%", "R: 100%", "HOST USB");
    sample = (struct toucan_status){73, 41, TOUCAN_STATUS_USB, true};
    step();
    assert(!memcmp(pixels, initial_pixels, sizeof(pixels)) && "Text updates must erase old pixels");
    printf("PASS real LVGL (%s): percentages, host states, bounds, recovery and no redraw for unchanged status (16KB heap)\n",
           argc == 3 ? "full" : "partial");
}

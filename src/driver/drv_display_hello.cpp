#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_display_shared.h"

#if defined(PLATFORM_ESPIDF) && defined(ENABLE_DISPLAY)

#include <lvgl.h>
#include "display/lvgl_v8_port.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"

static lv_obj_t *screen_main;
static lv_obj_t *counter_label;
static lv_obj_t *tick_label;
static lv_obj_t *mem_label;
static lv_obj_t *chip_label;
static int click_count = 0;
static int tick_count = 0;

static void btn_event_cb(lv_event_t *e) {
    click_count++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicks: %d", click_count);
    lv_label_set_text(counter_label, buf);
}

static void gui_build_main_screen() {
    screen_main = lv_obj_create(NULL);
    // White background
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // Title label
    lv_obj_t *title = lv_label_create(screen_main);
    lv_label_set_text(title, "OpenBeken Display Hello");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_color(title, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);

    // Counter label
    counter_label = lv_label_create(screen_main);
    lv_label_set_text(counter_label, "Clicks: 0");
    lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_color(counter_label, lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_set_style_text_font(counter_label, &lv_font_montserrat_14, LV_PART_MAIN);
    
    // Tick label
    tick_label = lv_label_create(screen_main);
    lv_label_set_text(tick_label, "Ticks: 0");
    lv_obj_align(tick_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(tick_label, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_set_style_text_font(tick_label, &lv_font_montserrat_14, LV_PART_MAIN);

    // Button
    lv_obj_t *btn = lv_btn_create(screen_main);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1976D2), (lv_style_selector_t)((int)LV_PART_MAIN | (int)LV_STATE_PRESSED));
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Button label
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, LV_PART_MAIN);

    // Memory Label (Bottom Right)
    mem_label = lv_label_create(screen_main);
    lv_label_set_text(mem_label, "Mem: --");
    lv_obj_align(mem_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_text_color(mem_label, lv_color_hex(0x777777), LV_PART_MAIN);
    lv_obj_set_style_text_font(mem_label, &lv_font_montserrat_14, LV_PART_MAIN);

    // Chip Info Label (Bottom Left)
    chip_label = lv_label_create(screen_main);
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    char cbuf[128];
    snprintf(cbuf, sizeof(cbuf), "Cores: %d, Rev: %d", chip_info.cores, chip_info.revision);
    lv_label_set_text(chip_label, cbuf);
    lv_obj_align(chip_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_text_color(chip_label, lv_color_hex(0x777777), LV_PART_MAIN);
    lv_obj_set_style_text_font(chip_label, &lv_font_montserrat_14, LV_PART_MAIN);
}

extern "C" void DisplayHello_Init() {
    if (!Display_PreInit()) {
        return;
    }
    
    lvgl_port_lock(-1);
    gui_build_main_screen();
    lv_scr_load(screen_main);
    lvgl_port_unlock();

    Display_PostInit();
}

extern "C" void DisplayHello_OnEverySecond() {
    if(!g_display_ready) return;
    
    tick_count++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Ticks: %d", tick_count);

    char mbuf[128];
    size_t free_int = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t max_int = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    size_t free_spi = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t max_spi = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    snprintf(mbuf, sizeof(mbuf), "INT: %u (%u max)\nSPI: %u (%u max)", 
            (unsigned)free_int, (unsigned)max_int, (unsigned)free_spi, (unsigned)max_spi);
    
    // Properly use LVGL locks as this is called from the main OBK task, not the LVGL task!
    lvgl_port_lock(-1);
    if(tick_label) {
        lv_label_set_text(tick_label, buf);
    }
    if(mem_label) {
        lv_label_set_text(mem_label, mbuf);
    }
    lvgl_port_unlock();
}

extern "C" void DisplayHello_Shutdown() {
    Display_Shutdown();
}

#else

extern "C" void DisplayHello_Init() {}
extern "C" void DisplayHello_OnEverySecond() {}
extern "C" void DisplayHello_Shutdown() {}

#endif

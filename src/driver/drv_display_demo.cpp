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
static lv_obj_t *mem_label;
static lv_obj_t *chip_label;
static lv_obj_t *chart;
static lv_chart_series_t * ser;
static int tick_count = 0;

static void gui_build_main_screen() {
    screen_main = lv_obj_create(NULL);
    // Dark background
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x121220), LV_PART_MAIN);

    // Create a tab view
    lv_obj_t * tabview = lv_tabview_create(screen_main, LV_DIR_TOP, 50);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x121220), LV_PART_MAIN);
    
    // Get the buttons part of the tabview and style it
    lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xFFFFFF), (lv_style_selector_t)((int)LV_PART_ITEMS | (int)LV_STATE_CHECKED));

    // Add 3 tabs
    lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Controls");
    lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Dashboard");
    lv_obj_t * tab3 = lv_tabview_add_tab(tabview, "System");

    // --- TAB 1: Controls ---
    // A slider
    lv_obj_t * slider = lv_slider_create(tab1);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_width(slider, 200);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x0F3460), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xE94560), LV_PART_INDICATOR);

    // A switch
    lv_obj_t * sw = lv_switch_create(tab1);
    lv_obj_align(sw, LV_ALIGN_CENTER, -60, 20);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xE94560), (lv_style_selector_t)((int)LV_PART_INDICATOR | (int)LV_STATE_CHECKED));

    // An arc
    lv_obj_t * arc = lv_arc_create(tab1);
    lv_obj_set_size(arc, 100, 100);
    lv_obj_align(arc, LV_ALIGN_CENTER, 60, 20);
    lv_arc_set_value(arc, 40);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x0F3460), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0xE94560), LV_PART_INDICATOR);

    // --- TAB 2: Dashboard ---
    chart = lv_chart_create(tab2);
    lv_obj_set_size(chart, 240, 150);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x16213E), LV_PART_MAIN);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x00FF00), LV_PART_ITEMS);
    
    // Add data series
    ser = lv_chart_add_series(chart, lv_color_hex(0xE94560), LV_CHART_AXIS_PRIMARY_Y);
    for(int i = 0; i < 10; i++) {
        lv_chart_set_next_value(chart, ser, lv_rand(10, 90));
    }

    // --- TAB 3: System ---
    mem_label = lv_label_create(tab3);
    lv_label_set_text(mem_label, "Mem: --");
    lv_obj_align(mem_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(mem_label, lv_color_hex(0xAAAAAA), LV_PART_MAIN);

    chip_label = lv_label_create(tab3);
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    char cbuf[128];
    snprintf(cbuf, sizeof(cbuf), "Cores: %d\nRev: %d", chip_info.cores, chip_info.revision);
    lv_label_set_text(chip_label, cbuf);
    lv_obj_align(chip_label, LV_ALIGN_TOP_LEFT, 10, 60);
    lv_obj_set_style_text_color(chip_label, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
}

extern "C" void DisplayDemo_Init() {
    if (!Display_PreInit()) {
        return;
    }
    
    lvgl_port_lock(-1);
    gui_build_main_screen();
    lv_scr_load(screen_main);
    lvgl_port_unlock();

    Display_PostInit();
}

extern "C" void DisplayDemo_OnEverySecond() {
    if(!g_display_ready) return;
    
    tick_count++;

    char mbuf[128];
    size_t free_int = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t max_int = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    size_t free_spi = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t max_spi = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    snprintf(mbuf, sizeof(mbuf), "INT: %u (%u max)\nSPI: %u (%u max)", 
            (unsigned)free_int, (unsigned)max_int, (unsigned)free_spi, (unsigned)max_spi);
    
    lvgl_port_lock(-1);
    if(mem_label) {
        lv_label_set_text(mem_label, mbuf);
    }
    if (chart && ser) {
        lv_chart_set_next_value(chart, ser, lv_rand(10, 90));
    }
    lvgl_port_unlock();
}

extern "C" void DisplayDemo_Shutdown() {
    Display_Shutdown();
}

#else

extern "C" void DisplayDemo_Init() {}
extern "C" void DisplayDemo_OnEverySecond() {}
extern "C" void DisplayDemo_Shutdown() {}

#endif

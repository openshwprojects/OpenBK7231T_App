#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"

#if defined(PLATFORM_ESPIDF)

#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "display/lvgl_v8_port.h"
#include "esp_log.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static bool g_display_ready = false;
static lv_obj_t *screen_main;

static void gui_build_main_screen() {
    screen_main = lv_obj_create(NULL);
    // Set background color to Red for testing (we can change it)
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0xFF0000), LV_PART_MAIN);
    
    // Create a label
    lv_obj_t * label = lv_label_create(screen_main);
    lv_label_set_text(label, "OpenBeken Display Text 2");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

extern "C" void Display_Init() {
    ADDLOG_INFO(LOG_FEATURE_DRV, "Initializing Display Board...");

    // Initialize Board and Display
    Board *board = new Board();
    ADDLOG_INFO(LOG_FEATURE_DRV, "Board created, calling init()...");
    board->init();
    
    ADDLOG_INFO(LOG_FEATURE_DRV, "Board init() done, calling begin()...");
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    if(!board->begin()) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "Board begin() failed!");
        delete board;
        return;
    }
    ADDLOG_INFO(LOG_FEATURE_DRV, "Board begin() OK!");

    LCD *lcd = board->getLCD();
    Touch *tp = board->getTouch();
    ADDLOG_INFO(LOG_FEATURE_DRV, "LCD=%p Touch=%p", lcd, tp);

    ADDLOG_INFO(LOG_FEATURE_DRV, "Initializing LVGL Port...");
    if(!lvgl_port_init(lcd, tp)) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "lvgl_port_init failed!");
        return;
    }
    ADDLOG_INFO(LOG_FEATURE_DRV, "LVGL Port initialized OK!");

    lvgl_port_lock(-1);
    gui_build_main_screen();
    lv_scr_load(screen_main);
    lvgl_port_unlock();

    g_display_ready = true;
    ADDLOG_INFO(LOG_FEATURE_DRV, "Display driver ready!");
}

extern "C" void Display_RunFrame() {
    if(!g_display_ready) return;
    lvgl_port_lock(-1);
    lv_timer_handler();
    lvgl_port_unlock();
}

#else

extern "C" void Display_Init() {
}

extern "C" void Display_RunFrame() {
}

#endif

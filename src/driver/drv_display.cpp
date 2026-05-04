#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"

#if defined(PLATFORM_ESPIDF) && defined(ENABLE_DISPLAY)

#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "display/lvgl_v8_port.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static bool g_display_ready = false;
static TaskHandle_t g_lvgl_task_handle = NULL;
static lv_obj_t *screen_main;
static lv_obj_t *counter_label;
static int click_count = 0;

static void btn_event_cb(lv_event_t *e) {
    click_count++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Clicks: %d", click_count);
    lv_label_set_text(counter_label, buf);
}

static void gui_build_main_screen() {
    screen_main = lv_obj_create(NULL);
    // Dark background
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x1A1A2E), LV_PART_MAIN);

    // Title label
    lv_obj_t *title = lv_label_create(screen_main);
    lv_label_set_text(title, "OpenBeken Display Demo 2");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);

    // Counter label
    counter_label = lv_label_create(screen_main);
    lv_label_set_text(counter_label, "Clicks: 0");
    lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_color(counter_label, lv_color_hex(0xE94560), LV_PART_MAIN);
    lv_obj_set_style_text_font(counter_label, &lv_font_montserrat_14, LV_PART_MAIN);

    // Button
    lv_obj_t *btn = lv_btn_create(screen_main);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0F3460), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x16213E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Button label
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, LV_PART_MAIN);
}

static void lvgl_task(void *arg) {
    ADDLOG_INFO(LOG_FEATURE_DRV, "LVGL task started");
    while(1) {
        lvgl_port_lock(-1);
        lv_timer_handler();
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
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

    // Spawn dedicated LVGL task with 8KB stack
    xTaskCreate(lvgl_task, "lvgl", 8192, NULL, 5, &g_lvgl_task_handle);

    g_display_ready = true;
    ADDLOG_INFO(LOG_FEATURE_DRV, "Display driver ready!");
}

extern "C" void Display_Shutdown() {
    if (g_lvgl_task_handle != NULL) {
        vTaskDelete(g_lvgl_task_handle);
        g_lvgl_task_handle = NULL;
    }
    g_display_ready = false;
    ADDLOG_INFO(LOG_FEATURE_DRV, "Display driver shut down!");
}

extern "C" void Display_RunFrame() {
    // LVGL is handled by its own dedicated task now
}

#else

extern "C" void Display_Init() {
}

extern "C" void Display_Shutdown() {
}

extern "C" void Display_RunFrame() {
}

#endif

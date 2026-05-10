#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_display_shared.h"

#if defined(PLATFORM_ESPIDF) && defined(ENABLE_DISPLAY)

#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "display/lvgl_v8_port.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

bool g_display_ready = false;
static TaskHandle_t g_lvgl_task_handle = NULL;
static Board *g_board = NULL;

static void lvgl_task(void *arg) {
    ADDLOG_INFO(LOG_FEATURE_DRV, "LVGL task started");
    while(1) {
        lvgl_port_lock(-1);
        lv_timer_handler();
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" bool Display_PreInit() {
    ADDLOG_INFO(LOG_FEATURE_DRV, "Initializing Display Board...");

    // Initialize Board and Display
    g_board = new Board();
    ADDLOG_INFO(LOG_FEATURE_DRV, "Board created, calling init()...");
    g_board->init();
    
    ADDLOG_INFO(LOG_FEATURE_DRV, "Board init() done, calling begin()...");
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    if(!g_board->begin()) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "Board begin() failed!");
        delete g_board;
        g_board = NULL;
        return false;
    }
    ADDLOG_INFO(LOG_FEATURE_DRV, "Board begin() OK!");

    LCD *lcd = g_board->getLCD();
    Touch *tp = g_board->getTouch();
    ADDLOG_INFO(LOG_FEATURE_DRV, "LCD=%p Touch=%p", lcd, tp);

    ADDLOG_INFO(LOG_FEATURE_DRV, "Initializing LVGL Port...");
    if(!lvgl_port_init(lcd, tp)) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "lvgl_port_init failed!");
        delete g_board;
        g_board = NULL;
        return false;
    }
    ADDLOG_INFO(LOG_FEATURE_DRV, "LVGL Port initialized OK!");
    
    return true;
}

extern "C" void Display_PostInit() {
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
    if (g_board) {
        delete g_board;
        g_board = NULL;
    }
    ADDLOG_INFO(LOG_FEATURE_DRV, "Display driver shut down!");
}

#else

extern "C" bool Display_PreInit() { return false; }
extern "C" void Display_PostInit() {}
extern "C" void Display_Shutdown() {}

#endif

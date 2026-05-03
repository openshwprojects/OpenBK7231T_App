/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

#if LVGL_PORT_AVOID_TEARING_MODE
    #error "This example does not support the avoid tearing function. Please set `LVGL_PORT_AVOID_TEARING_MODE` to `0` in the `lvgl_v8_port.h` file."
#endif

LV_IMG_DECLARE(img_esp_logo);

static lv_obj_t *lbl_rotation = NULL;
static lv_disp_rot_t rotation = LV_DISP_ROT_NONE;

static void rotateDisplay(lv_disp_t *disp, lv_disp_rot_t rotation)
{
    lvgl_port_lock(-1);
    lv_disp_set_rotation(disp, rotation);
    lv_label_set_text_fmt(lbl_rotation, "Rotation %d°", (int)rotation * 90);
    lvgl_port_unlock();
}

static void onRightBtnClickCallback(lv_event_t *e)
{
    if (rotation == LV_DISP_ROT_270) {
        rotation = LV_DISP_ROT_NONE;
    } else {
        rotation = (lv_disp_rot_t)(((int)rotation) + 1);
    }
    /* Rotate display */
    rotateDisplay(lv_disp_get_default(), rotation);
    Serial.println("Rotate right");
}

static void onLeftBtnClickCallback(lv_event_t *e)
{
    if (rotation == LV_DISP_ROT_NONE) {
        rotation = LV_DISP_ROT_270;
    } else {
        rotation = (lv_disp_rot_t)(((int)rotation) - 1);
    }
    /* Rotate display */
    rotateDisplay(lv_disp_get_default(), rotation);
    Serial.println("Rotate left");
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Initializing board");
    Board *board = new Board();
    board->init();
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *lbl = NULL;

    // Create image
    lv_obj_t *img_logo = lv_img_create(scr);
    lv_img_set_src(img_logo, &img_esp_logo);
    lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, 20);

    lbl_rotation = lv_label_create(scr);
    lv_label_set_text(lbl_rotation, "Rotation 0°");
    lv_obj_align(lbl_rotation, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *cont_row = lv_obj_create(scr);
    lv_obj_set_size(cont_row, board->getLcdHeight() - 10, 50);
    lv_obj_align(cont_row, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 5, 0);
    lv_obj_set_style_pad_bottom(cont_row, 5, 0);
    lv_obj_set_style_pad_left(cont_row, 5, 0);
    lv_obj_set_style_pad_right(cont_row, 5, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Button rotate left */
    lv_obj_t *btn_left = lv_btn_create(cont_row);
    lbl = lv_label_create(btn_left);
    lv_label_set_text_static(lbl, LV_SYMBOL_LEFT" Left");
    lv_obj_align(btn_left, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    /* Button event */
    lv_obj_add_event_cb(btn_left, onLeftBtnClickCallback, LV_EVENT_CLICKED, NULL);

    lbl = lv_label_create(cont_row);
    lv_label_set_text_static(lbl, " rotate ");

    /* Button rotate right */
    lv_obj_t *btn_right = lv_btn_create(cont_row);
    lbl = lv_label_create(btn_right);
    lv_label_set_text_static(lbl, "Right "LV_SYMBOL_RIGHT);
    lv_obj_align(btn_right, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    /* Button event */
    lv_obj_add_event_cb(btn_right, onRightBtnClickCallback, LV_EVENT_CLICKED, NULL);

    /* Release the mutex */
    lvgl_port_unlock();
}

void loop()
{
    Serial.println("IDLE loop");
    delay(1000);
}

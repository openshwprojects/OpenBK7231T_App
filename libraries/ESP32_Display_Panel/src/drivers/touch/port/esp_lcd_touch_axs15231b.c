/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../esp_panel_touch_conf_internal.h"
#if ESP_PANEL_DRIVERS_TOUCH_ENABLE_AXS15231B

#include <stdlib.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_touch.h"

#include "esp_lcd_touch_axs15231b.h"

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"

/*max point num*/
#define AXS_MAX_TOUCH_NUMBER                (1)

static const char *TAG = "AXS15231B";

static esp_err_t touch_axs15231b_read_data(esp_lcd_touch_handle_t tp);
static bool touch_axs15231b_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t touch_axs15231b_del(esp_lcd_touch_handle_t tp);
static esp_err_t touch_axs15231b_reset(esp_lcd_touch_handle_t tp);

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, int reg, uint8_t *data, uint8_t len);
static esp_err_t i2c_write_bytes(esp_lcd_touch_handle_t tp, int reg, const uint8_t *data, uint8_t len);

esp_err_t esp_lcd_touch_new_i2c_axs15231b(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *tp)
{
    ESP_LOGI(TAG, "version: %d.%d.%d", ESP_LCD_TOUCH_AXS15231B_VER_MAJOR, ESP_LCD_TOUCH_AXS15231B_VER_MINOR,
             ESP_LCD_TOUCH_AXS15231B_VER_PATCH);

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "Invalid io");
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "Invalid touch handle");

    /* Prepare main structure */
    esp_err_t ret = ESP_OK;
    esp_lcd_touch_handle_t axs15231b = calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(axs15231b, ESP_ERR_NO_MEM, err, TAG, "Touch handle malloc failed");

    /* Communication interface */
    axs15231b->io = io;
    /* Only supported callbacks are set */
    axs15231b->read_data = touch_axs15231b_read_data;
    axs15231b->get_xy = touch_axs15231b_get_xy;
    axs15231b->del = touch_axs15231b_del;
    /* Mutex */
    axs15231b->data.lock.owner = portMUX_FREE_VAL;
    /* Save config */
    memcpy(&axs15231b->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch interrupt */
    if (axs15231b->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = GPIO_INTR_NEGEDGE,
            .pin_bit_mask = BIT64(axs15231b->config.int_gpio_num)
        };
        ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO intr config failed");

        /* Register interrupt callback */
        if (axs15231b->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(axs15231b, axs15231b->config.interrupt_callback);
        }
    }
    /* Prepare pin for touch controller reset */
    if (axs15231b->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(axs15231b->config.rst_gpio_num)
        };
        ESP_GOTO_ON_ERROR(gpio_config(&rst_gpio_config), err, TAG, "GPIO reset config failed");
    }
    /* Reset controller */
    ESP_GOTO_ON_ERROR(touch_axs15231b_reset(axs15231b), err, TAG, "Reset failed");
    *tp = axs15231b;

    return ESP_OK;
err:
    if (axs15231b) {
        touch_axs15231b_del(axs15231b);
    }
    ESP_LOGE(TAG, "Initialization failed!");
    return ret;
}

static esp_err_t touch_axs15231b_read_data(esp_lcd_touch_handle_t tp)
{
    typedef struct {
        uint8_t gesture;    //AXS_TOUCH_GESTURE_POS:0
        uint8_t num;        //AXS_TOUCH_POINT_NUM:1
        uint8_t x_h : 4;    //AXS_TOUCH_X_H_POS:2
        uint8_t : 2;
        uint8_t event : 2;  //AXS_TOUCH_EVENT_POS:2
        uint8_t x_l;        //AXS_TOUCH_X_L_POS:3
        uint8_t y_h : 4;    //AXS_TOUCH_Y_H_POS:4
        uint8_t : 4;
        uint8_t y_l;        //AXS_TOUCH_Y_L_POS:5
    } __attribute__((packed)) touch_record_struct_t;

    touch_record_struct_t *p_touch_data = NULL;

    uint8_t data[AXS_MAX_TOUCH_NUMBER * 6 + 2] = {0}; /*1 Point:8;  2 Point: 14 */
    const uint8_t read_cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, (AXS_MAX_TOUCH_NUMBER * 6 + 2) >> 8, (AXS_MAX_TOUCH_NUMBER * 6 + 2) & 0xff, 0x00, 0x00, 0x00};

    ESP_RETURN_ON_ERROR(i2c_write_bytes(tp, -1, read_cmd, sizeof(read_cmd)), TAG, "I2C write failed");
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, -1, data, sizeof(data)), TAG, "I2C read failed");

    p_touch_data = (touch_record_struct_t *) data;
    if (p_touch_data->num && (AXS_MAX_TOUCH_NUMBER >= p_touch_data->num)) {
        portENTER_CRITICAL(&tp->data.lock);
        tp->data.points = p_touch_data->num;
        /* Fill all coordinates */
        for (int i = 0; i < tp->data.points; i++) {
            tp->data.coords[i].x = ((p_touch_data->x_h & 0x0F) << 8) | p_touch_data->x_l;
            tp->data.coords[i].y = ((p_touch_data->y_h & 0x0F) << 8) | p_touch_data->y_l;
        }
        portEXIT_CRITICAL(&tp->data.lock);
    }

    return ESP_OK;
}

static bool touch_axs15231b_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    portENTER_CRITICAL(&tp->data.lock);
    /* Count of points */
    *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);
    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    /* Invalidate */
    tp->data.points = 0;
    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t touch_axs15231b_del(esp_lcd_touch_handle_t tp)
{
    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }
    /* Release memory */
    free(tp);

    return ESP_OK;
}

static esp_err_t touch_axs15231b_reset(esp_lcd_touch_handle_t tp)
{
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    return ESP_OK;
}

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, int reg, uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Invalid data");

    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}

static esp_err_t i2c_write_bytes(esp_lcd_touch_handle_t tp, int reg, const uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Invalid data");

    return esp_lcd_panel_io_tx_param(tp->io, reg, data, len);
}

#endif // ESP_PANEL_DRIVERS_TOUCH_ENABLE_AXS15231B

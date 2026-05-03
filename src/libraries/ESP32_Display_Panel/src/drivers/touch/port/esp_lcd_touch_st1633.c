/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../esp_panel_touch_conf_internal.h"
#if ESP_PANEL_DRIVERS_TOUCH_ENABLE_ST1633

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_lcd_touch_st1633.h"

#define FW_VERSION_REG      (0x00)
#define XY_RES_H_REG        (0x04)
#define FW_REVISION_REG     (0x0C)
#define ADVANCED_INFO_REG   (0x10)

#define MAX_TOUCH_NUM       (10)
#define MAX_READ_TOUCH_NUM  (MAX(MAX_TOUCH_NUM, CONFIG_ESP_LCD_TOUCH_MAX_POINTS))

static const char *TAG = "ST1633";

static esp_err_t read_data(esp_lcd_touch_handle_t tp);
static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t del(esp_lcd_touch_handle_t tp);
static esp_err_t reset(esp_lcd_touch_handle_t tp);

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);
static esp_err_t read_fw_info(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_st1633(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *tp)
{
    ESP_LOGI(TAG, "version: %d.%d.%d", ESP_LCD_TOUCH_ST1633_VER_MAJOR, ESP_LCD_TOUCH_ST1633_VER_MINOR,
             ESP_LCD_TOUCH_ST1633_VER_PATCH);
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "Invalid io");
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "Invalid touch handle");

    /* Prepare main structure */
    esp_err_t ret = ESP_OK;
    esp_lcd_touch_handle_t st1633 = (esp_lcd_touch_handle_t)calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(st1633, ESP_ERR_NO_MEM, err, TAG, "Touch handle malloc failed");

    /* Communication interface */
    st1633->io = io;
    /* Only supported callbacks are set */
    st1633->read_data = read_data;
    st1633->get_xy = get_xy;
    st1633->del = del;
    /* Mutex */
    st1633->data.lock.owner = portMUX_FREE_VAL;
    /* Save config */
    memcpy(&st1633->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch interrupt */
    if (config->int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = (config->levels.interrupt) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE,
            .pin_bit_mask = BIT64(config->int_gpio_num)
        };
        ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO intr config failed");

        /* Register interrupt callback */
        if (config->interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(st1633, config->interrupt_callback);
        }
    }
    /* Prepare pin for touch controller reset */
    if (config->rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(config->rst_gpio_num)
        };
        ESP_GOTO_ON_ERROR(gpio_config(&rst_gpio_config), err, TAG, "GPIO reset config failed");
    }
    /* Reset controller */
    ESP_GOTO_ON_ERROR(reset(st1633), err, TAG, "Reset failed");
    ESP_GOTO_ON_ERROR(read_fw_info(st1633), err, TAG, "Read version failed");

    ESP_LOGI(TAG, "Touch panel create success, version: %d.%d.%d", ESP_LCD_TOUCH_ST1633_VER_MAJOR,
             ESP_LCD_TOUCH_ST1633_VER_MINOR, ESP_LCD_TOUCH_ST1633_VER_PATCH);

    *tp = st1633;

    return ESP_OK;
err:
    if (st1633) {
        del(st1633);
    }
    ESP_LOGE(TAG, "Initialization failed!");
    return ret;
}

static esp_err_t read_data(esp_lcd_touch_handle_t tp)
{
    typedef struct {
        uint8_t y_h: 3;
        uint8_t reserved_3: 1;
        uint8_t x_h: 3;
        uint8_t valid: 1;
        uint8_t x_l;
        uint8_t y_l;
        uint8_t reserved_24_31;
    } xy_coord_t;
    typedef struct {
        uint8_t gesture_type: 4;
        uint8_t reserved_4: 1;
        uint8_t water_flag: 1;
        uint8_t prox_flag: 1;
        uint8_t reserved_7: 1;
        uint8_t keys;
        xy_coord_t xy_coord[MAX_READ_TOUCH_NUM];
    } tp_report_t;
    static_assert((sizeof(xy_coord_t) == 4) && (sizeof(tp_report_t) == 42), "Invalid size of tp xy_coord_t or_report_t");

    tp_report_t tp_report = { 0 };
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, ADVANCED_INFO_REG, (uint8_t *)&tp_report, sizeof(tp_report)), TAG, "Read advanced info failed");

    uint16_t x = 0;
    uint16_t y = 0;
    portENTER_CRITICAL(&tp->data.lock);
    int j = 0;
    /* Fill all coordinates */
    for (int i = 0; i < MAX_READ_TOUCH_NUM; i++) {
        x = (((uint16_t)tp_report.xy_coord[i].x_h) << 8) | tp_report.xy_coord[i].x_l;
        y = (((uint16_t)tp_report.xy_coord[i].y_h) << 8) | tp_report.xy_coord[i].y_l;
        if (((x == 0) && (y == 0)) || (x > tp->config.x_max) || (y > tp->config.y_max)) {
            continue;
        }
        tp->data.coords[j].x = x;
        tp->data.coords[j].y = y;
        j++;
    }
    /* Expect Number of touched points */
    tp->data.points = j;
    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
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
    /* Clear available touch points count */
    tp->data.points = 0;
    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t del(esp_lcd_touch_handle_t tp)
{
    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }
    /* Release memory */
    free(tp);

    return ESP_OK;
}

static esp_err_t reset(esp_lcd_touch_handle_t tp)
{
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ESP_OK;
}

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE((len == 0) || (data != NULL), ESP_ERR_INVALID_ARG, TAG, "Invalid data");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_rx_param(tp->io, reg, data, len), TAG, "Read param failed");

    return ESP_OK;
}

static esp_err_t read_fw_info(esp_lcd_touch_handle_t tp)
{
    uint8_t version = 0;
    uint8_t revision[4] = { 0 };
    struct {
        uint8_t y_res_h: 3;
        uint8_t reserved_3: 1;
        uint8_t x_res_h: 3;
        uint8_t reserved_7: 1;
        uint8_t x_res_l;
        uint8_t y_res_l;
    } xy_res;
    static_assert(sizeof(xy_res) == 3, "Invalid size of info");

    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, FW_VERSION_REG, &version, 1), TAG, "Read version failed");
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, FW_REVISION_REG, (uint8_t *)&revision[0], sizeof(revision)), TAG, "Read revision failed");
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, XY_RES_H_REG, (uint8_t *)&xy_res, sizeof(xy_res)), TAG, "Read XY Resolution failed");

    ESP_LOGI(TAG, "Firmware version: %d(%d.%d.%d.%d), Max.X: %d, Max.Y: %d", version, revision[3], revision[2],
             revision[1], revision[0], (((uint16_t)xy_res.x_res_h) << 8) | xy_res.x_res_l,
             (((uint16_t)xy_res.y_res_h) << 8) | xy_res.y_res_l);

    return ESP_OK;
}

#endif // ESP_PANEL_DRIVERS_TOUCH_ENABLE_ST1633

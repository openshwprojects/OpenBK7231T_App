/*
 * SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../esp_panel_touch_conf_internal.h"
#if ESP_PANEL_DRIVERS_TOUCH_ENABLE_STMPE610

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_lcd_touch_stmpe610.h"

static const char *TAG = "STMPE610";

/* STMPE610 registers */
#define ESP_LCD_TOUCH_STMPE610_REG_CHIP_ID          (0x00)
#define ESP_LCD_TOUCH_STMPE610_REG_ID_VER           (0x02)
#define ESP_LCD_TOUCH_STMPE610_REG_SYS_CTRL1        (0x03)
#define ESP_LCD_TOUCH_STMPE610_REG_SYS_CTRL2        (0x04)
#define ESP_LCD_TOUCH_STMPE610_REG_SPI_CFG          (0x08)
#define ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL         (0x09)
#define ESP_LCD_TOUCH_STMPE610_REG_INT_EN           (0x0A)
#define ESP_LCD_TOUCH_STMPE610_REG_INT_STA          (0x0B)
#define ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL1        (0x20)
#define ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL2        (0x21)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL         (0x40)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG          (0x41)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_FRACTION_Z   (0x56)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_DATA         (0x57)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_I_DRIVE      (0x58)
#define ESP_LCD_TOUCH_STMPE610_REG_FIFO_TH          (0x4A)
#define ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA         (0x4B)
#define ESP_LCD_TOUCH_STMPE610_REG_FIFO_SIZE        (0x4C)


#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL_EN        (0x01)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL_XYZ       (0x00)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL_XY        (0x02)

#define ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL1_10BIT    (0x00)

#define ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL2_6_5MHZ   (0x02)

#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG_4SAMPLE    (0x80)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG_DELAY_1MS  (0x20)
#define ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG_SETTLE_5MS (0x04)

#define ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA_RESET     (0x01)
#define ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA_EMPTY     (0x20)

#define ESP_LCD_TOUCH_STMPE610_REG_TSC_I_DRIVE_50MA   (0x01)

#define ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL_POL_LOW   (0x00)
#define ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL_EDGE      (0x02)
#define ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL_ENABLE    (0x01)


/*******************************************************************************
* Function definitions
*******************************************************************************/
static esp_err_t esp_lcd_touch_stmpe610_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_stmpe610_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_stmpe610_del(esp_lcd_touch_handle_t tp);

/* I2C read/write */
static esp_err_t touch_stmpe610_read(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t *data, uint8_t len);
static esp_err_t touch_stmpe610_write(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t data);

/* STMPE610 reset and init */
static esp_err_t touch_stmpe610_init(esp_lcd_touch_handle_t tp);
/* Read status and config register */
static esp_err_t touch_stmpe610_read_cfg(esp_lcd_touch_handle_t tp);

static long data_convert(long x, long in_min, long in_max, long out_min, long out_max);

/*******************************************************************************
* Public API functions
*******************************************************************************/

esp_err_t esp_lcd_touch_new_spi_stmpe610(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch)
{
    ESP_LOGI(TAG, "version: %d.%d.%d", ESP_LCD_TOUCH_STMPE610_VER_MAJOR, ESP_LCD_TOUCH_STMPE610_VER_MINOR,
             ESP_LCD_TOUCH_STMPE610_VER_PATCH);
    esp_err_t ret = ESP_OK;

    assert(io != NULL);
    assert(config != NULL);
    assert(out_touch != NULL);

    /* Prepare main structure */
    // Use `calloc` instead of `heap_caps_calloc` for MicroPython compatibility
    esp_lcd_touch_handle_t esp_lcd_touch_stmpe610 = calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(esp_lcd_touch_stmpe610, ESP_ERR_NO_MEM, err, TAG, "no mem for STMPE610 controller");

    /* Communication interface */
    esp_lcd_touch_stmpe610->io = io;

    /* Only supported callbacks are set */
    esp_lcd_touch_stmpe610->read_data = esp_lcd_touch_stmpe610_read_data;
    esp_lcd_touch_stmpe610->get_xy = esp_lcd_touch_stmpe610_get_xy;
    esp_lcd_touch_stmpe610->del = esp_lcd_touch_stmpe610_del;

    /* Mutex */
    esp_lcd_touch_stmpe610->data.lock.owner = portMUX_FREE_VAL;

    /* Save config */
    memcpy(&esp_lcd_touch_stmpe610->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch interrupt */
    if (esp_lcd_touch_stmpe610->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = (esp_lcd_touch_stmpe610->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
            .pin_bit_mask = BIT64(esp_lcd_touch_stmpe610->config.int_gpio_num)
        };
        ret = gpio_config(&int_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");

        /* Register interrupt callback */
        if (esp_lcd_touch_stmpe610->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(esp_lcd_touch_stmpe610, esp_lcd_touch_stmpe610->config.interrupt_callback);
        }
    }

    /* Prepare pin for touch controller reset */
    if (esp_lcd_touch_stmpe610->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(esp_lcd_touch_stmpe610->config.rst_gpio_num)
        };
        ret = gpio_config(&rst_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");
    }

    /* Reset and init controller */
    ret = touch_stmpe610_init(esp_lcd_touch_stmpe610);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "STMPE610 reset failed");

    /* Read status and config info */
    ret = touch_stmpe610_read_cfg(esp_lcd_touch_stmpe610);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "STMPE610 init failed");

    *out_touch = esp_lcd_touch_stmpe610;

err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (0x%x)! Touch controller STMPE610 initialization failed!", ret);
        if (esp_lcd_touch_stmpe610) {
            esp_lcd_touch_stmpe610_del(esp_lcd_touch_stmpe610);
        }
    }

    return ret;
}

static esp_err_t esp_lcd_touch_stmpe610_read_data(esp_lcd_touch_handle_t tp)
{
    uint8_t buf[100];
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t z = 0;
    uint8_t cnt = 0;
    uint8_t fifo_status = 0;

    assert(tp != NULL);

    /* Read fifo status */
    ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA, (uint8_t *)&fifo_status, 1), TAG, "STMPE610 read error!");
    if (fifo_status & ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA_EMPTY) {
        return ESP_OK;
    }

    /* Read count of samples */
    ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_SIZE, (uint8_t *)&cnt, 1), TAG, "STMPE610 read error!");
    if (cnt == 0) {
        return ESP_OK;
    }

    for (int i = 0; i < cnt; i++) {
        /* Read XYZ data */
        //Note: There is not working read 4 bytes in one read. It reads bad data.
        ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_DATA, (uint8_t *)&buf[0], 1), TAG, "STMPE610 read error!");
        ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_DATA, (uint8_t *)&buf[1], 1), TAG, "STMPE610 read error!");
        ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_DATA, (uint8_t *)&buf[2], 1), TAG, "STMPE610 read error!");
        ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_DATA, (uint8_t *)&buf[3], 1), TAG, "STMPE610 read error!");

        /* Get XYZ from buffer data */
        x += (uint16_t)(((uint16_t)buf[0] << 4) | ((buf[1] >> 4) & 0x0F));
        y += (uint16_t)(((uint16_t)(buf[1] & 0x0F) << 8) | buf[2]);
        z += buf[3];
    }

    /* Reset FIFO */
    ESP_RETURN_ON_ERROR(touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA_RESET), TAG, "STMPE610 write error!");
    ESP_RETURN_ON_ERROR(touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA, 0), TAG, "STMPE610 write error!");
    /* Reset all ints */
    ESP_RETURN_ON_ERROR(touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_INT_STA, 0xFF), TAG, "STMPE610 write error!");

    portENTER_CRITICAL(&tp->data.lock);
    tp->data.coords[0].x = data_convert(x / cnt, 150, 3800, 0, tp->config.x_max);
    tp->data.coords[0].y = data_convert(y / cnt, 150, 3800, 0, tp->config.y_max);
    tp->data.coords[0].strength = z / cnt;
    tp->data.points = 1;
    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool esp_lcd_touch_stmpe610_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    assert(tp != NULL);
    assert(x != NULL);
    assert(y != NULL);
    assert(point_num != NULL);
    assert(max_point_num > 0);

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

static esp_err_t esp_lcd_touch_stmpe610_del(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }

    /* Reset GPIO pin settings */
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);

    return ESP_OK;
}

/*******************************************************************************
* Private API function
*******************************************************************************/

/* Reset controller */
static esp_err_t touch_stmpe610_init(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level error!");
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level error!");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Soft reset */
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_SYS_CTRL1, 0x02);

    /* Turn on clocks */
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_SYS_CTRL2, 0x0);

    /* XYZ and enable */
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL, ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL_XYZ | ESP_LCD_TOUCH_STMPE610_REG_TSC_CTRL_EN);
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_INT_EN, 0x01);

    /* 96 clocks per conversion */
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL1, ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL1_10BIT | (0x6 << 4));
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL2, ESP_LCD_TOUCH_STMPE610_REG_ADC_CTRL2_6_5MHZ);

    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG, ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG_4SAMPLE | ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG_DELAY_1MS | ESP_LCD_TOUCH_STMPE610_REG_TSC_CFG_SETTLE_5MS);
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_FRACTION_Z, 0x6);
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_TH, 1);

    /* Reset FIFO */
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA_RESET);
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_FIFO_STA, 0);

    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_TSC_I_DRIVE, ESP_LCD_TOUCH_STMPE610_REG_TSC_I_DRIVE_50MA);

    /* Reset all ints */
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_INT_STA, 0xFF);
    touch_stmpe610_write(tp, ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL, ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL_POL_LOW | ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL_EDGE | ESP_LCD_TOUCH_STMPE610_REG_INT_CTRL_ENABLE);

    return ESP_OK;
}

static esp_err_t touch_stmpe610_read_cfg(esp_lcd_touch_handle_t tp)
{
    uint8_t buf[3];
    uint16_t touch_id = 0;

    assert(tp != NULL);

    memset(buf, 0, sizeof(buf));

    /* Read chip ID */
    ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_CHIP_ID, (uint8_t *)&buf[0], 1), TAG, "STMPE610 read error!");
    ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_CHIP_ID + 1, (uint8_t *)&buf[1], 1), TAG, "STMPE610 read error!");
    /* Read chip version */
    ESP_RETURN_ON_ERROR(touch_stmpe610_read(tp, ESP_LCD_TOUCH_STMPE610_REG_ID_VER, (uint8_t *)&buf[2], 1), TAG, "STMPE610 read error!");

    touch_id = (uint16_t)(buf[0] << 8 | buf[1]);

    ESP_LOGI(TAG, "TouchPad ID: 0x%04x", touch_id);
    ESP_LOGI(TAG, "TouchPad Ver: 0x%02x", buf[2]);

    if (touch_id != 0x0811) {
        ESP_LOGE(TAG, "TouchPad ID is not right! It should be 0x0811!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t touch_stmpe610_read(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t *data, uint8_t len)
{
    assert(tp != NULL);
    assert(data != NULL);

    /* Read data */
    return esp_lcd_panel_io_rx_param(tp->io, (0x80 | reg), data, len);
}

static esp_err_t touch_stmpe610_write(esp_lcd_touch_handle_t tp, uint8_t reg, uint8_t data)
{
    assert(tp != NULL);

    // *INDENT-OFF*
    /* Write data */
    return esp_lcd_panel_io_tx_param(tp->io, reg, (uint8_t[]){data}, 1);
    // *INDENT-ON*
}

static long data_convert(long x, long in_min, long in_max, long out_min, long out_max)
{
    if (x < in_min) {
        x = in_min;
    }
    if (x > in_max) {
        x = in_max;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif // ESP_PANEL_DRIVERS_TOUCH_ENABLE_STMPE610

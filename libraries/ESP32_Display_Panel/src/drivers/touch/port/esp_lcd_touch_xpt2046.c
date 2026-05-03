/*
 * SPDX-FileCopyrightText: 2022 atanisoft (github.com/atanisoft)
 *
 * SPDX-License-Identifier: MIT
 */

#include "../esp_panel_touch_conf_internal.h"
#if ESP_PANEL_DRIVERS_TOUCH_ENABLE_XPT2046

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_lcd_panel_io.h>
#include <esp_rom_gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// This must be included after FreeRTOS includes due to missing include
// for portMUX_TYPE
#include "esp_lcd_touch.h"
#include <memory.h>

#include "sdkconfig.h"

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_lcd_touch_xpt2046.h"

static const char *TAG = "xpt2046";

#define CONFIG_XPT2046_Z_THRESHOLD              (ESP_PANEL_DRIVERS_TOUCH_XPT2046_Z_THRESHOLD)
#if ESP_PANEL_DRIVERS_TOUCH_XPT2046_INTERRUPT_MODE
#define CONFIG_XPT2046_INTERRUPT_MODE           (ESP_PANEL_DRIVERS_TOUCH_XPT2046_INTERRUPT_MODE)
#endif
#if ESP_PANEL_DRIVERS_TOUCH_XPT2046_VREF_ON_MODE
#define CONFIG_XPT2046_VREF_ON_MODE             (ESP_PANEL_DRIVERS_TOUCH_XPT2046_VREF_ON_MODE)
#endif
#define CONFIG_XPT2046_CONVERT_ADC_TO_COORDS    (ESP_PANEL_DRIVERS_TOUCH_XPT2046_CONVERT_ADC_TO_COORDS)

#ifdef CONFIG_XPT2046_INTERRUPT_MODE
#define XPT2046_PD0_BIT       (0x00)
#else
#define XPT2046_PD0_BIT       (0x01)
#endif

#ifdef CONFIG_XPT2046_VREF_ON_MODE
#define XPT2046_PD1_BIT   (0x02)
#else
#define XPT2046_PD1_BIT   (0x00)
#endif

#define XPT2046_PD_BITS       (XPT2046_PD1_BIT | XPT2046_PD0_BIT)

enum xpt2046_registers {
    // START  ADDR  MODE    SET/  VREF    ADC (PENIRQ)
    //              12/8b   DFR   INT/EXT ENA
    Z_VALUE_1   = 0xB0 | XPT2046_PD_BITS, // 1      011   0       0     X       X
    Z_VALUE_2   = 0xC0 | XPT2046_PD_BITS, // 1      100   0       0     X       X
    Y_POSITION  = 0x90 | XPT2046_PD_BITS, // 1      001   0       0     X       X
    X_POSITION  = 0xD0 | XPT2046_PD_BITS, // 1      101   0       0     X       X
    BATTERY     = 0xA6 | XPT2046_PD_BITS, // 1      010   0       1     1       X
    AUX_IN      = 0xE6 | XPT2046_PD_BITS, // 1      110   0       1     1       X
    TEMP0       = 0x86 | XPT2046_PD_BITS, // 1      000   0       1     1       X
    TEMP1       = 0xF6 | XPT2046_PD_BITS, // 1      111   0       1     1       X
};

#if CONFIG_XPT2046_ENABLE_LOCKING
#define XPT2046_LOCK(lock) portENTER_CRITICAL(lock)
#define XPT2046_UNLOCK(lock) portEXIT_CRITICAL(lock)
#else
#define XPT2046_LOCK(lock)
#define XPT2046_UNLOCK(lock)
#endif

static const uint16_t XPT2046_ADC_LIMIT = 4096;
// refer the TSC2046 datasheet https://www.ti.com/lit/ds/symlink/tsc2046.pdf rev F 2008
// TEMP0 reads approx 599.5 mV at 25C (Refer p8 TEMP0 diode voltage vs Vcc chart)
// Vref is approx 2.507V = 2507mV at moderate temperatures (refer p8 Vref vs Temperature chart)
// counts@25C = TEMP0_mV / Vref_mv * XPT2046_ADC_LIMIT
static const float XPT2046_TEMP0_COUNTS_AT_25C = (599.5 / 2507 * XPT2046_ADC_LIMIT);
static esp_err_t xpt2046_read_data(esp_lcd_touch_handle_t tp);
static bool xpt2046_get_xy(esp_lcd_touch_handle_t tp,
                           uint16_t *x, uint16_t *y,
                           uint16_t *strength,
                           uint8_t *point_num,
                           uint8_t max_point_num);
static esp_err_t xpt2046_del(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_spi_xpt2046(const esp_lcd_panel_io_handle_t io,
                                        const esp_lcd_touch_config_t *config,
                                        esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;
    esp_lcd_touch_handle_t handle = NULL;

    ESP_LOGI(TAG, "version: %d.%d.%d", ESP_LCD_TOUCH_XPT2046_VER_MAJOR, ESP_LCD_TOUCH_XPT2046_VER_MINOR,
             ESP_LCD_TOUCH_XPT2046_VER_PATCH);
    ESP_GOTO_ON_FALSE(io, ESP_ERR_INVALID_ARG, err, TAG,
                      "esp_lcd_panel_io_handle_t must not be NULL");
    ESP_GOTO_ON_FALSE(config, ESP_ERR_INVALID_ARG, err, TAG,
                      "esp_lcd_touch_config_t must not be NULL");

    handle = (esp_lcd_touch_handle_t)calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(handle, ESP_ERR_NO_MEM, err, TAG,
                      "No memory available for XPT2046 state");
    handle->io = io;
    handle->read_data = xpt2046_read_data;
    handle->get_xy = xpt2046_get_xy;
    handle->del = xpt2046_del;
    handle->data.lock.owner = portMUX_FREE_VAL;
    memcpy(&handle->config, config, sizeof(esp_lcd_touch_config_t));

    if (config->int_gpio_num != GPIO_NUM_NC) {
        ESP_GOTO_ON_FALSE(GPIO_IS_VALID_GPIO(config->int_gpio_num),
                          ESP_ERR_INVALID_ARG, err, TAG, "Invalid GPIO Interrupt Pin");
        gpio_config_t cfg;
        memset(&cfg, 0, sizeof(gpio_config_t));
        esp_rom_gpio_pad_select_gpio(config->int_gpio_num);
        cfg.pin_bit_mask = BIT64(config->int_gpio_num);
        cfg.mode = GPIO_MODE_INPUT;
        cfg.intr_type = (config->levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE);

        // If the user has provided a callback routine for the interrupt enable
        // the interrupt mode on the negative edge.
        if (config->interrupt_callback) {
            cfg.intr_type = GPIO_INTR_NEGEDGE;
        }

        ESP_GOTO_ON_ERROR(gpio_config(&cfg), err, TAG,
                          "Configure GPIO for Interrupt failed");

        // Connect the user interrupt callback routine.
        if (config->interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(handle, config->interrupt_callback);
        }
    }

    *out_touch = handle;

err:
    if (ret != ESP_OK) {
        if (handle) {
            xpt2046_del(handle);
            handle = NULL;
        }
    }

    return ret;
}

static esp_err_t xpt2046_del(esp_lcd_touch_handle_t tp)
{
    if (tp != NULL) {
        if (tp->config.int_gpio_num != GPIO_NUM_NC) {
            gpio_reset_pin(tp->config.int_gpio_num);
        }
    }
    free(tp);

    return ESP_OK;
}

static inline esp_err_t xpt2046_read_register(esp_lcd_touch_handle_t tp, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2] = {0, 0};
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_rx_param(tp->io, reg, buf, 2), TAG, "XPT2046 read error!");
    *value = ((buf[0] << 8) | (buf[1]));
    return ESP_OK;
}

static esp_err_t xpt2046_read_data(esp_lcd_touch_handle_t tp)
{
    uint16_t z1 = 0, z2 = 0, z = 0;
    uint32_t x = 0, y = 0;
    uint8_t point_count = 0;

#ifdef CONFIG_XPT2046_INTERRUPT_MODE
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        // Check the PENIRQ pin to see if there is a touch
        if (gpio_get_level(tp->config.int_gpio_num)) {
            XPT2046_LOCK(&tp->data.lock);
            tp->data.coords[0].x = 0;
            tp->data.coords[0].y = 0;
            tp->data.coords[0].strength = 0;
            tp->data.points = 0;
            XPT2046_UNLOCK(&tp->data.lock);

            return ESP_OK;
        }
    }
#endif

    ESP_RETURN_ON_ERROR(xpt2046_read_register(tp, Z_VALUE_1, &z1), TAG, "XPT2046 read error!");
    ESP_RETURN_ON_ERROR(xpt2046_read_register(tp, Z_VALUE_2, &z2), TAG, "XPT2046 read error!");

    // Convert the received values into a Z value.
    z = (z1 >> 3) + (XPT2046_ADC_LIMIT - (z2 >> 3));

    // If the Z (pressure) exceeds the threshold it is likely the user has
    // pressed the screen, read in and average the positions.
    if (z >= CONFIG_XPT2046_Z_THRESHOLD) {
        uint16_t discard_buf = 0;

        // read and discard a value as it is usually not reliable.
        ESP_RETURN_ON_ERROR(xpt2046_read_register(tp, X_POSITION, &discard_buf),
                            TAG, "XPT2046 read error!");

        for (uint8_t idx = 0; idx < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; idx++) {
            uint16_t x_temp = 0;
            uint16_t y_temp = 0;
            // Read X position and convert returned data to 12bit value
            ESP_RETURN_ON_ERROR(xpt2046_read_register(tp, X_POSITION, &x_temp),
                                TAG, "XPT2046 read error!");
            // drop lowest three bits to convert to 12-bit position
            x_temp >>= 3;

            // Read Y position and convert returned data to 12bit value
            ESP_RETURN_ON_ERROR(xpt2046_read_register(tp, Y_POSITION, &y_temp),
                                TAG, "XPT2046 read error!");
            // drop lowest three bits to convert to 12-bit position
            y_temp >>= 3;

            // Test if the readings are valid (50 < reading < max - 50)
            if ((x_temp >= 50) && (x_temp <= XPT2046_ADC_LIMIT - 50) && (y_temp >= 50) && (y_temp <= XPT2046_ADC_LIMIT - 50)) {
#if CONFIG_XPT2046_CONVERT_ADC_TO_COORDS
                // Convert the raw ADC value into a screen coordinate and store it
                // for averaging.
                x += ((x_temp / (double)XPT2046_ADC_LIMIT) * tp->config.x_max);
                y += ((y_temp / (double)XPT2046_ADC_LIMIT) * tp->config.y_max);
#else
                // store the raw ADC values and let the user convert them to screen
                // coordinates.
                x += x_temp;
                y += y_temp;
#endif // CONFIG_XPT2046_CONVERT_ADC_TO_COORDS
                point_count++;
            }
        }

        // Check we had enough valid values
        const int minimum_count = (1 == CONFIG_ESP_LCD_TOUCH_MAX_POINTS ? 1 : CONFIG_ESP_LCD_TOUCH_MAX_POINTS / 2);
        if (point_count >= minimum_count) {
            // Average the accumulated coordinate data points.
            x /= point_count;
            y /= point_count;
            point_count = 1;
        } else {
            z = 0;
            point_count = 0;
        }
    }

    XPT2046_LOCK(&tp->data.lock);
    tp->data.coords[0].x = x;
    tp->data.coords[0].y = y;
    tp->data.coords[0].strength = z;
    tp->data.points = point_count;
    XPT2046_UNLOCK(&tp->data.lock);

    return ESP_OK;
}

static bool xpt2046_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                           uint16_t *strength, uint8_t *point_num,
                           uint8_t max_point_num)
{
    XPT2046_LOCK(&tp->data.lock);

    // Determine how many touch points that are available.
    if (tp->data.points > max_point_num) {
        *point_num = max_point_num;
    } else {
        *point_num = tp->data.points;
    }

    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    // Invalidate stored touch data.
    tp->data.points = 0;

    XPT2046_UNLOCK(&tp->data.lock);

    if (*point_num) {
        ESP_LOGD(TAG, "Touch point: %dx%d", x[0], y[0]);
    } else {
        ESP_LOGV(TAG, "No touch points");
    }

    return (*point_num > 0);
}

esp_err_t esp_lcd_touch_xpt2046_read_battery_level(const esp_lcd_touch_handle_t handle, float *output)
{
    uint16_t level;
#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // First read is to turn on the Vref, so it has extra time to stabilise before we read it for real
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, BATTERY, &level), TAG, "XPT2046 read error!");
#endif
    // Read the battery voltage
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, BATTERY, &level), TAG, "XPT2046 read error!");
    // drop lowest three bits to convert to 12-bit value
    level >>= 3;

    // battery voltage is reported as 1/4 the actual voltage due to logic in
    // the chip.
    *output = level * 4.0;

    // adjust for internal vref of 2.5v
    *output *= 2.507f;

    // adjust for ADC bit count
    *output /= 4096.0f;

#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // Final read is to turn the Vref off
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, Z_VALUE_1, &level), TAG, "XPT2046 read error!");
#endif

    return ESP_OK;
}

esp_err_t esp_lcd_touch_xpt2046_read_aux_level(const esp_lcd_touch_handle_t handle, float *output)
{
    uint16_t level;
#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // First read is to turn on the Vref, so it has extra time to stabilise before we read it for real
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, AUX_IN, &level), TAG, "XPT2046 read error!");
#endif
    // Read the aux input voltage
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, AUX_IN, &level), TAG, "XPT2046 read error!");
    // drop lowest three bits to convert to 12-bit value
    level >>= 3;
    *output = level;

    // adjust for internal vref of 2.5v
    *output *= 2.507f;

    // adjust for ADC bit count
    *output /= 4096.0f;

#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // Final read is to turn on the ADC and the Vref off
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, Z_VALUE_1, &level), TAG, "XPT2046 read error!");
#endif

    return ESP_OK;
}

esp_err_t esp_lcd_touch_xpt2046_read_temp0_level(const esp_lcd_touch_handle_t handle, float *output)
{
    uint16_t temp0;
#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // First read is to turn on the Vref, so it has extra time to stabilise before we read it for real
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, TEMP0, &temp0), TAG, "XPT2046 read error!");
#endif
    // Read the temp0 value
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, TEMP0, &temp0), TAG, "XPT2046 read error!");
    // drop lowest three bits to convert to 12-bit value
    temp0 >>= 3;
    *output = temp0;
    // Convert to temperature in degrees C
    *output = (XPT2046_TEMP0_COUNTS_AT_25C - *output) * (2.507 / 4096.0) / 0.0021 + 25.0;

#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // Final read is to turn on the ADC and the Vref off
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, Z_VALUE_1, &temp0), TAG, "XPT2046 read error!");
#endif

    return ESP_OK;
}

esp_err_t esp_lcd_touch_xpt2046_read_temp1_level(const esp_lcd_touch_handle_t handle, float *output)
{
    uint16_t temp0;
    uint16_t temp1;
#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // First read is to turn on the Vref, so it has extra time to stabilise before we read it for real
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, TEMP0, &temp0), TAG, "XPT2046 read error!");
#endif
    // Read the temp0 value
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, TEMP0, &temp0), TAG, "XPT2046 read error!");
    // Read the temp1 value
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, TEMP1, &temp1), TAG, "XPT2046 read error!");
    // drop lowest three bits to convert to 12-bit value
    temp0 >>= 3;
    temp1 >>= 3;
    *output = temp1 - temp0;
    *output = *output * 1000.0 * (2.507 / 4096.0) * 2.573 - 273.0;

#ifndef CONFIG_XPT2046_VREF_ON_MODE
    // Final read is to turn on the ADC and the Vref off
    ESP_RETURN_ON_ERROR(xpt2046_read_register(handle, Z_VALUE_1, &temp0), TAG, "XPT2046 read error!");
#endif

    return ESP_OK;
}

#endif // ESP_PANEL_DRIVERS_TOUCH_ENABLE_XPT2046

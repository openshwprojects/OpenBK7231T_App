/*
 * SPDX-FileCopyrightText: 2022 atanisoft (github.com/atanisoft)
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "esp_idf_version.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LCD_TOUCH_XPT2046_VER_MAJOR    (1)
#define ESP_LCD_TOUCH_XPT2046_VER_MINOR    (0)
#define ESP_LCD_TOUCH_XPT2046_VER_PATCH    (5)

/**
 * @brief Recommended clock for SPI read of the XPT2046
 *
 */
#define ESP_LCD_TOUCH_SPI_CLOCK_HZ   (1 * 1000 * 1000)

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(touch_cs)   \
    {                                                   \
        .cs_gpio_num = (gpio_num_t)touch_cs,            \
        .dc_gpio_num = GPIO_NUM_NC,                     \
        .spi_mode = 0,                                  \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,          \
        .trans_queue_depth = 3,                         \
        .on_color_trans_done = NULL,                    \
        .user_ctx = NULL,                               \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
        .flags =                                        \
        {                                               \
            .dc_as_cmd_phase = 0,                       \
            .dc_low_on_data = 0,                        \
            .octal_mode = 0,                            \
            .lsb_first = 0                              \
        }                                               \
    }
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,1,0)

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(touch_cs)   \
    {                                                   \
        .cs_gpio_num = (gpio_num_t)touch_cs,            \
        .dc_gpio_num = GPIO_NUM_NC,                     \
        .spi_mode = 0,                                  \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,          \
        .trans_queue_depth = 3,                         \
        .on_color_trans_done = NULL,                    \
        .user_ctx = NULL,                               \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
        .flags =                                        \
        {                                               \
            .dc_low_on_data = 0,                        \
            .octal_mode = 0,                            \
            .lsb_first = 0                              \
        }                                               \
    }
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,1,2)

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(touch_cs)   \
    {                                                   \
        .cs_gpio_num = (gpio_num_t)touch_cs,            \
        .dc_gpio_num = GPIO_NUM_NC,                     \
        .spi_mode = 0,                                  \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,          \
        .trans_queue_depth = 3,                         \
        .on_color_trans_done = NULL,                    \
        .user_ctx = NULL,                               \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
        .flags =                                        \
        {                                               \
            .dc_low_on_data = 0,                        \
            .octal_mode = 0,                            \
            .sio_mode = 0,                              \
            .lsb_first = 0,                             \
            .cs_high_active = 0                         \
        }                                               \
    }
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,1,3)

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(touch_cs)   \
    {                                                   \
        .cs_gpio_num = (gpio_num_t)touch_cs,            \
        .dc_gpio_num = GPIO_NUM_NC,                     \
        .spi_mode = 0,                                  \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,          \
        .trans_queue_depth = 3,                         \
        .on_color_trans_done = NULL,                    \
        .user_ctx = NULL,                               \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
        .flags =                                        \
        {                                               \
            .dc_low_on_data = 0,                        \
            .octal_mode = 0,                            \
            .quad_mode = 0,                             \
            .sio_mode = 0,                              \
            .lsb_first = 0,                             \
            .cs_high_active = 0                         \
        }                                               \
    }
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,4,0)

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(touch_cs)   \
    {                                                   \
        .cs_gpio_num = (gpio_num_t)touch_cs,            \
        .dc_gpio_num = GPIO_NUM_NC,                     \
        .spi_mode = 0,                                  \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,          \
        .trans_queue_depth = 3,                         \
        .on_color_trans_done = NULL,                    \
        .user_ctx = NULL,                               \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
        .flags =                                        \
        {                                               \
            .dc_high_on_cmd = 0,                        \
            .dc_low_on_data = 0,                        \
            .dc_low_on_param = 0,                        \
            .octal_mode = 0,                            \
            .quad_mode = 0,                             \
            .sio_mode = 0,                              \
            .lsb_first = 0,                             \
            .cs_high_active = 0                         \
        }                                               \
    }
#else

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(touch_cs)   \
    {                                                   \
        .cs_gpio_num = (gpio_num_t)touch_cs,            \
        .dc_gpio_num = GPIO_NUM_NC,                     \
        .spi_mode = 0,                                  \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,          \
        .trans_queue_depth = 3,                         \
        .on_color_trans_done = NULL,                    \
        .user_ctx = NULL,                               \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
        .cs_ena_pretrans = 0,                           \
        .cs_ena_posttrans = 0,                          \
        .flags =                                        \
        {                                               \
            .dc_high_on_cmd = 0,                        \
            .dc_low_on_data = 0,                        \
            .dc_low_on_param = 0,                        \
            .octal_mode = 0,                            \
            .quad_mode = 0,                             \
            .sio_mode = 0,                              \
            .lsb_first = 0,                             \
            .cs_high_active = 0                         \
        }                                               \
    }
#endif // IDF v5.1.3

/**
 * @brief Create a new XPT2046 touch driver
 *
 * @note The SPI communication should be initialized before use this function.
 *
 * @param io: LCD/Touch panel IO handle.
 * @param config: Touch configuration.
 * @param out_touch: XPT2046 instance handle.
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is insufficient memory for allocating main structure.
 *      - ESP_ERR_INVALID_ARG       if @param io or @param config are null.
 */
esp_err_t esp_lcd_touch_new_spi_xpt2046(const esp_lcd_panel_io_handle_t io,
                                        const esp_lcd_touch_config_t *config,
                                        esp_lcd_touch_handle_t *out_touch);

/**
 * @brief Reads the voltage from the v-bat pin of the XPT2046.
 *
 * @param handle: XPT2046 instance handle.
 * @param out_level: Approximate voltage read in from the v-bat pin.
 * @return
 *      - ESP_OK on success, otherwise returns ESP_ERR_xxx
 *
 * @note The v-bat pin has a voltage range of 0.0 to 6.0 volts.
 */
esp_err_t esp_lcd_touch_xpt2046_read_battery_level(const esp_lcd_touch_handle_t handle, float *out_level);

/**
 * @brief Reads the voltage from the aux pin of the XPT2046.
 *
 * @param handle: XPT2046 instance handle.
 * @param out_level: Approximate voltage read in from the aux pin.
 * @return
 *      - ESP_OK on success, otherwise returns ESP_ERR_xxx
 *
 * @note The aux pin has a voltage range of 0.0 to 2.5 volts.
 */
esp_err_t esp_lcd_touch_xpt2046_read_aux_level(const esp_lcd_touch_handle_t handle, float *out_level);

/**
 * @brief Reads the temperature from the XPT2046 using a one-point reading.
 *        High precision (0.3 degrees C) but low accuracy requires a
 *        calibration offset for accurate results.
 *
 * @param handle: XPT2046 instance handle.
 * @param out_level: Approximate temperature of the TSC2046 in degrees C
 * @return
 *      - ESP_OK on success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_lcd_touch_xpt2046_read_temp0_level(const esp_lcd_touch_handle_t handle, float *output);

/**
 * @brief Reads the temperature from the XPT2046 using a two-point reading.
 *        Low precision (1.6 degrees C) but high accuracy requires no calibration.
 *
 * @param handle: XPT2046 instance handle.
 * @param out_level: Approximate temperature of the TSC2046 in degrees C
 * @return
 *      - ESP_OK on success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_lcd_touch_xpt2046_read_temp1_level(const esp_lcd_touch_handle_t handle, float *output);

#ifdef __cplusplus
}
#endif

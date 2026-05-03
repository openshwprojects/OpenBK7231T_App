/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LCD_HX8399_VER_MAJOR    (1)
#define ESP_LCD_HX8399_VER_MINOR    (0)
#define ESP_LCD_HX8399_VER_PATCH    (1)

/**
 * @brief Create LCD panel for model HX8399
 *
 * @note  Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for initialization sequence code.
 *
 * @param[in]  io LCD panel IO handle
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *      - ESP_ERR_INVALID_ARG   if parameter is invalid
 *      - ESP_OK                on success
 *      - Otherwise             on fail
 */
esp_err_t esp_lcd_new_panel_hx8399(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief MIPI DSI bus configuration structure
 *
 */
#define HX8399_PANEL_BUS_DSI_2CH_CONFIG()                 \
    {                                                     \
        .bus_id = 0,                                      \
        .num_data_lanes = 2,                              \
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,      \
        .lane_bit_rate_mbps = 950,                        \
    }

/**
 * @brief MIPI DBI panel IO configuration structure
 *
 */
#define HX8399_PANEL_IO_DBI_CONFIG() \
    {                                 \
        .virtual_channel = 0,         \
        .lcd_cmd_bits = 8,            \
        .lcd_param_bits = 8,          \
    }

/**
 * @brief MIPI DPI configuration structure
 *
 * @note  refresh_rate = (dpi_clock_freq_mhz * 1000000) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *                                                      / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 *
 */
#define HX8399_1080_1920_PANEL_30HZ_DPI_CONFIG(px_format)            \
    {                                                            \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,             \
        .dpi_clock_freq_mhz = 75,                                \
        .virtual_channel = 0,                                    \
        .pixel_format = px_format,                               \
        .num_fbs = 1,                                            \
        .video_timing = {                                        \
            .h_size = 1080,                                      \
            .v_size = 1920,                                      \
            .hsync_back_porch = 140,                             \
            .hsync_pulse_width = 40,                             \
            .hsync_front_porch = 40,                             \
            .vsync_back_porch = 16,                              \
            .vsync_pulse_width = 4,                              \
            .vsync_front_porch = 16,                             \
        },                                                       \
        .flags.use_dma2d = true,                                 \
    }

#ifdef __cplusplus
}
#endif
#endif // SOC_MIPI_DSI_SUPPORTED

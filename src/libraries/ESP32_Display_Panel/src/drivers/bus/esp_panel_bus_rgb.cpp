/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_bus_conf_internal.h"
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB

#include <cstdlib>
#include <cstring>
#include "esp_lcd_panel_io.h"
#include "utils/esp_panel_utils_log.h"
#include "esp_panel_bus_rgb.hpp"

namespace esp_panel::drivers {

void BusRGB::Config::convertPartialToFull()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (isControlPanelValid() && std::holds_alternative<ControlPanelPartialConfig>(control_panel.value())) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printControlPanelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<ControlPanelPartialConfig>(control_panel.value());
        control_panel =  ControlPanelFullConfig{
            .line_config = {
                .cs_io_type = static_cast<panel_io_type_t>(config.cs_io_type),
                .cs_gpio_num = config.cs_gpio_num,
                .scl_io_type = static_cast<panel_io_type_t>(config.scl_io_type),
                .scl_gpio_num = config.scl_gpio_num,
                .sda_io_type = static_cast<panel_io_type_t>(config.sda_io_type),
                .sda_gpio_num = config.sda_gpio_num,
            },
            .expect_clk_speed = PANEL_IO_SPI_CLK_MAX,
            .spi_mode = static_cast<uint32_t>(config.spi_mode),
            .lcd_cmd_bytes = static_cast<uint32_t>(config.lcd_cmd_bytes),
            .lcd_param_bytes = static_cast<uint32_t>(config.lcd_param_bytes),
            .flags = {
                .use_dc_bit = static_cast<uint32_t>(config.flags_use_dc_bit),
                .dc_zero_on_data = 0,
                .lsb_first = 0,
                .cs_high_active = 0,
                .del_keep_cs_inactive = 1,
            },
        };
    }

    if (std::holds_alternative<RefreshPanelPartialConfig>(refresh_panel)) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printRefreshPanelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<RefreshPanelPartialConfig>(refresh_panel);
        refresh_panel = RefreshPanelFullConfig{
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .timings = {
                .pclk_hz = static_cast<uint32_t>(config.pclk_hz),
                .h_res = static_cast<uint32_t>(config.h_res),
                .v_res = static_cast<uint32_t>(config.v_res),
                .hsync_pulse_width = static_cast<uint32_t>(config.hsync_pulse_width),
                .hsync_back_porch = static_cast<uint32_t>(config.hsync_back_porch),
                .hsync_front_porch = static_cast<uint32_t>(config.hsync_front_porch),
                .vsync_pulse_width = static_cast<uint32_t>(config.vsync_pulse_width),
                .vsync_back_porch = static_cast<uint32_t>(config.vsync_back_porch),
                .vsync_front_porch = static_cast<uint32_t>(config.vsync_front_porch),
                .flags = {
                    .hsync_idle_low = 0,
                    .vsync_idle_low = 0,
                    .de_idle_high = 0,
                    .pclk_active_neg = config.flags_pclk_active_neg,
                    .pclk_idle_high = 0,
                },
            },
            .data_width = static_cast<size_t>(config.data_width),
            .bits_per_pixel = static_cast<size_t>(config.bits_per_pixel),
            .num_fbs = 1,
            .bounce_buffer_size_px = static_cast<size_t>(config.bounce_buffer_size_px),
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
            .dma_burst_size = 64,
#else
            .sram_trans_align = 4,
            .psram_trans_align = 64,
#endif // ESP_IDF_VERSION
            .hsync_gpio_num = config.hsync_gpio_num,
            .vsync_gpio_num = config.vsync_gpio_num,
            .de_gpio_num = config.de_gpio_num,
            .pclk_gpio_num = config.pclk_gpio_num,
            .disp_gpio_num = config.disp_gpio_num,
            .data_gpio_nums = {
                config.data_gpio_nums[0], config.data_gpio_nums[1],
                config.data_gpio_nums[2], config.data_gpio_nums[3],
                config.data_gpio_nums[4], config.data_gpio_nums[5],
                config.data_gpio_nums[6], config.data_gpio_nums[7],
#if ESP_PANEL_BUS_RGB_DATA_BITS >= 16
                config.data_gpio_nums[8], config.data_gpio_nums[9],
                config.data_gpio_nums[10], config.data_gpio_nums[11],
                config.data_gpio_nums[12], config.data_gpio_nums[13],
                config.data_gpio_nums[14], config.data_gpio_nums[15],
#endif // ESP_PANEL_BUS_RGB_DATA_BITS
            },
            .flags = {
                .disp_active_low = 0,
                .refresh_on_demand = 0,
                .fb_in_psram = 1,
                .double_fb = 0,
                .no_fb = 0,
                .bb_invalidate_cache = 0,
            },
        };
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BusRGB::Config::printControlPanelConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (!isControlPanelValid()) {
        ESP_UTILS_LOGI("\n\t{Control panel config}[skipped]");
        goto end;
    }

    if (std::holds_alternative<ControlPanelFullConfig>(control_panel.value())) {
        auto &config = std::get<ControlPanelFullConfig>(control_panel.value());
        ESP_UTILS_LOGI(
            "\n\t{Control panel config}[full]"
            "\n\t\t-> {line_config}"
            "\n\t\t\t-> [cs_io_type]: %d"
            "\n\t\t\t-> [cs_gpio_num]: %d"
            "\n\t\t\t-> [scl_io_type]: %d"
            "\n\t\t\t-> [scl_gpio_num]: %d"
            "\n\t\t\t-> [sda_io_type]: %d"
            "\n\t\t\t-> [sda_gpio_num]: %d"
            , static_cast<int>(config.line_config.cs_io_type)
            , static_cast<int>(config.line_config.cs_gpio_num)
            , static_cast<int>(config.line_config.scl_io_type)
            , static_cast<int>(config.line_config.scl_gpio_num)
            , static_cast<int>(config.line_config.sda_io_type)
            , static_cast<int>(config.line_config.sda_gpio_num)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> [expect_clk_speed]: %d"
            "\n\t\t-> [spi_mode]: %d"
            "\n\t\t-> [lcd_cmd_bytes]: %d"
            "\n\t\t-> [lcd_param_bytes]: %d"
            , static_cast<int>(config.expect_clk_speed)
            , static_cast<int>(config.spi_mode)
            , static_cast<int>(config.lcd_cmd_bytes)
            , static_cast<int>(config.lcd_param_bytes)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> {flags}"
            "\n\t\t\t-> [use_dc_bit]: %d"
            "\n\t\t\t-> [dc_zero_on_data]: %d"
            "\n\t\t\t-> [lsb_first]: %d"
            "\n\t\t\t-> [cs_high_active]: %d"
            "\n\t\t\t-> [del_keep_cs_inactive]: %d"
            , static_cast<int>(config.flags.use_dc_bit)
            , static_cast<int>(config.flags.dc_zero_on_data)
            , static_cast<int>(config.flags.lsb_first)
            , static_cast<int>(config.flags.cs_high_active)
            , static_cast<int>(config.flags.del_keep_cs_inactive)
        );
    } else {
        auto &config = std::get<ControlPanelPartialConfig>(control_panel.value());
        ESP_UTILS_LOGI(
            "\n\t{Control panel config}[partial]"
            "\n\t\t-> [cs_io_type]: %d"
            "\n\t\t-> [cs_gpio_num]: %d"
            "\n\t\t-> [scl_io_type]: %d"
            "\n\t\t-> [scl_gpio_num]: %d"
            "\n\t\t-> [sda_io_type]: %d"
            "\n\t\t-> [sda_gpio_num]: %d"
            , static_cast<int>(config.cs_io_type)
            , static_cast<int>(config.cs_gpio_num)
            , static_cast<int>(config.scl_io_type)
            , static_cast<int>(config.scl_gpio_num)
            , static_cast<int>(config.sda_io_type)
            , static_cast<int>(config.sda_gpio_num)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> [spi_mode]: %d"
            "\n\t\t-> [lcd_cmd_bytes]: %d"
            "\n\t\t-> [lcd_param_bytes]: %d"
            "\n\t\t-> [flags_use_dc_bit]: %d"
            , static_cast<int>(config.spi_mode)
            , static_cast<int>(config.lcd_cmd_bytes)
            , static_cast<int>(config.lcd_param_bytes)
            , static_cast<int>(config.flags_use_dc_bit)
        );
    }

end:
    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BusRGB::Config::printRefreshPanelConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<RefreshPanelFullConfig>(refresh_panel)) {
        auto &config = std::get<RefreshPanelFullConfig>(refresh_panel);
        ESP_UTILS_LOGI(
            "\n\t{Refresh panel config}[full]"
            "\n\t\t-> [clk_src]: %d"
            , static_cast<int>(config.clk_src)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> {timings}"
            "\n\t\t\t-> [pclk_hz]: %d"
            "\n\t\t\t-> [h_res]: %d"
            "\n\t\t\t-> [v_res]: %d"
            "\n\t\t\t-> [hsync_pulse_width]: %d"
            "\n\t\t\t-> [hsync_back_porch]: %d"
            , static_cast<int>(config.timings.pclk_hz)
            , static_cast<int>(config.timings.h_res)
            , static_cast<int>(config.timings.v_res)
            , static_cast<int>(config.timings.hsync_pulse_width)
            , static_cast<int>(config.timings.hsync_back_porch)
        );
        ESP_UTILS_LOGI(
            "\n\t\t\t-> [hsync_front_porch]: %d"
            "\n\t\t\t-> [vsync_pulse_width]: %d"
            "\n\t\t\t-> [vsync_back_porch]: %d"
            "\n\t\t\t-> [vsync_front_porch]: %d"
            , static_cast<int>(config.timings.hsync_front_porch)
            , static_cast<int>(config.timings.vsync_pulse_width)
            , static_cast<int>(config.timings.vsync_back_porch)
            , static_cast<int>(config.timings.vsync_front_porch)
        );
        ESP_UTILS_LOGI(
            "\n\t\t\t-> {flags}"
            "\n\t\t\t\t-> [hsync_idle_low]: %d"
            "\n\t\t\t\t-> [vsync_idle_low]: %d"
            "\n\t\t\t\t-> [de_idle_high]: %d"
            "\n\t\t\t\t-> [pclk_active_neg]: %d"
            "\n\t\t\t\t-> [pclk_idle_high]: %d"
            , static_cast<int>(config.timings.flags.hsync_idle_low)
            , static_cast<int>(config.timings.flags.vsync_idle_low)
            , static_cast<int>(config.timings.flags.de_idle_high)
            , static_cast<int>(config.timings.flags.pclk_active_neg)
            , static_cast<int>(config.timings.flags.pclk_idle_high)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> [data_width]: %d"
            "\n\t\t-> [bits_per_pixel]: %d"
            "\n\t\t-> [num_fbs]: %d"
            "\n\t\t-> [bounce_buffer_size_px]: %d"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
            "\n\t\t-> [dma_burst_size]: %d"
#else
            "\n\t\t-> [sram_trans_align]: %d"
            "\n\t\t-> [psram_trans_align]: %d"
#endif // ESP_IDF_VERSION
            "\n\t\t-> [hsync_gpio_num]: %d"
            "\n\t\t-> [vsync_gpio_num]: %d"
            "\n\t\t-> [de_gpio_num]: %d"
            "\n\t\t-> [pclk_gpio_num]: %d"
            "\n\t\t-> [disp_gpio_num]: %d"
            , static_cast<int>(config.data_width)
            , static_cast<int>(config.bits_per_pixel)
            , static_cast<int>(config.num_fbs)
            , static_cast<int>(config.bounce_buffer_size_px)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
            , static_cast<int>(config.dma_burst_size)
#else
            , static_cast<int>(config.sram_trans_align)
            , static_cast<int>(config.psram_trans_align)
#endif // ESP_IDF_VERSION
            , static_cast<int>(config.hsync_gpio_num)
            , static_cast<int>(config.vsync_gpio_num)
            , static_cast<int>(config.de_gpio_num)
            , static_cast<int>(config.pclk_gpio_num)
            , static_cast<int>(config.disp_gpio_num)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> {data_gpio_nums}"
            "\n\t\t\t-> [0-7]: %d, %d, %d, %d, %d, %d, %d, %d"
#if ESP_PANEL_BUS_RGB_DATA_BITS >= 16
            "\n\t\t\t-> [8-15]: %d, %d, %d, %d, %d, %d, %d, %d"
#endif // ESP_PANEL_BUS_RGB_DATA_BITS
            , config.data_gpio_nums[0], config.data_gpio_nums[1], config.data_gpio_nums[2], config.data_gpio_nums[3]
            , config.data_gpio_nums[4], config.data_gpio_nums[5], config.data_gpio_nums[6], config.data_gpio_nums[7]
#if ESP_PANEL_BUS_RGB_DATA_BITS >= 16
            , config.data_gpio_nums[8], config.data_gpio_nums[9], config.data_gpio_nums[10], config.data_gpio_nums[11]
            , config.data_gpio_nums[12], config.data_gpio_nums[13], config.data_gpio_nums[14], config.data_gpio_nums[15]
#endif // ESP_PANEL_BUS_RGB_DATA_BITS
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> {flags}"
            "\n\t\t\t-> [disp_active_low]: %d"
            "\n\t\t\t-> [refresh_on_demand]: %d"
            "\n\t\t\t-> [fb_in_psram]: %d"
            "\n\t\t\t-> [double_fb]: %d"
            "\n\t\t\t-> [no_fb]: %d"
            "\n\t\t\t-> [bb_invalidate_cache]: %d"
            , static_cast<int>(config.flags.disp_active_low)
            , static_cast<int>(config.flags.refresh_on_demand)
            , static_cast<int>(config.flags.fb_in_psram)
            , static_cast<int>(config.flags.double_fb)
            , static_cast<int>(config.flags.no_fb)
            , static_cast<int>(config.flags.bb_invalidate_cache)
        );
    } else {
        auto &config = std::get<RefreshPanelPartialConfig>(refresh_panel);
        ESP_UTILS_LOGI(
            "\n\t{Refresh panel config}[partial]"
            "\n\t\t-> [pclk_hz]: %d"
            "\n\t\t-> [h_res]: %d"
            "\n\t\t-> [v_res]: %d"
            , static_cast<int>(config.pclk_hz)
            , static_cast<int>(config.h_res)
            , static_cast<int>(config.v_res)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> [hsync_pulse_width]: %d"
            "\n\t\t-> [hsync_back_porch]: %d"
            "\n\t\t-> [hsync_front_porch]: %d"
            "\n\t\t-> [vsync_pulse_width]: %d"
            "\n\t\t-> [vsync_back_porch]: %d"
            "\n\t\t-> [vsync_front_porch]: %d"
            , static_cast<int>(config.hsync_pulse_width)
            , static_cast<int>(config.hsync_back_porch)
            , static_cast<int>(config.hsync_front_porch)
            , static_cast<int>(config.vsync_pulse_width)
            , static_cast<int>(config.vsync_back_porch)
            , static_cast<int>(config.vsync_front_porch)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> [data_width]: %d"
            "\n\t\t-> [bits_per_pixel]: %d"
            "\n\t\t-> [bounce_buffer_size_px]: %d"
            "\n\t\t-> [hsync_gpio_num]: %d"
            "\n\t\t-> [vsync_gpio_num]: %d"
            "\n\t\t-> [de_gpio_num]: %d"
            "\n\t\t-> [pclk_gpio_num]: %d"
            "\n\t\t-> [disp_gpio_num]: %d"
            , static_cast<int>(config.data_width)
            , static_cast<int>(config.bits_per_pixel)
            , static_cast<int>(config.bounce_buffer_size_px)
            , static_cast<int>(config.hsync_gpio_num)
            , static_cast<int>(config.vsync_gpio_num)
            , static_cast<int>(config.de_gpio_num)
            , static_cast<int>(config.pclk_gpio_num)
            , static_cast<int>(config.disp_gpio_num)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> {data_gpio_nums}"
            "\n\t\t\t-> [0-7]: %d, %d, %d, %d, %d, %d, %d, %d"
#if ESP_PANEL_BUS_RGB_DATA_BITS >= 16
            "\n\t\t\t-> [8-15]: %d, %d, %d, %d, %d, %d, %d, %d"
#endif // ESP_PANEL_BUS_RGB_DATA_BITS
            "\n\t\t-> [flags_pclk_active_neg]: %d"
            , config.data_gpio_nums[0], config.data_gpio_nums[1], config.data_gpio_nums[2], config.data_gpio_nums[3]
            , config.data_gpio_nums[4], config.data_gpio_nums[5], config.data_gpio_nums[6], config.data_gpio_nums[7]
#if ESP_PANEL_BUS_RGB_DATA_BITS >= 16
            , config.data_gpio_nums[8], config.data_gpio_nums[9], config.data_gpio_nums[10], config.data_gpio_nums[11]
            , config.data_gpio_nums[12], config.data_gpio_nums[13], config.data_gpio_nums[14], config.data_gpio_nums[15]
#endif // ESP_PANEL_BUS_RGB_DATA_BITS
            , static_cast<int>(config.flags_pclk_active_neg)
        );
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

BusRGB::~BusRGB()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BusRGB::configSPI_IO_Type(bool cs_use_expander, bool sck_use_expander, bool sda_use_expander)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");
    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelUsed(), false, "Not using control panel");

    ESP_UTILS_LOGD(
        "Param: cs_use_expander(%d), sck_use_expander(%d), sda_use_expander(%d)",
        cs_use_expander, sck_use_expander, sda_use_expander
    );
    auto &config = getControlPanelFullConfig();
    if (cs_use_expander) {
        if (config.line_config.cs_io_type == IO_TYPE_GPIO) {
            config.line_config.cs_expander_pin = (esp_io_expander_pin_num_t)BIT(config.line_config.cs_gpio_num);
        }
        config.line_config.cs_io_type = IO_TYPE_EXPANDER;
    }
    if (sck_use_expander) {
        if (config.line_config.scl_io_type == IO_TYPE_GPIO) {
            config.line_config.scl_expander_pin = (esp_io_expander_pin_num_t)BIT(config.line_config.scl_gpio_num);
        }
        config.line_config.scl_io_type = IO_TYPE_EXPANDER;
    }
    if (sda_use_expander) {
        if (config.line_config.sda_io_type == IO_TYPE_GPIO) {
            config.line_config.sda_expander_pin = (esp_io_expander_pin_num_t)BIT(config.line_config.sda_gpio_num);
        }
        config.line_config.sda_io_type = IO_TYPE_EXPANDER;
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configSPI_IO_Expander(esp_io_expander_t *handle)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");
    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelUsed(), false, "Not using control panel");

    ESP_UTILS_LOGD("Param: handle(@%p)", handle);
    getControlPanelFullConfig().line_config.io_expander = handle;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configSPI_Mode(uint8_t mode)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");
    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelUsed(), false, "Not using control panel");

    ESP_UTILS_LOGD("Param: mode(%d)", mode);
    getControlPanelFullConfig().spi_mode = mode;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configSPI_CommandDataBytes(uint8_t command_bytes, uint8_t data_bytes)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");
    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelUsed(), false, "Not using control panel");

    ESP_UTILS_LOGD("Param: command_bytes(%d), data_bytes(%d)", command_bytes, data_bytes);
    getControlPanelFullConfig().lcd_cmd_bytes = command_bytes;
    getControlPanelFullConfig().lcd_param_bytes = data_bytes;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRGB_FreqHz(uint32_t hz)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: hz(%d)", static_cast<int>(hz));
    getRefreshPanelFullConfig().timings.pclk_hz = hz;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRGB_FrameBufferNumber(uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", num);
    ESP_UTILS_CHECK_FALSE_RETURN(num > 0, false, "Frame buffer number must be greater than 0");
    getRefreshPanelFullConfig().num_fbs = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRGB_BounceBufferSize(uint32_t size_in_pixel)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(
        !isOverState(State::INIT), false, "Should be called before `init()`"
    );

    ESP_UTILS_LOGD("Param: size_in_pixel(%d)", static_cast<int>(size_in_pixel));

    auto &config = getRefreshPanelFullConfig();
    uint32_t half_total_pixels = config.timings.h_res * config.timings.v_res / 2;

    // Check if half_total_pixels is divisible by size_in_pixel
    if (half_total_pixels % size_in_pixel != 0) {
        // Find next valid size by incrementing until we find one that works
        uint32_t new_size = size_in_pixel;
        while (new_size <= half_total_pixels) {
            if (half_total_pixels % new_size == 0) {
                ESP_UTILS_LOGW(
                    "Adjusted bounce buffer size from %d to %d pixels to ensure proper alignment",
                    static_cast<int>(size_in_pixel), static_cast<int>(new_size)
                );
                size_in_pixel = new_size;
                break;
            }
            new_size++;
        }

        ESP_UTILS_CHECK_FALSE_RETURN(new_size <= half_total_pixels, false, "Could not find valid bounce buffer size");
    }

    config.bounce_buffer_size_px = size_in_pixel;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRGB_TimingFlags(
    bool hsync_idle_low, bool vsync_idle_low, bool de_idle_high, bool pclk_active_neg, bool pclk_idle_high
)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(
        !isOverState(State::INIT), false, "Should be called before `init()`"
    );

    ESP_UTILS_LOGD(
        "Param: hsync_idle_low(%d), vsync_idle_low(%d), de_idle_high(%d), pclk_active_neg(%d), pclk_idle_high(%d)",
        hsync_idle_low, vsync_idle_low, de_idle_high, pclk_active_neg, pclk_idle_high
    );
    auto &config = getRefreshPanelFullConfig();
    config.timings.flags.hsync_idle_low = hsync_idle_low;
    config.timings.flags.vsync_idle_low = vsync_idle_low;
    config.timings.flags.de_idle_high = de_idle_high;
    config.timings.flags.pclk_active_neg = pclk_active_neg;
    config.timings.flags.pclk_idle_high = pclk_idle_high;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configSpiLine(
    bool cs_use_expander, bool sck_use_expander, bool sda_use_expander, esp_expander::Base *io_expander
)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(
        !isOverState(State::INIT), false, "Should be called before `init()`"
    );

    ESP_UTILS_LOGD(
        "Param: cs_use_expander(%d), sck_use_expander(%d), sda_use_expander(%d), io_expander(@%p)",
        cs_use_expander, sck_use_expander, sda_use_expander, io_expander
    );
    ESP_UTILS_CHECK_FALSE_RETURN(
        configSPI_IO_Expander(io_expander->getDeviceHandle()), false, "config SPI IO expander failed"
    );
    ESP_UTILS_CHECK_FALSE_RETURN(
        configSPI_IO_Type(cs_use_expander, sck_use_expander, sda_use_expander), false, "config SPI IO type failed"
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRgbFlagDispActiveLow()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(
        !isOverState(State::INIT), false, "Should be called before `init()`"
    );

    getRefreshPanelFullConfig().flags.disp_active_low = 1;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRgbTimingFreqHz(uint32_t hz)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(
        !isOverState(State::INIT), false, "Should be called before `init()`"
    );

    ESP_UTILS_LOGD("Param: hz(%d)", (int)hz);
    getRefreshPanelFullConfig().timings.pclk_hz = hz;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::configRgbTimingPorch(
    uint16_t hpw, uint16_t hbp, uint16_t hfp, uint16_t vpw, uint16_t vbp, uint16_t vfp
)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(
        !isOverState(State::INIT), false, "Should be called before `init()`"
    );

    ESP_UTILS_LOGD("Param: hpw(%d), hbp(%d), hfp(%d), vpw(%d), vbp(%d), vfp(%d)", hpw, hbp, hfp, vpw, vbp, vfp);
    auto &config = getRefreshPanelFullConfig();
    config.timings.hsync_pulse_width = hpw;
    config.timings.hsync_back_porch = hbp;
    config.timings.hsync_front_porch = hfp;
    config.timings.vsync_pulse_width = vpw;
    config.timings.vsync_back_porch = vbp;
    config.timings.vsync_front_porch = vfp;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::init()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");

    // Convert the partial configuration to full configuration
    _config.convertPartialToFull();
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _config.printControlPanelConfig();
    _config.printRefreshPanelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    setState(State::INIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    // Initialize the bus if not initialized
    if (!isOverState(State::INIT)) {
        ESP_UTILS_CHECK_FALSE_RETURN(init(), false, "Init failed");
    }

    // Create the control panel if needed
    if (isControlPanelUsed()) {
        ESP_UTILS_CHECK_ERROR_RETURN(
            esp_lcd_new_panel_io_3wire_spi(&getControlPanelFullConfig(), &control_panel), false,
            "create control panel failed"
        );
        ESP_UTILS_LOGD("Create control panel @%p", control_panel);
    }

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusRGB::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    // Delete the control panel if valid
    if (isControlPanelUsed() && isControlPanelValid()) {
        ESP_UTILS_CHECK_FALSE_RETURN(delControlPanel(), false, "Delete control panel failed");
    }

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

BusRGB::ControlPanelFullConfig &BusRGB::getControlPanelFullConfig()
{
    if (std::holds_alternative<ControlPanelPartialConfig>(_config.control_panel.value())) {
        _config.convertPartialToFull();
    }

    return std::get<ControlPanelFullConfig>(_config.control_panel.value());
}

BusRGB::RefreshPanelFullConfig &BusRGB::getRefreshPanelFullConfig()
{
    if (std::holds_alternative<RefreshPanelPartialConfig>(_config.refresh_panel)) {
        _config.convertPartialToFull();
    }

    return std::get<RefreshPanelFullConfig>(_config.refresh_panel);
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_RGB

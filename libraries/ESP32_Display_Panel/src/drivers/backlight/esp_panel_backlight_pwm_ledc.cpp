/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_backlight_conf_internal.h"
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_PWM_LEDC

#include "utils/esp_panel_utils_log.h"
#include "esp_panel_backlight_pwm_ledc.hpp"

namespace esp_panel::drivers {

void BacklightPWM_LEDC::Config::convertPartialToFull()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<LEDC_TimerPartialConfig>(ledc_timer)) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printLEDC_TimerConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<LEDC_TimerPartialConfig>(ledc_timer);
        ledc_timer = ledc_timer_config_t{
            .speed_mode = LEDC_SPEED_MODE_DEFAULT,
            .duty_resolution = static_cast<ledc_timer_bit_t>(config.duty_resolution),
            .timer_num = LEDC_TIMER_NUM_DEFAULT,
            .freq_hz = static_cast<uint32_t>(config.freq_hz),
            .clk_cfg = LEDC_AUTO_CLK,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
            .deconfigure = false,
#endif
        };
    }

    if (std::holds_alternative<LEDC_ChannelPartialConfig>(ledc_channel)) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printLEDC_ChannelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<LEDC_ChannelPartialConfig>(ledc_channel);
        ledc_channel = ledc_channel_config_t{
            .gpio_num = config.io_num,
            .speed_mode = LEDC_SPEED_MODE_DEFAULT,
            .channel = LEDC_CHANNEL_0,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_NUM_DEFAULT,
            .duty = 0,
            .hpoint = 0,
            .flags = {
                .output_invert = !config.on_level,
            },
        };
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BacklightPWM_LEDC::Config::printLEDC_TimerConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<LEDC_TimerPartialConfig>(ledc_timer)) {
        auto &config = std::get<LEDC_TimerPartialConfig>(ledc_timer);
        ESP_UTILS_LOGI(
            "\n\t{LEDC timer config}[Partial]"
            "\n\t\t-> [freq_hz]: %d"
            "\n\t\t-> [duty_resolution]: %d"
            , static_cast<int>(config.freq_hz)
            , static_cast<int>(config.duty_resolution)
        );
    } else if (std::holds_alternative<LEDC_TimerFullConfig>(ledc_timer)) {
        auto &config = std::get<LEDC_TimerFullConfig>(ledc_timer);
        ESP_UTILS_LOGI(
            "\n\t{LEDC timer config}[full]"
            "\n\t\t-> [speed_mode]: %d"
            "\n\t\t-> [duty_resolution]: %d"
            "\n\t\t-> [timer_num]: %d"
            "\n\t\t-> [freq_hz]: %d"
            "\n\t\t-> [clk_cfg]: %d"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
            "\n\t\t-> [deconfigure]: %d"
#endif
            , static_cast<int>(config.speed_mode)
            , static_cast<int>(config.duty_resolution)
            , static_cast<int>(config.timer_num)
            , static_cast<int>(config.freq_hz)
            , static_cast<int>(config.clk_cfg)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
            , static_cast<int>(config.deconfigure)
#endif
        );
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BacklightPWM_LEDC::Config::printLEDC_ChannelConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<LEDC_ChannelFullConfig>(ledc_channel)) {
        auto &config = std::get<LEDC_ChannelFullConfig>(ledc_channel);
        ESP_UTILS_LOGI(
            "\n\t{LEDC channel config}[full]"
            "\n\t\t-> [gpio_num]: %d"
            "\n\t\t-> [speed_mode]: %d"
            "\n\t\t-> [channel]: %d"
            "\n\t\t-> [intr_type]: %d"
            "\n\t\t-> [timer_sel]: %d"
            "\n\t\t-> [duty]: %d"
            "\n\t\t-> [hpoint]: %d"
            "\n\t\t-> {flags}"
            "\n\t\t\t-> [output_invert]: %d"
            , static_cast<int>(config.gpio_num)
            , static_cast<int>(config.speed_mode)
            , static_cast<int>(config.channel)
            , static_cast<int>(config.intr_type)
            , static_cast<int>(config.timer_sel)
            , static_cast<int>(config.duty)
            , static_cast<int>(config.hpoint)
            , static_cast<int>(config.flags.output_invert)
        );
    } else {
        auto &config = std::get<LEDC_ChannelPartialConfig>(ledc_channel);
        ESP_UTILS_LOGI(
            "\n\t{LEDC channel config}[Partial]"
            "\n\t\t-> [io_num]: %d"
            "\n\t\t-> [on_level]: %d"
            , static_cast<int>(config.io_num)
            , static_cast<int>(config.on_level)
        );
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

BacklightPWM_LEDC::~BacklightPWM_LEDC()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BacklightPWM_LEDC::configLEDC_FreqHz(int hz)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Should be called before begin()");

    ESP_UTILS_LOGD("Param: hz(%d)", hz);
    getLEDC_TimerConfig().freq_hz = static_cast<uint32_t>(hz);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightPWM_LEDC::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    _config.convertPartialToFull();
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _config.printLEDC_TimerConfig();
    _config.printLEDC_ChannelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    ESP_UTILS_CHECK_ERROR_RETURN(ledc_timer_config(&getLEDC_TimerConfig()), false, "LEDC timer config failed");
    ESP_UTILS_CHECK_ERROR_RETURN(ledc_channel_config(&getLEDC_ChannelConfig()), false, "LEDC channel config failed");

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightPWM_LEDC::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (isOverState(State::BEGIN)) {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
        auto &channel_config = getLEDC_ChannelConfig();
        ESP_UTILS_CHECK_ERROR_RETURN(
            ledc_stop(
                channel_config.speed_mode, channel_config.channel, !channel_config.flags.output_invert
            ), false, "LEDC stop failed"
        );
#else
        auto &timer_config = getLEDC_TimerConfig();
        ESP_UTILS_CHECK_ERROR_RETURN(
            ledc_timer_pause(timer_config.speed_mode, timer_config.timer_num), false, "LEDC timer stop failed"
        );
        timer_config.deconfigure = true;
        ESP_UTILS_CHECK_ERROR_RETURN(ledc_timer_config(&timer_config), false, "LEDC timer config failed");
        ESP_UTILS_LOGD("Stop LEDC timer");
#endif // ESP_IDF_VERSION

        setState(State::DEINIT);
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightPWM_LEDC::setBrightness(int percent)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: percent(%d)", percent);

    percent = std::clamp(percent, 0, 100);
    auto &channel_config = getLEDC_ChannelConfig();
    auto &timer_config = getLEDC_TimerConfig();
    uint32_t duty = ((1ULL << timer_config.duty_resolution) * percent) / 100;

    ESP_UTILS_CHECK_ERROR_RETURN(
        ledc_set_duty(channel_config.speed_mode, channel_config.channel, duty),
        false, "LEDC set duty failed"
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        ledc_update_duty(channel_config.speed_mode, channel_config.channel), false, "LEDC update duty failed"
    );

    setBrightnessValue(percent);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

BacklightPWM_LEDC::LEDC_TimerFullConfig &BacklightPWM_LEDC::getLEDC_TimerConfig()
{
    if (std::holds_alternative<LEDC_TimerPartialConfig>(_config.ledc_timer)) {
        _config.convertPartialToFull();
    }

    return std::get<LEDC_TimerFullConfig>(_config.ledc_timer);
}

BacklightPWM_LEDC::LEDC_ChannelFullConfig &BacklightPWM_LEDC::getLEDC_ChannelConfig()
{
    if (std::holds_alternative<LEDC_ChannelPartialConfig>(_config.ledc_channel)) {
        _config.convertPartialToFull();
    }

    return std::get<LEDC_ChannelFullConfig>(_config.ledc_channel);
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_PWM_LEDC

/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "inttypes.h"
#include "driver/i2c.h"
#include "esp_expander_utils.h"
#include "esp_expander_base.hpp"

// Check whether it is a valid pin number
#define IS_VALID_PIN(pin_num)   (pin_num < IO_COUNT_MAX)

namespace esp_expander {

void Base::Config::convertPartialToFull(void)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (isHostConfigValid() && std::holds_alternative<HostPartialConfig>(host.value())) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printHostConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<HostPartialConfig>(host.value());
        host = HostFullConfig{
            .mode = I2C_MODE_MASTER,
            .sda_io_num = config.sda_io_num,
            .scl_io_num = config.scl_io_num,
            .sda_pullup_en = config.sda_pullup_en,
            .scl_pullup_en = config.scl_pullup_en,
            .master = {
                .clk_speed = static_cast<uint32_t>(config.clk_speed),
            },
            .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
        };
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void Base::Config::printHostConfig(void) const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (!isHostConfigValid()) {
        ESP_UTILS_LOGI("\n\t{Host config}[skipped]");
        goto end;
    }

    if (std::holds_alternative<HostFullConfig>(host.value())) {
        auto &config = std::get<HostFullConfig>(host.value());
        ESP_UTILS_LOGI(
            "\n\t{Host config}[full]\n"
            "\t\t-> [host_id]: %d\n"
            "\t\t-> [mode]: %d\n"
            "\t\t-> [sda_io_num]: %d\n"
            "\t\t-> [scl_io_num]: %d\n"
            "\t\t-> [sda_pullup_en]: %d\n"
            "\t\t-> [scl_pullup_en]: %d\n"
            "\t\t-> [master.clk_speed]: %d\n"
            "\t\t-> [clk_flags]: %d"
            , static_cast<int>(host_id)
            , static_cast<int>(config.mode)
            , static_cast<int>(config.sda_io_num)
            , static_cast<int>(config.scl_io_num)
            , static_cast<int>(config.sda_pullup_en)
            , static_cast<int>(config.scl_pullup_en)
            , static_cast<int>(config.master.clk_speed)
            , static_cast<int>(config.clk_flags)
        );
    } else {
        auto &config = std::get<HostPartialConfig>(host.value());
        ESP_UTILS_LOGI(
            "\n\t{Host config}[partial]\n"
            "\t\t-> [host_id]: %d\n"
            "\t\t-> [sda_io_num]: %d\n"
            "\t\t-> [scl_io_num]: %d\n"
            "\t\t-> [sda_pullup_en]: %d\n"
            "\t\t-> [scl_pullup_en]: %d\n"
            "\t\t-> [clk_speed]: %d"
            , static_cast<int>(host_id)
            , static_cast<int>(config.sda_io_num)
            , static_cast<int>(config.scl_io_num)
            , static_cast<int>(config.sda_pullup_en)
            , static_cast<int>(config.scl_pullup_en)
            , static_cast<int>(config.clk_speed)
        );
    }

end:
    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void Base::Config::printDeviceConfig(void) const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGI(
        "\n\t{Device config}[partial]\n"
        "\t\t-> [host_id]: %d\n"
        "\t\t-> [address]: 0x%02X"
        , static_cast<int>(host_id)
        , static_cast<int>(device.address)
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool Base::configHostSkipInit(bool skip_init)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    _is_host_skip_init = skip_init;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Base::init(void)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");

    // Convert the partial configuration to full configuration
    _config.convertPartialToFull();
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _config.printHostConfig();
    _config.printDeviceConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    // Initialize the I2C host if not skipped
    if (!isHostSkipInit()) {
        i2c_port_t host_id = static_cast<i2c_port_t>(getConfig().host_id);
        ESP_UTILS_CHECK_ERROR_RETURN(
            i2c_param_config(host_id, getHostFullConfig()), false, "I2C param config failed"
        );
        ESP_UTILS_CHECK_ERROR_RETURN(
            i2c_driver_install(host_id, getHostFullConfig()->mode, 0, 0, 0), false, "I2C driver install failed"
        );
        ESP_UTILS_LOGD("Init I2C host(%d)", static_cast<int>(host_id));
    }

    setState(State::INIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Base::reset(void)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_reset(device_handle), false, "Reset failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Base::del(void)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (device_handle != nullptr) {
        ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_del(device_handle), false, "Delete failed");
        device_handle = nullptr;
        ESP_UTILS_LOGD("Delete @%p", device_handle);
    }

    if (isOverState(State::INIT) && !isHostSkipInit()) {
        i2c_port_t host_id = static_cast<i2c_port_t>(getConfig().host_id);
        ESP_UTILS_CHECK_ERROR_RETURN(i2c_driver_delete(host_id), false, "I2C driver delete failed");
        ESP_UTILS_LOGD("Delete I2C host(%d)", static_cast<int>(host_id));
    }

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Base::pinMode(uint8_t pin, uint8_t mode)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: pin(%d), mode(%d)", pin, mode);
    ESP_UTILS_CHECK_FALSE_RETURN(IS_VALID_PIN(pin), false, "Invalid pin");
    ESP_UTILS_CHECK_FALSE_RETURN((mode == INPUT) || (mode == OUTPUT), false, "Invalid mode");

    esp_io_expander_dir_t dir = (mode == INPUT) ? IO_EXPANDER_INPUT : IO_EXPANDER_OUTPUT;
    ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_set_dir(device_handle, BIT64(pin), dir), false, "Set dir failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Base::digitalWrite(uint8_t pin, uint8_t value)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: pin(%d), value(%d)", pin, value);
    ESP_UTILS_CHECK_FALSE_RETURN(IS_VALID_PIN(pin), false, "Invalid pin");

    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_io_expander_set_level(device_handle, BIT64(pin), value), false, "Set level failed"
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int Base::digitalRead(uint8_t pin)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: pin(%d)", pin);
    ESP_UTILS_CHECK_FALSE_RETURN(IS_VALID_PIN(pin), -1, "Invalid pin");

    uint32_t level = 0;
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_io_expander_get_level(device_handle, BIT64(pin), &level), -1, "Get level failed"
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return (level & BIT64(pin)) ? HIGH : LOW;
}

bool Base::multiPinMode(uint32_t pin_mask, uint8_t mode)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: pin_mask(0x%" PRIx32 "), mode(%d)", pin_mask, mode);
    ESP_UTILS_CHECK_FALSE_RETURN((mode == INPUT) || (mode == OUTPUT), false, "Invalid mode");

    esp_io_expander_dir_t dir = (mode == INPUT) ? IO_EXPANDER_INPUT : IO_EXPANDER_OUTPUT;
    ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_set_dir(device_handle, pin_mask, dir), false, "Set dir failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Base::multiDigitalWrite(uint32_t pin_mask, uint8_t value)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: pin_mask(0x%" PRIx32 "), value(%d)", pin_mask, value);

    ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_set_level(device_handle, pin_mask, value), false, "Set level failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int64_t Base::multiDigitalRead(uint32_t pin_mask)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: pin_mask(0x%" PRIx32 ")", pin_mask);

    uint32_t level = 0;
    ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_get_level(device_handle, pin_mask, &level), false, "Get level failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return level;
}

bool Base::printStatus(void) const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_CHECK_ERROR_RETURN(esp_io_expander_print_state(device_handle), false, "Print state failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

Base::HostFullConfig *Base::getHostFullConfig()
{
    if (std::holds_alternative<HostPartialConfig>(_config.host.value())) {
        _config.convertPartialToFull();
    }

    return &std::get<HostFullConfig>(_config.host.value());
}

} // namespace esp_expander

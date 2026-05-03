/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_bus_conf_internal.h"
#if ESP_PANEL_DRIVERS_BUS_ENABLE_I2C

#include "inttypes.h"
#include "utils/esp_panel_utils_log.h"
#include "drivers/host/esp_panel_host_i2c.hpp"
#include "esp_panel_bus_i2c.hpp"

namespace esp_panel::drivers {

void BusI2C::Config::convertPartialToFull()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (isHostConfigValid() && std::holds_alternative<HostPartialConfig>(host.value())) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printHostConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<HostPartialConfig>(host.value());
        host = i2c_config_t{
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

void BusI2C::Config::printHostConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (!isHostConfigValid()) {
        ESP_UTILS_LOGI("\n\t{Host config}[skipped]");
        goto end;
    }

    if (isHostConfigValid() && std::holds_alternative<HostFullConfig>(host.value())) {
        auto &config = std::get<HostFullConfig>(host.value());
        ESP_UTILS_LOGI(
            "\n\t{Host config}[full]"
            "\n\t\t-> [host_id]: %d"
            "\n\t\t-> [mode]: %d"
            "\n\t\t-> [sda_io_num]: %d"
            "\n\t\t-> [scl_io_num]: %d"
            "\n\t\t-> [sda_pullup_en]: %d"
            "\n\t\t-> [scl_pullup_en]: %d"
            "\n\t\t-> [master.clk_speed]: %d"
            "\n\t\t-> [clk_flags]: %d"
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
            "\n\t{Host config}[partial]"
            "\n\t\t-> [host_id]: %d"
            "\n\t\t-> [sda_io_num]: %d"
            "\n\t\t-> [scl_io_num]: %d"
            "\n\t\t-> [sda_pullup_en]: %d"
            "\n\t\t-> [scl_pullup_en]: %d"
            "\n\t\t-> [clk_speed]: %d"
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

void BusI2C::Config::printControlPanelConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGI(
        "\n\t{Control panel config}[full]"
        "\n\t\t-> [host_id]: %d"
        "\n\t\t-> [dev_addr]: 0x%02" PRIX32
        "\n\t\t-> [control_phase_bytes]: %d"
        "\n\t\t-> [dc_bit_offset]: %d"
        "\n\t\t-> [lcd_cmd_bits]: %d"
        "\n\t\t-> [lcd_param_bits]: %d"
        "\n\t\t-> {flags}"
        "\n\t\t\t-> [dc_low_on_data]: %d"
        "\n\t\t\t-> [disable_control_phase]: %d"
        , static_cast<int>(host_id)
        , control_panel.dev_addr
        , static_cast<int>(control_panel.control_phase_bytes)
        , static_cast<int>(control_panel.dc_bit_offset)
        , static_cast<int>(control_panel.lcd_cmd_bits)
        , static_cast<int>(control_panel.lcd_param_bits)
        , static_cast<int>(control_panel.flags.dc_low_on_data)
        , static_cast<int>(control_panel.flags.disable_control_phase)
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

BusI2C::~BusI2C()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BusI2C::configI2C_HostSkipInit()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    _config.host = std::nullopt;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_PullupEnable(bool sda_pullup_en, bool scl_pullup_en)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");
    ESP_UTILS_CHECK_FALSE_RETURN(!isHostSkipInit(), false, "Host is skipped initialization");

    ESP_UTILS_LOGD("Param: sda_pullup_en(%d), scl_pullup_en(%d)", sda_pullup_en, scl_pullup_en);
    auto &host_config = getHostFullConfig();
    host_config.sda_pullup_en = sda_pullup_en ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    host_config.scl_pullup_en = scl_pullup_en ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_FreqHz(uint32_t hz)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");
    ESP_UTILS_CHECK_FALSE_RETURN(!isHostSkipInit(), false, "Host is skipped initialization");

    ESP_UTILS_LOGD("Param: hz(%d)", static_cast<int>(hz));
    getHostFullConfig().master.clk_speed = hz;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_Address(uint8_t address)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: address(0x%02X)", address);
    getControlPanelFullConfig().dev_addr = address;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_CtrlPhaseBytes(uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", num);
    getControlPanelFullConfig().control_phase_bytes = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_DC_BitOffset(uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", num);
    getControlPanelFullConfig().dc_bit_offset = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_CommandBits(uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", num);
    getControlPanelFullConfig().lcd_cmd_bits = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_ParamBits(uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", num);
    getControlPanelFullConfig().lcd_param_bits = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::configI2C_Flags(bool dc_low_on_data, bool disable_control_phase)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: dc_low_on_data(%d), disable_control_phase(%d)", dc_low_on_data, disable_control_phase);
    auto &config = getControlPanelFullConfig();
    config.flags.dc_low_on_data = dc_low_on_data;
    config.flags.disable_control_phase = disable_control_phase;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::init()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");

    // Convert the partial configuration to full configuration
    _config.convertPartialToFull();
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _config.printHostConfig();
    _config.printControlPanelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    // Get the host instance if not skipped
    if (!isHostSkipInit()) {
        auto host_id = getConfig().host_id;
        _host = HostI2C::getInstance(host_id, getHostFullConfig());
        ESP_UTILS_CHECK_NULL_RETURN(_host, false, "Get I2C host(%d) instance failed", host_id);
        ESP_UTILS_LOGD("Get I2C host(%d) instance", host_id);
    }

    setState(State::INIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    // Initialize the bus if not initialized
    if (!isOverState(State::INIT)) {
        ESP_UTILS_CHECK_FALSE_RETURN(init(), false, "Init failed");
    }

    // Startup the host if not skipped
    auto host_id = getConfig().host_id;
    if (_host != nullptr) {
        ESP_UTILS_CHECK_FALSE_RETURN(_host->begin(), false, "Begin I2C host(%d) failed", host_id);
        ESP_UTILS_LOGD("Begin I2C host(%d)", host_id);
    }

    // Create the control panel
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 2, 0)
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_new_panel_io_i2c(
            reinterpret_cast<esp_lcd_i2c_bus_handle_t>(host_id), &getControlPanelFullConfig(), &control_panel
        ), false, "create control panel failed"
    );
#else
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_new_panel_io_i2c_v1(
            static_cast<esp_lcd_i2c_bus_handle_t>(host_id), &getControlPanelFullConfig(), &control_panel
        ), false, "create control panel failed"
    );
#endif // ESP_IDF_VERSION
    ESP_UTILS_LOGD("Create control panel @%p", control_panel);

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusI2C::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    // Delete the control panel if valid
    if (isControlPanelValid()) {
        ESP_UTILS_CHECK_FALSE_RETURN(delControlPanel(), false, "Delete control panel failed");
    }

    // Release the host instance if valid
    if (_host != nullptr) {
        _host = nullptr;
        auto host_id = getConfig().host_id;
        ESP_UTILS_CHECK_FALSE_RETURN(
            HostI2C::tryReleaseInstance(host_id), false, "Release I2C host(%d) failed", host_id
        );
    }

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

BusI2C::HostFullConfig &BusI2C::getHostFullConfig()
{
    if (std::holds_alternative<HostPartialConfig>(_config.host.value())) {
        _config.convertPartialToFull();
    }

    return std::get<HostFullConfig>(_config.host.value());
}

BusI2C::ControlPanelFullConfig &BusI2C::getControlPanelFullConfig()
{
    return _config.control_panel;
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_I2C

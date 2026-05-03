/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_bus_conf_internal.h"
#if ESP_PANEL_DRIVERS_BUS_ENABLE_SPI

#include "utils/esp_panel_utils_log.h"
#include "drivers/host/esp_panel_host_spi.hpp"
#include "esp_panel_bus_spi.hpp"

namespace esp_panel::drivers {

void BusSPI::Config::convertPartialToFull()
{
    if (isHostConfigValid() && std::holds_alternative<HostPartialConfig>(host.value())) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printHostConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<HostPartialConfig>(host.value());
        host = HostFullConfig{
            .mosi_io_num = config.mosi_io_num,
            .miso_io_num = config.miso_io_num,
            .sclk_io_num = config.sclk_io_num,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
            .max_transfer_sz = ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE,
            .flags = SPICOMMON_BUSFLAG_MASTER,
            .intr_flags = 0,
        };
    }

    if (std::holds_alternative<ControlPanelPartialConfig>(control_panel)) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printControlPanelConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<ControlPanelPartialConfig>(control_panel);
        control_panel = ControlPanelFullConfig{
            .cs_gpio_num = config.cs_gpio_num,
            .dc_gpio_num = config.dc_gpio_num,
            .spi_mode = config.spi_mode,
            .pclk_hz = static_cast<unsigned int>(config.pclk_hz),
            .trans_queue_depth = 10,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .lcd_cmd_bits = config.lcd_cmd_bits,
            .lcd_param_bits = config.lcd_param_bits,
            .flags = {
                .dc_low_on_data = 0,
                .octal_mode = 0,
                .quad_mode = 0,
                .sio_mode = 0,
                .lsb_first = 0,
                .cs_high_active = 0,
            },
        };
    }
}

void BusSPI::Config::printHostConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (!isHostConfigValid()) {
        ESP_UTILS_LOGI("\n\t{Host config}[skipped]");
        goto end;
    }

    if (std::holds_alternative<HostFullConfig>(host.value())) {
        auto &config = std::get<HostFullConfig>(host.value());
        ESP_UTILS_LOGI(
            "\n\t{Host config}[full]"
            "\n\t\t-> [host_id]: %d"
            "\n\t\t-> [mosi_io_num]: %d"
            "\n\t\t-> [miso_io_num]: %d"
            "\n\t\t-> [sclk_io_num]: %d"
            "\n\t\t-> [max_transfer_sz]: %d"
            "\n\t\t-> [flags]: %d"
            "\n\t\t-> [intr_flags]: %d"
            , static_cast<int>(host_id)
            , static_cast<int>(config.mosi_io_num)
            , static_cast<int>(config.miso_io_num)
            , static_cast<int>(config.sclk_io_num)
            , static_cast<int>(config.max_transfer_sz)
            , static_cast<int>(config.flags)
            , static_cast<int>(config.intr_flags)
        );
    } else {
        auto &config = std::get<HostPartialConfig>(host.value());
        ESP_UTILS_LOGI(
            "\n\t{Host config}[Partial]"
            "\n\t\t-> [host_id]: %d"
            "\n\t\t-> [mosi_io_num]: %d"
            "\n\t\t-> [miso_io_num]: %d"
            "\n\t\t-> [sclk_io_num]: %d"
            , static_cast<int>(host_id)
            , static_cast<int>(config.mosi_io_num)
            , static_cast<int>(config.miso_io_num)
            , static_cast<int>(config.sclk_io_num)
        );
    }

end:
    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BusSPI::Config::printControlPanelConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<ControlPanelFullConfig>(control_panel)) {
        auto &config = std::get<ControlPanelFullConfig>(control_panel);
        // Here to split the log to avoid the log buffer overflow
        ESP_UTILS_LOGI(
            "\n\t{Control panel config}[full]"
            "\n\t\t-> [host_id]: %d"
            "\n\t\t-> [cs_gpio_num]: %d"
            "\n\t\t-> [dc_gpio_num]: %d"
            "\n\t\t-> [spi_mode]: %d"
            "\n\t\t-> [pclk_hz]: %d"
            "\n\t\t-> [trans_queue_depth]: %d"
            "\n\t\t-> [lcd_cmd_bits]: %d"
            "\n\t\t-> [lcd_param_bits]: %d"
            , static_cast<int>(host_id)
            , static_cast<int>(config.cs_gpio_num)
            , static_cast<int>(config.dc_gpio_num)
            , static_cast<int>(config.spi_mode)
            , static_cast<int>(config.pclk_hz)
            , static_cast<int>(config.trans_queue_depth)
            , static_cast<int>(config.lcd_cmd_bits)
            , static_cast<int>(config.lcd_param_bits)
        );
        ESP_UTILS_LOGI(
            "\n\t\t-> {flags}"
            "\n\t\t\t-> [dc_high_on_cmd]: %d"
            "\n\t\t\t-> [dc_low_on_data]: %d"
            "\n\t\t\t-> [dc_low_on_param]: %d"
            "\n\t\t\t-> [octal_mode]: %d"
            "\n\t\t\t-> [quad_mode]: %d"
            "\n\t\t\t-> [sio_mode]: %d"
            "\n\t\t\t-> [lsb_first]: %d"
            "\n\t\t\t-> [cs_high_active]: %d"
            , static_cast<int>(config.flags.dc_high_on_cmd)
            , static_cast<int>(config.flags.dc_low_on_data)
            , static_cast<int>(config.flags.dc_low_on_param)
            , static_cast<int>(config.flags.octal_mode)
            , static_cast<int>(config.flags.quad_mode)
            , static_cast<int>(config.flags.sio_mode)
            , static_cast<int>(config.flags.lsb_first)
            , static_cast<int>(config.flags.cs_high_active)
        );
    } else {
        auto &config = std::get<ControlPanelPartialConfig>(control_panel);
        ESP_UTILS_LOGI(
            "\n\t{Control panel config}[Partial]"
            "\n\t\t-> [host_id]: %d"
            "\n\t\t-> [cs_gpio_num]: %d"
            "\n\t\t-> [dc_gpio_num]: %d"
            "\n\t\t-> [spi_mode]: %d"
            "\n\t\t-> [pclk_hz]: %d"
            "\n\t\t-> [lcd_cmd_bits]: %d"
            "\n\t\t-> [lcd_param_bits]: %d"
            , static_cast<int>(host_id)
            , static_cast<int>(config.cs_gpio_num)
            , static_cast<int>(config.dc_gpio_num)
            , static_cast<int>(config.spi_mode)
            , static_cast<int>(config.pclk_hz)
            , static_cast<int>(config.lcd_cmd_bits)
            , static_cast<int>(config.lcd_param_bits)
        );
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

BusSPI::~BusSPI()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BusSPI::configSPI_HostSkipInit()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    _config.host = std::nullopt;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::configSPI_Mode(uint8_t mode)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: mode(%d)", (int)mode);
    getControlPanelFullConfig().spi_mode = mode;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::configSPI_FreqHz(uint32_t hz)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: hz(%d)", (int)hz);
    getControlPanelFullConfig().pclk_hz = hz;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::configSPI_CommandBits(uint32_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", (int)num);
    getControlPanelFullConfig().lcd_cmd_bits = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::configSPI_ParamBits(uint32_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: num(%d)", (int)num);
    getControlPanelFullConfig().lcd_param_bits = num;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::configSPI_TransQueueDepth(uint8_t depth)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: depth(%d)", (int)depth);
    getControlPanelFullConfig().trans_queue_depth = depth;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::init()
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
        _host = HostSPI::getInstance(host_id, getHostFullConfig());
        ESP_UTILS_CHECK_NULL_RETURN(_host, false, "Get SPI host(%d) instance failed", host_id);
        ESP_UTILS_LOGD("Get SPI host(%d) instance", host_id);
    }

    setState(State::INIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BusSPI::begin()
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
        ESP_UTILS_CHECK_FALSE_RETURN(_host->begin(), false, "Begin SPI host(%d) failed", host_id);
        ESP_UTILS_LOGD("Begin SPI host(%d)", host_id);
    }

    // Create the control panel
    ESP_UTILS_CHECK_ERROR_RETURN(
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
        esp_lcd_new_panel_io_spi(
            reinterpret_cast<esp_lcd_spi_bus_handle_t>(host_id), &getControlPanelFullConfig(), &control_panel
#else
        esp_lcd_new_panel_io_spi(
            static_cast<esp_lcd_spi_bus_handle_t>(host_id), &getControlPanelFullConfig(), &control_panel
#endif
        ), false, "create panel IO failed"
    );
    ESP_UTILS_LOGD("Create control panel @%p", control_panel);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    setState(State::BEGIN);

    return true;
}

bool BusSPI::del()
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
            HostSPI::tryReleaseInstance(host_id), false, "Release SPI host(%d) failed", host_id
        );
    }

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

BusSPI::HostFullConfig &BusSPI::getHostFullConfig()
{
    if (std::holds_alternative<HostPartialConfig>(_config.host.value())) {
        _config.convertPartialToFull();
    }

    return std::get<HostFullConfig>(_config.host.value());
}

BusSPI::ControlPanelFullConfig &BusSPI::getControlPanelFullConfig()
{
    if (std::holds_alternative<ControlPanelPartialConfig>(_config.control_panel)) {
        _config.convertPartialToFull();
    }

    return std::get<ControlPanelFullConfig>(_config.control_panel);
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_SPI

/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include "utils/esp_panel_utils_log.h"
#include "esp_panel_bus.hpp"

namespace esp_panel::drivers {

bool Bus::readRegisterData(uint32_t address, void *data, uint32_t data_size) const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelValid(), false, "Invalid control panel");

    ESP_UTILS_LOGD(
        "Param: address(0x%" PRIx32 "), data(%p), data_size(%d)", address, data, static_cast<int>(data_size)
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_panel_io_rx_param(control_panel, address, data, data_size), false, "Read register failed"
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Bus::writeRegisterData(uint32_t address, const void *data, uint32_t data_size) const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelValid(), false, "Invalid control panel");

    ESP_UTILS_LOGD(
        "Param: address(0x%" PRIx32 "), data(%p), data_size(%d)", address, data, static_cast<int>(data_size)
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_panel_io_tx_param(control_panel, address, data, data_size), false, "Write register failed"
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Bus::writeColorData(uint32_t address, const void *color, uint32_t color_size) const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelValid(), false, "Invalid control panel");

    ESP_UTILS_LOGD(
        "Param: address(0x%" PRIx32 "), color(%p), color_size(%d)", address, color, static_cast<int>(color_size)
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_panel_io_tx_param(control_panel, address, color, color_size), false, "Write color failed"
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Bus::delControlPanel()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isControlPanelValid(), false, "Invalid control panel");

    ESP_UTILS_CHECK_ERROR_RETURN(esp_lcd_panel_io_del(control_panel), false, "Delete control panel failed");
    ESP_UTILS_LOGD("Delete control panel @%p", control_panel);
    control_panel = nullptr;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers

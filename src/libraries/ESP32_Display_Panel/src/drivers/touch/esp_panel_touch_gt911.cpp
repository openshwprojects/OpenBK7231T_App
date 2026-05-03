/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_touch_conf_internal.h"
#if ESP_PANEL_DRIVERS_TOUCH_ENABLE_GT911

#include "utils/esp_panel_utils_log.h"
#include "drivers/bus/esp_panel_bus_i2c.hpp"
#include "esp_panel_touch_gt911.hpp"

namespace esp_panel::drivers {

TouchGT911::~TouchGT911()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool TouchGT911::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    // Initialize the touch if not initialized
    if (!isOverState(State::INIT)) {
        ESP_UTILS_CHECK_FALSE_RETURN(init(), false, "Init failed");
    }

    // Since GT911 has two I2C addresses, we can decide which one to use
    auto bus = static_cast<BusI2C *>(getBus());
    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = bus->getI2cAddress(),
    };
    setDriverData(&tp_gt911_config);

    // Create touch panel
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_touch_new_i2c_gt911(bus->getControlPanelHandle(), getConfig().getDeviceFullConfig(), &touch_panel),
        false, "Create touch panel failed"
    );
    ESP_UTILS_LOGD("Create touch panel(@%p)", touch_panel);

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_TOUCH_ENABLE_GT911

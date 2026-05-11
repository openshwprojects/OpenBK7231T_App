/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_expander_utils.h"
#include "port/esp_io_expander_tca9554.h"
#include "esp_expander_tca95xx_8bit.hpp"

namespace esp_expander {

TCA95XX_8BIT::~TCA95XX_8BIT()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool TCA95XX_8BIT::begin(void)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    // Initialize the device if not initialized
    if (!isOverState(State::INIT)) {
        ESP_UTILS_CHECK_FALSE_RETURN(init(), false, "Init failed");
    }

    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_io_expander_new_i2c_tca9554(
            static_cast<i2c_port_t>(getConfig().host_id), getConfig().device.address, &device_handle
        ), false, "Create TCA95XX_8BIT failed"
    );
    ESP_UTILS_LOGD("Create TCA95XX_8BIT @%p", device_handle);

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_expander

/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/esp_panel_utils_log.h"
#include "esp_panel_touch.hpp"

namespace esp_panel::drivers {

constexpr int THREAD_CHECK_STOP_INTERVAL_MS = 100;

void TouchPoint::print() const
{
    ESP_UTILS_LOGI("x(%d), y(%d), strength(%d)", x, y, strength);
}

void Touch::BasicAttributes::print() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGI(
        "\n\t{Basic attributes}"
        "\n\t\t-> [name]: %s"
        "\n\t\t-> [max_points_num]: %d"
        "\n\t\t-> [max_buttons_num]: %d"
        , name
        , static_cast<int>(max_points_num)
        , static_cast<int>(max_buttons_num)
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void Touch::Config::convertPartialToFull()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<DevicePartialConfig>(device)) {
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
        printDeviceConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG
        auto &config = std::get<DevicePartialConfig>(device);
        device = esp_lcd_touch_config_t{
            .x_max = static_cast<uint16_t>(config.x_max),
            .y_max = static_cast<uint16_t>(config.y_max),
            .rst_gpio_num = static_cast<gpio_num_t>(config.rst_gpio_num),
            .int_gpio_num = static_cast<gpio_num_t>(config.int_gpio_num),
            .levels = {
                .reset = static_cast<unsigned int>(config.levels_reset),
                .interrupt = static_cast<unsigned int>(config.levels_interrupt),
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
            .process_coordinates = nullptr,
            .interrupt_callback = nullptr,
            .user_data = nullptr,
            .driver_data = nullptr,
        };
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

const Touch::DeviceFullConfig *Touch::Config::getDeviceFullConfig() const
{
    ESP_UTILS_CHECK_FALSE_RETURN(std::holds_alternative<DeviceFullConfig>(device), nullptr, "Partial config");

    return &std::get<DeviceFullConfig>(device);
}

void Touch::Config::printDeviceConfig() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<DeviceFullConfig>(device)) {
        auto &config = std::get<DeviceFullConfig>(device);
        ESP_UTILS_LOGI(
            "\n\t{Device config}[full]"
            "\n\t\t-> [x_max]: %d"
            "\n\t\t-> [y_max]: %d"
            "\n\t\t-> [rst_gpio_num]: %d"
            "\n\t\t-> [int_gpio_num]: %d"
            "\n\t\t-> {levels}"
            "\n\t\t\t-> [reset]: %d"
            "\n\t\t\t-> [interrupt]: %d"
            "\n\t\t-> {flags}"
            "\n\t\t\t-> [swap_xy]: %d"
            "\n\t\t\t-> [mirror_x]: %d"
            "\n\t\t\t-> [mirror_y]: %d"
            , static_cast<int>(config.x_max)
            , static_cast<int>(config.y_max)
            , static_cast<int>(config.rst_gpio_num)
            , static_cast<int>(config.int_gpio_num)
            , static_cast<int>(config.levels.reset)
            , static_cast<int>(config.levels.interrupt)
            , static_cast<int>(config.flags.swap_xy)
            , static_cast<int>(config.flags.mirror_x)
            , static_cast<int>(config.flags.mirror_y)
        );
    } else {
        auto &config = std::get<DevicePartialConfig>(device);
        ESP_UTILS_LOGI(
            "\n\t{Device config}[partial]"
            "\n\t\t-> [x_max]: %d"
            "\n\t\t-> [y_max]: %d"
            "\n\t\t-> [rst_gpio_num]: %d"
            "\n\t\t-> [int_gpio_num]: %d"
            "\n\t\t-> [levels_reset]: %d"
            "\n\t\t-> [levels_interrupt]: %d"
            , static_cast<int>(config.x_max)
            , static_cast<int>(config.y_max)
            , static_cast<int>(config.rst_gpio_num)
            , static_cast<int>(config.int_gpio_num)
            , static_cast<int>(config.levels_reset)
            , static_cast<int>(config.levels_interrupt)
        );
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void Touch::configResetActiveLevel(int level)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(!isOverState(State::INIT), "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: level(%d)", level);
    getDeviceFullConfig().levels.reset = level;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void Touch::configInterruptActiveLevel(int level)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(!isOverState(State::INIT), "Should be called before `init()`");

    ESP_UTILS_LOGD("Param: level(%d)", level);
    getDeviceFullConfig().levels.interrupt = level;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool Touch::init()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");
    ESP_UTILS_CHECK_FALSE_RETURN(isBusValid(), false, "Invalid bus");

    // Begin the bus if it is not begun
    auto bus = getBus();
    if (!bus->isOverState(Bus::State::BEGIN)) {
        ESP_UTILS_CHECK_FALSE_RETURN(bus->begin(), false, "Bus begin failed");
    }

    // Convert the device partial configuration to full configuration
    _config.convertPartialToFull();
#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _basic_attributes.print();
    _config.printDeviceConfig();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    if (isInterruptEnabled()) {
        ESP_UTILS_LOGD("Enable interruption");

        std::shared_ptr<Interruption> interruption = nullptr;
        ESP_UTILS_CHECK_EXCEPTION_RETURN(
            interruption = utils::make_shared<Interruption>(), false, "Create interruption failed"
        );

        interruption->on_active_sem = xSemaphoreCreateBinaryStatic(&interruption->on_active_sem_buffer);
        interruption->data.touch_ptr = this;

        auto &device_config = getDeviceFullConfig();
        device_config.interrupt_callback = onInterruptActive;
        device_config.user_data = &interruption->data;

        _interruption = interruption;
    } else {
        ESP_UTILS_LOGD("Disable interruption");
    }

    setState(State::INIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (touch_panel != nullptr) {
        ESP_UTILS_CHECK_ERROR_RETURN(
            esp_lcd_touch_del(touch_panel), false, "Delete touch panel(@%p) failed", touch_panel
        );
        ESP_UTILS_LOGD("Touch panel(@%p) deleted", touch_panel);
        touch_panel = nullptr;
    }

    _transformation = {};
    _points.clear();
    _buttons.clear();
    _interruption = nullptr;

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::attachInterruptCallback(FunctionInterruptCallback callback, void *user_data)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::INIT), false, "Not initialized");
    ESP_UTILS_CHECK_FALSE_RETURN(isInterruptEnabled(), false, "Interruption is not enabled");

    ESP_UTILS_LOGD("Param: callback(@%p), user_data(@%p)", callback, user_data);
    _interruption->on_active_callback = callback;
    _interruption->data.user_data = user_data;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::swapXY(bool en)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Swap XY: %d", en);
    ESP_UTILS_CHECK_ERROR_RETURN(esp_lcd_touch_set_swap_xy(touch_panel, en), false, "Swap axes failed");
    _transformation.swap_xy = en;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::mirrorX(bool en)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: en(%d)", en);
    ESP_UTILS_CHECK_ERROR_RETURN(esp_lcd_touch_set_mirror_x(touch_panel, en), false, "Mirror X failed");
    _transformation.mirror_x = en;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::mirrorY(bool en)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: en(%d)", en);
    ESP_UTILS_CHECK_ERROR_RETURN(esp_lcd_touch_set_mirror_y(touch_panel, en), false, "Mirror Y failed");
    _transformation.mirror_y = en;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::readRawData(int points_num, int buttons_num, int timeout_ms)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: points_num(%d), buttons_num(%d), timeout_ms(%d)", points_num, buttons_num, timeout_ms);

    // Wait for the interruption if it is enabled, then read the raw data
    if (isInterruptEnabled()  && (timeout_ms != 0)) {
        ESP_UTILS_LOGD("Wait for interruption");
        BaseType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        if (xSemaphoreTake(_interruption->on_active_sem, timeout_ticks) != pdTRUE) {
            ESP_UTILS_LOGD("Wait timeout");
            return true;
        }
    }

    // Read the raw data
    ESP_UTILS_CHECK_ERROR_RETURN(esp_lcd_touch_read_data(touch_panel), false, "Read data failed");

    // Get the points
    ESP_UTILS_CHECK_FALSE_RETURN(readRawDataPoints(points_num), false, "Read points failed");

#if CONFIG_ESP_LCD_TOUCH_MAX_BUTTONS > 0
    // Get the buttons
    ESP_UTILS_CHECK_FALSE_RETURN(readRawDataButtons(buttons_num), false, "Read buttons failed");
#endif

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int Touch::getPoints(TouchPoint points[], uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: points(@%p), num(%d)", points, num);
    ESP_UTILS_CHECK_FALSE_RETURN((num == 0) || (points != nullptr), -1, "Invalid points or num");

    int i = 0;
    std::unique_lock lock(_resource_mutex);
    for (auto &point : _points) {
        if (i >= num) {
            break;
        }
        points[i++] = point;
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return i;
}

bool Touch::getPoints(utils::vector<TouchPoint> &points)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: points(@%p)", &points);
    std::unique_lock lock(_resource_mutex);
    points = _points;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::getPoints(std::vector<TouchPoint> &points)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: points(@%p)", &points);
    std::unique_lock lock(_resource_mutex);
    points = std::vector<TouchPoint>(_points.begin(), _points.end());

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int Touch::getButtons(TouchButton buttons[], uint8_t num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: buttons(@%p), num(%d)", buttons, num);
    ESP_UTILS_CHECK_FALSE_RETURN((num == 0) || (buttons != nullptr), false, "Invalid buttons or num");

    std::unique_lock lock(_resource_mutex);
    int i = 0;
    for (auto &button : _buttons) {
        if (i >= num) {
            break;
        }
        buttons[i++] = button;
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return i;
}

bool Touch::getButtons(utils::vector<TouchButton> &buttons)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: buttons(%p)", &buttons);
    std::unique_lock lock(_resource_mutex);
    buttons = _buttons;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::getButtons(std::vector<TouchButton> &buttons)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: buttons(@%p)", &buttons);
    std::unique_lock lock(_resource_mutex);
    buttons = std::vector<TouchButton>(_buttons.begin(), _buttons.end());

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int Touch::getButtonState(int index)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: index(%d)", index);
    bool is_found = false;
    TouchButton ret_button = {};

    std::unique_lock lock(_resource_mutex);
    for (auto &button : _buttons) {
        if (button.first == index) {
            is_found = true;
            ret_button = button;
            break;
        }
    }

    ESP_UTILS_CHECK_FALSE_RETURN(is_found, -1, "Index not found");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return ret_button.second;
}

int Touch::readPoints(TouchPoint points[], int num, int timeout_ms)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: points(@%p), num(%d), timeout_ms(%d)", points, num, timeout_ms);
    ESP_UTILS_CHECK_FALSE_RETURN(readRawData(num, 0, timeout_ms), -1, "Read raw data failed");
    int ret_num = getPoints(points, num);
    ESP_UTILS_CHECK_FALSE_RETURN(ret_num >= 0, -1, "Get points failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return ret_num;
}

bool Touch::readPoints(utils::vector<TouchPoint> &points, int timeout_ms)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: points(@%p), timeout_ms(%d)", &points, timeout_ms);
    ESP_UTILS_CHECK_FALSE_RETURN(readRawData(-1, 0, timeout_ms), false, "Read raw data failed");
    getPoints(points);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int Touch::readButtons(TouchButton buttons[], int num, int timeout_ms)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: buttons(@%p), num(%d), timeout_ms(%d)", buttons, num, timeout_ms);
    ESP_UTILS_CHECK_FALSE_RETURN(readRawData(0, num, timeout_ms), false, "Read raw data failed");
    int ret_num = getButtons(buttons, num);
    ESP_UTILS_CHECK_FALSE_RETURN(ret_num >= 0, -1, "Get buttons failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return ret_num;
}

bool Touch::readButtons(utils::vector<TouchButton> &buttons, int timeout_ms)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: buttons(@%p), timeout_ms(%d)", &buttons, timeout_ms);
    ESP_UTILS_CHECK_FALSE_RETURN(readRawData(0, -1, timeout_ms), false, "Read raw data failed");
    getButtons(buttons);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

int Touch::readButtonState(uint8_t index, int timeout_ms)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: index(%d), timeout_ms(%d)", index, timeout_ms);
    ESP_UTILS_CHECK_FALSE_RETURN(readRawData(0, index + 1, timeout_ms), -1, "Read raw data failed");
    int ret_state = getButtonState(index);
    ESP_UTILS_CHECK_FALSE_RETURN(ret_state, -1, "Get button state failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return ret_state;
}

bool Touch::isInterruptEnabled() const
{
    if (std::holds_alternative<DeviceFullConfig>(_config.device)) {
        return (std::get<DeviceFullConfig>(_config.device).int_gpio_num >= 0);
    }

    return (std::get<DevicePartialConfig>(_config.device).int_gpio_num >= 0);
}

Touch::DeviceFullConfig &Touch::getDeviceFullConfig()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (std::holds_alternative<DevicePartialConfig>(_config.device)) {
        _config.convertPartialToFull();
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return std::get<DeviceFullConfig>(_config.device);
}

bool Touch::readRawDataPoints(int points_num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGD("Param: points_num(%d)", points_num);

    // If less than 0, use the default points number
    if (points_num < 0) {
        points_num = getBasicAttributes().max_points_num;
    }
    // Limit the max points number
    if (points_num > POINTS_MAX_NUM) {
        ESP_UTILS_LOGW(
            "The target points number(%d) out of range, use the max number(%d) instead", points_num, POINTS_MAX_NUM
        );
        points_num = POINTS_MAX_NUM;
    }
    if (points_num <= 0) {
        ESP_UTILS_LOGD("Ignore to read points");
        return true;
    }
    ESP_UTILS_LOGD("Try to read %d points", points_num);

    utils::vector<uint16_t> x(points_num);
    utils::vector<uint16_t> y(points_num);
    utils::vector<uint16_t> strength(points_num);
    auto x_buf = x.data();
    auto y_buf = y.data();
    auto strength_buf = strength.data();
    uint8_t ret_points_num = 0;

    // Get the point coordinates from the raw data
    esp_lcd_touch_get_coordinates(touch_panel, x_buf, y_buf, strength_buf, &ret_points_num, points_num);
    ESP_UTILS_LOGD("Get %d points number", ret_points_num);

    // Update the points
    std::unique_lock lock(_resource_mutex);
    _points.clear();
    for (int i = 0; i < ret_points_num; i++) {
        _points.emplace_back(static_cast<int>(x_buf[i]), static_cast<int>(y_buf[i]), static_cast<int>(strength_buf[i]));
    }
    lock.unlock();

#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    for (auto &point : _points) {
        point.print();
    }
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Touch::readRawDataButtons(int buttons_num)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGD("Param: buttons_num(%d)", buttons_num);

    // If less than 0, use the default points number
    if (buttons_num < 0) {
        buttons_num = getBasicAttributes().max_buttons_num;
    }
    // Limit the max buttons number
    if (buttons_num > BUTTONS_MAX_NUM) {
        ESP_UTILS_LOGW(
            "The target buttons number(%d) out of range, use the max number(%d) instead", buttons_num, BUTTONS_MAX_NUM
        );
        buttons_num = BUTTONS_MAX_NUM;
    }
    if (buttons_num <= 0) {
        ESP_UTILS_LOGD("Ignore to read buttons");
        return true;
    }
    ESP_UTILS_LOGD("Try to read %d buttons", buttons_num);

    // Get the buttons state from the raw data
    utils::vector<TouchButton> buttons;
    uint8_t button_state = 0;

    for (int i = 0; i < buttons_num; i++) {
        button_state = 0;
#if CONFIG_ESP_LCD_TOUCH_MAX_BUTTONS > 0
        auto ret = esp_lcd_touch_get_button_state(touch_panel, i, &button_state);
        if (ret == ESP_ERR_INVALID_ARG) {
            ESP_UTILS_LOGD("Button(%d) is not supported", i);
            break;
        }
        ESP_UTILS_CHECK_ERROR_RETURN(ret, false, "Get button(%d) state failed", i);
#endif
        buttons.emplace_back(i, button_state);
    }

    std::unique_lock lock(_resource_mutex);
    _buttons = std::move(buttons);
    lock.unlock();

#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    for (auto &button : _buttons) {
        ESP_UTILS_LOGD("Button(%d): %d", button.first, button.second);
    }
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

void Touch::onInterruptActive(PanelHandle panel)
{
    if ((panel == nullptr) || (panel->config.user_data == nullptr)) {
        return;
    }

    Touch *touch = (Touch *)((Interruption::CallbackData *)panel->config.user_data)->touch_ptr;
    if (touch == nullptr) {
        return;
    }

    auto interruption = touch->_interruption;
    if (interruption == nullptr) {
        return;
    }

    BaseType_t need_yield = pdFALSE;
    if (interruption->on_active_callback != nullptr) {
        need_yield = interruption->on_active_callback(interruption->data.user_data) ? pdTRUE : need_yield;
    }
    if (interruption->on_active_sem != nullptr) {
        xSemaphoreGiveFromISR(interruption->on_active_sem, &need_yield);
    }
    if (need_yield == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

} // namespace esp_panel::drivers

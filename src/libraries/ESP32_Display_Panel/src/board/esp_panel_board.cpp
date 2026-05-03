/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>
#include "utils/esp_panel_utils_log.h"
#include "drivers/io_expander/esp_panel_io_expander_adapter.hpp"
#include "esp_panel_board.hpp"
#include "esp_panel_board_private.hpp"
#include "esp_panel_board_default_config.hpp"

#undef _TO_DRIVERS_CLASS
#undef TO_DRIVERS_CLASS
#define _TO_DRIVERS_CLASS(type, name)  drivers::type ## name
#define TO_DRIVERS_CLASS(type, name)   _TO_DRIVERS_CLASS(type, name)

#undef _TO_STR
#undef TO_STR
#define _TO_STR(name) #name
#define TO_STR(name) _TO_STR(name)

namespace esp_panel::board {

#if ESP_PANEL_BOARD_USE_DEFAULT
Board::Board():
    Board(ESP_PANEL_BOARD_DEFAULT_CONFIG)
{
    _use_default_config = true;
}
#else
Board::Board()
{
    _use_default_config = true;
}
#endif

Board::~Board()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool Board::init()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");
    if (!_config.isValid()) {
#if !ESP_PANEL_BOARD_USE_DEFAULT
        ESP_UTILS_CHECK_FALSE_RETURN(
            !_use_default_config, false,
            "\nNo default board configuration detected. There are three ways to provide a default configuration: "
            "\n\t1. Use the `esp_panel_board_supported_conf.h` file to enable a supported board. "
            "\n\t2. Use the `esp_panel_board_custom_conf.h` file to define a custom board. "
            "\n\t3. Use menuconfig to enable a supported board or define a custom board. "
        );
#endif
        ESP_UTILS_CHECK_FALSE_RETURN(false, false, "Invalid board configuration");
    }

    ESP_UTILS_LOGI("Initializing board (%s)", _config.name);

    // Create LCD device if it is used
    std::shared_ptr<drivers::Bus> lcd_bus = nullptr;
    std::shared_ptr<drivers::LCD> lcd_device = nullptr;
    if (isLCD_Used()) {
        auto &lcd_config = _config.lcd.value();
        ESP_UTILS_LOGD("Creating LCD (%s)", lcd_config.device_name);

#if ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_LCD
        // If the LCD is configured by default, it will be created by the constructor
        if (_use_default_config) {
            using Bus_Class = TO_DRIVERS_CLASS(Bus, ESP_PANEL_BOARD_LCD_BUS_NAME);
            using LCD_Class = TO_DRIVERS_CLASS(LCD_, ESP_PANEL_BOARD_LCD_CONTROLLER);

            ESP_UTILS_CHECK_FALSE_RETURN(
                std::holds_alternative<Bus_Class::Config>(lcd_config.bus_config), false,
                "LCD bus config is not a " TO_STR(ESP_PANEL_BOARD_LCD_BUS_NAME) " bus config"
            );
            ESP_UTILS_CHECK_EXCEPTION_RETURN(
                (lcd_bus = utils::make_shared<Bus_Class>(std::get<Bus_Class::Config>(lcd_config.bus_config))), false,
                "Create LCD bus failed"
            );
            ESP_UTILS_CHECK_EXCEPTION_RETURN(
                (lcd_device = utils::make_shared<LCD_Class>(lcd_bus.get(), lcd_config.device_config)), false,
                "Create LCD device failed"
            );
        }
#endif // ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_LCD

        // If the LCD is not configured by default, it will be created by the factory function
        if (!_use_default_config) {
            lcd_device =
                drivers::LCD_Factory::create(lcd_config.device_name, lcd_config.bus_config, lcd_config.device_config);
            ESP_UTILS_CHECK_NULL_RETURN(lcd_device, false, "Create LCD device failed");
        }

        ESP_UTILS_CHECK_NULL_RETURN(lcd_device, false, "Create LCD failed");
        ESP_UTILS_LOGD("LCD create success");
    }

    // Create touch device if it is used
    std::shared_ptr<drivers::Bus> touch_bus = nullptr;
    std::shared_ptr<drivers::Touch> touch_device = nullptr;
    if (isTouchUsed()) {
        auto &touch_config = _config.touch.value();
        ESP_UTILS_LOGD("Creating touch (%s)", touch_config.device_name);

#if ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_TOUCH
        // If the touch is configured by default, it will be created by the constructor
        if (_use_default_config) {
            using Bus_Class = TO_DRIVERS_CLASS(Bus, ESP_PANEL_BOARD_TOUCH_BUS_NAME);
            using Touch_Class = TO_DRIVERS_CLASS(Touch, ESP_PANEL_BOARD_TOUCH_CONTROLLER);

            ESP_UTILS_CHECK_FALSE_RETURN(
                std::holds_alternative<Bus_Class::Config>(touch_config.bus_config), false,
                "Touch bus config is not a " TO_STR(ESP_PANEL_BOARD_TOUCH_BUS_NAME) " bus config"
            );
            ESP_UTILS_CHECK_EXCEPTION_RETURN(
                (touch_bus = utils::make_shared<Bus_Class>(std::get<Bus_Class::Config>(touch_config.bus_config))),
                false, "Create touch bus failed"
            );
            ESP_UTILS_CHECK_EXCEPTION_RETURN(
                (touch_device = utils::make_shared<Touch_Class>(touch_bus.get(), touch_config.device_config)),
                false, "Create touch device failed"
            );
        }
#endif // ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_TOUCH

        // If the touch is not configured by default, it will be created by the factory function
        if (!_use_default_config) {
            touch_device =
                drivers::TouchFactory::create(
                    touch_config.device_name, touch_config.bus_config, touch_config.device_config
                );
            ESP_UTILS_CHECK_NULL_RETURN(touch_device, false, "Create touch device failed");
        }

        ESP_UTILS_CHECK_NULL_RETURN(touch_device, false, "Create touch failed");
        ESP_UTILS_LOGD("Touch create success");
    }

    // Create backlight device if it is used
    std::shared_ptr<drivers::Backlight> backlight = nullptr;
    if (isBacklightUsed()) {
        auto &backlight_config = _config.backlight.value();
        auto type = drivers::BacklightFactory::getConfigType(backlight_config.config);
        ESP_UTILS_LOGD("Creating backlight (%s[%d])", drivers::BacklightFactory::getTypeNameString(type).c_str(), type);

        // If the backlight is a custom backlight, the user data should be set to the board instance `this`
        if (type == ESP_PANEL_BACKLIGHT_TYPE_CUSTOM) {
            using BacklightConfig = drivers::BacklightCustom::Config;

            ESP_UTILS_CHECK_FALSE_RETURN(
                std::holds_alternative<BacklightConfig>(backlight_config.config), false,
                "Backlight config is not a custom backlight config"
            );
            auto &config = std::get<BacklightConfig>(backlight_config.config);
            config.user_data = this;
        }

#if ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_BACKLIGHT
        // If the backlight is configured by default, it will be created by the constructor
        if (_use_default_config) {
            using BacklightClass = TO_DRIVERS_CLASS(Backlight, ESP_PANEL_BOARD_BACKLIGHT_NAME);

            ESP_UTILS_CHECK_FALSE_RETURN(
                std::holds_alternative<BacklightClass::Config>(backlight_config.config), false,
                "Backlight config is not a " TO_STR(ESP_PANEL_BOARD_BACKLIGHT_NAME) " backlight config"
            );
            ESP_UTILS_CHECK_EXCEPTION_RETURN(
                (backlight =
                     utils::make_shared<BacklightClass>(std::get<BacklightClass::Config>(backlight_config.config))),
                false, "Create backlight device failed"
            );
        }
#endif // ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_BACKLIGHT

        // If the backlight is not configured by default, it will be created by the factory function
        if (!_use_default_config) {
            backlight = drivers::BacklightFactory::create(backlight_config.config);
            ESP_UTILS_CHECK_NULL_RETURN(backlight, false, "Create backlight device failed");
        }

        ESP_UTILS_CHECK_NULL_RETURN(backlight, false, "Create backlight failed");
        ESP_UTILS_LOGD("Backlight create success");
    }

    // Create IO expander device if it is used
    // If the IO expander is already configured, it will not be created again
    std::shared_ptr<drivers::IO_Expander> io_expander = nullptr;
    if (isIO_ExpanderUsed() && getIO_Expander() == nullptr) {
        auto &expander_config = _config.io_expander.value();
        ESP_UTILS_LOGD("Creating IO Expander (%s)", expander_config.name);

#if ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_EXPANDER
        // If the IO expander is configured by default, it will be created by the default constructor
        if (_use_default_config) {
            using IO_ExpanderClass = drivers::IO_ExpanderAdapter<esp_expander::ESP_PANEL_BOARD_EXPANDER_CHIP>;
            ESP_UTILS_CHECK_EXCEPTION_RETURN(
                (io_expander =
                     utils::make_shared<IO_ExpanderClass>(
                         drivers::IO_Expander::BasicAttributes{TO_STR(ESP_PANEL_BOARD_EXPANDER_CHIP)},
                         expander_config.config
                     )), false, "Create IO expander device failed"
            );
        }
#endif // ESP_PANEL_BOARD_USE_DEFAULT && ESP_PANEL_BOARD_USE_EXPANDER

        // If the IO expander is not configured by default, it will be created by the factory function
        if (!_use_default_config) {
            io_expander = drivers::IO_ExpanderFactory::create(expander_config.name, expander_config.config);
            ESP_UTILS_CHECK_NULL_RETURN(io_expander, false, "Create IO expander device failed");
        }

        ESP_UTILS_CHECK_NULL_RETURN(io_expander, false, "Create IO expander failed");

        ESP_UTILS_LOGD("IO Expander create success");
    }

    _lcd_bus = lcd_bus;
    _lcd_device = lcd_device;
    _touch_bus = touch_bus;
    _touch_device = touch_device;
    _backlight = backlight;
    _io_expander = io_expander;

    setState(State::INIT);

    ESP_UTILS_LOGI("Board initialize success");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Board::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    // Initialize the board if not initialized
    if (!isOverState(State::INIT)) {
        ESP_UTILS_CHECK_FALSE_RETURN(init(), false, "Init failed");
    }

    ESP_UTILS_LOGI("Beginning board (%s)", _config.name);

    auto &config = getConfig();
    if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_BOARD_BEGIN] != nullptr) {
        ESP_UTILS_LOGD("Board pre-begin");
        ESP_UTILS_CHECK_FALSE_RETURN(
            config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_BOARD_BEGIN](this), false, "Board pre-begin failed"
        );
    }

    // Begin the IO expander if it is used
    // If the IO expander is already begun, it will not be begun again
    auto io_expander = getIO_Expander();
    if (io_expander != nullptr && !io_expander->isOverState(esp_expander::Base::State::BEGIN)) {
        ESP_UTILS_LOGD("Beginning IO Expander");

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_EXPANDER_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("IO expander pre-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_EXPANDER_BEGIN](this), false,
                "IO expander pre-begin failed"
            );
        }

        ESP_UTILS_CHECK_FALSE_RETURN(io_expander->begin(), false, "IO expander begin failed");

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_EXPANDER_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("IO expander post-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_EXPANDER_BEGIN](this), false,
                "IO expander post-begin failed"
            );
        }

        ESP_UTILS_LOGD("IO expander begin success");
    }

    // Begin the LCD if it is used
    auto lcd_device = getLCD();
    if (lcd_device != nullptr) {
        ESP_UTILS_LOGD("Beginning LCD");

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_LCD_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("LCD pre-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_LCD_BEGIN](this), false, "LCD pre-begin failed"
            );
        }

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
        drivers::Bus *lcd_bus = lcd_device->getBus();
        // When using "3-wire SPI + RGB" LCD, the IO expander should be configured first
        if ((lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) &&
                std::get<drivers::BusRGB::Config>(config.lcd.value().bus_config).isControlPanelValid() &&
                (_io_expander != nullptr)) {
            ESP_UTILS_CHECK_FALSE_RETURN(
                static_cast<drivers::BusRGB *>(lcd_bus)->configSPI_IO_Expander(
                    io_expander->getBase()->getDeviceHandle()
                ), false, "\"3-wire SPI + RGB \" LCD bus config IO expander failed"
            );
        }
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
        ESP_UTILS_CHECK_FALSE_RETURN(lcd_device->begin(), false, "LCD device begin failed");
        if (lcd_device->isFunctionSupported(drivers::LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) {
            ESP_UTILS_CHECK_FALSE_RETURN(lcd_device->setDisplayOnOff(true), false, "LCD device set display on failed");
        } else {
            ESP_UTILS_LOGD("LCD device doesn't support display on/off function");
        }

        auto &lcd_config = _config.lcd.value();
        if (lcd_device->isFunctionSupported(drivers::LCD::BasicBusSpecification::FUNC_INVERT_COLOR)) {
            ESP_UTILS_CHECK_FALSE_RETURN(
                lcd_device->invertColor(lcd_config.pre_process.invert_color), false, "LCD device invert color failed"
            );
        } else {
            ESP_UTILS_LOGD("LCD device doesn't support invert color function");
        }
        if (lcd_device->isFunctionSupported(drivers::LCD::BasicBusSpecification::FUNC_SWAP_XY)) {
            ESP_UTILS_CHECK_FALSE_RETURN(
                lcd_device->swapXY(lcd_config.pre_process.swap_xy), false, "LCD device swap XY failed"
            );
        } else {
            ESP_UTILS_LOGD("LCD device doesn't support swap XY function");
        }
        if (lcd_device->isFunctionSupported(drivers::LCD::BasicBusSpecification::FUNC_MIRROR_X)) {
            ESP_UTILS_CHECK_FALSE_RETURN(
                lcd_device->mirrorX(lcd_config.pre_process.mirror_x), false, "LCD device mirror X failed"
            );
        } else {
            ESP_UTILS_LOGD("LCD device doesn't support mirror X function");
        }
        if (lcd_device->isFunctionSupported(drivers::LCD::BasicBusSpecification::FUNC_MIRROR_Y)) {
            ESP_UTILS_CHECK_FALSE_RETURN(
                lcd_device->mirrorY(lcd_config.pre_process.mirror_y), false, "LCD device mirror Y failed"
            );
        } else {
            ESP_UTILS_LOGD("LCD device doesn't support mirror X function");
        }
        if (lcd_device->isFunctionSupported(drivers::LCD::BasicBusSpecification::FUNC_GAP)) {
            ESP_UTILS_CHECK_FALSE_RETURN(
                lcd_device->setGapX(lcd_config.pre_process.gap_x), false, "LCD device set gap X failed"
            );
            ESP_UTILS_CHECK_FALSE_RETURN(
                lcd_device->setGapY(lcd_config.pre_process.gap_y), false, "LCD device set gap Y failed"
            );
        } else {
            ESP_UTILS_LOGD("LCD device doesn't support gap function");
        }

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_LCD_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("LCD post-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_LCD_BEGIN](this), false, "LCD post-begin failed"
            );
        }

        ESP_UTILS_LOGD("LCD begin success");
    }

    // Begin the touch if it is used
    auto touch_device = getTouch();
    if (touch_device != nullptr) {
        ESP_UTILS_LOGD("Beginning touch");

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_TOUCH_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("Touch pre-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_TOUCH_BEGIN](this), false,
                "Touch pre-begin failed"
            );
        }

        ESP_UTILS_CHECK_FALSE_RETURN(touch_device->begin(), false, "Touch device begin failed");

        auto &touch_config = _config.touch.value();
        ESP_UTILS_CHECK_FALSE_RETURN(
            touch_device->swapXY(touch_config.pre_process.swap_xy), false, "Touch device swap XY failed"
        );
        ESP_UTILS_CHECK_FALSE_RETURN(
            touch_device->mirrorX(touch_config.pre_process.mirror_x), false, "Touch device mirror X failed"
        );
        ESP_UTILS_CHECK_FALSE_RETURN(
            touch_device->mirrorY(touch_config.pre_process.mirror_y), false, "Touch device mirror Y failed"
        );

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_TOUCH_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("Touch post-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_TOUCH_BEGIN](this), false,
                "Touch post-begin failed"
            );
        }

        ESP_UTILS_LOGD("Touch begin success");
    }

    // Begin the backlight if it is used
    auto backlight = getBacklight();
    if (backlight != nullptr) {
        ESP_UTILS_LOGD("Beginning backlight");

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_BACKLIGHT_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("Backlight pre-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_BACKLIGHT_BEGIN](this), false,
                "Backlight pre-begin failed"
            );
        }

        auto &backlight_config = _config.backlight.value();
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_EXPANDER
        // If the backlight is a switch expander, the IO expander should be configured
        if (drivers::BacklightFactory::getConfigType(backlight_config.config) ==
                ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER) {
            auto *temp_backlight = static_cast<drivers::BacklightSwitchExpander *>(backlight);
            // Only configure the IO expander if it is not already configured
            if (temp_backlight->getIO_Expander() == nullptr) {
                ESP_UTILS_CHECK_NULL_RETURN(io_expander, false, "Need IO expander to control backlight");
                temp_backlight->configIO_Expander(io_expander->getBase());
            }
        }
#endif // ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_EXPANDER

        ESP_UTILS_CHECK_FALSE_RETURN(backlight->begin(), false, "Backlight begin failed");
        if (backlight_config.pre_process.idle_off) {
            ESP_UTILS_CHECK_FALSE_RETURN(backlight->off(), false, "Backlight off failed");
        } else {
            ESP_UTILS_CHECK_FALSE_RETURN(backlight->on(), false, "Backlight on failed");
        }

        if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_BACKLIGHT_BEGIN] != nullptr) {
            ESP_UTILS_LOGD("Backlight post-begin");
            ESP_UTILS_CHECK_FALSE_RETURN(
                config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_BACKLIGHT_BEGIN](this), false,
                "Backlight post-begin failed"
            );
        }

        ESP_UTILS_LOGD("Backlight begin success");
    }

    if (config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_BOARD_BEGIN] != nullptr) {
        ESP_UTILS_LOGD("Board post-begin");
        ESP_UTILS_CHECK_FALSE_RETURN(
            config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_BOARD_BEGIN](this), false, "Board post-begin failed"
        );
    }

    setState(State::BEGIN);

    ESP_UTILS_LOGI("Board begin success");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Board::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    auto &config = getConfig();

    if (!isOverState(State::INIT)) {
        goto end;
    }

    ESP_UTILS_LOGI("Deleting board (%s)", config.name);

    if (isOverState(State::BEGIN) && config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_BOARD_DEL] != nullptr) {
        ESP_UTILS_LOGD("Board pre-delete");
        ESP_UTILS_CHECK_FALSE_RETURN(
            config.stage_callbacks[BoardConfig::STAGE_CALLBACK_PRE_BOARD_DEL](this), false, "Board pre-delete failed"
        );
    }

    _backlight = nullptr;
    _lcd_device = nullptr;
    _lcd_bus = nullptr;
    _touch_device = nullptr;
    _touch_bus = nullptr;
    _io_expander = nullptr;

    if (isOverState(State::BEGIN) && config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_BOARD_DEL] != nullptr) {
        ESP_UTILS_LOGD("Board post-delete");
        ESP_UTILS_CHECK_FALSE_RETURN(
            config.stage_callbacks[BoardConfig::STAGE_CALLBACK_POST_BOARD_DEL](this), false, "Board post-delete failed"
        );
    }

    setState(State::DEINIT);

    ESP_UTILS_LOGI("Board delete success");

end:
    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Board::configIO_Expander(drivers::IO_Expander *expander)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");

    _io_expander = std::shared_ptr<drivers::IO_Expander>(expander, [](drivers::IO_Expander * expander) {
        ESP_UTILS_LOGD("Skip delete IO expander");
    });

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool Board::configCallback(board::BoardConfig::StageCallbackType type, BoardConfig::FunctionStageCallback callback)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");
    ESP_UTILS_CHECK_FALSE_RETURN(type < BoardConfig::STAGE_CALLBACK_MAX, false, "Invalid callback type");

    _config.stage_callbacks[type] = callback;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel

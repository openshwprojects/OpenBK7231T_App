/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <optional>
#include "drivers/bus/esp_panel_bus_factory.hpp"
#include "drivers/lcd/esp_panel_lcd_factory.hpp"
#include "drivers/touch/esp_panel_touch_factory.hpp"
#include "drivers/backlight/esp_panel_backlight_factory.hpp"
#include "drivers/io_expander/esp_panel_io_expander_factory.hpp"

namespace esp_panel::board {

/**
 * @brief Configuration structure for ESP Panel board
 *
 * Contains all configurations needed for initializing an ESP Panel board
 */
struct BoardConfig {
    /**
     * @brief Function pointer type for board operation stage callback functions
     *
     * @param[in] args User arguments passed to the callback function, usually the pointer of the board itself
     * @return `true` if successful, `false` otherwise
     */
    using FunctionStageCallback = bool (*)(void *args);

    /**
     * @brief Stage callback type enum
     */
    enum StageCallbackType {
        STAGE_CALLBACK_PRE_BOARD_BEGIN = 0,
        STAGE_CALLBACK_POST_BOARD_BEGIN,
        STAGE_CALLBACK_PRE_BOARD_DEL,
        STAGE_CALLBACK_POST_BOARD_DEL,
        STAGE_CALLBACK_PRE_EXPANDER_BEGIN,
        STAGE_CALLBACK_POST_EXPANDER_BEGIN,
        STAGE_CALLBACK_PRE_LCD_BEGIN,
        STAGE_CALLBACK_POST_LCD_BEGIN,
        STAGE_CALLBACK_PRE_TOUCH_BEGIN,
        STAGE_CALLBACK_POST_TOUCH_BEGIN,
        STAGE_CALLBACK_PRE_BACKLIGHT_BEGIN,
        STAGE_CALLBACK_POST_BACKLIGHT_BEGIN,
        STAGE_CALLBACK_MAX,
    };

    /**
     * @brief LCD related configuration
     */
    struct LCD_Config {
        drivers::BusFactory::Config bus_config;     /*!< LCD bus configuration */
        const char *device_name = "";               /*!< LCD device name */
        drivers::LCD::Config device_config;         /*!< LCD device configuration */
        struct PreProcess {
            int invert_color: 1;                    /*!< Invert color if set to 1 */
            int swap_xy: 1;                         /*!< Swap X and Y coordinates if set to 1 */
            int mirror_x: 1;                        /*!< Mirror X coordinate if set to 1 */
            int mirror_y: 1;                        /*!< Mirror Y coordinate if set to 1 */
            int gap_x;                              /*!< Gap of X start coordinate */
            int gap_y;                              /*!< Gap of Y start coordinate */
        } pre_process;                              /*!< LCD pre-process flags */
    };

    /**
     * @brief Touch related configuration
     */
    struct TouchConfig {
        drivers::BusFactory::Config bus_config;     /*!< Touch bus configuration */
        const char *device_name = "";               /*!< Touch device name */
        drivers::Touch::Config device_config;       /*!< Touch device configuration */
        struct PreProcess {
            int swap_xy: 1;                         /*!< Swap X and Y coordinates if set to 1 */
            int mirror_x: 1;                        /*!< Mirror X coordinate if set to 1 */
            int mirror_y: 1;                        /*!< Mirror Y coordinate if set to 1 */
        } pre_process;                              /*!< Touch pre-process flags */
    };

    /**
     * @brief Backlight related configuration
     */
    struct BacklightConfig {
        drivers::BacklightFactory::Config config;   /*!< Backlight device configuration */
        struct PreProcess {
            int idle_off: 1;                        /*!< Turn off backlight in idle mode if set to 1 */
        } pre_process;                              /*!< Backlight pre-process flags */
    };

    /**
     * @brief IO expander related configuration
     */
    struct IO_ExpanderConfig {
        const char *name = "";                      /*!< IO expander device name */
        drivers::IO_Expander::Config config;        /*!< IO expander device configuration */
    };

    bool isValid() const
    {
        return (name != nullptr) && (strlen(name) > 0);
    }

    const char *name = "";                          /*!< Board name */
    std::optional<LCD_Config> lcd;                  /*!< LCD configuration */
    std::optional<TouchConfig> touch;               /*!< Touch configuration */
    std::optional<BacklightConfig> backlight;       /*!< Backlight configuration */
    std::optional<IO_ExpanderConfig> io_expander;   /*!< IO expander configuration */
    std::array<FunctionStageCallback, STAGE_CALLBACK_MAX> stage_callbacks; /*!< Stage callback functions */
};

} // namespace esp_panel

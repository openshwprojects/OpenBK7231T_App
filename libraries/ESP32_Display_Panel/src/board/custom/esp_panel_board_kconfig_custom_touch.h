/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

// *INDENT-OFF*

// Bool type: default to 0 if not defined
#ifndef ESP_PANEL_BOARD_USE_TOUCH
    #ifdef CONFIG_ESP_PANEL_BOARD_USE_TOUCH
        #define ESP_PANEL_BOARD_USE_TOUCH CONFIG_ESP_PANEL_BOARD_USE_TOUCH
    #else
        #define ESP_PANEL_BOARD_USE_TOUCH 0
    #endif
#endif

#if ESP_PANEL_BOARD_USE_TOUCH
    // Controller (Non-bool: error if not defined)
    #ifndef ESP_PANEL_BOARD_TOUCH_CONTROLLER
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_AXS15231B
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER AXS15231B
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_CST816S)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER CST816S
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_FT5x06)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER FT5x06
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_GT911)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER GT911
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_GT1151)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER GT1151
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_SPD2010)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER SPD2010
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_ST1633)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER ST1633
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_ST7123)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER ST7123
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_STMPE610)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER STMPE610
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_TT21100)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER TT21100
        #elif defined(CONFIG_ESP_PANEL_BOARD_TOUCH_CONTROLLER_XPT2046)
            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER XPT2046
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_CONTROLLER"
        #endif
    #endif

    // Bus Settings
    // Non-bool: error if not defined
    #ifndef ESP_PANEL_BOARD_TOUCH_BUS_TYPE
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_BUS_TYPE
            #define ESP_PANEL_BOARD_TOUCH_BUS_TYPE CONFIG_ESP_PANEL_BOARD_TOUCH_BUS_TYPE
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_BUS_TYPE"
        #endif
    #endif

    // Bool type: default to 0 if not defined
    #if (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI) || \
        (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C)
        #ifndef ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
            #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
                #define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST CONFIG_ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
            #else
                #define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST 0
            #endif
        #endif
    #endif

    #if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C  // I2C
        #ifndef ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID
            #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID
                #define ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID"
            #endif
        #endif

        // Non-bool: error if not defined
        #ifndef ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS
            #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS
                #define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS"
            #endif
        #endif

        #if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
            #ifndef ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ
                    #define ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL
                    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA
                    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA"
                #endif
            #endif

            // Bool type: default to 0 if not defined
            #ifndef ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP
                    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP
                #else
                    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP 0
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP
                    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP CONFIG_ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP
                #else
                    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP 0
                #endif
            #endif
        #endif

    #elif ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI  // SPI

        #ifndef ESP_PANEL_BOARD_TOUCH_SPI_HOST_ID
            #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_HOST_ID
                #define ESP_PANEL_BOARD_TOUCH_SPI_HOST_ID CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_HOST_ID
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_SPI_HOST_ID"
            #endif
        #endif

        #if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
            #ifndef ESP_PANEL_BOARD_TOUCH_SPI_CLK_HZ
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_CLK_HZ
                    #define ESP_PANEL_BOARD_TOUCH_SPI_CLK_HZ CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_CLK_HZ
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_SPI_CLK_HZ"
                #endif
            #endif
        #endif

        // Non-bool: error if not defined
        #ifndef ESP_PANEL_BOARD_TOUCH_SPI_IO_CS
            #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_CS
                #define ESP_PANEL_BOARD_TOUCH_SPI_IO_CS CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_CS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_SPI_IO_CS"
            #endif
        #endif

        #if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
            #ifndef ESP_PANEL_BOARD_TOUCH_SPI_IO_SCK
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_SCK
                    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_SCK CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_SCK
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_SPI_IO_SCK"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_TOUCH_SPI_IO_MOSI
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_MOSI
                    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_MOSI CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_MOSI
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_SPI_IO_MOSI"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_TOUCH_SPI_IO_MISO
                #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_MISO
                    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_MISO CONFIG_ESP_PANEL_BOARD_TOUCH_SPI_IO_MISO
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_SPI_IO_MISO"
                #endif
            #endif
        #endif
    #endif /* ESP_PANEL_BOARD_TOUCH_BUS_TYPE */

    // Transformation Settings (Bool type: default to 0 if not defined)
    #ifndef ESP_PANEL_BOARD_TOUCH_SWAP_XY
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_SWAP_XY
            #define ESP_PANEL_BOARD_TOUCH_SWAP_XY CONFIG_ESP_PANEL_BOARD_TOUCH_SWAP_XY
        #else
            #define ESP_PANEL_BOARD_TOUCH_SWAP_XY 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_TOUCH_MIRROR_X
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_MIRROR_X
            #define ESP_PANEL_BOARD_TOUCH_MIRROR_X CONFIG_ESP_PANEL_BOARD_TOUCH_MIRROR_X
        #else
            #define ESP_PANEL_BOARD_TOUCH_MIRROR_X 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_TOUCH_MIRROR_Y
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_MIRROR_Y
            #define ESP_PANEL_BOARD_TOUCH_MIRROR_Y CONFIG_ESP_PANEL_BOARD_TOUCH_MIRROR_Y
        #else
            #define ESP_PANEL_BOARD_TOUCH_MIRROR_Y 0
        #endif
    #endif

    // Control Pins
    // Non-bool: error if not defined
    #ifndef ESP_PANEL_BOARD_TOUCH_RST_IO
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_RST_IO
            #define ESP_PANEL_BOARD_TOUCH_RST_IO CONFIG_ESP_PANEL_BOARD_TOUCH_RST_IO
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_RST_IO"
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_TOUCH_INT_IO
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_INT_IO
            #define ESP_PANEL_BOARD_TOUCH_INT_IO CONFIG_ESP_PANEL_BOARD_TOUCH_INT_IO
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_TOUCH_INT_IO"
        #endif
    #endif

    // Bool type: default to 0 if not defined
    #ifndef ESP_PANEL_BOARD_TOUCH_RST_LEVEL
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_RST_LEVEL
            #define ESP_PANEL_BOARD_TOUCH_RST_LEVEL CONFIG_ESP_PANEL_BOARD_TOUCH_RST_LEVEL
        #else
            #define ESP_PANEL_BOARD_TOUCH_RST_LEVEL 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_TOUCH_INT_LEVEL
        #ifdef CONFIG_ESP_PANEL_BOARD_TOUCH_INT_LEVEL
            #define ESP_PANEL_BOARD_TOUCH_INT_LEVEL CONFIG_ESP_PANEL_BOARD_TOUCH_INT_LEVEL
        #else
            #define ESP_PANEL_BOARD_TOUCH_INT_LEVEL 0
        #endif
    #endif
#endif // ESP_PANEL_BOARD_USE_TOUCH

// *INDENT-ON*

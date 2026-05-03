/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

// *INDENT-OFF*

// Bool type: default to 0 if not defined
#ifndef ESP_PANEL_BOARD_USE_LCD
    #ifdef CONFIG_ESP_PANEL_BOARD_USE_LCD
        #define ESP_PANEL_BOARD_USE_LCD CONFIG_ESP_PANEL_BOARD_USE_LCD
    #else
        #define ESP_PANEL_BOARD_USE_LCD 0
    #endif
#endif

#if ESP_PANEL_BOARD_USE_LCD
    // Controller (Non-bool: error if not defined)
    #ifndef ESP_PANEL_BOARD_LCD_CONTROLLER
        #ifdef CONFIG_ESP_PANEL_LCD_CONTROLLER_AXS15231B
            #define ESP_PANEL_BOARD_LCD_CONTROLLER AXS15231B
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_EK9716B)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER EK9716B
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_EK79007)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER EK79007
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_GC9A01)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER GC9A01
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_GC9B71)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER GC9B71
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_GC9503)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER GC9503
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_HX8399)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER HX8399
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ILI9341)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ILI9341
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ILI9881C)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ILI9881C
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_JD9165)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER JD9165
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_JD9365)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER JD9365
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_NV3022B)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER NV3022B
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_SH8601)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER SH8601
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_SPD2010)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER SPD2010
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST7262)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST7262
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST7701)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST7701
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST7703)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST7703
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST7789)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST7789
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST7796)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST7796
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST77903)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST77903
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST77916)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST77916
        #elif defined(CONFIG_ESP_PANEL_LCD_CONTROLLER_ST77922)
            #define ESP_PANEL_BOARD_LCD_CONTROLLER ST77922
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_LCD_CONTROLLER"
        #endif
    #endif

    // Bus Settings
    // Non-bool: error if not defined
    #ifndef ESP_PANEL_BOARD_LCD_BUS_TYPE
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_BUS_TYPE
            #define ESP_PANEL_BOARD_LCD_BUS_TYPE CONFIG_ESP_PANEL_BOARD_LCD_BUS_TYPE
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_LCD_BUS_TYPE"
        #endif
    #endif

    // Bool type: default to 0 if not defined
    #if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI) || \
        (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI)
        #ifndef ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
                #define ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST CONFIG_ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
            #else
                #define ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST 0
            #endif
        #endif
    #endif

    // SPI Bus Settings
    #if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI) // SPI
        // Non-bool: error if not defined
        #ifndef ESP_PANEL_BOARD_LCD_SPI_HOST_ID
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_HOST_ID
                #define ESP_PANEL_BOARD_LCD_SPI_HOST_ID CONFIG_ESP_PANEL_BOARD_LCD_SPI_HOST_ID
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_HOST_ID"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_SPI_MODE
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_MODE
                #define ESP_PANEL_BOARD_LCD_SPI_MODE CONFIG_ESP_PANEL_BOARD_LCD_SPI_MODE
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_MODE"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_SPI_CLK_HZ
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_CLK_HZ
                #define ESP_PANEL_BOARD_LCD_SPI_CLK_HZ CONFIG_ESP_PANEL_BOARD_LCD_SPI_CLK_HZ
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_CLK_HZ"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_SPI_CMD_BITS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_CMD_BITS
                #define ESP_PANEL_BOARD_LCD_SPI_CMD_BITS CONFIG_ESP_PANEL_BOARD_LCD_SPI_CMD_BITS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_CMD_BITS"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_SPI_PARAM_BITS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_PARAM_BITS
                #define ESP_PANEL_BOARD_LCD_SPI_PARAM_BITS CONFIG_ESP_PANEL_BOARD_LCD_SPI_PARAM_BITS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_PARAM_BITS"
            #endif
        #endif

        // SPI Pins
        #ifndef ESP_PANEL_BOARD_LCD_SPI_IO_CS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_CS
                #define ESP_PANEL_BOARD_LCD_SPI_IO_CS CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_CS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_IO_CS"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_SPI_IO_DC
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_DC
                #define ESP_PANEL_BOARD_LCD_SPI_IO_DC CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_DC
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_IO_DC"
            #endif
        #endif

        #if !ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
            #ifndef ESP_PANEL_BOARD_LCD_SPI_IO_SCK
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_SCK
                    #define ESP_PANEL_BOARD_LCD_SPI_IO_SCK CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_SCK
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_IO_SCK"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_SPI_IO_MOSI
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_MOSI
                    #define ESP_PANEL_BOARD_LCD_SPI_IO_MOSI CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_MOSI
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_SPI_IO_MOSI"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_SPI_IO_MISO
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_MISO
                    #define ESP_PANEL_BOARD_LCD_SPI_IO_MISO CONFIG_ESP_PANEL_BOARD_LCD_SPI_IO_MISO
                #else
                    #define ESP_PANEL_BOARD_LCD_SPI_IO_MISO -1
                #endif
            #endif
        #endif
    #endif

    // QSPI Bus Settings
    #if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI) // QSPI
        // Non-bool: error if not defined
        #ifndef ESP_PANEL_BOARD_LCD_QSPI_HOST_ID
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_HOST_ID
                #define ESP_PANEL_BOARD_LCD_QSPI_HOST_ID CONFIG_ESP_PANEL_BOARD_LCD_QSPI_HOST_ID
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_HOST_ID"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_QSPI_MODE
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_MODE
                #define ESP_PANEL_BOARD_LCD_QSPI_MODE CONFIG_ESP_PANEL_BOARD_LCD_QSPI_MODE
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_MODE"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ
                #define ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ CONFIG_ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS
                #define ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS CONFIG_ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS
                #define ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS CONFIG_ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS"
            #endif
        #endif

        // QSPI Pins
        #ifndef ESP_PANEL_BOARD_LCD_QSPI_IO_CS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_CS
                #define ESP_PANEL_BOARD_LCD_QSPI_IO_CS CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_CS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_IO_CS"
            #endif
        #endif

        #if !ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
            #ifndef ESP_PANEL_BOARD_LCD_QSPI_IO_SCK
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_SCK
                    #define ESP_PANEL_BOARD_LCD_QSPI_IO_SCK CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_SCK
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_IO_SCK"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0
                    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0 CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1
                    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1 CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2
                    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2 CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3
                    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3 CONFIG_ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3"
                #endif
            #endif
        #endif
    #endif

    // RGB Bus Settings
    #if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB) // RGB
        // Bool type: default to 0 if not defined
        #ifndef ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
                #define ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL CONFIG_ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
            #else
                #define ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL 0
            #endif
        #endif

        #if ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
            // Non-bool: error if not defined
            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_IO_CS
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_IO_CS
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_IO_CS CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_IO_CS
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_SPI_IO_CS"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SCK
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SCK
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SCK CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SCK
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SCK"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SDA
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SDA
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SDA CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SDA
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SDA"
                #endif
            #endif

            // Bool type: default to 0 if not defined
            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_CS_USE_EXPNADER
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_CS_USE_EXPNADER
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_CS_USE_EXPNADER CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_CS_USE_EXPNADER
                #else
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_CS_USE_EXPNADER 0
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_SCL_USE_EXPNADER
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_SCL_USE_EXPNADER
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_SCL_USE_EXPNADER CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_SCL_USE_EXPNADER
                #else
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_SCL_USE_EXPNADER 0
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_SDA_USE_EXPNADER
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_SDA_USE_EXPNADER
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_SDA_USE_EXPNADER CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_SDA_USE_EXPNADER
                #else
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_SDA_USE_EXPNADER 0
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_MODE
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_MODE
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_MODE CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_MODE
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_SPI_MODE"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_CMD_BYTES
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_CMD_BYTES
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_CMD_BYTES CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_CMD_BYTES
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_SPI_CMD_BYTES"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_PARAM_BYTES
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_PARAM_BYTES
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_PARAM_BYTES CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_PARAM_BYTES
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_SPI_PARAM_BYTES"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_SPI_USE_DC_BIT
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_USE_DC_BIT
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_USE_DC_BIT CONFIG_ESP_PANEL_BOARD_LCD_RGB_SPI_USE_DC_BIT
                #else
                    #define ESP_PANEL_BOARD_LCD_RGB_SPI_USE_DC_BIT 0
                #endif
            #endif
        #endif // ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL

        // RGB Interface Settings
        // Non-bool: error if not defined
        #ifndef ESP_PANEL_BOARD_LCD_RGB_HPW
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_HPW
                #define ESP_PANEL_BOARD_LCD_RGB_HPW CONFIG_ESP_PANEL_BOARD_LCD_RGB_HPW
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_HPW"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_HBP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_HBP
                #define ESP_PANEL_BOARD_LCD_RGB_HBP CONFIG_ESP_PANEL_BOARD_LCD_RGB_HBP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_HBP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_HFP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_HFP
                #define ESP_PANEL_BOARD_LCD_RGB_HFP CONFIG_ESP_PANEL_BOARD_LCD_RGB_HFP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_HFP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_VPW
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_VPW
                #define ESP_PANEL_BOARD_LCD_RGB_VPW CONFIG_ESP_PANEL_BOARD_LCD_RGB_VPW
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_VPW"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_VBP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_VBP
                #define ESP_PANEL_BOARD_LCD_RGB_VBP CONFIG_ESP_PANEL_BOARD_LCD_RGB_VBP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_VBP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_VFP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_VFP
                #define ESP_PANEL_BOARD_LCD_RGB_VFP CONFIG_ESP_PANEL_BOARD_LCD_RGB_VFP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_VFP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_CLK_HZ
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_CLK_HZ
                #define ESP_PANEL_BOARD_LCD_RGB_CLK_HZ CONFIG_ESP_PANEL_BOARD_LCD_RGB_CLK_HZ
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_CLK_HZ"
            #endif
        #endif

        // Bool type: default to 0 if not defined
        #ifndef ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG
                #define ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG CONFIG_ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG
            #else
                #define ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG 0
            #endif
        #endif

        // Non-bool: error if not defined
        #ifndef ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE
                #define ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE CONFIG_ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH
                #define ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH CONFIG_ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS
                #define ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS CONFIG_ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS"
            #endif
        #endif

        // RGB Pins
        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC
                #define ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC
                #define ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DE
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DE
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DE CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DE
            #else
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DE -1
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_PCLK
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_PCLK
                #define ESP_PANEL_BOARD_LCD_RGB_IO_PCLK CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_PCLK
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_PCLK"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DISP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DISP
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DISP CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DISP
            #else
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DISP -1
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA0
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA0
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA0 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA0
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA0"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA1
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA1
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA1 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA1
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA1"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA2
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA2
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA2 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA2
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA2"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA3
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA3
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA3 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA3
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA3"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA4
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA4
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA4 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA4
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA4"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA5
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA5
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA5 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA5
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA5"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA6
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA6
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA6 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA6
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA6"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA7
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA7
                #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA7 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA7
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA7"
            #endif
        #endif

        #if ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH > 8
            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA8
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA8
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA8 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA8
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA8"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA9
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA9
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA9 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA9
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA9"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA10
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA10
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA10 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA10
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA10"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA11
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA11
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA11 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA11
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA11"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA12
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA12
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA12 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA12
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA12"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA13
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA13
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA13 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA13
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA13"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA14
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA14
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA14 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA14
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA14"
                #endif
            #endif

            #ifndef ESP_PANEL_BOARD_LCD_RGB_IO_DATA15
                #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA15
                    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA15 CONFIG_ESP_PANEL_BOARD_LCD_RGB_IO_DATA15
                #else
                    #error "Missing configuration: ESP_PANEL_BOARD_LCD_RGB_IO_DATA15"
                #endif
            #endif
        #endif /* ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH > 8 */

    #elif (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI) // MIPI DSI
        // DSI settings
        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_NUM
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_NUM
                #define ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_NUM CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_NUM
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_NUM"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_RATE_MBPS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_RATE_MBPS
                #define ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_RATE_MBPS CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_RATE_MBPS
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_RATE_MBPS"
            #endif
        #endif

        // DPI settings
        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_HPW
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_HPW
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_HPW CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_HPW
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_HPW"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_HBP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_HBP
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_HBP CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_HBP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_HBP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_HFP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_HFP
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_HFP CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_HFP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_HFP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_VPW
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_VPW
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_VPW CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_VPW
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_VPW"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_VBP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_VBP
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_VBP CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_VBP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_VBP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_VFP
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_VFP
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_VFP CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_VFP
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_VFP"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_CLK_MHZ
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_CLK_MHZ
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_CLK_MHZ CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_CLK_MHZ
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_CLK_MHZ"
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_BITS
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_RGB565
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_BITS 16
            #elif defined(CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_RGB666)
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_BITS 18
            #elif defined(CONFIG_ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_RGB888)
                #define ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_BITS 24
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_BITS"
            #endif
        #endif

        // PHY Power
        #ifndef ESP_PANEL_BOARD_LCD_MIPI_PHY_LDO_ID
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIPI_PHY_LDO_ID
                #define ESP_PANEL_BOARD_LCD_MIPI_PHY_LDO_ID CONFIG_ESP_PANEL_BOARD_LCD_MIPI_PHY_LDO_ID
            #else
                #error "Missing configuration: ESP_PANEL_BOARD_LCD_MIPI_PHY_LDO_ID"
            #endif
        #endif
    #endif /* ESP_PANEL_BOARD_LCD_BUS_TYPE */

    #if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB) && ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
        #ifndef ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX
                #define ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX CONFIG_ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX
            #else
                #define ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX 0
            #endif
        #endif

        #ifndef ESP_PANEL_BOARD_LCD_FLAGS_MIRROR_BY_CMD
            #ifdef CONFIG_ESP_PANEL_BOARD_LCD_FLAGS_MIRROR_BY_CMD
                #define ESP_PANEL_BOARD_LCD_FLAGS_MIRROR_BY_CMD CONFIG_ESP_PANEL_BOARD_LCD_FLAGS_MIRROR_BY_CMD
            #else
                #define ESP_PANEL_BOARD_LCD_FLAGS_MIRROR_BY_CMD 0
            #endif
        #endif
    #endif

    // Color Settings
    // Non-bool: error if not defined
    #ifndef ESP_PANEL_BOARD_LCD_COLOR_BITS
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_COLOR_BITS
            #define ESP_PANEL_BOARD_LCD_COLOR_BITS CONFIG_ESP_PANEL_BOARD_LCD_COLOR_BITS
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_LCD_COLOR_BITS"
        #endif
    #endif

    // Bool type: default to 0 if not defined
    #ifndef ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER
            #define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER CONFIG_ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER
        #else
            #define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT
            #define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT CONFIG_ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT
        #else
            #define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT 0
        #endif
    #endif

    // Transformation Settings (Bool type: default to 0 if not defined)
    #ifndef ESP_PANEL_BOARD_LCD_SWAP_XY
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_SWAP_XY
            #define ESP_PANEL_BOARD_LCD_SWAP_XY CONFIG_ESP_PANEL_BOARD_LCD_SWAP_XY
        #else
            #define ESP_PANEL_BOARD_LCD_SWAP_XY 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_LCD_MIRROR_X
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIRROR_X
            #define ESP_PANEL_BOARD_LCD_MIRROR_X CONFIG_ESP_PANEL_BOARD_LCD_MIRROR_X
        #else
            #define ESP_PANEL_BOARD_LCD_MIRROR_X 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_LCD_MIRROR_Y
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_MIRROR_Y
            #define ESP_PANEL_BOARD_LCD_MIRROR_Y CONFIG_ESP_PANEL_BOARD_LCD_MIRROR_Y
        #else
            #define ESP_PANEL_BOARD_LCD_MIRROR_Y 0
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_LCD_GAP_X
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_GAP_X
            #define ESP_PANEL_BOARD_LCD_GAP_X CONFIG_ESP_PANEL_BOARD_LCD_GAP_X
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_LCD_GAP_X"
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_LCD_GAP_Y
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_GAP_Y
            #define ESP_PANEL_BOARD_LCD_GAP_Y CONFIG_ESP_PANEL_BOARD_LCD_GAP_Y
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_LCD_GAP_Y"
        #endif
    #endif

    // Reset Settings
    // Non-bool: error if not defined
    #ifndef ESP_PANEL_BOARD_LCD_RST_IO
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RST_IO
            #define ESP_PANEL_BOARD_LCD_RST_IO CONFIG_ESP_PANEL_BOARD_LCD_RST_IO
        #else
            #error "Missing configuration: ESP_PANEL_BOARD_LCD_RST_IO"
        #endif
    #endif

    // Bool type: default to 0 if not defined
    #ifndef ESP_PANEL_BOARD_LCD_RST_LEVEL
        #ifdef CONFIG_ESP_PANEL_BOARD_LCD_RST_LEVEL
            #define ESP_PANEL_BOARD_LCD_RST_LEVEL CONFIG_ESP_PANEL_BOARD_LCD_RST_LEVEL
        #else
            #define ESP_PANEL_BOARD_LCD_RST_LEVEL 0
        #endif
    #endif
#endif // ESP_PANEL_BOARD_USE_LCD

// *INDENT-ON*

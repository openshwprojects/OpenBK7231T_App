/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file   BOARD_M5CORE2.h
 * @brief  Configuration file for M5STACK M5CORE2
 * @author Espressif@MacChu0315-Espressif & Espressif@lzw655
 * @link   https://docs.m5stack.com/en/core/core2
 */

#pragma once

// *INDENT-OFF*

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure general panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Board name
 */
#define ESP_PANEL_BOARD_NAME                "M5Stack:M5CORE2"

/**
 * @brief Panel resolution configuration in pixels
 */
#define ESP_PANEL_BOARD_WIDTH               (320)   // Panel width (horizontal, in pixels)
#define ESP_PANEL_BOARD_HEIGHT              (240)   // Panel height (vertical, in pixels)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure the LCD panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief LCD panel configuration flag (0/1)
 *
 * Set to `1` to enable LCD panel support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_LCD             (1)

#if ESP_PANEL_BOARD_USE_LCD
/**
 * @brief LCD controller selection
 */
#define ESP_PANEL_BOARD_LCD_CONTROLLER      ILI9341

/**
 * @brief LCD bus type selection
 */
#define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_SPI)

#if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI) || \
    (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI)
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
 * ensure that the host is initialized only once.
 */
#define ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST      (0)     // 0/1. Typically set to 0
#endif

/**
 * @brief LCD bus parameters configuration
 *
 * Configure parameters based on the selected bus type. Parameters for other bus types will be ignored.
 * For detailed parameter explanations, see:
 * https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/api-reference/peripherals/lcd/index.html
 * https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/index.html
 */
#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI

    /**
     * @brief SPI bus
     */
    /* For general */
    #define ESP_PANEL_BOARD_LCD_SPI_HOST_ID         (1)     // Typically set to 1
#if !ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
    /* For host */
    #define ESP_PANEL_BOARD_LCD_SPI_IO_SCK          (18)
    #define ESP_PANEL_BOARD_LCD_SPI_IO_MOSI         (23)
    #define ESP_PANEL_BOARD_LCD_SPI_IO_MISO         (-1)    // -1 if not used
#endif // ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
    /* For panel */
    #define ESP_PANEL_BOARD_LCD_SPI_IO_CS           (5)     // -1 if not used
    #define ESP_PANEL_BOARD_LCD_SPI_IO_DC           (15)
    #define ESP_PANEL_BOARD_LCD_SPI_MODE            (0)     // 0-3. Typically set to 0
    #define ESP_PANEL_BOARD_LCD_SPI_CLK_HZ          (40 * 1000 * 1000)
                                                            // Should be an integer divisor of 80M, typically set to 40M
    #define ESP_PANEL_BOARD_LCD_SPI_CMD_BITS        (8)     // Typically set to 8
    #define ESP_PANEL_BOARD_LCD_SPI_PARAM_BITS      (8)     // Typically set to 8

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE

/**
 * @brief LCD color configuration
 */
#define ESP_PANEL_BOARD_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB565)
                                                        // ESP_PANEL_LCD_COLOR_BITS_RGB565/RGB666/RGB888
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER     (1)     // 0: RGB, 1: BGR
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT    (1)     // 0/1

/**
 * @brief LCD transformation configuration
 */
#define ESP_PANEL_BOARD_LCD_SWAP_XY             (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_X            (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_Y            (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_GAP_X               (0)     // [0, ESP_PANEL_BOARD_WIDTH]
#define ESP_PANEL_BOARD_LCD_GAP_Y               (0)     // [0, ESP_PANEL_BOARD_HEIGHT]

/**
 * @brief LCD reset pin configuration
 */
#define ESP_PANEL_BOARD_LCD_RST_IO              (-1)    // Reset pin, -1 if not used
#define ESP_PANEL_BOARD_LCD_RST_LEVEL           (0)     // Reset active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_LCD

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure the touch panel ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Touch panel configuration flag (0/1)
 *
 * Set to `1` to enable touch panel support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_TOUCH               (1)

#if ESP_PANEL_BOARD_USE_TOUCH
/**
 * @brief Touch controller selection
 */
#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        FT5x06

/**
 * @brief Touch bus type selection
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_I2C)

#if (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C) || \
    (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI)
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
 * ensure that the host is initialized only once.
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST        (0)     // 0/1. Typically set to 0
#endif

/**
 * @brief Touch bus parameters configuration
 */
#if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C

    /**
     * @brief I2C bus
     */
    /* For general */
    #define ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID           (0)     // Typically set to 0
#if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
    /* For host */
    #define ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ            (400 * 1000)
                                                                // Typically set to 400K
    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP        (0)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP        (0)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL            (22)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA            (21)
#endif
    /* For panel */
    #define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS           (0)     // Typically set to 0 to use the default address.
                                                                // - For touchs with only one address, set to 0
                                                                // - For touchs with multiple addresses, set to 0 or
                                                                //   the address. Like GT911, there are two addresses:
                                                                //   0x5D(default) and 0x14

#endif // ESP_PANEL_BOARD_TOUCH_BUS_TYPE

/**
 * @brief Touch panel transformation flags
 */
#define ESP_PANEL_BOARD_TOUCH_SWAP_XY           (0)     // 0/1
#define ESP_PANEL_BOARD_TOUCH_MIRROR_X          (0)     // 0/1
#define ESP_PANEL_BOARD_TOUCH_MIRROR_Y          (0)     // 0/1

/**
 * @brief Touch panel control pins
 */
#define ESP_PANEL_BOARD_TOUCH_RST_IO            (-1)    // Reset pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_RST_LEVEL         (0)     // Reset active level, 0: low, 1: high
#define ESP_PANEL_BOARD_TOUCH_INT_IO            (-1)    // Interrupt pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_INT_LEVEL         (0)     // Interrupt active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_TOUCH

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the backlight ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Backlight configuration flag (0/1)
 *
 * Set to `1` to enable backlight support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_BACKLIGHT           (0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the IO expander //////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief IO expander configuration flag (0/1)
 *
 * Set to `1` to enable IO expander support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_EXPANDER            (0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please utilize the following macros to execute any additional code if required /////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Post-begin function for board initialization
 *
 * @param[in] p Pointer to the board object
 * @return true on success, false on failure
 */
#define ESP_PANEL_BOARD_POST_BEGIN_FUNCTION(p) \
    {  \
        constexpr uint8_t AXP_ADDR = 0x34; \
        constexpr i2c_port_t i2c_port = I2C_NUM_0; \
        constexpr uint32_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(1000); \
        uint8_t device_id = 0; \
        uint8_t read_value = 0; \
        uint8_t write_value = 0; \
        uint8_t reg_addr = 0; \
        uint8_t write_buf[2] = {0}; \
        reg_addr = 0x03; \
        ESP_UTILS_CHECK_ERROR_RETURN( \
            i2c_master_write_read_device( \
                i2c_port, AXP_ADDR, &reg_addr, 1, &device_id, 1, I2C_TIMEOUT_TICKS \
            ), false, "Failed to read device ID" \
        ); \
        if (device_id == 0x03) { \
            reg_addr = 0x28; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_read_device( \
                    i2c_port, AXP_ADDR, &reg_addr, 1, &read_value, 1, I2C_TIMEOUT_TICKS \
                ), false, "Failed to read value" \
            ); \
            write_value = (read_value & 0x0F) | ((((3300 - 1800) / 100) & 0x0F) << 4); \
            write_buf[0] = 0x28; \
            write_buf[1] = write_value; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_to_device( \
                    i2c_port, AXP_ADDR, write_buf, sizeof(write_buf), I2C_TIMEOUT_TICKS \
                ), false, "Failed to write value" \
            ); \
            reg_addr = 0x12; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_read_device( \
                    i2c_port, AXP_ADDR, &reg_addr, 1, &read_value, 1, I2C_TIMEOUT_TICKS \
                ), false, "Failed to read value" \
            ); \
            write_value = (read_value | (0x01 << 2)); \
            write_buf[0] = 0x12; \
            write_buf[1] = write_value; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_to_device( \
                    i2c_port, AXP_ADDR, write_buf, sizeof(write_buf), I2C_TIMEOUT_TICKS \
                ), false, "Failed to write value" \
            ); \
            reg_addr = 0x12; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_read_device( \
                    i2c_port, AXP_ADDR, &reg_addr, 1, &read_value, 1, I2C_TIMEOUT_TICKS \
                ), false, "Failed to read value" \
            ); \
            write_value = (read_value | (0x01 << 1)); \
            write_buf[0] = 0x12; \
            write_buf[1] = write_value; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_to_device( \
                    i2c_port, AXP_ADDR, write_buf, sizeof(write_buf), I2C_TIMEOUT_TICKS \
                ), false, "Failed to write value" \
            ); \
        } else if (device_id == 0x4A) { \
            reg_addr = 0x90; \
            read_value = device_id; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_read_device( \
                    i2c_port, AXP_ADDR, &reg_addr, 1, &read_value, 1, I2C_TIMEOUT_TICKS \
                ), false, "Failed to read value" \
            ); \
            write_value = (read_value | (1 << 4)); \
            write_buf[0] = 0x90; \
            write_buf[1] = write_value; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_to_device( \
                    i2c_port, AXP_ADDR, write_buf, sizeof(write_buf), I2C_TIMEOUT_TICKS \
                ), false, "Failed to write value" \
            ); \
            write_value = (3000 - 500) / 100; \
            write_buf[0] = 0x94; \
            write_buf[1] = write_value; \
            ESP_UTILS_CHECK_ERROR_RETURN( \
                i2c_master_write_to_device( \
                    i2c_port, AXP_ADDR, write_buf, sizeof(write_buf), I2C_TIMEOUT_TICKS \
                ), false, "Failed to write value" \
            ); \
        } \
        return true;    \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// File Version ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Do not change the following versions. These version numbers are used to check compatibility between this
 * configuration file and the library. Rules for version numbers:
 * 1. Major version mismatch: Configurations are incompatible, must use library version
 * 2. Minor version mismatch: May be missing new configurations, recommended to update
 * 3. Patch version mismatch: No impact on functionality
 */
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR 0
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH 0

// *INDENT-ON*

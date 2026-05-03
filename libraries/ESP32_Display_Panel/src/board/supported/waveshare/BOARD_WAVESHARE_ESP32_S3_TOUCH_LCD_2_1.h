/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file   BOARD_ESP32_S3_TOUCH_LCD_2_1.h
 * @brief  Configuration file for Waveshare ESP32_S3_TOUCH_LCD_2_1
 * @author Waveshare@H-sw123
 * @link   https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm
 */

#pragma once

// *INDENT-OFF*

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure general panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Board name
 */
#define ESP_PANEL_BOARD_NAME                "Waveshare:ESP32_S3_TOUCH_LCD_2_1"

/**
 * @brief Panel resolution configuration in pixels
 */
#define ESP_PANEL_BOARD_WIDTH               (480)   // Panel width (horizontal, in pixels)
#define ESP_PANEL_BOARD_HEIGHT              (480)   // Panel height (vertical, in pixels)

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
#define ESP_PANEL_BOARD_LCD_CONTROLLER      ST7701

/**
 * @brief LCD bus type selection
 */
#define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_RGB)

/**
 * @brief LCD bus parameters configuration
 *
 * Configure parameters based on the selected bus type. Parameters for other bus types will be ignored.
 * For detailed parameter explanations, see:
 * https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/api-reference/peripherals/lcd/index.html
 * https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/index.html
 */
#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB

    /**
     * @brief RGB bus
     */
    /**
     * Set to 0 if using simple "RGB" interface which does not contain "3-wire SPI" interface.
     */
    #define ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL       (1) // 0/1. Typically set to 1

#if ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
    /* For control panel (3wire-SPI) */
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_IO_CS               (2)
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SCK              (2)
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_IO_SDA              (1)
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_CS_USE_EXPNADER     (1) // Set to 1 if the signal is controlled by an IO expander
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_SCL_USE_EXPNADER    (0) // Set to 1 if the signal is controlled by an IO expander
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_SDA_USE_EXPNADER    (0) // Set to 1 if the signal is controlled by an IO expander
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_MODE                (0) // 0-3, typically set to 0
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_CMD_BYTES           (1) // Typically set to 8
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_PARAM_BYTES         (1) // Typically set to 8
    #define ESP_PANEL_BOARD_LCD_RGB_SPI_USE_DC_BIT          (1) // 0/1. Typically set to 1
#endif // ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
    /* For refresh panel (RGB) */
    #define ESP_PANEL_BOARD_LCD_RGB_CLK_HZ          (16 * 1000 * 1000)
                                                            // To increase the upper limit of the PCLK, see: https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html#how-can-i-increase-the-upper-limit-of-pclk-settings-on-esp32-s3-while-ensuring-normal-rgb-screen-display
    #define ESP_PANEL_BOARD_LCD_RGB_HPW             (8)
    #define ESP_PANEL_BOARD_LCD_RGB_HBP             (10)
    #define ESP_PANEL_BOARD_LCD_RGB_HFP             (50)
    #define ESP_PANEL_BOARD_LCD_RGB_VPW             (3)
    #define ESP_PANEL_BOARD_LCD_RGB_VBP             (8)
    #define ESP_PANEL_BOARD_LCD_RGB_VFP             (8)
    #define ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG (0)     // 0: rising edge, 1: falling edge. Typically set to 0
                                                                                        // The following sheet shows the valid combinations of
                                                                                        // data width and pixel bits:
                                                                                        // ┏---------------------------------┳- -------------------------------┓
    #define ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH      (16)                                // |                16               |               8                 |
    #define ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS      (ESP_PANEL_LCD_COLOR_BITS_RGB565)   // | ESP_PANEL_LCD_COLOR_BITS_RGB565 | ESP_PANEL_LCD_COLOR_BITS_RGB888 |
                                                                                        // ┗---------------------------------┻---------------------------------┛
                                                            // To understand color format of RGB LCD, see: https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/rgb_lcd.html#color-formats
    #define ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE (ESP_PANEL_BOARD_WIDTH * 10)
                                                            // Bounce buffer size in bytes. It is used to avoid screen drift
                                                            // for ESP32-S3. Typically set to `ESP_PANEL_BOARD_WIDTH * 10`
                                                            // The size should satisfy `size * N = LCD_width * LCD_height`,
                                                            // where N is an even number.
                                                            // For more details, see: https://github.com/esp-arduino-libs/ESP32_Display_Panel/blob/master/docs/FAQ.md#how-to-fix-screen-drift-issue-when-driving-rgb-lcd-with-esp32-s3
    #define ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC        (38)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC        (39)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DE           (40)    // -1 if not used
    #define ESP_PANEL_BOARD_LCD_RGB_IO_PCLK         (41)
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DISP         (-1)    // -1 if not used. Typically set to -1

                                                            // The following sheet shows the mapping of ESP GPIOs to
                                                            // LCD data pins with different data width and color format:
                                                            // ┏------┳- ------------┳--------------------------┓
                                                            // | ESP: | 8-bit RGB888 |      16-bit RGB565       |
                                                            // |------|--------------|--------------------------|
                                                            // | LCD: |    RGB888    | RGB565 | RGB666 | RGB888 |
                                                            // ┗------|--------------|--------|--------|--------|
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA0        (5)     //        |      D0      |   B0   |  B0-1  |   B0-3 |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA1        (45)    //        |      D1      |   B1   |  B2    |   B4   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA2        (48)    //        |      D2      |   B2   |  B3    |   B5   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA3        (47)    //        |      D3      |   B3   |  B4    |   B6   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA4        (21)    //        |      D4      |   B4   |  B5    |   B7   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA5        (14)    //        |      D5      |   G0   |  G0    |   G0-2 |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA6        (13)    //        |      D6      |   G1   |  G1    |   G3   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA7        (12)    //        |      D7      |   G2   |  G2    |   G4   |
#if ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH > 8                  //        ┗--------------┫--------|--------|--------|
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA8        (11)    //                       |   G3   |  G3    |   G5   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA9        (10)    //                       |   G4   |  G4    |   G6   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA10       (9)     //                       |   G5   |  G5    |   G7   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA11       (46)    //                       |   R0   |  R0-1  |   R0-3 |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA12       (3)     //                       |   R1   |  R2    |   R4   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA13       (8)     //                       |   R2   |  R3    |   R5   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA14       (18)    //                       |   R3   |  R4    |   R6   |
    #define ESP_PANEL_BOARD_LCD_RGB_IO_DATA15       (17)    //                       |   R4   |  R5    |   R7   |
                                                            //                       ┗--------┻--------┻--------┛
#endif // ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE

/**
 * @brief LCD specific flags configuration
 *
 * These flags are specific to the "3-wire SPI + RGB" bus.
 */
#if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB) && ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL
/**
 * @brief Enable IO multiplex
 *
 * Set to 1 if the 3-wire SPI pins are sharing other pins of the RGB interface to save GPIOs. Then, the control panel
 * and its pins (except CS signal) will be released after LCD call `init()`. All `*_by_cmd` flags will be invalid.
 */
#define ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX       (0) // typically set to 0
/**
 * @brief Mirror by command
 *
 * Set to 1 if the `mirror()` function will be implemented by LCD command. Otherwise, the function will be implemented by
 * software. Only valid when `ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX` is 0.
 */
#define ESP_PANEL_BOARD_LCD_FLAGS_MIRROR_BY_CMD             (!ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX)
#endif // ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL

/**
 * @brief LCD vendor initialization commands
 *
 * Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for
 * initialization sequence code. Please uncomment and change the following macro definitions. Otherwise, the LCD driver
 * will use the default initialization sequence code.
 *
 * The initialization sequence can be specified in two formats:
 * 1. Raw format:
 *    {command, (uint8_t []){data0, data1, ...}, data_size, delay_ms}
 * 2. Helper macros:
 *    - ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, {data0, data1, ...})
 *    - ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
 */
#define ESP_PANEL_BOARD_LCD_VENDOR_INIT_CMD() \
    { \
        {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0}, \
        {0xC0, (uint8_t []){0x3B, 0x00}, 2, 0}, \
        {0xC1, (uint8_t []){0x0B, 0x02}, 2, 0}, \
        {0xC2, (uint8_t []){0x07, 0x02}, 2, 0}, \
        {0xCC, (uint8_t []){0x10}, 1, 0}, \
        {0xCD, (uint8_t []){0x08}, 1, 0}, \
        {0xB0, (uint8_t []){0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09, 0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31, \
                            0x18}, 16, 0}, \
        {0xB1, (uint8_t []){0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09, 0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31, \
                            0x18}, 16, 0}, \
        {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0}, \
        {0xB0, (uint8_t []){0x6D}, 1, 0}, \
        {0xB1, (uint8_t []){0x37}, 1, 0}, \
        {0xB2, (uint8_t []){0x81}, 1, 0}, \
        {0xB3, (uint8_t []){0x80}, 1, 0}, \
        {0xB5, (uint8_t []){0x43}, 1, 0}, \
        {0xB7, (uint8_t []){0x85}, 1, 0}, \
        {0xB8, (uint8_t []){0x20}, 1, 0}, \
        {0xC1, (uint8_t []){0x78}, 1, 0}, \
        {0xC2, (uint8_t []){0x78}, 1, 0}, \
        {0xD0, (uint8_t []){0x88}, 1, 0}, \
        {0xE0, (uint8_t []){0x00, 0x00, 0x02}, 3, 0}, \
        {0xE1, (uint8_t []){0x03, 0xA0, 0x00, 0x00, 0x04, 0xA0, 0x00, 0x00, 0x00, 0x20, 0x20}, 11, 0}, \
        {0xE2, (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 13, 0}, \
        {0xE3, (uint8_t []){0x00, 0x00, 0x11, 0x00}, 4, 0}, \
        {0xE4, (uint8_t []){0x22, 0x00}, 2, 0}, \
        {0xE5, (uint8_t []){0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                            0x00}, 16, 0}, \
        {0xE6, (uint8_t []){0x00, 0x00, 0x11, 0x00}, 4, 0}, \
        {0xE7, (uint8_t []){0x22, 0x00}, 2, 0}, \
        {0xE8, (uint8_t []){0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                            0x00}, 16, 0}, \
        {0xEB, (uint8_t []){0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, 7, 0}, \
        {0xED, (uint8_t []){0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF, 0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF, 0xFF, \
                            0xFF}, 16, 0}, \
        {0xEF, (uint8_t []){0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0}, \
        {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0}, \
        {0xEF, (uint8_t []){0x08}, 1, 0}, \
        {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0}, \
        {0x36, (uint8_t []){0x00}, 1, 0}, \
        {0x3A, (uint8_t []){0x66}, 1, 0}, \
        {0x11, (uint8_t []){0x00}, 0, 480}, \
        {0x20, (uint8_t []){0x00}, 0, 120}, \
        {0x29, (uint8_t []){0x00}, 0, 0}, \
    }

/**
 * @brief LCD color configuration
 */
#define ESP_PANEL_BOARD_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB565)
                                                        // ESP_PANEL_LCD_COLOR_BITS_RGB565/RGB666/RGB888
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER     (0)     // 0: RGB, 1: BGR
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT    (0)     // 0/1

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
#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        CST816S  // Actually a CST820 but the data structure is similar

/**
 * @brief Touch bus type selection
 * - `ESP_PANEL_BUS_TYPE_SPI`
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
    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP        (1)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP        (1)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL            (7)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA            (15)
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
#define ESP_PANEL_BOARD_TOUCH_INT_IO            (16)    // Interrupt pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_INT_LEVEL         (1)     // Interrupt active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_TOUCH

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the backlight ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Backlight configuration flag (0/1)
 *
 * Set to `1` to enable backlight support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_BACKLIGHT           (1)

#if ESP_PANEL_BOARD_USE_BACKLIGHT
/**
 * @brief Backlight control type selection
 */
#define ESP_PANEL_BOARD_BACKLIGHT_TYPE          (ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

#if (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

    /**
     * @brief Backlight control pin configuration
     */
    #define ESP_PANEL_BOARD_BACKLIGHT_IO        (6)     // Output GPIO pin number
    #define ESP_PANEL_BOARD_BACKLIGHT_ON_LEVEL  (1)     // Active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_BACKLIGHT_TYPE

/**
 * @brief Backlight idle state configuration (0/1)
 *
 * Set to 1 if want to turn off the backlight after initializing. Otherwise, the backlight will be on.
 */
#define ESP_PANEL_BOARD_BACKLIGHT_IDLE_OFF      (0)

#endif // ESP_PANEL_BOARD_USE_BACKLIGHT

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the IO expander //////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief IO expander configuration flag (0/1)
 *
 * Set to `1` to enable IO expander support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_EXPANDER            (1)

#if ESP_PANEL_BOARD_USE_EXPANDER
/**
 * @brief IO expander chip selection
 */
#define ESP_PANEL_BOARD_EXPANDER_CHIP           TCA95XX_8BIT

/**
 * @brief IO expander I2C bus parameters configuration
 */
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other devices, please set the macro to `1` ensure that the
 * host is initialized only once.
 */
#define ESP_PANEL_BOARD_EXPANDER_SKIP_INIT_HOST     (0)     // 0/1
/* For general */
#define ESP_PANEL_BOARD_EXPANDER_I2C_HOST_ID        (0)     // Typically set to 0
/* For host */
#if !ESP_PANEL_BOARD_EXPANDER_SKIP_INIT_HOST
#define ESP_PANEL_BOARD_EXPANDER_I2C_CLK_HZ         (400 * 1000)
                                                            // Typically set to 400K
#define ESP_PANEL_BOARD_EXPANDER_I2C_SCL_PULLUP     (1)     // 0/1. Typically set to 1
#define ESP_PANEL_BOARD_EXPANDER_I2C_SDA_PULLUP     (1)     // 0/1. Typically set to 1
#define ESP_PANEL_BOARD_EXPANDER_I2C_IO_SCL         (7)
#define ESP_PANEL_BOARD_EXPANDER_I2C_IO_SDA         (15)
#endif // ESP_PANEL_BOARD_EXPANDER_SKIP_INIT_HOST
/* For device */
#define ESP_PANEL_BOARD_EXPANDER_I2C_ADDRESS        (0x20)  // The actual I2C address. Even for the same model of IC,
                                                            // the I2C address may be different, and confirmation based on
                                                            // the actual hardware connection is required
#endif // ESP_PANEL_BOARD_USE_EXPANDER

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please utilize the following macros to execute any additional code if required /////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Pre-begin function for LCD initialization
 *
 * @param[in] p Pointer to the board object
 * @return true on success, false on failure
 */
#define ESP_PANEL_BOARD_LCD_PRE_BEGIN_FUNCTION(p) \
    {  \
        constexpr int LCD_RST = 0; \
        auto board = static_cast<Board *>(p);  \
        auto expander = board->getIO_Expander()->getBase(); \
        /* LCD reset */ \
        expander->pinMode(LCD_RST, OUTPUT);       \
        expander->digitalWrite(LCD_RST, LOW);     \
        vTaskDelay(pdMS_TO_TICKS(10));          \
        expander->digitalWrite(LCD_RST, HIGH);    \
        vTaskDelay(pdMS_TO_TICKS(100));          \
        return true;    \
    }

/**
 * @brief Pre-begin function for touch panel initialization
 *
 * @param[in] p Pointer to the board object
 * @return true on success, false on failure
 */
#define ESP_PANEL_BOARD_TOUCH_PRE_BEGIN_FUNCTION(p) \
    {  \
        constexpr int TP_RST = 1; \
        auto board = static_cast<Board *>(p);  \
        auto expander = board->getIO_Expander()->getBase(); \
        /* Touch reset */ \
        expander->pinMode(TP_RST, OUTPUT);       \
        expander->digitalWrite(TP_RST, LOW);     \
        vTaskDelay(pdMS_TO_TICKS(30));          \
        expander->digitalWrite(TP_RST, HIGH);    \
        vTaskDelay(pdMS_TO_TICKS(50));          \
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

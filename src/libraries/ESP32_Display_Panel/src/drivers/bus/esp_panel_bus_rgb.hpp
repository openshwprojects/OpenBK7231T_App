/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_panel_bus_conf_internal.h"
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB

#include <memory>
#include <optional>
#include <variant>
#include "esp_lcd_panel_rgb.h"
#include "esp_io_expander.hpp"
#include "port/esp_lcd_panel_io_additions.h"
#include "esp_panel_bus.hpp"

/**
 * @brief Define RGB data width based on SOC capabilities
 */
// *INDENT-OFF*
#ifdef SOC_LCDCAM_RGB_DATA_WIDTH
    #define ESP_PANEL_BUS_RGB_DATA_BITS SOC_LCDCAM_RGB_DATA_WIDTH
#elif defined(SOC_LCD_RGB_DATA_WIDTH)
    #define ESP_PANEL_BUS_RGB_DATA_BITS SOC_LCD_RGB_DATA_WIDTH
#else
    #warning "`SOC_LCDCAM_RGB_DATA_WIDTH` or `SOC_LCD_RGB_DATA_WIDTH` not defined, using default value 16"
    #define ESP_PANEL_BUS_RGB_DATA_BITS 16
#endif // SOC_LCDCAM_RGB_DATA_WIDTH
// *INDENT-ON*

namespace esp_panel::drivers {

/**
 * @brief The RGB bus class for ESP Panel
 *
 * This class is derived from `Bus` class and provides RGB bus implementation for ESP Panel
 *
 * For the ESP32-S3, the RGB peripheral only supports 16-bit RGB565 and 8-bit RGB888 color formats.
 * For more details, please refer to the part `Display > LCD Screen > RGB LCD Introduction` in ESP-IoT-Solution
 * Programming Guide (https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/rgb_lcd.html#color-formats)
 */
class BusRGB: public Bus {
public:
    /**
     * @brief Default values for RGB bus configuration
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BUS_TYPE_RGB,
        .name = "RGB",
    };
    static constexpr int RGB_PCLK_HZ_DEFAULT = 16 * 1000 * 1000;
    static constexpr int RGB_DATA_WIDTH_DEFAULT = 16;

    /**
     * @brief Partial control panel configuration structure
     */
    struct ControlPanelPartialConfig {
        int cs_io_type = static_cast<int>(IO_TYPE_GPIO);      ///< IO type for CS signal
        int scl_io_type = static_cast<int>(IO_TYPE_GPIO);     ///< IO type for SCL signal
        int sda_io_type = static_cast<int>(IO_TYPE_GPIO);     ///< IO type for SDA signal
        int cs_gpio_num = -1;                                 ///< GPIO number for CS signal
        int scl_gpio_num = -1;                                ///< GPIO number for SCL signal
        int sda_gpio_num = -1;                                ///< GPIO number for SDA signal
        int spi_mode = 0;                                     ///< Traditional SPI mode (0 ~ 3)
        int lcd_cmd_bytes = 1;                                ///< Bytes of LCD command
        int lcd_param_bytes = 1;                              ///< Bytes of LCD parameter
        bool flags_use_dc_bit = true;                         ///< If this flag is enabled, transmit DC bit at the beginning of every command and data
    };
    using ControlPanelFullConfig = esp_lcd_panel_io_3wire_spi_config_t;

// *INDENT-OFF*
    /**
     * @brief Partial refresh panel configuration structure
     */
    struct RefreshPanelPartialConfig {
        int pclk_hz = RGB_PCLK_HZ_DEFAULT;                   ///< Pixel clock frequency in Hz
        int h_res = 0;                                       ///< Horizontal resolution
        int v_res = 0;                                       ///< Vertical resolution
        int hsync_pulse_width = 10;                          ///< HSYNC pulse width
        int hsync_back_porch = 10;                           ///< HSYNC back porch
        int hsync_front_porch = 20;                          ///< HSYNC front porch
        int vsync_pulse_width = 10;                          ///< VSYNC pulse width
        int vsync_back_porch = 10;                           ///< VSYNC back porch
        int vsync_front_porch = 10;                          ///< VSYNC front porch
        int data_width = RGB_DATA_WIDTH_DEFAULT;             ///< Data width in bits
        int bits_per_pixel = RGB_DATA_WIDTH_DEFAULT;         ///< Bits per pixel
        int bounce_buffer_size_px = 0;                       ///< Bounce buffer size in pixels
        int hsync_gpio_num = -1;                             ///< GPIO number for HSYNC signal
        int vsync_gpio_num = -1;                             ///< GPIO number for VSYNC signal
        int de_gpio_num = -1;                                ///< GPIO number for DE signal
        int pclk_gpio_num = -1;                              ///< GPIO number for PCLK signal
        int disp_gpio_num = -1;                              ///< GPIO number for DISP signal
        int data_gpio_nums[ESP_PANEL_BUS_RGB_DATA_BITS] = {  ///< GPIO numbers for data signals
            -1, -1, -1, -1, -1, -1, -1, -1,
#if ESP_PANEL_BUS_RGB_DATA_BITS >= 16
            -1, -1, -1, -1, -1, -1, -1, -1
#endif // ESP_PANEL_BUS_RGB_DATA_BITS
        };
        bool flags_pclk_active_neg = false;                  ///< PCLK active on negative edge if true
    };
    using RefreshPanelFullConfig = esp_lcd_rgb_panel_config_t;
// *INDENT-ON*

    using ControlPanelConfig = std::variant<ControlPanelPartialConfig, ControlPanelFullConfig>;
    using RefreshPanelConfig = std::variant<RefreshPanelPartialConfig, RefreshPanelFullConfig>;

    /**
     * @brief The RGB bus configuration structure
     */
    struct Config {
        /**
         * @brief Convert partial configurations to full configurations
         */
        void convertPartialToFull();

        /**
         * @brief Print control panel configuration for debugging
         */
        void printControlPanelConfig() const;

        /**
         * @brief Print refresh panel configuration for debugging
         */
        void printRefreshPanelConfig() const;

        /**
         * @brief Check if control panel configuration is set
         */
        bool isControlPanelValid() const
        {
            return control_panel.has_value();
        }

        std::optional<ControlPanelConfig> control_panel;    ///< Control panel config. If not set, the bus will be used as a single "RGB" bus
        RefreshPanelConfig refresh_panel = RefreshPanelPartialConfig{}; ///< Refresh panel config
    };

// *INDENT-OFF*
    /**
     * @brief Construct the "3-wire SPI + 16-bit RGB" bus with separate parameters
     *
     * @param[in] cs_io    3-wire SPI CS pin
     * @param[in] sck_io   3-wire SPI SCK pin
     * @param[in] sda_io   3-wire SPI SDA pin
     * @param[in] d[N]_io  RGB data pins, N is [0, 15]
     * @param[in] hsync_io RGB HSYNC pin
     * @param[in] vsync_io RGB VSYNC pin
     * @param[in] pclk_io  RGB PCLK pin
     * @param[in] de_io    RGB DE pin, set to -1 if not used
     * @param[in] disp_io  RGB DISP pin, default is -1
     * @param[in] clk_hz   The clock frequency of the RGB bus
     * @param[in] h_res    The width of the panel (horizontal resolution)
     * @param[in] v_res    The height of the panel (vertical resolution)
     * @param[in] hpw      The HSYNC pulse width
     * @param[in] hbp      The HSYNC back porch
     * @param[in] hfp      The HSYNC front porch
     * @param[in] vpw      The VSYNC pulse width
     * @param[in] vbp      The VSYNC back porch
     * @param[in] vfp      The VSYNC front porch
     *
     * @note This function uses some default values to config the bus, use `config*()` functions to change them
     */
    BusRGB(
        /* 3-wire SPI IOs */
        int cs_io, int sck_io, int sda_io,
        /* 16-bit RGB IOs */
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int d8_io, int d9_io, int d10_io, int d11_io, int d12_io, int d13_io, int d14_io, int d15_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io,
        /* RGB timings */
        int clk_hz, int h_res, int v_res, int hpw, int hbp, int hfp, int vpw, int vbp, int vfp
    ):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Control Panel
            .control_panel = ControlPanelPartialConfig{
                .cs_gpio_num = cs_io,
                .scl_gpio_num = sck_io,
                .sda_gpio_num = sda_io,
            },
            // Refresh Panel
            .refresh_panel = RefreshPanelPartialConfig{
                .pclk_hz = clk_hz,
                .h_res = h_res,
                .v_res = v_res,
                .hsync_pulse_width = hpw,
                .hsync_back_porch = hbp,
                .hsync_front_porch = hfp,
                .vsync_pulse_width = vpw,
                .vsync_back_porch = vbp,
                .vsync_front_porch = vfp,
                .data_width = 16,
                .bits_per_pixel = 16,
                .hsync_gpio_num = hsync_io,
                .vsync_gpio_num = vsync_io,
                .de_gpio_num = de_io,
                .pclk_gpio_num = pclk_io,
                .disp_gpio_num = disp_io,
                .data_gpio_nums = {
                    d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io,
                    d8_io, d9_io, d10_io, d11_io, d12_io, d13_io, d14_io, d15_io,
                },
            },
        }
    {
    }

    /**
     * @brief Construct the single "16-bit RGB" bus with separate parameters
     *
     * @param[in] d[N]_io  RGB data pins, N is [0, 15]
     * @param[in] hsync_io RGB HSYNC pin
     * @param[in] vsync_io RGB VSYNC pin
     * @param[in] pclk_io  RGB PCLK pin
     * @param[in] de_io    RGB DE pin, set to -1 if not used
     * @param[in] disp_io  RGB DISP pin, default is -1
     * @param[in] clk_hz   The clock frequency of the RGB bus
     * @param[in] h_res    The width of the panel (horizontal resolution)
     * @param[in] v_res    The height of the panel (vertical resolution)
     * @param[in] hpw      The HSYNC pulse width
     * @param[in] hbp      The HSYNC back porch
     * @param[in] hfp      The HSYNC front porch
     * @param[in] vpw      The VSYNC pulse width
     * @param[in] vbp      The VSYNC back porch
     * @param[in] vfp      The VSYNC front porch
     *
     * @note This function uses some default values to config the bus, use `config*()` functions to change them
     */
    BusRGB(
        /* 16-bit RGB IOs */
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int d8_io, int d9_io, int d10_io, int d11_io, int d12_io, int d13_io, int d14_io, int d15_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io,
        /* RGB timings */
        int clk_hz, int h_res, int v_res, int hpw, int hbp, int hfp, int vpw, int vbp, int vfp
    ):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Refresh Panel
            .refresh_panel = RefreshPanelPartialConfig{
                .pclk_hz = clk_hz,
                .h_res = h_res,
                .v_res = v_res,
                .hsync_pulse_width = hpw,
                .hsync_back_porch = hbp,
                .hsync_front_porch = hfp,
                .vsync_pulse_width = vpw,
                .vsync_back_porch = vbp,
                .vsync_front_porch = vfp,
                .data_width = 16,
                .bits_per_pixel = 16,
                .hsync_gpio_num = hsync_io,
                .vsync_gpio_num = vsync_io,
                .de_gpio_num = de_io,
                .pclk_gpio_num = pclk_io,
                .disp_gpio_num = disp_io,
                .data_gpio_nums = {
                    d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io,
                    d8_io, d9_io, d10_io, d11_io, d12_io, d13_io, d14_io, d15_io,
                },
            },
        }
    {
    }

    /**
     * @brief Construct the "3-wire SPI + 8-bit RGB" bus with separate parameters
     *
     * @param[in] cs_io    3-wire SPI CS pin
     * @param[in] sck_io   3-wire SPI SCK pin
     * @param[in] sda_io   3-wire SPI SDA pin
     * @param[in] d[N]_io  RGB data pins, N is [0, 7]
     * @param[in] hsync_io RGB HSYNC pin
     * @param[in] vsync_io RGB VSYNC pin
     * @param[in] pclk_io  RGB PCLK pin
     * @param[in] de_io    RGB DE pin, set to -1 if not used
     * @param[in] disp_io  RGB DISP pin, default is -1
     * @param[in] clk_hz   The clock frequency of the RGB bus
     * @param[in] h_res    The width of the panel (horizontal resolution)
     * @param[in] v_res    The height of the panel (vertical resolution)
     * @param[in] hpw      The HSYNC pulse width
     * @param[in] hbp      The HSYNC back porch
     * @param[in] hfp      The HSYNC front porch
     * @param[in] vpw      The VSYNC pulse width
     * @param[in] vbp      The VSYNC back porch
     * @param[in] vfp      The VSYNC front porch
     *
     * @note This function uses some default values to config the bus, use `config*()` functions to change them
     */
    BusRGB(
        /* 3-wire SPI IOs */
        int cs_io, int sck_io, int sda_io,
        /* 8-bit RGB IOs */
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io,
        /* RGB timings */
        int clk_hz, int h_res, int v_res, int hpw, int hbp, int hfp, int vpw, int vbp, int vfp
    ):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Control Panel
            .control_panel = ControlPanelPartialConfig{
                .cs_gpio_num = cs_io,
                .scl_gpio_num = sck_io,
                .sda_gpio_num = sda_io,
            },
            // Refresh Panel
            .refresh_panel = RefreshPanelPartialConfig{
                .pclk_hz = clk_hz,
                .h_res = h_res,
                .v_res = v_res,
                .hsync_pulse_width = hpw,
                .hsync_back_porch = hbp,
                .hsync_front_porch = hfp,
                .vsync_pulse_width = vpw,
                .vsync_back_porch = vbp,
                .vsync_front_porch = vfp,
                .data_width = 8,
                .bits_per_pixel = 24,
                .hsync_gpio_num = hsync_io,
                .vsync_gpio_num = vsync_io,
                .de_gpio_num = de_io,
                .pclk_gpio_num = pclk_io,
                .disp_gpio_num = disp_io,
                .data_gpio_nums = {
                    d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io
                },
            },
        }
    {
    }

    /**
     * @brief Construct the single "8-bit RGB" bus with separate parameters
     *
     * @param[in] d[N]_io  RGB data pins, N is [0, 7]
     * @param[in] hsync_io RGB HSYNC pin
     * @param[in] vsync_io RGB VSYNC pin
     * @param[in] pclk_io  RGB PCLK pin
     * @param[in] de_io    RGB DE pin, set to -1 if not used
     * @param[in] disp_io  RGB DISP pin, default is -1
     * @param[in] clk_hz   The clock frequency of the RGB bus
     * @param[in] h_res    The width of the panel (horizontal resolution)
     * @param[in] v_res    The height of the panel (vertical resolution)
     * @param[in] hpw      The HSYNC pulse width
     * @param[in] hbp      The HSYNC back porch
     * @param[in] hfp      The HSYNC front porch
     * @param[in] vpw      The VSYNC pulse width
     * @param[in] vbp      The VSYNC back porch
     * @param[in] vfp      The VSYNC front porch
     *
     * @note This function uses some default values to config the bus, use `config*()` functions to change them
     */
    BusRGB(
        /* 8-bit RGB IOs */
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io,
        /* RGB timings */
        int clk_hz, int h_res, int v_res, int hpw, int hbp, int hfp, int vpw, int vbp, int vfp
    ):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Refresh Panel
            .refresh_panel = RefreshPanelPartialConfig{
                .pclk_hz = clk_hz,
                .h_res = h_res,
                .v_res = v_res,
                .hsync_pulse_width = hpw,
                .hsync_back_porch = hbp,
                .hsync_front_porch = hfp,
                .vsync_pulse_width = vpw,
                .vsync_back_porch = vbp,
                .vsync_front_porch = vfp,
                .data_width = 8,
                .bits_per_pixel = 24,
                .hsync_gpio_num = hsync_io,
                .vsync_gpio_num = vsync_io,
                .de_gpio_num = de_io,
                .pclk_gpio_num = pclk_io,
                .disp_gpio_num = disp_io,
                .data_gpio_nums = {
                    d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io
                },
            },
        }
    {
    }

    /**
     * @brief Construct the RGB bus with configuration
     *
     * @param[in] config RGB bus configuration
     */
    BusRGB(const Config &config):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config(config)
    {
    }
// *INDENT-ON*

    /**
     * @brief Destroy the RGB bus
     */
    ~BusRGB() override;

    /**
     * @brief Configure SPI IO types
     *
     * @param[in] cs_use_expander  Whether CS pin uses IO expander
     * @param[in] sck_use_expander Whether SCK pin uses IO expander
     * @param[in] sda_use_expander Whether SDA pin uses IO expander
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configSPI_IO_Type(bool cs_use_expander, bool sck_use_expander, bool sda_use_expander);

    /**
     * @brief Configure SPI IO expander
     *
     * @param[in] handle Handle to IO expander instance
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configSPI_IO_Expander(esp_io_expander_t *handle);

    /**
     * @brief Configure SPI mode
     *
     * @param[in] mode SPI mode (0-3)
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configSPI_Mode(uint8_t mode);

    /**
     * @brief Configure SPI command and data bytes
     *
     * @param[in] command_bytes Command bytes
     * @param[in] data_bytes Data bytes
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configSPI_CommandDataBytes(uint8_t command_bytes, uint8_t data_bytes);

    /**
     * @brief Configure RGB clock frequency
     *
     * @param[in] hz Clock frequency in Hz
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configRGB_FreqHz(uint32_t hz);

    /**
     * @brief Configure number of frame buffers
     *
     * @param[in] num Number of frame buffers
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configRGB_FrameBufferNumber(uint8_t num);

    /**
     * @brief Configure bounce buffer size
     *
     * @param[in] size_in_pixel Size of bounce buffer in pixels
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configRGB_BounceBufferSize(uint32_t size_in_pixel);

    /**
     * @brief Configure RGB timing flags
     *
     * @param[in] hsync_idle_low  HSYNC is low in idle state if true
     * @param[in] vsync_idle_low  VSYNC is low in idle state if true
     * @param[in] de_idle_high    DE is high in idle state if true
     * @param[in] pclk_active_neg PCLK is active on negative edge if true
     * @param[in] pclk_idle_high  PCLK is high in idle state if true
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configRGB_TimingFlags(
        bool hsync_idle_low, bool vsync_idle_low, bool de_idle_high, bool pclk_active_neg, bool pclk_idle_high
    );

    /**
     * @brief Initialize the bus
     *
     * @return `true` if initialization succeeds, `false` otherwise
     */
    bool init() override;

    /**
     * @brief Start the bus operation
     *
     * @return `true` if startup succeeds, `false` otherwise
     */
    bool begin() override;

    /**
     * @brief Delete the bus instance and release resources
     *
     * @return `true` if deletion succeeds, `false` otherwise
     */
    bool del() override;

    /**
     * @brief Get the current bus configuration
     *
     * @return Reference to the current bus configuration
     */
    const Config &getConfig() const
    {
        return _config;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructors instead
     */
    [[deprecated("Use other constructors instead")]]
    BusRGB(
        uint16_t width, uint16_t height,
        int cs_io, int sck_io, int sda_io,
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int d8_io, int d9_io, int d10_io, int d11_io, int d12_io, int d13_io, int d14_io, int d15_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io = -1
    ):
        BusRGB(
            cs_io, sck_io, sda_io,
            d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io,
            d8_io, d9_io, d10_io, d11_io, d12_io, d13_io, d14_io, d15_io,
            hsync_io, vsync_io, pclk_io, de_io, disp_io,
            RGB_PCLK_HZ_DEFAULT, width, height, 10, 10, 20, 10, 10, 10
        )
    {
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructors instead
     */
    [[deprecated("Use other constructors instead")]]
    BusRGB(
        uint16_t width, uint16_t height,
        int cs_io, int sck_io, int sda_io,
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io = -1
    ):
        BusRGB(
            cs_io, sck_io, sda_io,
            d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io,
            hsync_io, vsync_io, pclk_io, de_io, disp_io,
            RGB_PCLK_HZ_DEFAULT, width, height, 10, 10, 20, 10, 10, 10
        )
    {
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructors instead
     */
    [[deprecated("Use other constructors instead")]]
    BusRGB(
        uint16_t width, uint16_t height,
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int d8_io, int d9_io, int d10_io, int d11_io, int d12_io, int d13_io, int d14_io, int d15_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io = -1
    ):
        BusRGB(
            d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io,
            d8_io, d9_io, d10_io, d11_io, d12_io, d13_io, d14_io, d15_io,
            hsync_io, vsync_io, pclk_io, de_io, disp_io,
            RGB_PCLK_HZ_DEFAULT, width, height, 10, 10, 20, 10, 10, 10
        )
    {
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructors instead
     */
    [[deprecated("Use other constructors instead")]]
    BusRGB(
        uint16_t width, uint16_t height,
        int d0_io, int d1_io, int d2_io, int d3_io, int d4_io, int d5_io, int d6_io, int d7_io,
        int hsync_io, int vsync_io, int pclk_io, int de_io, int disp_io = -1
    ):
        BusRGB(
            d0_io, d1_io, d2_io, d3_io, d4_io, d5_io, d6_io, d7_io,
            hsync_io, vsync_io, pclk_io, de_io, disp_io,
            RGB_PCLK_HZ_DEFAULT, width, height, 10, 10, 20, 10, 10, 10
        )
    {
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configSPI_IO_Expander()` instead
     */
    [[deprecated("Use `configSPI_IO_Expander()` instead")]]
    bool configSpiLine(
        bool cs_use_expaneder, bool sck_use_expander, bool sda_use_expander, esp_expander::Base *io_expander
    );

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configRGB_FrameBufferNumber()` instead
     */
    [[deprecated("Use `configRGB_FrameBufferNumber()` instead")]]
    bool configRgbFrameBufferNumber(uint8_t num)
    {
        return configRGB_FrameBufferNumber(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configRGB_BounceBufferSize()` instead
     */
    [[deprecated("Use `configRGB_BounceBufferSize()` instead")]]
    bool configRgbBounceBufferSize(uint32_t size_in_pixel)
    {
        return configRGB_BounceBufferSize(size_in_pixel);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configRGB_TimingFlags()` instead
     */
    [[deprecated("Use `configRGB_TimingFlags()` instead")]]
    bool configRgbTimingFlags(
        bool hsync_idle_low, bool vsync_idle_low, bool de_idle_high, bool pclk_active_neg, bool pclk_idle_high
    )
    {
        return configRGB_TimingFlags(hsync_idle_low, vsync_idle_low, de_idle_high, pclk_active_neg, pclk_idle_high);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configRgbFlagDispActiveLow()` instead
     */
    [[deprecated("Use `configRgbFlagDispActiveLow()` instead")]]
    bool configRgbFlagDispActiveLow();

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configRgbTimingFreqHz()` instead
     */
    [[deprecated("Use `configRgbTimingFreqHz()` instead")]]
    bool configRgbTimingFreqHz(uint32_t hz);

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configRgbTimingPorch()` instead
     */
    [[deprecated("Use `configRgbTimingPorch()` instead")]]
    bool configRgbTimingPorch(uint16_t hpw, uint16_t hbp, uint16_t hfp, uint16_t vpw, uint16_t vbp, uint16_t vfp);

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getRefreshPanelFullConfig()` instead
     */
    [[deprecated("Use `getRefreshPanelFullConfig()` instead")]]
    const esp_lcd_rgb_panel_config_t *getRgbConfig()
    {
        return &getRefreshPanelFullConfig();
    }

private:
    /**
     * @brief Check if control panel is used
     *
     * @return `true` if control panel is used, `false` otherwise
     */
    bool isControlPanelUsed() const
    {
        return _config.isControlPanelValid();
    }

    /**
     * @brief Get mutable reference to control panel full configuration
     *
     * @return Reference to control panel full configuration
     */
    ControlPanelFullConfig &getControlPanelFullConfig();

    /**
     * @brief Get mutable reference to refresh panel full configuration
     *
     * @return Reference to refresh panel full configuration
     */
    RefreshPanelFullConfig &getRefreshPanelFullConfig();

    Config _config = {};  ///< RGB bus configuration
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::BusRGB` instead
 */
using ESP_PanelBusRGB [[deprecated("Use `esp_panel::drivers::BusRGB` instead")]] = esp_panel::drivers::BusRGB;

#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_RGB

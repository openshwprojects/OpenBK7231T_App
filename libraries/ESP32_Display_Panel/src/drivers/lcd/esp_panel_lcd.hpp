/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>
#include <map>
#include <memory>
#include "soc/soc_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "utils/esp_panel_utils_cxx.hpp"
#include "drivers/bus/esp_panel_bus_factory.hpp"
#include "port/esp_panel_lcd_vendor_types.h"
#include "esp_panel_lcd_conf_internal.h"

namespace esp_panel::drivers {

/**
 * @brief Base class for all LCD devices
 *
 * This class provides the common interface and functionality for LCD devices.
 * Implements core LCD functionality including initialization, coordinate transformation,
 * bitmap drawing, and interrupt handling.
 *
 * @note This is an abstract base class and cannot be instantiated directly
 */
class LCD {
public:
    /**
     * @brief Default maximum number of frame buffers supported
     */
    static constexpr int FRAME_BUFFER_MAX_NUM = 3;

    /**
     * @brief Panel handle type definition for refresh operations
     */
    using RefreshPanelHandle = esp_lcd_panel_handle_t;

    /**
     * @brief Function pointer type for bitmap drawing completion callback
     *
     * @param[in] user_data User provided data pointer that will be passed to the callback
     * @return `true` if a context switch is required, `false` otherwise
     */
    using FunctionDrawBitmapFinishCallback = bool (*)(void *user_data);

    /**
     * @brief Function pointer type for refresh completion callback
     *
     * @param[in] user_data User provided data pointer that will be passed to the callback
     * @return `true` if a context switch is required, `false` otherwise
     */
    using FunctionRefreshFinishCallback = bool (*)(void *user_data);

    /**
     * @brief Basic bus specification structure for LCD devices
     */
    struct BasicBusSpecification {
        /**
         * @brief Supported color bits enumeration
         */
        enum ColorBits : uint8_t {
            COLOR_BITS_RGB565_16 = 0,  /*!< RGB565 16-bit per pixel */
            COLOR_BITS_RGB666_18,      /*!< RGB666 18-bit per pixel */
            COLOR_BITS_RGB888_24,      /*!< RGB888 24-bit per pixel */
            COLOR_BITS_MAX,            /*!< Maximum color bits index */
        };

        /**
         * @brief Supported LCD functions enumeration
         */
        enum Function : uint8_t {
            FUNC_INVERT_COLOR = 0,  /*!< Invert display colors */
            FUNC_MIRROR_X,          /*!< Mirror display along X axis */
            FUNC_MIRROR_Y,          /*!< Mirror display along Y axis */
            FUNC_SWAP_XY,           /*!< Swap X and Y coordinates */
            FUNC_GAP,               /*!< Set display gap */
            FUNC_DISPLAY_ON_OFF,    /*!< Control display on/off */
            FUNC_MAX,               /*!< Maximum function index */
        };

        /**
         * @brief Print bus specification information for debugging
         *
         * @param[in] bus_name Optional bus name to include in output
         */
        void print(utils::string bus_name = "") const;

        /**
         * @brief Get string representation of supported color bits
         *
         * @return String describing supported color bit depths
         */
        utils::string getColorBitsString() const;

        /**
         * @brief Get supported color bits
         *
         * @return Supported color bits
         */
        utils::vector<int> getColorBitsVector() const;

        /**
         * @brief Check if a specific function is supported
         *
         * @param[in] func Function to check support for
         * @return `true` if function is supported, `false` otherwise
         */
        bool isFunctionValid(Function func) const
        {
            return functions.test(func);
        }

        int x_coord_align = 1;  /*!< Required X coordinate alignment in pixels (default: 1, must be power of 2) */
        int y_coord_align = 1;  /*!< Required Y coordinate alignment in pixels (default: 1, must be power of 2) */
        std::bitset<COLOR_BITS_MAX> color_bits; /*!< List of supported color bit depths */
        std::bitset<FUNC_MAX> functions;        /*!< Bitmap of supported functions */
    };

    /**
     * @brief Map type for storing bus specifications by bus type
     */
    using BasicBusSpecificationMap = utils::map<int, BasicBusSpecification>;

    /**
     * @brief Basic attributes structure for LCD device
     */
    struct BasicAttributes {
        /**
         * @brief Print information for debugging
         */
        void print() const;

        const char *name = "";                  /*!< LCD controller name, defaults to `""` */
        BasicBusSpecification basic_bus_spec;   /*!< Bus interface specifications */
    };

    /**
     * @brief Simplified device configuration structure
     */
    struct DevicePartialConfig {
        int reset_gpio_num = -1;                    /*!< Reset GPIO pin number (-1 if unused) */
        int rgb_ele_order = static_cast<int>(LCD_RGB_ELEMENT_ORDER_RGB); /*!< RGB color element order */
        int bits_per_pixel = 16;                    /*!< Color depth in bits per pixel */
        bool flags_reset_active_high = 0;           /*!< Reset signal active high flag */
    };
    using DeviceFullConfig = esp_lcd_panel_dev_config_t;
    using DeviceConfig = std::variant<DevicePartialConfig, DeviceFullConfig>;

    /**
     * @brief Simplified vendor configuration structure
     */
    struct VendorPartialConfig {
        int hor_res = 0;    /*!< Horizontal resolution of the panel (in pixels) */
        int ver_res = 0;    /*!< Vertical resolution of the panel (in pixels) */
        const esp_panel_lcd_vendor_init_cmd_t *init_cmds = nullptr; /*!< Vendor initialization commands */
        int init_cmds_size = 0;                     /*!< Size of initialization commands array (in bytes) */
        bool flags_mirror_by_cmd = 1;               /*!< Enable mirroring via commands */
        bool flags_enable_io_multiplex = 0;         /*!< Enable IO pin multiplexing */
    };
    using VendorFullConfig = esp_panel_lcd_vendor_config_t;
    using VendorConfig = std::variant<VendorPartialConfig, VendorFullConfig>;

    /**
     * @brief Configuration structure for LCD device
     */
    struct Config {
        /**
         * @brief Convert partial configurations to full configurations
         */
        void convertPartialToFull();

        /**
         * @brief Print device configuration for debugging
         */
        void printDeviceConfig() const;

        /**
         * @brief Print vendor configuration for debugging
         */
        void printVendorConfig() const;

        /**
         * @brief Get pointer to full device configuration
         *
         * @return Pointer to full device configuration, nullptr if not available
         */
        const DeviceFullConfig *getDeviceFullConfig() const
        {
            return std::holds_alternative<DeviceFullConfig>(device) ? &std::get<DeviceFullConfig>(device) : nullptr;
        }

        /**
         * @brief Get pointer to full vendor configuration
         *
         * @return Pointer to full vendor configuration, nullptr if not available
         */
        const VendorFullConfig *getVendorFullConfig() const
        {
            return std::holds_alternative<VendorFullConfig>(vendor) ? &std::get<VendorFullConfig>(vendor) : nullptr;
        }

        DeviceConfig device = DevicePartialConfig{};     /*!< Device configuration storage */
        VendorConfig vendor = VendorPartialConfig{};     /*!< Vendor configuration storage */
    };

    /**
     * @brief LCD coordinate transformation settings
     */
    struct Transformation {
        bool swap_xy = false;    /*!< Swap X/Y coordinates when true */
        bool mirror_x = false;   /*!< Mirror X coordinate when true */
        bool mirror_y = false;   /*!< Mirror Y coordinate when true */
        int gap_x = 0;          /*!< X axis gap offset in pixels */
        int gap_y = 0;          /*!< Y axis gap offset in pixels */
    };

    /**
     * @brief Driver state enumeration
     */
    enum class State : uint8_t {
        DEINIT = 0,     /*!< Driver is not initialized */
        INIT,           /*!< Driver is initialized */
        RESET,          /*!< Driver is reset */
        BEGIN,          /*!< Driver is started and ready */
    };

#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    /**
     * @brief DSI color bar test pattern types
     */
    enum class DSI_ColorBarPattern : uint8_t {
        NONE = MIPI_DSI_PATTERN_NONE,                       /*!< No test pattern */
        BAR_HORIZONTAL = MIPI_DSI_PATTERN_BAR_HORIZONTAL,   /*!< Horizontal color bars */
        BAR_VERTICAL = MIPI_DSI_PATTERN_BAR_VERTICAL,       /*!< Vertical color bars */
        BER_VERTICAL = MIPI_DSI_PATTERN_BER_VERTICAL,       /*!< Vertical BER pattern */
    };
#endif

// *INDENT-OFF*
    /**
     * @brief Construct the LCD device with individual parameters
     *
     * @param[in] attr Basic attributes for the LCD device
     * @param[in] bus Bus interface for communicating with the LCD device
     * @param[in] width Width of the panel (horizontal, in pixels)
     * @param[in] height Height of the panel (vertical, in pixels)
     * @param[in] color_bits Color depth in bits per pixel (16 for RGB565, 18 for RGB666, 24 for RGB888)
     * @param[in] rst_io Reset GPIO pin number (-1 if not used)
     * @note This constructor uses default values for most configuration parameters. Use config*() functions to
     *       customize
     * @note Vendor initialization commands may vary between manufacturers. Consult LCD supplier for them and use
     *       `configVendorCommands()` to configure
     */
    LCD(const BasicAttributes &attr, Bus *bus, int width, int height, int color_bits, int rst_io):
        _basic_attributes(attr),
        _bus(std::shared_ptr<Bus>(bus, [](Bus *bus) {})),
        _config{
            .device = DevicePartialConfig{
                .reset_gpio_num = rst_io,
                .bits_per_pixel = color_bits,
            },
            .vendor = VendorPartialConfig{
                .hor_res = width,
                .ver_res = height,
            },
        }
    {
    }

    /**
     * @brief Construct the LCD device with full configuration
     *
     * @param[in] attr Basic attributes for the LCD device
     * @param[in] bus Bus interface for communicating with the LCD device
     * @param[in] config Complete LCD configuration structure
     * @note Vendor initialization commands may vary between manufacturers. Consult LCD supplier for them
     */
    LCD(const BasicAttributes &attr, Bus *bus, const Config &config):
        _basic_attributes(attr),
        _bus(std::shared_ptr<Bus>(bus, [](Bus *bus) {})),
        _config(config)
    {
    }

    /**
     * @brief Construct the LCD device with bus configuration and device configuration
     *
     * @param[in] attr Basic attributes for the LCD device
     * @param[in] bus_config Bus configuration
     * @param[in] lcd_config LCD configuration
     * @note This constructor creates a new bus instance using the provided bus configuration
     * @note Vendor initialization commands may vary between manufacturers. Consult LCD supplier for them
     */
    LCD(const BasicAttributes &attr, const BusFactory::Config &bus_config, const Config &lcd_config):
        _basic_attributes(attr),
        _bus(BusFactory::create(bus_config)),
        _config(lcd_config)
    {
    }
// *INDENT-OFF*

    /**
     * @brief Destroy the LCD device and free resources
     */
    virtual ~LCD() = default;

    /**
     * @brief Configure the vendor initialization commands
     *
     * @param[in] init_cmd The initialization commands
     * @param[in] init_cmd_size The size of the initialization commands in bytes
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     * @note This function is invalid for the single "RGB" bus which doesn't have a control panel to transmit commands
     * @note Vendor specific initialization commands can be different between manufacturers, should consult the LCD
     *       supplier for them
     * @note There are two formats for the sequence code:
     *       1. Raw data: {command, (uint8_t []){ data0, data1, ... }, data_size, delay_ms}
     *       2. Formatter: ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, { data0, data1, ... }) and
     *                     ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
     */
    bool configVendorCommands(const esp_panel_lcd_vendor_init_cmd_t init_cmd[], uint32_t init_cmd_size);

    /**
     * @brief Configure driver to mirror by command
     *
     * @param[in] en true: enable, false: disable
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     * @note After using this function, the `mirror()` function will be implemented by LCD command. Otherwise, the
     *       `mirror()`function will be implemented by software
     * @note This function is conflict with `configAutoReleaseBus()`, please don't use them at the same time
     * @note This function is only valid for the "3-wire SPI + RGB" bus
     */
    bool configMirrorByCommand(bool en);

    /**
     * @brief Configure driver to enable IO multiplex function
     *
     * @param[in] en true: enable, false: disable
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     * @note If the "3-wire SPI" interface are sharing pins of the "RGB" interface to save GPIOs, please call this
     *       function to release the bus object and pins (except CS signal)
     * @note The control panel will be deleted automatically after calling `init()` function
     * @note This function is conflict with `configMirrorByCommand()`, please don't use them at the same time
     * @note This function is only valid for the "3-wire SPI + RGB" bus
     */
    bool configEnableIO_Multiplex(bool en);

    /**
     * @brief Configure the color order of LCD
     *
     * @param[in] reverse_order true: BGR order, false: RGB order
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configColorRGB_Order(bool reverse_order);

    /**
     * @brief Configure the reset active level of LCD
     *
     * @param[in] level 0: low level, 1: high level
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configResetActiveLevel(int level);

    /**
     * @brief Configure the number of frame buffers
     *
     * @param[in] num Number of frame buffers
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     * @note This function is only valid for the MIPI-DSI/RGB bus
     */
    bool configFrameBufferNumber(int num);

    /**
     * @brief Initialize the LCD device
     *
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after bus is begun
     * @note This function typically calls `esp_lcd_new_panel_*()` to create the refresh panel
     */
    virtual bool init() = 0;

    /**
     * @brief Startup the LCD device
     *
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `init()`
     * @note This function typically calls `esp_lcd_panel_init()` to initialize the refresh panel
     */
    bool begin();

    /**
     * @brief Reset the LCD
     *
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `init()`
     * @note This function typically calls `esp_lcd_panel_reset()` to reset the refresh panel
     * @note If the RESET pin is not set, this function will do reset by software instead of hardware
     */
    virtual bool reset();

    /**
     * @brief Delete the LCD device, release the resources
     *
     * @return `true` if successful, `false` otherwise
     * @note This function typically calls `esp_lcd_panel_del()` to delete the refresh panel
     */
    bool del();

    /**
     * @brief Draw the bitmap to the LCD without waiting for the drawing to finish
     *
     * @param[in] x_start X coordinate of the start point, the range is [0, lcd_width - 1]
     * @param[in] y_start Y coordinate of the start point, the range is [0, lcd_height - 1]
     * @param[in] width Width of the bitmap, the range is [0, lcd_width - x_start]
     * @param[in] height Height of the bitmap, the range is [0, lcd_height - y_start]
     * @param[in] color_data Pointer of the color data array
     * @param[in] timeout_ms Wait timeout for drawing to finish in milliseconds, default is 0, -1 means wait forever
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_draw_bitmap()` to draw the bitmap
     * @note When `timeout_ms` is 0, this function is non-blocking, the drawing will be finished in the background
     * @note When `timeout_ms` is not 0, this function is blocking until drawing finishes or timeout, so bitmap data can
     *       be modified after return
     * @note The bitmap data should not be modified until the drawing is finished
     * @note For bus which not use DMA operation (like RGB), this function typically uses `memcpy()` to copy the
     *       bitmap data to frame buffer. So the bitmap data can be immediately modified
     */
    bool drawBitmap(int x_start, int y_start, int width, int height, const uint8_t *color_data, int timeout_ms = 0);

    /**
     * @brief Mirror the X axis
     *
     * @param[in] en true to enable, false to disable
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_mirror()` to mirror the axis
     */
    bool mirrorX(bool en);

    /**
     * @brief Mirror the Y axis
     *
     * @param[in] en true to enable, false to disable
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_mirror()` to mirror the axis
     */
    bool mirrorY(bool en);

    /**
     * @brief Swap the X and Y axes
     *
     * @param[in] en true to enable, false to disable
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_swap_xy()` to swap the axes
     */
    bool swapXY(bool en);

    /**
     * @brief Set the gap in X axis
     *
     * @param[in] gap Gap in pixels
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_set_gap()` to set the gap
     */
    bool setGapX(uint16_t gap);

    /**
     * @brief Set the gap in Y axis
     *
     * @param[in] gap Gap in pixels
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_set_gap()` to set the gap
     */
    bool setGapY(uint16_t gap);

    /**
     * @brief Invert the color of each pixel
     *
     * @param[in] en true to invert, false to restore
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_invert_color()` to invert the color
     */
    bool invertColor(bool en);

    /**
     * @brief Turn the display on or off
     *
     * @param[in] enable_on true to turn on, false to turn off
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_disp_on_off()` to control the display
     */
    bool setDisplayOnOff(bool enable_on);

    /**
     * @brief Attach a callback function to be called when bitmap drawing finishes
     *
     * @param[in] callback Function to be called on completion
     * @param[in] user_data User data to pass to callback function
     * @return `true` if successful, `false` otherwise
     * @note For bus which not use DMA operation (like RGB), callback is called at end of draw function
     * @note For other bus types, callback is called when DMA operation completes
     */
    bool attachDrawBitmapFinishCallback(FunctionDrawBitmapFinishCallback callback, void *user_data = nullptr);

    /**
     * @brief Attach a callback function to be called when frame buffer refresh finishes
     *
     * @param[in] callback Function to be called on completion
     * @param[in] user_data User data to pass to callback function
     * @return `true` if successful, `false` otherwise
     * @note Only valid for RGB/MIPI-DSI bus which maintains frame buffer (GRAM)
     * @note Callback should be in IRAM if:
     *       1. For MIPI-DSI bus when `CONFIG_LCD_DSI_ISR_IRAM_SAFE` is set
     *       2. For RGB bus when `CONFIG_LCD_RGB_ISR_IRAM_SAFE` is set and XIP on PSRAM disabled
     */
    bool attachRefreshFinishCallback(FunctionRefreshFinishCallback callback, void *user_data = nullptr);

    /**
     * @brief Switch to the specified frame buffer
     *
     * @param[in] frame_buffer Frame buffer which will be switched to
     * @return `true` if successful, `false` otherwise
     * @note This function is only valid for RGB/MIPI-DSI bus which maintains frame buffer (GRAM)
     * @note This function should be called after `begin()`
     * @note This function typically calls `esp_lcd_panel_draw_bitmap()` to switch to the specified frame buffer
     */
    bool switchFrameBufferTo(void *frame_buffer);

    /**
     * @brief Draw color bars for testing
     *
     * @param[in] width Width of color bars, typically LCD width
     * @param[in] height Height of color bars, typically LCD height
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note Each bar represents 1 color bit. For 16-bit color depth, there will be 16 bars
     * @note If height not divisible by bits_per_pixel, remaining area filled white
     */
    bool colorBarTest(uint16_t width, uint16_t height);

    /**
     * @brief Draw color bars for testing
     *
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note Each bar represents 1 color bit. For 16-bit color depth, there will be 16 bars
     * @note If height not divisible by bits_per_pixel, remaining area filled white
     */
    bool colorBarTest();

#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    /**
     * @brief Show DSI color bar test pattern
     *
     * @param[in] pattern Color bar pattern to display
     * @return `true` if successful, `false` otherwise
     * @note This function should be called after `begin()`
     * @note Used for testing MIPI-DSI bus functionality
     */
    bool DSI_ColorBarPatternTest(DSI_ColorBarPattern pattern);
#endif

    /**
     * @brief Check if driver has reached specified state
     *
     * @param[in] state State to check against
     * @return `true` if current state >= given state, `false` otherwise
     */
    bool isOverState(State state)
    {
        return (_state >= state);
    }

    /**
     * @brief Check if LCD function is supported
     *
     * @param[in] function Function to check
     * @return `true` if function is supported, `false` otherwise
     */
    bool isFunctionSupported(BasicBusSpecification::Function function)
    {
        return _basic_attributes.basic_bus_spec.isFunctionValid(function);
    }

    /**
     * @brief Get frame width (horizontal, in pixels)
     */
    int getFrameWidth()
    {
        return getVendorFullConfig().hor_res;
    }

    /**
     * @brief Get frame height (vertical, in pixels)
     */
    int getFrameHeight()
    {
        return getVendorFullConfig().ver_res;
    }

    /**
     * @brief Get frame buffer color depth in bits
     *
     * @return Color depth in bits, or -1 if failed
     */
    int getFrameColorBits();

    /**
     * @brief Get frame buffer by index
     *
     * @param[in] index Frame buffer index, range [0, LCD::FRAME_BUFFER_MAX_NUM-1]
     * @return Frame buffer pointer, or nullptr if failed
     * @note This function should be called after `begin()`
     * @note Only valid for RGB/MIPI-DSI bus which maintains frame buffer (GRAM)
     */
    void *getFrameBufferByIndex(uint8_t index = 0);

    /**
     * @brief Get LCD basic attributes
     *
     * @return Reference to basic attributes structure
     */
    const BasicAttributes &getBasicAttributes()
    {
        return _basic_attributes;
    }

    /**
     * @brief Get LCD transformation settings
     *
     * @return Reference to transformation structure
     */
    const Transformation &getTransformation()
    {
        return _transformation;
    }

    /**
     * @brief Get LCD configuration
     *
     * @return Reference to configuration structure
     */
    const Config &getConfig()
    {
        return _config;
    }

    /**
     * @brief Get LCD bus interface
     *
     * @return Pointer to bus interface
     */
    Bus *getBus()
    {
        return _bus.get();
    }

    /**
     * @brief Get LCD refresh panel handle
     *
     * @return Panel handle if available, nullptr otherwise
     * @note Can be used to call low-level esp_lcd_panel_* functions directly
     */
    RefreshPanelHandle getRefreshPanelHandle()
    {
        return refresh_panel;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructor instead
     */
    [[deprecated("Use other constructor instead")]]
    LCD(const BasicAttributes &attr, Bus *bus, int color_bits, int rst_io):
        LCD(attr, bus, 0, 0, color_bits, rst_io)
    {
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configColorRGB_Order()` instead
     */
    [[deprecated("Use `configColorRGB_Order()` instead")]]
    bool configColorRgbOrder(bool reverse_order)
    {
        return configColorRGB_Order(reverse_order);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configEnableIO_Multiplex()` instead
     */
    [[deprecated("Use `configEnableIO_Multiplex()` instead")]]
    bool configAutoReleaseBus(bool en)
    {
        return configEnableIO_Multiplex(en);
    }

#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `DSI_ColorBarPatternTest()` instead
     */
    [[deprecated("Use `DSI_ColorBarPatternTest()` instead")]]
    bool showDsiPattern(DSI_ColorBarPattern pattern)
    {
        return DSI_ColorBarPatternTest(pattern);
    }
#endif

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `setDisplayOnOff(true)` instead
     */
    [[deprecated("Use `setDisplayOnOff(true)` instead")]]
    bool displayOn()
    {
        return setDisplayOnOff(true);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `setDisplayOnOff(false)` instead
     */
    [[deprecated("Use `setDisplayOnOff(false)` instead")]]
    bool displayOff()
    {
        return setDisplayOnOff(false);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `drawBitmap()` instead
     */
    [[deprecated("Use `drawBitmap()` instead")]]
    bool drawBitmapWaitUntilFinish(
        uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height, const uint8_t *color_data,
        int timeout_ms = -1
    )
    {
        return drawBitmap(x_start, y_start, width, height, color_data, timeout_ms);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getTransformation().swap_xy` instead
     */
    [[deprecated("Use `getTransformation().swap_xy` instead")]]
    bool getSwapXYFlag()
    {
        return getTransformation().swap_xy;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getTransformation().mirror_x` instead
     */
    [[deprecated("Use `getTransformation().mirror_x` instead")]]
    bool getMirrorXFlag()
    {
        return getTransformation().mirror_x;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getTransformation().mirror_y` instead
     */
    [[deprecated("Use `getTransformation().mirror_y` instead")]]
    bool getMirrorYFlag()
    {
        return getTransformation().mirror_y;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getFrameColorBits()` instead
     */
    [[deprecated("Use `getFrameColorBits()` instead")]]
    int getColorBits()
    {
        return getFrameColorBits();
    }

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getFrameBufferByIndex()` instead
     */
    [[deprecated("Use `getFrameBufferByIndex()` instead")]]
    void *getRgbBufferByIndex(uint8_t index = 0)
    {
        return getFrameBufferByIndex(index);
    }
#endif

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getBasicAttributes().basic_bus_spec.x_coord_align` instead
     */
    [[deprecated("Use `getBasicAttributes().basic_bus_spec.x_coord_align` instead")]]
    uint8_t getXCoordAlign()
    {
        return getBasicAttributes().basic_bus_spec.x_coord_align;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getBasicAttributes().basic_bus_spec.y_coord_align` instead
     */
    [[deprecated("Use `getBasicAttributes().basic_bus_spec.y_coord_align` instead")]]
    uint8_t getYCoordAlign()
    {
        return getBasicAttributes().basic_bus_spec.y_coord_align;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getRefreshPanelHandle()` instead
     */
    [[deprecated("Use `getRefreshPanelHandle()` instead")]]
    RefreshPanelHandle getHandle()
    {
        return getRefreshPanelHandle();
    }

protected:
    /**
     * @brief Process device initialization
     *
     * @param[in] bus_specs Bus specifications from derived class
     * @return `true` if successful, `false` otherwise
     * @note Must be called at start of derived class init()
     */
    bool processDeviceOnInit(const BasicBusSpecificationMap &bus_specs);

    /**
     * @brief Check if bus interface is valid
     *
     * @return `true` if valid, `false` otherwise
     */
    bool isBusValid()
    {
        return (_bus != nullptr);
    }

    /**
     * @brief Set driver state
     *
     * @param[in] state New state to set
     */
    void setState(State state)
    {
        _state = state;
    }

    RefreshPanelHandle refresh_panel = nullptr;      /*!< Refresh panel handle */

private:
    /**
     * @brief Interrupt handling structure
     */
    struct Interruption {
        /**
         * @brief Callback data structure
         */
        struct CallbackData {
            void *lcd_ptr = nullptr;      /*!< Pointer to LCD instance */
            void *user_data = nullptr;    /*!< User provided data */
        };

        CallbackData data = {};                                           /*!< Callback data */
        FunctionDrawBitmapFinishCallback on_draw_bitmap_finish = nullptr; /*!< Draw completion callback */
        FunctionRefreshFinishCallback on_refresh_finish = nullptr;        /*!< Refresh completion callback */
        SemaphoreHandle_t draw_bitmap_finish_sem = nullptr;              /*!< Draw completion semaphore */
        std::shared_ptr<StaticSemaphore_t> on_draw_bitmap_finish_sem_buffer = nullptr; /*!< Semaphore buffer */
    };

    /**
     * @brief Get device full configuration
     *
     * Convert partial configuration to full configuration if necessary
     *
     * @return Reference to full device configuration
     */
    DeviceFullConfig &getDeviceFullConfig();

    /**
     * @brief Get vendor full configuration
     *
     * Convert partial configuration to full configuration if necessary
     *
     * @return Reference to full vendor configuration
     */
    VendorFullConfig &getVendorFullConfig();

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
    /**
     * @brief Get the RGB bus refresh panel full configuration
     *
     * @return Pointer to the RGB bus refresh panel full configuration, or `nullptr` if RGB bus is not enabled
     */
    const BusRGB::RefreshPanelFullConfig *getBusRGB_RefreshPanelFullConfig();
#endif

#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    /**
     * @brief Get the MIPI-DSI bus refresh panel full configuration
     *
     * @return Pointer to the MIPI-DSI bus refresh panel full configuration, or `nullptr` if MIPI-DSI bus is not enabled
     */
    const BusDSI::RefreshPanelFullConfig *getBusDSI_RefreshPanelFullConfig();
#endif

    IRAM_ATTR static bool onDrawBitmapFinish(void *panel_io, void *edata, void *user_ctx);
    IRAM_ATTR static bool onRefreshFinish(void *panel_io, void *edata, void *user_ctx);

    BasicAttributes _basic_attributes = {};     /*!< Basic device attributes */
    std::shared_ptr<Bus> _bus = nullptr;        /*!< Bus interface pointer */
    Config _config = {};                        /*!< Device configuration */
    State _state = State::DEINIT;               /*!< Current driver state */
    Transformation _transformation = {};        /*!< Coordinate transformation settings */
    Interruption _interruption = {};            /*!< Interrupt handling */
};

} // namespace esp_panel::drivers

#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::LCD::DSI_ColorBarPattern` instead
 */
using DsiPatternType [[deprecated("Use `esp_panel::drivers::LCD::DSI_ColorBarPattern` instead")]] =
    esp_panel::drivers::LCD::DSI_ColorBarPattern;
#endif

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::LCD` instead
 */
using ESP_PanelLcd [[deprecated("Use `esp_panel::drivers::LCD` instead")]] = esp_panel::drivers::LCD;

/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: ESP32_S3_BOX_3, ILI9341' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following SPI LCDs:
 *      - AXS15231B
 *      - GC9A01, GC9B71
 *      - ILI9341
 *      - NV3022B
 *      - SH8601
 *      - SPD2010
 *      - ST7789, ST7796, ST77916, ST77922
 */
#define EXAMPLE_LCD_NAME                ILI9341
#define EXAMPLE_LCD_WIDTH               (320)
#define EXAMPLE_LCD_HEIGHT              (240)
#define EXAMPLE_LCD_COLOR_BITS          (16)
#define EXAMPLE_LCD_SPI_FREQ_HZ         (40 * 1000 * 1000)
#define EXAMPLE_LCD_USE_EXTERNAL_CMD    (1)
#if EXAMPLE_LCD_USE_EXTERNAL_CMD
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
const esp_panel_lcd_vendor_init_cmd_t lcd_init_cmd[] = {
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC8, {0xFF, 0x93, 0x42}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x0E, 0x0E}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC5, {0xD0}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB4, {0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE0, {0x00, 0x03, 0x08, 0x06, 0x13, 0x09, 0x39, 0x39, 0x48, 0x02, 0x0a, 0x08,
                                                0x17, 0x17, 0x0F}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE1, {0x00, 0x28, 0x29, 0x01, 0x0d, 0x03, 0x3f, 0x33, 0x52, 0x04, 0x0f, 0x0e,
                                                0x37, 0x38, 0x0F}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {00, 0x1B}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB7, {0x06}),
    ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(100, 0x11),
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_SPI_IO_CS       (5)
#define EXAMPLE_LCD_SPI_IO_DC       (4)
#define EXAMPLE_LCD_SPI_IO_SCK      (7)
#define EXAMPLE_LCD_SPI_IO_SDA      (6)
#define EXAMPLE_LCD_SPI_IO_SDO      (-1)
#define EXAMPLE_LCD_RST_IO          (48)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_IO           (47)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_ON_LEVEL     (1)
#define EXAMPLE_LCD_BL_OFF_LEVEL    (!EXAMPLE_LCD_BL_ON_LEVEL)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please update the following configuration according to your test ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG   (0)
#define EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK (1)

#define _EXAMPLE_LCD_CLASS(name, ...)   LCD_##name(__VA_ARGS__)
#define EXAMPLE_LCD_CLASS(name, ...)    _EXAMPLE_LCD_CLASS(name, ##__VA_ARGS__)

static LCD *create_lcd_without_config(void)
{
    BusSPI *bus = new BusSPI(
        EXAMPLE_LCD_SPI_IO_CS, EXAMPLE_LCD_SPI_IO_DC, EXAMPLE_LCD_SPI_IO_SCK,
        EXAMPLE_LCD_SPI_IO_SDA, EXAMPLE_LCD_SPI_IO_SDO
    );

    /**
     * Take `ILI9341` as an example, the following is the actual code after macro expansion:
     *      LCD_ILI9341(bus, 320, 240, 16, 48);
     */
    return new EXAMPLE_LCD_CLASS(
        EXAMPLE_LCD_NAME, bus, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COLOR_BITS, EXAMPLE_LCD_RST_IO
    );
}

static LCD *create_lcd_with_config(void)
{
    BusSPI::Config bus_config = {
        .host = BusSPI::HostPartialConfig{
            .mosi_io_num = EXAMPLE_LCD_SPI_IO_SDA,
            .sclk_io_num = EXAMPLE_LCD_SPI_IO_SCK,
        },
        .control_panel = BusSPI::ControlPanelPartialConfig{
            .cs_gpio_num = EXAMPLE_LCD_SPI_IO_CS,
            .dc_gpio_num = EXAMPLE_LCD_SPI_IO_DC,
            .pclk_hz = EXAMPLE_LCD_SPI_FREQ_HZ,
        },
    };
    LCD::Config lcd_config = {
        .device = LCD::DevicePartialConfig{
            .reset_gpio_num = EXAMPLE_LCD_RST_IO,
            .bits_per_pixel = EXAMPLE_LCD_COLOR_BITS,
            .flags_reset_active_high = EXAMPLE_LCD_RST_LEVEL,
        },
        .vendor = LCD::VendorPartialConfig{
            .hor_res = EXAMPLE_LCD_WIDTH,
            .ver_res = EXAMPLE_LCD_HEIGHT,
#if EXAMPLE_LCD_USE_EXTERNAL_CMD
            .init_cmds = lcd_init_cmd,
            .init_cmds_size = sizeof(lcd_init_cmd) / sizeof(lcd_init_cmd[0]),
#endif
        },
    };

    /**
     * Take `ILI9341` as an example, the following is the actual code after macro expansion:
     *      LCD_ILI9341(bus_config, lcd_config);
     */
    return new EXAMPLE_LCD_CLASS(EXAMPLE_LCD_NAME, bus_config, lcd_config);
}

#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
IRAM_ATTR bool onLCD_DrawFinishCallback(void *user_data)
{
    esp_rom_printf("LCD draw finish callback\n");

    return false;
}
#endif

void setup()
{
    Serial.begin(115200);

#if EXAMPLE_LCD_BL_IO >= 0
    Serial.println("Initializing backlight and turn it off");
    BacklightPWM_LEDC *backlight = new BacklightPWM_LEDC(EXAMPLE_LCD_BL_IO, EXAMPLE_LCD_BL_ON_LEVEL);
    backlight->begin();
    backlight->off();
#endif

#if EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"SPI\" LCD with config");
    auto lcd = create_lcd_with_config();
#else
    Serial.println("Initializing \"SPI\" LCD without config");
    auto lcd = create_lcd_without_config();
#endif

    /* Configure bus and lcd before startup */
    auto bus = static_cast<BusSPI *>(lcd->getBus());
    bus->configSPI_FreqHz(EXAMPLE_LCD_SPI_FREQ_HZ);
#if EXAMPLE_LCD_USE_EXTERNAL_CMD && !EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    // Configure external LCD initialization commands
    lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd)/sizeof(lcd_init_cmd[0]));
#endif
    // lcd->configColorRGB_Order(true);
    // lcd->configResetActiveLevel(1);
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
    // Attach a callback function which will be called when every bitmap drawing is completed
    lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif

    /* Startup the LCD and operate it */
    assert(lcd->begin());
    // lcd->mirrorX(true);
    // lcd->mirrorY(true);
    lcd->displayOn();

    Serial.println("Draw color bar from top left to bottom right, the order is B - G - R");
    lcd->colorBarTest();

#if EXAMPLE_LCD_BL_IO >= 0
    Serial.println("Turn on the backlight");
    backlight->on();
#endif
}

void loop()
{
    Serial.println("IDLE loop");
    delay(1000);
}

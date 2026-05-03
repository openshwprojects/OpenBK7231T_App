/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: Custom, ST77922' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following QSPI LCDs:
 *      - AXS15231B
 *      - GC9B71
 *      - SH8601
 *      - SPD2010
 *      - ST77916, ST77922
 */
#define EXAMPLE_LCD_NAME                ST77922
#define EXAMPLE_LCD_WIDTH               (532)
#define EXAMPLE_LCD_HEIGHT              (300)
#define EXAMPLE_LCD_COLOR_BITS          (16)
#define EXAMPLE_LCD_QSPI_FREQ_HZ        (40 * 1000 * 1000)
#define EXAMPLE_LCD_USE_EXTERNAL_CMD    (0)
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
    // ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x10}),
    // ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x3B, 0x00}),
    // ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x0D, 0x02}),
    // ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(120, 0x29),
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_QSPI_IO_CS          (9)
#define EXAMPLE_LCD_QSPI_IO_SCK         (10)
#define EXAMPLE_LCD_QSPI_IO_DATA0       (11)
#define EXAMPLE_LCD_QSPI_IO_DATA1       (12)
#define EXAMPLE_LCD_QSPI_IO_DATA2       (13)
#define EXAMPLE_LCD_QSPI_IO_DATA3       (14)
#define EXAMPLE_LCD_RST_IO              (3)     // Set to -1 if not used
#define EXAMPLE_LCD_BL_IO               (-1)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_ON_LEVEL         (1)
#define EXAMPLE_LCD_BL_OFF_LEVEL        (!EXAMPLE_LCD_BL_ON_LEVEL)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please update the following configuration according to your test ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG   (0)
#define EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK (1)

#define _EXAMPLE_LCD_CLASS(name, ...)   LCD_##name(__VA_ARGS__)
#define EXAMPLE_LCD_CLASS(name, ...)    _EXAMPLE_LCD_CLASS(name, ##__VA_ARGS__)

static LCD *create_lcd_without_config(void)
{
    BusQSPI *bus = new BusQSPI(
        EXAMPLE_LCD_QSPI_IO_CS, EXAMPLE_LCD_QSPI_IO_SCK,
        EXAMPLE_LCD_QSPI_IO_DATA0, EXAMPLE_LCD_QSPI_IO_DATA1, EXAMPLE_LCD_QSPI_IO_DATA2, EXAMPLE_LCD_QSPI_IO_DATA3
    );

    /**
     * Take `ST77922` as an example, the following is the actual code after macro expansion:
     *      LCD_ST77922(bus, 532, 300, 16, 3);
     */
    return new EXAMPLE_LCD_CLASS(
        EXAMPLE_LCD_NAME, bus, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COLOR_BITS, EXAMPLE_LCD_RST_IO
    );
}

static LCD *create_lcd_with_config(void)
{
    BusQSPI::Config bus_config = {
        .host = BusQSPI::HostPartialConfig{
            .sclk_io_num = EXAMPLE_LCD_QSPI_IO_SCK,
            .data0_io_num = EXAMPLE_LCD_QSPI_IO_DATA0,
            .data1_io_num = EXAMPLE_LCD_QSPI_IO_DATA1,
            .data2_io_num = EXAMPLE_LCD_QSPI_IO_DATA2,
            .data3_io_num = EXAMPLE_LCD_QSPI_IO_DATA3,
        },
        .control_panel = BusQSPI::ControlPanelPartialConfig{
            .cs_gpio_num = EXAMPLE_LCD_QSPI_IO_CS,
            .pclk_hz = EXAMPLE_LCD_QSPI_FREQ_HZ,
        },
    };
    LCD::Config lcd_config = {
        .device = LCD::DevicePartialConfig{
            .reset_gpio_num = EXAMPLE_LCD_RST_IO,
            .bits_per_pixel = EXAMPLE_LCD_COLOR_BITS,
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
     * Take `ST77922` as an example, the following is the actual code after macro expansion:
     *      LCD_ST77922(bus_config, lcd_config);
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
    Serial.println("Initializing \"QSPI\" LCD with config");
    auto lcd = create_lcd_with_config();
#else
    Serial.println("Initializing \"QSPI\" LCD without config");
    auto lcd = create_lcd_without_config();
#endif

    /* Configure bus and lcd before startup */
    auto bus = static_cast<BusQSPI *>(lcd->getBus());
    bus->configQSPI_FreqHz(EXAMPLE_LCD_QSPI_FREQ_HZ);
#if EXAMPLE_LCD_USE_EXTERNAL_CMD && !EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    // Configure external LCD initialization commands
    lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd)/sizeof(lcd_init_cmd[0]));
#endif
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
    // Attach a callback function which will be called when every bitmap drawing is completed
    lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif

    /* Startup the LCD and operate it */
    assert(lcd->begin());
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

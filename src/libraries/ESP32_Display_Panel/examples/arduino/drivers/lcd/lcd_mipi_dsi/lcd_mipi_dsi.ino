/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: ESP32-P4-Function-EV-Board, EK79007' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following MIPI-DSI LCDs:
 *      - EK79007
 *      - HX8399
 *      - ILI9881C
 *      - JD9165, JD9365
 *      - ST7701, ST7703, ST7796, ST77922
 */
#define EXAMPLE_LCD_NAME                EK79007
#define EXAMPLE_LCD_WIDTH               (1024)
#define EXAMPLE_LCD_HEIGHT              (600)
#define EXAMPLE_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB888)
                                                // or `ESP_PANEL_LCD_COLOR_BITS_RGB565`
#define EXAMPLE_LCD_DSI_PHY_LDO_ID      (3)     // -1 if not used
#define EXAMPLE_LCD_DSI_LANE_NUM        (2)     // ESP32-P4 supports 1 or 2 lanes
#define EXAMPLE_LCD_DSI_LANE_RATE_MBPS  (1000)  /* Single lane bit rate, should consult the LCD supplier or check the
                                                 * LCD drive IC datasheet for the supported lane rate.
                                                 * ESP32-P4 supports max 1500Mbps
                                                 */
#define EXAMPLE_LCD_DPI_CLK_MHZ         (52)
#define EXAMPLE_LCD_DPI_COLOR_BITS      (EXAMPLE_LCD_COLOR_BITS)
#define EXAMPLE_LCD_DPI_TIMINGS_HPW     (10)
#define EXAMPLE_LCD_DPI_TIMINGS_HBP     (160)
#define EXAMPLE_LCD_DPI_TIMINGS_HFP     (160)
#define EXAMPLE_LCD_DPI_TIMINGS_VPW     (1)
#define EXAMPLE_LCD_DPI_TIMINGS_VBP     (23)
#define EXAMPLE_LCD_DPI_TIMINGS_VFP     (12)
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
#define EXAMPLE_LCD_RST_IO          (27)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_IO           (26)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_ON_LEVEL     (1)
#define EXAMPLE_LCD_BL_OFF_LEVEL    (!EXAMPLE_LCD_BL_ON_LEVEL)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please update the following configuration according to your test ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG   (0)
#define EXAMPLE_LCD_ENABLE_PRINT_FPS            (1)
#define EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK (1)
#define EXAMPLE_LCD_ENABLE_DSI_PATTERN_TEST     (1)

#define _EXAMPLE_LCD_CLASS(name, ...)   LCD_##name(__VA_ARGS__)
#define EXAMPLE_LCD_CLASS(name, ...)    _EXAMPLE_LCD_CLASS(name, ##__VA_ARGS__)

static LCD *create_lcd_without_config(void)
{
    BusDSI *bus = new BusDSI(
        /* DSI */
        EXAMPLE_LCD_DSI_LANE_NUM, EXAMPLE_LCD_DSI_LANE_RATE_MBPS,
        /* DPI */
        EXAMPLE_LCD_DPI_CLK_MHZ, EXAMPLE_LCD_DPI_COLOR_BITS, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT,
        EXAMPLE_LCD_DPI_TIMINGS_HPW, EXAMPLE_LCD_DPI_TIMINGS_HBP, EXAMPLE_LCD_DPI_TIMINGS_HFP,
        EXAMPLE_LCD_DPI_TIMINGS_VPW, EXAMPLE_LCD_DPI_TIMINGS_VBP, EXAMPLE_LCD_DPI_TIMINGS_VFP,
        /* PHY LDO */
        EXAMPLE_LCD_DSI_PHY_LDO_ID
    );

    /**
     * Take `ILI9881C` as an example, the following is the actual code after macro expansion:
     *      LCD_ILI9881C(bus, 1024, 600, 16, 27);
     */
    return new EXAMPLE_LCD_CLASS(
        EXAMPLE_LCD_NAME, bus, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COLOR_BITS, EXAMPLE_LCD_RST_IO
    );
}

static LCD *create_lcd_with_config(void)
{
    BusDSI::Config bus_config = {
        .host = BusDSI::HostPartialConfig{
            .num_data_lanes = EXAMPLE_LCD_DSI_LANE_NUM,
            .lane_bit_rate_mbps = EXAMPLE_LCD_DSI_LANE_RATE_MBPS,
        },
        .refresh_panel = BusDSI::RefreshPanelPartialConfig{
            .dpi_clock_freq_mhz = EXAMPLE_LCD_DPI_CLK_MHZ,
            .bits_per_pixel = EXAMPLE_LCD_DPI_COLOR_BITS,
            .h_size = EXAMPLE_LCD_WIDTH,
            .v_size = EXAMPLE_LCD_HEIGHT,
            .hsync_pulse_width = EXAMPLE_LCD_DPI_TIMINGS_HPW,
            .hsync_back_porch = EXAMPLE_LCD_DPI_TIMINGS_HBP,
            .hsync_front_porch = EXAMPLE_LCD_DPI_TIMINGS_HFP,
            .vsync_pulse_width = EXAMPLE_LCD_DPI_TIMINGS_VPW,
            .vsync_back_porch = EXAMPLE_LCD_DPI_TIMINGS_VBP,
            .vsync_front_porch = EXAMPLE_LCD_DPI_TIMINGS_VFP,
        },
        .phy_ldo = BusDSI::PHY_LDO_PartialConfig{
            .chan_id = EXAMPLE_LCD_DSI_PHY_LDO_ID,
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
     * Take `ILI9881C` as an example, the following is the actual code after macro expansion:
     *      LCD_ILI9881C(bus_config, lcd_config);
     */
    return new EXAMPLE_LCD_CLASS(EXAMPLE_LCD_NAME, bus_config, lcd_config);
}

#if EXAMPLE_LCD_ENABLE_PRINT_FPS
#define EXAMPLE_LCD_PRINT_FPS_PERIOD_MS         (1000)
#define EXAMPLE_LCD_PRINT_FPS_COUNT_MAX         (50)

DRAM_ATTR int frame_count = 0;
DRAM_ATTR int fps = 0;
DRAM_ATTR long start_time = 0;

IRAM_ATTR bool onLCD_RefreshFinishCallback(void *user_data)
{
    if (start_time == 0) {
        start_time = millis();

        return false;
    }

    frame_count++;
    if (frame_count >= EXAMPLE_LCD_PRINT_FPS_COUNT_MAX) {
        fps = EXAMPLE_LCD_PRINT_FPS_COUNT_MAX * 1000 / (millis() - start_time);
        esp_rom_printf("LCD FPS: %d\n", fps);
        frame_count = 0;
        start_time = millis();
    }

    return false;
}
#endif // EXAMPLE_LCD_ENABLE_PRINT_FPS

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
    Serial.println("Initializing backlight and turn it on");
    BacklightPWM_LEDC *backlight = new BacklightPWM_LEDC(EXAMPLE_LCD_BL_IO, EXAMPLE_LCD_BL_ON_LEVEL);
    backlight->begin();
    backlight->on();
#endif

#if EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"MIPI-DSI\" LCD with config");
    auto lcd = create_lcd_with_config();
#else
    Serial.println("Initializing \"MIPI-DSI\" LCD without config");
    auto lcd = create_lcd_without_config();
#endif

    /* Configure bus and lcd before startup */
#if EXAMPLE_LCD_USE_EXTERNAL_CMD && !EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    // Configure external LCD initialization commands
    lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd)/sizeof(lcd_init_cmd[0]));
#endif
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
    // Attach a callback function which will be called when every bitmap drawing is completed
    lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif
#if EXAMPLE_LCD_ENABLE_PRINT_FPS
    // Attach a callback function which will be called when the Vsync signal is detected
    lcd->attachRefreshFinishCallback(onLCD_RefreshFinishCallback);
#endif

    /* Startup the LCD and operate it */
    assert(lcd->begin());
    if (lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) {
        lcd->setDisplayOnOff(true);
    }

#if EXAMPLE_LCD_ENABLE_DSI_PATTERN_TEST
    Serial.println("Show MIPI-DSI color bar patterns");
    lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::BAR_HORIZONTAL);
    delay(1000);
    lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::BAR_VERTICAL);
    delay(1000);
    lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::BER_VERTICAL);
    delay(1000);
    lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::NONE);
#endif

    Serial.println("Draw color bar from top left to bottom right, the order is B - G - R");
    lcd->colorBarTest();
}

void loop()
{
    Serial.println("IDLE loop");
    delay(1000);
}

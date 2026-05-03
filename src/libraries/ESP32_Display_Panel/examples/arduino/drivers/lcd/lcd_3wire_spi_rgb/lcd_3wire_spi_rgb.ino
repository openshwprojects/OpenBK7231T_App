/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'jingcai: ESP32_4848S040C_I_Y_3, ST7701' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following "3-wire SPI + RGB" LCDs:
 *      - GC9503
 *      - ST7701, ST77903, ST77922
 */
#define EXAMPLE_LCD_NAME                    ST7701
#define EXAMPLE_LCD_WIDTH                   (480)
#define EXAMPLE_LCD_HEIGHT                  (480)
                                                    // | 8-bit RGB888 | 16-bit RGB565 |
#define EXAMPLE_LCD_COLOR_BITS              (18)    // |      24      |   16/18/24    |
#define EXAMPLE_LCD_RGB_DATA_WIDTH          (16)    // |      8       |      16       |
#define EXAMPLE_LCD_RGB_COLOR_BITS          (16)    // |      24      |      16       |
#define EXAMPLE_LCD_RGB_TIMING_FREQ_HZ      (26 * 1000 * 1000)
#define EXAMPLE_LCD_RGB_TIMING_HPW          (10)
#define EXAMPLE_LCD_RGB_TIMING_HBP          (10)
#define EXAMPLE_LCD_RGB_TIMING_HFP          (20)
#define EXAMPLE_LCD_RGB_TIMING_VPW          (10)
#define EXAMPLE_LCD_RGB_TIMING_VBP          (10)
#define EXAMPLE_LCD_RGB_TIMING_VFP          (10)
#define EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE  (EXAMPLE_LCD_WIDTH * 10)
#define EXAMPLE_LCD_USE_EXTERNAL_CMD        (1)
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
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x10}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x3B, 0x00}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x0D, 0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC2, {0x31, 0x05}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xCD, {0x00}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB0, {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12,
                                                0x0F, 0xAA, 0x31, 0x18}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11,
                                                0x11, 0xA9, 0x32, 0x18}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x11}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB0, {0x60}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {0x32}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB2, {0x07}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB3, {0x80}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB5, {0x49}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB7, {0x85}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB8, {0x21}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x78}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC2, {0x78}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE0, {0x00, 0x1B, 0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE1, {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE2, {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE3, {0x00, 0x00, 0x11, 0x11}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE4, {0x44, 0x44}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE5, {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0,
                                                0x10, 0xEF, 0xD8, 0xA0}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE6, {0x00, 0x00, 0x11, 0x11}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE7, {0x44, 0x44}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE8, {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0,
                                                0x0F, 0xEE, 0xD8, 0xA0}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xEB, {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xEC, {0x3C, 0x00}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xED, {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20,
                                                0x45, 0x67, 0x98, 0xBA}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x13}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE5, {0xE4}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x00}),
    ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(120, 0x11),
};
#endif // EXAMPLE_LCD_USE_EXTERNAL_CMD

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_RGB_IO_DISP         (-1)
#define EXAMPLE_LCD_RGB_IO_VSYNC        (17)
#define EXAMPLE_LCD_RGB_IO_HSYNC        (16)
#define EXAMPLE_LCD_RGB_IO_DE           (18)
#define EXAMPLE_LCD_RGB_IO_PCLK         (21)
                                                // The following sheet shows the mapping of ESP GPIOs to
                                                // LCD data pins with different data width and color format:
                                                // ┏------┳- ------------┳--------------------------┓
                                                // | ESP: | 8-bit RGB888 |      16-bit RGB565       |
                                                // |------|--------------|--------------------------|
                                                // | LCD: |    RGB888    | RGB565 | RGB666 | RGB888 |
                                                // ┗------|--------------|--------|--------|--------|
#define EXAMPLE_LCD_RGB_IO_DATA0        (4)     //        |      D0      |   B0   |  B0-1  |   B0-3 |
#define EXAMPLE_LCD_RGB_IO_DATA1        (5)     //        |      D1      |   B1   |  B2    |   B4   |
#define EXAMPLE_LCD_RGB_IO_DATA2        (6)     //        |      D2      |   B2   |  B3    |   B5   |
#define EXAMPLE_LCD_RGB_IO_DATA3        (7)     //        |      D3      |   B3   |  B4    |   B6   |
#define EXAMPLE_LCD_RGB_IO_DATA4        (15)    //        |      D4      |   B4   |  B5    |   B7   |
#define EXAMPLE_LCD_RGB_IO_DATA5        (8)     //        |      D5      |   G0   |  G0    |   G0-2 |
#define EXAMPLE_LCD_RGB_IO_DATA6        (20)    //        |      D6      |   G1   |  G1    |   G3   |
#define EXAMPLE_LCD_RGB_IO_DATA7        (3)     //        |      D7      |   G2   |  G2    |   G4   |
#if EXAMPLE_LCD_RGB_DATA_WIDTH > 8              //        ┗--------------┫--------|--------|--------|
#define EXAMPLE_LCD_RGB_IO_DATA8        (46)    //                       |   G3   |  G3    |   G5   |
#define EXAMPLE_LCD_RGB_IO_DATA9        (9)     //                       |   G4   |  G4    |   G6   |
#define EXAMPLE_LCD_RGB_IO_DATA10       (10)    //                       |   G5   |  G5    |   G7   |
#define EXAMPLE_LCD_RGB_IO_DATA11       (11)    //                       |   R0   |  R0-1  |   R0-3 |
#define EXAMPLE_LCD_RGB_IO_DATA12       (12)    //                       |   R1   |  R2    |   R4   |
#define EXAMPLE_LCD_RGB_IO_DATA13       (13)    //                       |   R2   |  R3    |   R5   |
#define EXAMPLE_LCD_RGB_IO_DATA14       (14)    //                       |   R3   |  R4    |   R6   |
#define EXAMPLE_LCD_RGB_IO_DATA15       (0)     //                       |   R4   |  R5    |   R7   |
                                                //                       ┗--------┻--------┻--------┛
#endif
#define EXAMPLE_LCD_SPI_IO_CS           (39)
#define EXAMPLE_LCD_SPI_IO_SCK          (48)
#define EXAMPLE_LCD_SPI_IO_SDA          (47)
#define EXAMPLE_LCD_RST_IO              (-1)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_IO               (38)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_ON_LEVEL         (1)
#define EXAMPLE_LCD_BL_OFF_LEVEL        (!EXAMPLE_LCD_BL_ON_LEVEL)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please update the following configuration according to your test ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG   (0)
#define EXAMPLE_LCD_ENABLE_PRINT_FPS            (1)
#define EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK (1)

#define _EXAMPLE_LCD_CLASS(name, ...) LCD_##name(__VA_ARGS__)
#define EXAMPLE_LCD_CLASS(name, ...)  _EXAMPLE_LCD_CLASS(name, ##__VA_ARGS__)

static LCD *create_lcd_without_config(void)
{
    BusRGB *bus = new BusRGB(
#if EXAMPLE_LCD_RGB_DATA_WIDTH == 8
        /* 3-wire SPI IOs */
        EXAMPLE_LCD_SPI_IO_CS, EXAMPLE_LCD_SPI_IO_SCK, EXAMPLE_LCD_SPI_IO_SDA,
        /* 8-bit RGB IOs */
        EXAMPLE_LCD_RGB_IO_DATA0, EXAMPLE_LCD_RGB_IO_DATA1, EXAMPLE_LCD_RGB_IO_DATA2, EXAMPLE_LCD_RGB_IO_DATA3,
        EXAMPLE_LCD_RGB_IO_DATA4, EXAMPLE_LCD_RGB_IO_DATA5, EXAMPLE_LCD_RGB_IO_DATA6, EXAMPLE_LCD_RGB_IO_DATA7,
        EXAMPLE_LCD_RGB_IO_HSYNC, EXAMPLE_LCD_RGB_IO_VSYNC, EXAMPLE_LCD_RGB_IO_PCLK, EXAMPLE_LCD_RGB_IO_DE,
        EXAMPLE_LCD_RGB_IO_DISP,
        /* RGB timings */
        EXAMPLE_LCD_RGB_TIMING_FREQ_HZ, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT,
        EXAMPLE_LCD_RGB_TIMING_HPW, EXAMPLE_LCD_RGB_TIMING_HBP, EXAMPLE_LCD_RGB_TIMING_HFP,
        EXAMPLE_LCD_RGB_TIMING_VPW, EXAMPLE_LCD_RGB_TIMING_VBP, EXAMPLE_LCD_RGB_TIMING_VFP
#elif EXAMPLE_LCD_RGB_DATA_WIDTH == 16
        /* 3-wire SPI IOs */
        EXAMPLE_LCD_SPI_IO_CS, EXAMPLE_LCD_SPI_IO_SCK, EXAMPLE_LCD_SPI_IO_SDA,
        /* 16-bit RGB IOs */
        EXAMPLE_LCD_RGB_IO_DATA0, EXAMPLE_LCD_RGB_IO_DATA1, EXAMPLE_LCD_RGB_IO_DATA2, EXAMPLE_LCD_RGB_IO_DATA3,
        EXAMPLE_LCD_RGB_IO_DATA4, EXAMPLE_LCD_RGB_IO_DATA5, EXAMPLE_LCD_RGB_IO_DATA6, EXAMPLE_LCD_RGB_IO_DATA7,
        EXAMPLE_LCD_RGB_IO_DATA8, EXAMPLE_LCD_RGB_IO_DATA9, EXAMPLE_LCD_RGB_IO_DATA10, EXAMPLE_LCD_RGB_IO_DATA11,
        EXAMPLE_LCD_RGB_IO_DATA12, EXAMPLE_LCD_RGB_IO_DATA13, EXAMPLE_LCD_RGB_IO_DATA14, EXAMPLE_LCD_RGB_IO_DATA15,
        EXAMPLE_LCD_RGB_IO_HSYNC, EXAMPLE_LCD_RGB_IO_VSYNC, EXAMPLE_LCD_RGB_IO_PCLK, EXAMPLE_LCD_RGB_IO_DE,
        EXAMPLE_LCD_RGB_IO_DISP,
        /* RGB timings */
        EXAMPLE_LCD_RGB_TIMING_FREQ_HZ, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT,
        EXAMPLE_LCD_RGB_TIMING_HPW, EXAMPLE_LCD_RGB_TIMING_HBP, EXAMPLE_LCD_RGB_TIMING_HFP,
        EXAMPLE_LCD_RGB_TIMING_VPW, EXAMPLE_LCD_RGB_TIMING_VBP, EXAMPLE_LCD_RGB_TIMING_VFP
#endif
    );

    /**
     * Take `ST7701` as an example, the following is the actual code after macro expansion:
     *      LCD_ST7701(bus, 18, -1);
     */
    return new EXAMPLE_LCD_CLASS(
        EXAMPLE_LCD_NAME, bus, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COLOR_BITS, EXAMPLE_LCD_RST_IO
    );
}

static LCD *create_lcd_with_config(void)
{
    BusRGB::Config bus_config = {
        .control_panel = BusRGB::ControlPanelPartialConfig{
            .cs_gpio_num = EXAMPLE_LCD_SPI_IO_CS,
            .scl_gpio_num = EXAMPLE_LCD_SPI_IO_SCK,
            .sda_gpio_num = EXAMPLE_LCD_SPI_IO_SDA,
        },
        .refresh_panel = BusRGB::RefreshPanelPartialConfig{
            .pclk_hz = EXAMPLE_LCD_RGB_TIMING_FREQ_HZ,
            .h_res = EXAMPLE_LCD_WIDTH,
            .v_res = EXAMPLE_LCD_HEIGHT,
            .hsync_pulse_width = EXAMPLE_LCD_RGB_TIMING_HPW,
            .hsync_back_porch = EXAMPLE_LCD_RGB_TIMING_HBP,
            .hsync_front_porch = EXAMPLE_LCD_RGB_TIMING_HFP,
            .vsync_pulse_width = EXAMPLE_LCD_RGB_TIMING_VPW,
            .vsync_back_porch = EXAMPLE_LCD_RGB_TIMING_VBP,
            .vsync_front_porch = EXAMPLE_LCD_RGB_TIMING_VFP,
            .data_width = EXAMPLE_LCD_RGB_DATA_WIDTH,
            .bits_per_pixel = EXAMPLE_LCD_RGB_COLOR_BITS,
            .bounce_buffer_size_px = EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE,
            .hsync_gpio_num = EXAMPLE_LCD_RGB_IO_HSYNC,
            .vsync_gpio_num = EXAMPLE_LCD_RGB_IO_VSYNC,
            .de_gpio_num = EXAMPLE_LCD_RGB_IO_DE,
            .pclk_gpio_num = EXAMPLE_LCD_RGB_IO_PCLK,
            .disp_gpio_num = EXAMPLE_LCD_RGB_IO_DISP,
            .data_gpio_nums = {
                EXAMPLE_LCD_RGB_IO_DATA0, EXAMPLE_LCD_RGB_IO_DATA1, EXAMPLE_LCD_RGB_IO_DATA2, EXAMPLE_LCD_RGB_IO_DATA3,
                EXAMPLE_LCD_RGB_IO_DATA4, EXAMPLE_LCD_RGB_IO_DATA5, EXAMPLE_LCD_RGB_IO_DATA6, EXAMPLE_LCD_RGB_IO_DATA7,
#if EXAMPLE_LCD_RGB_DATA_WIDTH > 8
                EXAMPLE_LCD_RGB_IO_DATA8, EXAMPLE_LCD_RGB_IO_DATA9, EXAMPLE_LCD_RGB_IO_DATA10, EXAMPLE_LCD_RGB_IO_DATA11,
                EXAMPLE_LCD_RGB_IO_DATA12, EXAMPLE_LCD_RGB_IO_DATA13, EXAMPLE_LCD_RGB_IO_DATA14, EXAMPLE_LCD_RGB_IO_DATA15,
#endif
            },
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
     * Take `ST7701` as an example, the following is the actual code after macro expansion:
     *      LCD_ST7701(bus_config, lcd_config);
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
    Serial.println("Initializing backlight and turn it off");
    BacklightPWM_LEDC *backlight = new BacklightPWM_LEDC(EXAMPLE_LCD_BL_IO, EXAMPLE_LCD_BL_ON_LEVEL);
    backlight->begin();
    backlight->off();
#endif

#if EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"3-wire SPI + RGB\" LCD with config");
    auto lcd = create_lcd_with_config();
#else
    Serial.println("Initializing \"3-wire SPI + RGB\" LCD without config");
    auto lcd = create_lcd_without_config();
#endif

    /* Configure bus and lcd before startup */
    // Configure bounce buffer to avoid screen drift
    auto bus = static_cast<BusRGB *>(lcd->getBus());
    bus->configRGB_BounceBufferSize(EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE); // Set bounce buffer to avoid screen drift
#if EXAMPLE_LCD_USE_EXTERNAL_CMD
    // Configure external LCD initialization commands
    lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd)/sizeof(lcd_init_cmd[0]));
#endif
    // lcd->configEnableIO_Multiplex(true); // If the "3-wire SPI" interface are sharing pins of the "RGB" interface to
                                            // save GPIOs, please enable this function to release the bus object and pins
                                            // (except CS signal). And then, the "3-wire SPI" interface cannot be used to
                                            // transmit commands any more.
    // lcd->configMirrorByCommand(true);    // This function is conflict with `configAutoReleaseBus(true)`, please don't
                                            // enable them at the same time
#if EXAMPLE_LCD_ENABLE_PRINT_FPS
    // Attach a callback function which will be called when the Vsync signal is detected
    lcd->attachRefreshFinishCallback(onLCD_RefreshFinishCallback);
#endif
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
    // Attach a callback function which will be called when every bitmap drawing is completed
    lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif

    /* Startup the LCD and operate it */
    assert(lcd->begin());
    if (lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) {
        lcd->setDisplayOnOff(true);
    }

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

/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: ESP32_S3_LCD_EV_BOARD_2_V1_5, ST7262' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following RGB (without 3-wire SPI) LCDs:
 *      - EK9716B
 *      - ST7262
 */
#define EXAMPLE_LCD_NAME                    ST7262
#define EXAMPLE_LCD_WIDTH                   (800)
#define EXAMPLE_LCD_HEIGHT                  (480)
                                                    // | 8-bit RGB888 | 16-bit RGB565 |
#define EXAMPLE_LCD_COLOR_BITS              (24)    // |      24      |   16/18/24    |
#define EXAMPLE_LCD_RGB_DATA_WIDTH          (16)    // |      8       |      16       |
#define EXAMPLE_LCD_RGB_COLOR_BITS          (16)    // |      24      |      16       |
#define EXAMPLE_LCD_RGB_TIMING_FREQ_HZ      (16 * 1000 * 1000)
#define EXAMPLE_LCD_RGB_TIMING_HPW          (40)
#define EXAMPLE_LCD_RGB_TIMING_HBP          (40)
#define EXAMPLE_LCD_RGB_TIMING_HFP          (48)
#define EXAMPLE_LCD_RGB_TIMING_VPW          (23)
#define EXAMPLE_LCD_RGB_TIMING_VBP          (32)
#define EXAMPLE_LCD_RGB_TIMING_VFP          (13)
#define EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE  (EXAMPLE_LCD_WIDTH * 10)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_RGB_IO_DISP         (-1)
#define EXAMPLE_LCD_RGB_IO_VSYNC        (3)
#define EXAMPLE_LCD_RGB_IO_HSYNC        (46)
#define EXAMPLE_LCD_RGB_IO_DE           (17)
#define EXAMPLE_LCD_RGB_IO_PCLK         (9)
                                                // The following sheet shows the mapping of ESP GPIOs to
                                                // LCD data pins with different data width and color format:
                                                // ┏------┳- ------------┳--------------------------┓
                                                // | ESP: | 8-bit RGB888 |      16-bit RGB565       |
                                                // |------|--------------|--------------------------|
                                                // | LCD: |    RGB888    | RGB565 | RGB666 | RGB888 |
                                                // ┗------|--------------|--------|--------|--------|
#define EXAMPLE_LCD_RGB_IO_DATA0        (10)    //        |      D0      |   B0   |  B0-1  |   B0-3 |
#define EXAMPLE_LCD_RGB_IO_DATA1        (11)    //        |      D1      |   B1   |  B2    |   B4   |
#define EXAMPLE_LCD_RGB_IO_DATA2        (12)    //        |      D2      |   B2   |  B3    |   B5   |
#define EXAMPLE_LCD_RGB_IO_DATA3        (13)    //        |      D3      |   B3   |  B4    |   B6   |
#define EXAMPLE_LCD_RGB_IO_DATA4        (14)    //        |      D4      |   B4   |  B5    |   B7   |
#define EXAMPLE_LCD_RGB_IO_DATA5        (21)    //        |      D5      |   G0   |  G0    |   G0-2 |
#define EXAMPLE_LCD_RGB_IO_DATA6        (8)     //        |      D6      |   G1   |  G1    |   G3   |
#define EXAMPLE_LCD_RGB_IO_DATA7        (18)    //        |      D7      |   G2   |  G2    |   G4   |
#if EXAMPLE_LCD_RGB_DATA_WIDTH > 8              //        ┗--------------┫--------|--------|--------|
#define EXAMPLE_LCD_RGB_IO_DATA8        (45)    //                       |   G3   |  G3    |   G5   |
#define EXAMPLE_LCD_RGB_IO_DATA9        (38)    //                       |   G4   |  G4    |   G6   |
#define EXAMPLE_LCD_RGB_IO_DATA10       (39)    //                       |   G5   |  G5    |   G7   |
#define EXAMPLE_LCD_RGB_IO_DATA11       (40)    //                       |   R0   |  R0-1  |   R0-3 |
#define EXAMPLE_LCD_RGB_IO_DATA12       (41)    //                       |   R1   |  R2    |   R4   |
#define EXAMPLE_LCD_RGB_IO_DATA13       (42)    //                       |   R2   |  R3    |   R5   |
#define EXAMPLE_LCD_RGB_IO_DATA14       (2)     //                       |   R3   |  R4    |   R6   |
#define EXAMPLE_LCD_RGB_IO_DATA15       (1)     //                       |   R4   |  R5    |   R7   |
                                                //                       ┗--------┻--------┻--------┛
#endif
#define EXAMPLE_LCD_RST_IO              (-1)    // Set to -1 if not used
#define EXAMPLE_LCD_BL_IO               (-1)    // Set to -1 if not used
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
     * Take `ST7262` as an example, the following is the actual code after macro expansion:
     *      LCD_ST7262(bus, 24, -1);
     */
    return new EXAMPLE_LCD_CLASS(
        EXAMPLE_LCD_NAME, bus, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COLOR_BITS, EXAMPLE_LCD_RST_IO
    );
}

static LCD *create_lcd_with_config(void)
{
    BusRGB::Config bus_config = {
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
        },
    };

    /**
     * Take `ST7262` as an example, the following is the actual code after macro expansion:
     *      LCD_ST7262(bus_config, lcd_config);
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

    delay(3000);

#if EXAMPLE_LCD_BL_IO >= 0
    Serial.println("Initializing backlight and turn it off");
    BacklightPWM_LEDC *backlight = new BacklightPWM_LEDC(EXAMPLE_LCD_BL_IO, EXAMPLE_LCD_BL_ON_LEVEL);
    backlight->begin();
    backlight->off();
#endif

#if EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"RGB\" LCD with config");
    auto lcd = create_lcd_with_config();
#else
    Serial.println("Initializing \"RGB\" LCD without config");
    auto lcd = create_lcd_without_config();
#endif

    // Configure bounce buffer to avoid screen drift
    auto bus = static_cast<BusRGB *>(lcd->getBus());
    bus->configRGB_BounceBufferSize(EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE); // Set bounce buffer to avoid screen drift

    lcd->init();
#if EXAMPLE_LCD_ENABLE_PRINT_FPS
    // Attach a callback function which will be called when the Vsync signal is detected
    lcd->attachRefreshFinishCallback(onLCD_RefreshFinishCallback);
#endif
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
    // Attach a callback function which will be called when every bitmap drawing is completed
    lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif
    lcd->reset();
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

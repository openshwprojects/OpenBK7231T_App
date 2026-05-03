/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <vector>
#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: ESP32_S3_BOX_3, GT911' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your touch spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following I2C touch devices:
 *      - AXS15231B
 *      - CHSC6540
 *      - CST816S
 *      - FT5x06
 *      - GT911, GT1151
 *      - TT21100
 *      - ST1633, ST7123
 */
#define EXAMPLE_TOUCH_NAME              GT911
#define EXAMPLE_TOUCH_ADDRESS           (0)     // Typically set to 0 to use the default address.
                                                // - For touchs with only one address, set to 0
                                                // - For touchs with multiple addresses, set to 0 or the address
                                                //   Like GT911, there are two addresses: 0x5D(default) and 0x14
#define EXAMPLE_TOUCH_WIDTH             (320)
#define EXAMPLE_TOUCH_HEIGHT            (240)
#define EXAMPLE_TOUCH_I2C_FREQ_HZ       (400 * 1000)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_TOUCH_I2C_IO_SCL        (18)
#define EXAMPLE_TOUCH_I2C_IO_SDA        (8)
#define EXAMPLE_TOUCH_I2C_SCL_PULLUP    (1)  // 0/1
#define EXAMPLE_TOUCH_I2C_SDA_PULLUP    (1)  // 0/1
#define EXAMPLE_TOUCH_RST_IO            (48) // Set to `-1` if not used
                                             // For GT911, the RST pin is also used to configure the I2C address
#define EXAMPLE_TOUCH_RST_ACTIVE_LEVEL  (1)  // Set to `0` if reset is active low
#define EXAMPLE_TOUCH_INT_IO            (3)  // Set to `-1` if not used
                                             // For GT911, the INT pin is also used to configure the I2C address

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please update the following configuration according to your test ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_TOUCH_ENABLE_CREATE_WITH_CONFIG (0)
#define EXAMPLE_TOUCH_ENABLE_INTERRUPT_CALLBACK (1)
#define EXAMPLE_TOUCH_READ_PERIOD_MS            (30)

#define _EXAMPLE_TOUCH_CLASS(name, ...) Touch##name(__VA_ARGS__)
#define EXAMPLE_TOUCH_CLASS(name, ...)  _EXAMPLE_TOUCH_CLASS(name, ##__VA_ARGS__)

#if EXAMPLE_TOUCH_ENABLE_INTERRUPT_CALLBACK
IRAM_ATTR static bool onTouchInterruptCallback(void *user_data)
{
    esp_rom_printf("Touch interrupt callback\n");

    return false;
}
#endif

Touch *touch = nullptr;

static Touch *create_touch_without_config(void)
{
    BusI2C *bus = new BusI2C(
        EXAMPLE_TOUCH_I2C_IO_SCL, EXAMPLE_TOUCH_I2C_IO_SDA,
#if EXAMPLE_TOUCH_ADDRESS == 0
        (BusI2C::ControlPanelFullConfig)ESP_PANEL_TOUCH_I2C_CONTROL_PANEL_CONFIG(EXAMPLE_TOUCH_NAME)
#else
        (BusI2C::ControlPanelFullConfig)ESP_PANEL_TOUCH_I2C_CONTROL_PANEL_CONFIG_WITH_ADDR(EXAMPLE_TOUCH_NAME, EXAMPLE_TOUCH_ADDRESS)
#endif
    );

    /**
     * Take GT911 as an example, the following is the actual code after macro expansion:
     *      TouchGT911(bus, 320, 240, 13, 14);
     */
    return new EXAMPLE_TOUCH_CLASS(
        EXAMPLE_TOUCH_NAME, bus, EXAMPLE_TOUCH_WIDTH, EXAMPLE_TOUCH_HEIGHT,
        EXAMPLE_TOUCH_RST_IO, EXAMPLE_TOUCH_INT_IO
    );
}

static Touch *create_touch_with_config(void)
{
    BusI2C::Config bus_config = {
        .host = BusI2C::HostPartialConfig{
            .sda_io_num = EXAMPLE_TOUCH_I2C_IO_SDA,
            .scl_io_num = EXAMPLE_TOUCH_I2C_IO_SCL,
            .sda_pullup_en = EXAMPLE_TOUCH_I2C_SDA_PULLUP,
            .scl_pullup_en = EXAMPLE_TOUCH_I2C_SCL_PULLUP,
            .clk_speed = EXAMPLE_TOUCH_I2C_FREQ_HZ,
        },
        .control_panel = (BusI2C::ControlPanelFullConfig)
#if EXAMPLE_TOUCH_ADDRESS == 0
            ESP_PANEL_TOUCH_I2C_CONTROL_PANEL_CONFIG(EXAMPLE_TOUCH_NAME),
#else
            ESP_PANEL_TOUCH_I2C_CONTROL_PANEL_CONFIG_WITH_ADDR(EXAMPLE_TOUCH_NAME, EXAMPLE_TOUCH_ADDRESS),
#endif
    };
    Touch::Config touch_config = {
        .device = Touch::DevicePartialConfig{
            .x_max = EXAMPLE_TOUCH_WIDTH,
            .y_max = EXAMPLE_TOUCH_HEIGHT,
            .rst_gpio_num = EXAMPLE_TOUCH_RST_IO,
            .int_gpio_num = EXAMPLE_TOUCH_INT_IO,
            .levels_reset = EXAMPLE_TOUCH_RST_ACTIVE_LEVEL,
        },
    };

    /**
     * Take GT911 as an example, the following is the actual code after macro expansion:
     *      TouchGT911(bus_config, touch_config);
     */
    return new EXAMPLE_TOUCH_CLASS(EXAMPLE_TOUCH_NAME, bus_config, touch_config);
}

void setup()
{
    Serial.begin(115200);

#if EXAMPLE_TOUCH_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"I2C\" touch with config");
    touch = create_touch_with_config();
#else
    Serial.println("Initializing \"I2C\" touch without config");
    touch = create_touch_without_config();
#endif

#if !EXAMPLE_TOUCH_ENABLE_CREATE_WITH_CONFIG
    /* Configure bus and touch before startup */
    auto bus = static_cast<BusI2C *>(touch->getBus());
    bus->configI2C_FreqHz(EXAMPLE_TOUCH_I2C_FREQ_HZ);
    bus->configI2C_PullupEnable(EXAMPLE_TOUCH_I2C_SDA_PULLUP, EXAMPLE_TOUCH_I2C_SCL_PULLUP);
    touch->configResetActiveLevel(EXAMPLE_TOUCH_RST_ACTIVE_LEVEL);
#endif

    /* Startup the LCD and operate it */
    assert(touch->begin());
#if EXAMPLE_TOUCH_ENABLE_INTERRUPT_CALLBACK
    if (touch->isInterruptEnabled()) {
        touch->attachInterruptCallback(onTouchInterruptCallback);
    }
#endif

    Serial.println("Reading touch points and buttons...");
}

void loop()
{
    // Read all touch points and buttons
    touch->readRawData(-1, -1, EXAMPLE_TOUCH_READ_PERIOD_MS);

    std::vector<TouchPoint> points;
    int i = 0;
    touch->getPoints(points);
    for (auto &point : points) {
        Serial.printf("Touch point(%d): x %d, y %d, strength %d\n", i++, point.x, point.y, point.strength);
    }

    std::vector<TouchButton> buttons;
    i = 0;
    touch->getButtons(buttons);
    for (auto &button : buttons) {
        Serial.printf("Touch button(%d): %d\n", i++, button.second);
    }

    if (!touch->isInterruptEnabled()) {
        delay(EXAMPLE_TOUCH_READ_PERIOD_MS);
    }
}

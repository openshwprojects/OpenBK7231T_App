/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <vector>
#include <Arduino.h>
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: Custom, XPT2046' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your touch spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Currently, the library supports the following SPI touch devices:
 *      - STMPE610
 *      - XPT2046
 */
#define EXAMPLE_TOUCH_NAME              XPT2046
#define EXAMPLE_TOUCH_WIDTH             (240)
#define EXAMPLE_TOUCH_HEIGHT            (320)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_TOUCH_SPI_IO_CS         (46)
#define EXAMPLE_TOUCH_SPI_IO_SCK        (10)
#define EXAMPLE_TOUCH_SPI_IO_MOSI       (14)
#define EXAMPLE_TOUCH_SPI_IO_MISO       (8)
#define EXAMPLE_TOUCH_RST_IO            (-1)
#define EXAMPLE_TOUCH_INT_IO            (-1)

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

static Touch *create_touch_without_config(void)
{
    BusSPI *bus = new BusSPI(
        EXAMPLE_TOUCH_SPI_IO_SCK, EXAMPLE_TOUCH_SPI_IO_MOSI, EXAMPLE_TOUCH_SPI_IO_MISO,
        (BusSPI::ControlPanelFullConfig)ESP_PANEL_TOUCH_SPI_CONTROL_PANEL_CONFIG(
            EXAMPLE_TOUCH_NAME, EXAMPLE_TOUCH_SPI_IO_CS
        )
    );

    /**
     * Take XPT2046 as an example, the following is the actual code after macro expansion:
     *      TouchXPT2046(bus, 240, 320, -1, -1);
     */
    return new EXAMPLE_TOUCH_CLASS(
        EXAMPLE_TOUCH_NAME, bus, EXAMPLE_TOUCH_WIDTH, EXAMPLE_TOUCH_HEIGHT,
        EXAMPLE_TOUCH_RST_IO, EXAMPLE_TOUCH_INT_IO
    );
}

static Touch *create_touch_with_config(void)
{
    BusSPI::Config bus_config = {
        .host = BusSPI::HostPartialConfig{
            .mosi_io_num = EXAMPLE_TOUCH_SPI_IO_MOSI,
            .miso_io_num = EXAMPLE_TOUCH_SPI_IO_MISO,
            .sclk_io_num = EXAMPLE_TOUCH_SPI_IO_SCK,
        },
        .control_panel = (BusSPI::ControlPanelFullConfig)ESP_PANEL_TOUCH_SPI_CONTROL_PANEL_CONFIG(
            EXAMPLE_TOUCH_NAME, EXAMPLE_TOUCH_SPI_IO_CS
        )
    };
    Touch::Config touch_config = {
        .device = Touch::DevicePartialConfig{
            .x_max = EXAMPLE_TOUCH_WIDTH,
            .y_max = EXAMPLE_TOUCH_HEIGHT,
            .rst_gpio_num = EXAMPLE_TOUCH_RST_IO,
            .int_gpio_num = EXAMPLE_TOUCH_INT_IO,
        },
    };

    /**
     * Take XPT2046 as an example, the following is the actual code after macro expansion:
     *      TouchXPT2046(bus_config, touch_config);
     */
    return new EXAMPLE_TOUCH_CLASS(EXAMPLE_TOUCH_NAME, bus_config, touch_config);
}

Touch *touch = nullptr;

void setup()
{
    Serial.begin(115200);

#if EXAMPLE_TOUCH_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"SPI\" touch with config");
    touch = create_touch_with_config();
#else
    Serial.println("Initializing \"SPI\" touch without config");
    touch = create_touch_without_config();
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

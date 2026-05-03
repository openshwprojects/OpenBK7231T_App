/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */

#include <vector>
#include <Arduino.h>
#include <esp_display_panel.hpp>
#include "board_external_config.hpp"

using namespace esp_panel::board;
using namespace esp_panel::drivers;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please update the following configuration according to your test ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_ENABLE_PRINT_FPS            (1)
#define EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK (1)
#define EXAMPLE_TOUCH_ENABLE_INTERRUPT_CALLBACK (1)
#define EXAMPLE_TOUCH_READ_PERIOD_MS            (30)

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

#if EXAMPLE_TOUCH_ENABLE_INTERRUPT_CALLBACK
IRAM_ATTR static bool onTouchInterruptCallback(void *user_data)
{
    esp_rom_printf("Touch interrupt callback\n");

    return false;
}
#endif

Board *board = nullptr;

void setup()
{
    Serial.begin(115200);

    Serial.println("Initializing board with external config");
    // Dynamically load the external board configuration
    board = new Board(BOARD_EXTERNAL_CONFIG);
    assert(board->begin());

    auto backlight = board->getBacklight();
    if (backlight != nullptr) {
        Serial.println("Turn off the backlight");
        backlight->off();
    } else {
        Serial.println("Backlight is not available");
    }

    Serial.println("Testing LCD");
    auto lcd = board->getLCD();
    if (lcd != nullptr) {
#if EXAMPLE_LCD_ENABLE_PRINT_FPS
        // Only RGB and MIPI-DSI bus types support refresh finish callback
        auto bus_type = lcd->getBus()->getBasicAttributes().type;
        if ((bus_type == ESP_PANEL_BUS_TYPE_RGB) || (bus_type == ESP_PANEL_BUS_TYPE_MIPI_DSI)) {
            lcd->attachRefreshFinishCallback(onLCD_RefreshFinishCallback);
        }
#endif
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
        lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif
        Serial.println("Draw color bar from top to bottom, the order is B - G - R");
        lcd->colorBarTest();
    } else {
        Serial.println("LCD is not available");
    }

    if (backlight != nullptr) {
        Serial.println("Turn on the backlight");
        backlight->on();
    }

    Serial.println("Testing Touch");
    auto touch = board->getTouch();
    if (touch != nullptr) {
#if EXAMPLE_TOUCH_ENABLE_INTERRUPT_CALLBACK
        if (touch->isInterruptEnabled()) {
            touch->attachInterruptCallback(onTouchInterruptCallback, nullptr);
        }
#endif
        Serial.println("Reading touch points...");
    } else {
        Serial.println("Touch is not available");
    }
}

void loop()
{
    auto touch = board->getTouch();
    if (touch != nullptr) {
        // Read all touch points and buttons
        touch->readRawData(-1, -1, EXAMPLE_TOUCH_READ_PERIOD_MS);

        std::vector<TouchPoint> points;
        int i = 0;
        touch->getPoints(points);
        for (auto &point : points) {
            Serial.printf("Touch point(%d): x %d, y %d, strength %d\n", i++, point.x, point.y, point.strength);
        }

        // std::vector<TouchButton> buttons;
        // i = 0;
        // touch->getButtons(buttons);
        // for (auto &button : buttons) {
        //     Serial.printf("Touch button(%d): %d\n", i++, button.second);
        // }

        if (!touch->isInterruptEnabled()) {
            delay(EXAMPLE_TOUCH_READ_PERIOD_MS);
        }
    } else {
        Serial.println("IDLE loop");
        delay(1000);
    }
}

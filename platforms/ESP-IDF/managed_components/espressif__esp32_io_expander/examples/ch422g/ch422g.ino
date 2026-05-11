/**
 * | Supported IO Expanders    | CH422G |
 * | ------------------------- | ------ |
 *
 * # CH422G Test Example
 *
 * The hardware device used in this example is waveshare ESP32-S3-Touch-LCD-4.3B-BOX. To test the simultaneous use of I/O input and OC output, connect DO0 to DI0, and connect DO1 to DI1.
 *
 * ## How to use
 *
 * 1. Enable USB CDC.
 * 2. Verify and upload the example to your board.
 *
 * ## Serial Output
 *
 * ```
 * ...
 * Test begin
 * Set the OC pin to push-pull output mode.
 * Set the IO0-7 pin to input mode.
 * Set pint 8 and 9 to:0, 1
 *
 * Read pin 0 and 5 level: 0, 1
 *
 * Set pint 8 and 9 to:1, 0
 *
 * Read pin 0 and 5 level: 1, 0
 * ...
 * ```
 *
 * ## Troubleshooting
 *
 * The driver initialization by default sets CH422G's IO0-7 to output high-level mode.
  Since the input/output mode of CH422G's IO0-7 must remain consistent, the driver will only set IO0-7 to
  input mode when it determines that all pins are configured as input.
  Using pinMode and multiPinMode will be invalid. You can only set the pin working mode through enableAllIO_Input, enableAllIO_Output, enableOC_PushPull and enableOC_OpenDrain
 */

#include <Arduino.h>
#include <esp_io_expander.hpp>

#define EXAMPLE_I2C_SDA_PIN (8)
#define EXAMPLE_I2C_SCL_PIN (9)
#define EXAMPLE_I2C_ADDR    (ESP_IO_EXPANDER_I2C_CH422G_ADDRESS)

esp_expander::CH422G *expander = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Test begin");

  expander = new esp_expander::CH422G(EXAMPLE_I2C_SCL_PIN, EXAMPLE_I2C_SDA_PIN, EXAMPLE_I2C_ADDR);
  expander->init();
  expander->begin();

  Serial.println("Set the OC pin to push-pull output mode.");
  expander->enableOC_PushPull();

  // Serial.println("Set the OC pin to open_drain output mode.");
  // expander->enableOC_OpenDrain();

  Serial.println("Set the IO0-7 pin to input mode.");
  expander->enableAllIO_Input();

  // Serial.println("Set the IO0-7 pin to output mode.");
  // expander->enableAllIO_Output();
}

int level[2] = { 0, 0 };

void loop() {
  for (int i = 0; i < 100; i++) {
    bool toggle = i % 2;

    Serial.print("Set pint 8 and 9 to:");
    Serial.print(toggle);
    Serial.print(", ");
    Serial.println(!toggle);
    Serial.println();

    // Set pin 8 and 9 level
    expander->digitalWrite(8, toggle);
    expander->digitalWrite(9, !toggle);
    delay(1);

    // Read pin 0 and 5 level
    level[0] = expander->digitalRead(0);
    level[1] = expander->digitalRead(5);

    Serial.print("Read pin 0 and 5 level: ");
    Serial.print(level[0]);
    Serial.print(", ");
    Serial.println(level[1]);
    Serial.println();

    delay(1000);
  }
}

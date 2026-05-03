#include <Arduino.h>
#include <esp_io_expander.hpp>

/* The following default configurations are for the board 'Espressif: ESP32_S3_LCD_EV_BOARD_V1_5, TCA9554' */
/**
 * Choose one of the following chip names:
 *      - TCA95XX_8BIT
 *      - TCA95XX_16BIT
 *      - HT8574
 *      - CH422G
 */
#define EXAMPLE_CHIP_NAME       TCA95XX_8BIT
#define EXAMPLE_I2C_SDA_PIN     (47)
#define EXAMPLE_I2C_SCL_PIN     (48)
#define EXAMPLE_I2C_ADDR        (ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000)   // Change this value according to the
                                                                            // hardware address

#define _EXAMPLE_CHIP_CLASS(name, ...)   esp_expander::name(__VA_ARGS__)
#define EXAMPLE_CHIP_CLASS(name, ...)    _EXAMPLE_CHIP_CLASS(name, ##__VA_ARGS__)

esp_expander::Base *expander = nullptr;

void setup()
{
  Serial.begin(115200);
  Serial.println("Test begin");

  /**
   * Taking `TCA95XX_8BIT` as an example, the following is the code after macro expansion:
   *      expander = new esp_expander::TCA95XX_8BIT((48), (47), (0x20))
   */
  expander = new EXAMPLE_CHIP_CLASS(
      EXAMPLE_CHIP_NAME, EXAMPLE_I2C_SCL_PIN, EXAMPLE_I2C_SDA_PIN, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000
  );
  expander->init();
  expander->begin();

  /* For CH422G */
  // static_cast<esp_expander::CH422G *>(expander)->enableOC_OpenDrain();
  // static_cast<esp_expander::CH422G *>(expander)->enableOC_PushPull();
  // static_cast<esp_expander::CH422G *>(expander)->enableAllIO_Input();
  // static_cast<esp_expander::CH422G *>(expander)->enableAllIO_Output();

  Serial.println("Original status:");
  expander->printStatus();

  expander->pinMode(0, OUTPUT);
  expander->pinMode(1, OUTPUT);
  expander->multiPinMode(IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_3, OUTPUT);

  Serial.println("Set pint 0-3 to output mode:");
  expander->printStatus();

  expander->digitalWrite(0, LOW);
  expander->digitalWrite(1, LOW);
  expander->multiDigitalWrite(IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_3, LOW);

  Serial.println("Set pint 0-3 to low level:");
  expander->printStatus();

  expander->pinMode(0, INPUT);
  expander->pinMode(1, INPUT);
  expander->multiPinMode(IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_3, INPUT);

  Serial.println("Set pint 0-3 to input mode:");
  expander->printStatus();
}

int level[4] = {0, 0, 0, 0};
uint32_t level_temp;

void loop()
{
  // Read pin 0-3 level
  level[0] = expander->digitalRead(0);
  level[1] = expander->digitalRead(1);
  level_temp = expander->multiDigitalRead(IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_3);
  level[2] = level_temp & IO_EXPANDER_PIN_NUM_2 ? HIGH : LOW;
  level[3] = level_temp & IO_EXPANDER_PIN_NUM_3 ? HIGH : LOW;
  Serial.printf("Pin level: %d, %d, %d, %d\n", level[0], level[1], level[2], level[3]);

  delay(1000);
}

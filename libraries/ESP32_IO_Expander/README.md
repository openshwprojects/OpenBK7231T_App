[![Arduino Lint](https://github.com/esp-arduino-libs/ESP32_IO_Expander/actions/workflows/arduino_lint.yml/badge.svg)](https://github.com/esp-arduino-libs/ESP32_IO_Expander/actions/workflows/arduino_lint.yml) [![pre-commit](https://github.com/esp-arduino-libs/ESP32_IO_Expander/actions/workflows/pre-commit.yml/badge.svg)](https://github.com/esp-arduino-libs/ESP32_IO_Expander/actions/workflows/pre-commit.yml) [![Build Test Apps](https://github.com/esp-arduino-libs/ESP32_IO_Expander/actions/workflows/build_test.yml/badge.svg)](https://github.com/esp-arduino-libs/ESP32_IO_Expander/actions/workflows/build_test.yml)

**Latest Arduino Library Version**: [![GitHub Release](https://img.shields.io/github/v/release/esp-arduino-libs/ESP32_IO_Expander)](https://github.com/esp-arduino-libs/ESP32_IO_Expander/releases)

**Latest Espressif Component Version**: [![Espressif Release](https://components.espressif.com/components/espressif/esp32_io_expander/badge.svg)](https://components.espressif.com/components/espressif/esp32_io_expander)

# ESP32_IO_Expander

## Overview

`ESP32_IO_Expander` is a library designed for driving [IO expander chips](#supported-drivers) using ESP SoCs. It encapsulates various components from the [Espressif Components Registry](https://components.espressif.com/) and includes the following features:

* Supports various IO expander chips, such as TCA95xx, HT8574, and CH422G.
* Supports controlling individual IO pin with functions like `pinMode()`, `digitalWrite()`, and `digitalRead()`.
* Supports controlling multiple IO pins simultaneously with functions like `multiPinMode()`, `multiDigitalWrite()`, and `multiDigitalRead()`.
* Compatible with the `Arduino`, `ESP-IDF` and `MicroPython` for compilation.

## Table of Contents

- [ESP32\_IO\_Expander](#esp32_io_expander)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [Supported Drivers](#supported-drivers)
  - [How to Use](#how-to-use)
    - [ESP-IDF Framework](#esp-idf-framework)
      - [Dependencies and Versions](#dependencies-and-versions)
      - [Adding to Project](#adding-to-project)
      - [Configuration Instructions](#configuration-instructions)
    - [Arduino IDE](#arduino-ide)
      - [Dependencies and Versions](#dependencies-and-versions-1)
      - [Installing the Library](#installing-the-library)
      - [Configuration Instructions](#configuration-instructions-1)
    - [Examples](#examples)
    - [Detailed Usage](#detailed-usage)
  - [FAQ](#faq)
    - [Where is the directory for Arduino libraries?](#where-is-the-directory-for-arduino-libraries)
    - [How to Install ESP32\_IO\_Expander in Arduino IDE?](#how-to-install-esp32_io_expander-in-arduino-ide)

## Supported Drivers

|                                              **Driver**                                              | **Version** |
| ---------------------------------------------------------------------------------------------------- | ----------- |
| [esp_io_expander](https://components.espressif.com/components/espressif/esp_io_expander)             | 1.0.1       |
| [TCA95XX_8BIT](https://components.espressif.com/components/espressif/esp_io_expander_tca9554)        | 1.0.1       |
| [TCA95XX_16BIT](https://components.espressif.com/components/espressif/esp_io_expander_tca95xx_16bit) | 1.0.0       |
| [HT8574](https://components.espressif.com/components/espressif/esp_io_expander_ht8574)               | 1.0.0       |
| CH422G                                                                                               | x           |

## How to Use

### ESP-IDF Framework

#### Dependencies and Versions

|                           **Dependency**                           |     **Version**      |
| ------------------------------------------------------------------ | -------------------- |
| [esp-idf](https://github.com/espressif/esp-idf)                    | >= 5.1               |
| [esp-lib-utils](https://github.com/esp-arduino-libs/esp-lib-utils) | >= 0.1.0 && <= 0.2.0 |

#### Adding to Project

`ESP32_IO_Expander` has been uploaded to the [Espressif Component Registry](https://components.espressif.com/), and users can add it to their project using the `idf.py add-dependency` command, for example:

```bash
idf.py add-dependency "espressif/ESP32_IO_Expander"
```

Alternatively, users can create or modify the *idf_component.yml* file in the project directory. For more details, please refer to the [Espressif Documentation - IDF Component Manager](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

#### Configuration Instructions

Since `ESP32_IO_Expander` depends on the `esp-lib-utils` library which implements the `logging`, `checking`, and `memory` functions, to configure it when using ESP-IDF, please refer to the [instructions](https://github.com/esp-arduino-libs/esp-lib-utils#configuration-instructions).

### Arduino IDE

#### Dependencies and Versions

|                           **Dependency**                           |     **Version**      |
| ------------------------------------------------------------------ | -------------------- |
| [arduino-esp32](https://github.com/espressif/arduino-esp32)        | >= v3.0.0            |
| [esp-lib-utils](https://github.com/esp-arduino-libs/esp-lib-utils) | >= 0.1.0 && <= 0.2.0 |

#### Installing the Library

For installation of the `ESP32_IO_Expander` library, refer to [How to Install ESP32_IO_Expander in Arduino IDE](#how-to-install-ESP32_IO_Expander-in-arduino-ide).

#### Configuration Instructions

Since `ESP32_IO_Expander` depends on the `esp-lib-utils` library which implements the `logging`, `checking`, and `memory` functions, to configure it when using Arduino, please refer to the [instructions](https://github.com/esp-arduino-libs/esp-lib-utils#configuration-instructions-1).

### Examples

* [General](examples/general): Demonstrates how to use `ESP32_IO_Expander` and test general functions.
* [CH422G](examples/ch422g): Demonstrates how to use `ESP32_IO_Expander` specifically with the CH422G chip.

### Detailed Usage

```cpp
#include <esp_io_expander.hpp>

// Create and initialize the IO expander chip, such as TCA95XX_8BIT
esp_expander::Base *expander = new esp_expander::TCA95XX_8BIT(
    EXAMPLE_I2C_SCL_PIN, EXAMPLE_I2C_SDA_PIN, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000
);
expander->init();
expander->begin();

// Control a single pin (0-31)
expander->pinMode(0, OUTPUT);
expander->digitalWrite(0, HIGH);
expander->digitalWrite(0, LOW);
expander->pinMode(0, INPUT);
int level = expander->digitalRead(0);

// Control multiple pins (IO_EXPANDER_PIN_NUM_0 - IO_EXPANDER_PIN_NUM_31)
expander->multiPinMode(IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, OUTPUT);
expander->multiDigitalWrite(IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, HIGH);
expander->multiDigitalWrite(IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, LOW);
expander->multiPinMode(IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, INPUT);
uint32_t level = expander->multiDigitalRead(IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_3);

// Release the Base object
delete expander;
```

## FAQ

### Where is the directory for Arduino libraries?

Users can find and modify the directory path for Arduino libraries by selecting `File` > `Preferences` > `Settings` > `Sketchbook location` from the menu bar in the Arduino IDE.

### How to Install ESP32_IO_Expander in Arduino IDE?

- **If users want to install online**, navigate to `Sketch` > `Include Library` > `Manage Libraries...` in the Arduino IDE, then search for `ESP32_IO_Expander` and click the `Install` button to install it.
- **If users want to install manually**, download the required version of the `.zip` file from [ESP32_IO_Expander](https://github.com/esp-arduino-libs/ESP32_IO_Expander), then navigate to `Sketch` > `Include Library` > `Add .ZIP Library...` in the Arduino IDE, select the downloaded `.zip` file, and click `Open` to install it.
- Users can also refer to the guides on library installation in the [Arduino IDE v1.x.x](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries) or [Arduino IDE v2.x.x](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library) documentation.

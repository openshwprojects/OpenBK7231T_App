#include "OneWire.h"
#include "../hal/hal_pins.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// Implement delayMicroseconds function
void delayMicroseconds(unsigned int us) {
#ifdef _WIN32
    Sleep(us / 1000);  // Sleep function for Windows
#else
    usleep(us);  // usleep function for Unix-like systems
#endif
}

OneWire::OneWire(uint8_t pin) {
    this->pin = pin;
    HAL_PIN_Setup_Input(pin);
    HAL_PIN_SetOutputValue(pin, 0);
    reset_search();
}

uint8_t OneWire::reset(void) {
    HAL_PIN_SetOutputValue(pin, 0);
    delayMicroseconds(480);
    HAL_PIN_Setup_Input(pin);
    delayMicroseconds(70);
    uint8_t presence = HAL_PIN_ReadDigitalInput(pin);
    delayMicroseconds(410);
    return presence;
}

void OneWire::write(uint8_t v, uint8_t power) {
    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        if (v & bitMask) {
            HAL_PIN_SetOutputValue(pin, 0);
            delayMicroseconds(10);
            HAL_PIN_SetOutputValue(pin, 1);
            delayMicroseconds(55);
        } else {
            HAL_PIN_SetOutputValue(pin, 0);
            delayMicroseconds(65);
            HAL_PIN_SetOutputValue(pin, 1);
            delayMicroseconds(5);
        }
    }
    if (power) {
        HAL_PIN_Setup_Output(pin);
        HAL_PIN_SetOutputValue(pin, 1);
    }
}

uint8_t OneWire::read(void) {
    uint8_t result = 0;
    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        HAL_PIN_SetOutputValue(pin, 0);
        delayMicroseconds(3);
        HAL_PIN_Setup_Input(pin);
        delayMicroseconds(10);
        if (HAL_PIN_ReadDigitalInput(pin)) {
            result |= bitMask;
        }
        delayMicroseconds(53);
    }
    return result;
}

void OneWire::select(const uint8_t rom[8]) {
    write(0x55);
    for (int i = 0; i < 8; i++) {
        write(rom[i]);
    }
}

void OneWire::skip(void) {
    write(0xCC);
}

void OneWire::depower(void) {
    HAL_PIN_Setup_Input(pin);
}

void OneWire::reset_search(void) {
    // Implement reset search functionality
}

void OneWire::target_search(uint8_t family_code) {
    // Implement target search functionality
}

uint8_t OneWire::search(uint8_t *newAddr) {
    // Implement search functionality
    return 0;
}

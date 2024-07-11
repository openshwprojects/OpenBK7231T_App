#include "OneWire.h"
#include "hal_pins.h"  // Include your HAL header

OneWire::OneWire(uint8_t pin) {
    hal_set_pin_mode(pin, GPIO_MODE_INPUT);
    hal_write_pin(pin, LOW);
    reset_search();
}

uint8_t OneWire::reset(void) {
    hal_write_pin(pin, LOW);
    delayMicroseconds(480);
    hal_set_pin_mode(pin, INPUT);
    delayMicroseconds(70);
    uint8_t presence = hal_read_pin(pin);
    delayMicroseconds(410);
    return presence;
}

void OneWire::write(uint8_t v, uint8_t power) {
    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        if (v & bitMask) {
            hal_write_pin(pin, LOW);
            delayMicroseconds(10);
            hal_write_pin(pin, HIGH);
            delayMicroseconds(55);
        } else {
            hal_write_pin(pin, LOW);
            delayMicroseconds(65);
            hal_write_pin(pin, HIGH);
            delayMicroseconds(5);
        }
    }
    if (power) {
        hal_set_pin_mode(pin, GPIO_MODE_OUTPUT);
        hal_write_pin(pin, HIGH);
    }
}

uint8_t OneWire::read(void) {
    uint8_t result = 0;
    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        hal_write_pin(pin, LOW);
        delayMicroseconds(3);
        hal_set_pin_mode(pin, INPUT);
        delayMicroseconds(10);
        if (hal_read_pin(pin)) {
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
    hal_set_pin_mode(pin, GPIO_MODE_INPUT);
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

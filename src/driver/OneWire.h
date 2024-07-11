#ifndef OneWire_h
#define OneWire_h

#ifdef __cplusplus

#include <stdint.h>

// Include your HAL header for pin operations
#include "../hal/hal_pins.h"

// Define delay function
void delayMicroseconds(unsigned int us);

class OneWire {
  private:
    uint8_t pin;

  public:
    OneWire(uint8_t pin);
    uint8_t reset(void);
    void write(uint8_t v, uint8_t power = 0);
    uint8_t read(void);
    void select(const uint8_t rom[8]);
    void skip(void);
    void depower(void);
    void reset_search(void);
    void target_search(uint8_t family_code);
    uint8_t search(uint8_t *newAddr);
};

#endif // __cplusplus
#endif // OneWire_h

#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include "driver/gpio.h"
#if PLATFORM_ESP8266
#define GPIO_NUM_NC -1
#endif

typedef struct
{
	const char* name;
	gpio_num_t pin;
	bool isConfigured;
} espPinMapping_t;

extern espPinMapping_t g_pins[];
extern int g_numPins;

extern void ESP_ConfigurePin(gpio_num_t pin, gpio_mode_t mode, bool pup, bool pdown, gpio_int_type_t intr);

#endif // PLATFORM_ESPIDF

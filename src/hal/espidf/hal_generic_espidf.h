#ifdef PLATFORM_ESPIDF

#include "driver/gpio.h"

typedef struct espPinMapping_s
{
	const char* name;
	gpio_num_t pin;
	bool isConfigured;
} espPinMapping_t;

#endif // PLATFORM_ESPIDF

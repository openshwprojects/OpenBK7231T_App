#if PLATFORM_XR809

#include "../../../new_common.h"
#include "../../../logging/logging.h"

#include "../hal_pinmap_xradio.h"

xrpin_t g_pins[] = {
	{ "PA00",			GPIO_PORT_A, GPIO_PIN_0,	-1,					-1,				0U, -1, -1 },
	{ "PA01",			GPIO_PORT_A, GPIO_PIN_1,	-1,					-1,				0U, -1, -1 },
	{ "PA06",			GPIO_PORT_A, GPIO_PIN_6,	-1,					-1,				0U, -1, 2 },
	{ "PA07",			GPIO_PORT_A, GPIO_PIN_7,	-1,					-1,				0U, -1, 3 },
	{ "PA08 (ADC0)",	GPIO_PORT_A, GPIO_PIN_8,	ADC_CHANNEL_0,		-1,				0U, -1, -1 },
	{ "PA12 (ADC4)",	GPIO_PORT_A, GPIO_PIN_12,	ADC_CHANNEL_4,		-1,				0U, -1, -1 },
	{ "PA14 (ADC6)",	GPIO_PORT_A, GPIO_PIN_14,	ADC_CHANNEL_6,		-1,				0U, -1, -1 },
	{ "PA15",			GPIO_PORT_A, GPIO_PIN_15,	-1,					-1,				0U, -1, -1 },
	{ "PA16",			GPIO_PORT_A, GPIO_PIN_16,	-1,					-1,				0U, -1, -1 },
	{ "PA17 (TX1)",		GPIO_PORT_A, GPIO_PIN_17,	-1,					-1,				0U, -1, 4 },
	{ "PA18 (RX1)",		GPIO_PORT_A, GPIO_PIN_18,	-1,					-1,				0U, -1, 5 },
	{ "PA19",			GPIO_PORT_A, GPIO_PIN_19,	-1,					PWM_GROUP0_CH0,	4U, -1, 6 },
	{ "PA20",			GPIO_PORT_A, GPIO_PIN_20,	-1,					PWM_GROUP0_CH1,	4U, -1, 7 },
	{ "PA21",			GPIO_PORT_A, GPIO_PIN_21,	-1,					PWM_GROUP1_CH2,	4U, -1, 8 },
	{ "PA22",			GPIO_PORT_A, GPIO_PIN_22,	-1,					PWM_GROUP1_CH3,	4U, -1, 9 },
	{ "PB00 (TX0)",		GPIO_PORT_B, GPIO_PIN_0,	-1,					-1,				0U, -1, -1 },
	{ "PB01 (RX0)",		GPIO_PORT_B, GPIO_PIN_1,	-1,					-1,				0U, -1, -1 },
	{ "PB02",			GPIO_PORT_B, GPIO_PIN_2,	-1,					-1,				0U, -1, -1 },
	{ "PB03",			GPIO_PORT_B, GPIO_PIN_3,	-1,					-1,				0U, -1, -1 },
	{ "PB04",			GPIO_PORT_B, GPIO_PIN_4,	-1,					-1,				0U, -1, -1 },
	{ "PB05",			GPIO_PORT_B, GPIO_PIN_5,	-1,					-1,				0U, -1, -1 },
	{ "PB06",			GPIO_PORT_B, GPIO_PIN_6,	-1,					-1,				0U, -1, -1 },
	{ "PB07",			GPIO_PORT_B, GPIO_PIN_7,	-1,					-1,				0U, -1, -1 },
	// not a pin, just additional ADC channel
	{ "VBAT",			-1,			 -1,			ADC_CHANNEL_VBAT,	-1,				0U, -1, -1 },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]) - 1;

#endif

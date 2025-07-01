#if PLATFORM_XR872

#include "../../../new_common.h"
#include "../../../logging/logging.h"

#include "../hal_pinmap_xradio.h"

xrpin_t g_pins[] = {
	{ "PA00",			GPIO_PORT_A, GPIO_PIN_0,	-1,					-1,				0U, -1, -1 },
	{ "PA01",			GPIO_PORT_A, GPIO_PIN_1,	-1,					-1,				0U, -1, -1 },
	{ "PA02",			GPIO_PORT_A, GPIO_PIN_2,	-1,					-1,				0U, -1, -1 },
	{ "PA03",			GPIO_PORT_A, GPIO_PIN_3,	-1,					-1,				0U, -1, -1 },
	{ "PA04",			GPIO_PORT_A, GPIO_PIN_4,	-1,					-1,				0U, -1, 0 },
	{ "PA05",			GPIO_PORT_A, GPIO_PIN_5,	-1,					-1,				0U, -1, 1 },
	{ "PA06 (RX1)",		GPIO_PORT_A, GPIO_PIN_6,	-1,					-1,				0U, -1, 2 },
	{ "PA07 (TX1)",		GPIO_PORT_A, GPIO_PIN_7,	-1,					-1,				0U, -1, 3 },
	{ "PA08",			GPIO_PORT_A, GPIO_PIN_8,	-1,					PWM_GROUP0_CH0,	3U, -1, -1 },
	{ "PA09",			GPIO_PORT_A, GPIO_PIN_9,	-1,					PWM_GROUP0_CH1,	3U, -1, -1 },
	{ "PA10 (ADC0)",	GPIO_PORT_A, GPIO_PIN_10,	ADC_CHANNEL_0,		PWM_GROUP1_CH2,	3U, -1, -1 },
	{ "PA11 (ADC1)",	GPIO_PORT_A, GPIO_PIN_11,	ADC_CHANNEL_1,		PWM_GROUP1_CH3,	3U, -1, -1 },
	{ "PA12 (ADC2)",	GPIO_PORT_A, GPIO_PIN_12,	ADC_CHANNEL_2,		PWM_GROUP2_CH4,	3U, -1, -1 },
	{ "PA13 (ADC3)",	GPIO_PORT_A, GPIO_PIN_13,	ADC_CHANNEL_3,		PWM_GROUP2_CH5,	3U, -1, -1 },
	{ "PA14 (ADC4)",	GPIO_PORT_A, GPIO_PIN_14,	ADC_CHANNEL_4,		PWM_GROUP3_CH6,	3U, -1, -1 },
	{ "PA15 (ADC5)",	GPIO_PORT_A, GPIO_PIN_15,	ADC_CHANNEL_5,		PWM_GROUP3_CH7,	3U, -1, -1 },
	{ "PA16 (ADC6)",	GPIO_PORT_A, GPIO_PIN_16,	ADC_CHANNEL_6,		-1,				0U, -1, -1 },
	{ "PA17",			GPIO_PORT_A, GPIO_PIN_17,	-1,					-1,				0U, -1, 4 },
	{ "PA18",			GPIO_PORT_A, GPIO_PIN_18,	-1,					-1,				0U, -1 },
	{ "PA19",			GPIO_PORT_A, GPIO_PIN_19,	-1,					PWM_GROUP0_CH0,	4U, -1, 5 },
	{ "PA20",			GPIO_PORT_A, GPIO_PIN_20,	-1,					PWM_GROUP0_CH1,	4U, -1, 6 },
	{ "PA21",			GPIO_PORT_A, GPIO_PIN_21,	-1,					PWM_GROUP1_CH2,	4U, -1, 7 },
	{ "PA22",			GPIO_PORT_A, GPIO_PIN_22,	-1,					PWM_GROUP1_CH3,	4U, -1, 8 },
	{ "PA23",			GPIO_PORT_A, GPIO_PIN_23,	-1,					-1,				0U, -1, 9 },
	{ "PB00 (TX0)",		GPIO_PORT_B, GPIO_PIN_0,	-1,					PWM_GROUP2_CH4,	4U, -1, -1 },
	{ "PB01 (RX0)",		GPIO_PORT_B, GPIO_PIN_1,	-1,					PWM_GROUP2_CH5,	4U, -1, -1 },
	{ "PB02",			GPIO_PORT_B, GPIO_PIN_2,	-1,					PWM_GROUP3_CH6,	4U, -1, -1 },
	{ "PB03",			GPIO_PORT_B, GPIO_PIN_3,	-1,					PWM_GROUP3_CH7,	4U, -1, -1 },
	{ "PB04",			GPIO_PORT_B, GPIO_PIN_4,	-1,					-1,				0U, -1, -1 },
	{ "PB05",			GPIO_PORT_B, GPIO_PIN_5,	-1,					-1,				0U, -1, -1 },
	{ "PB06",			GPIO_PORT_B, GPIO_PIN_6,	-1,					-1,				0U, -1, -1 },
	{ "PB07",			GPIO_PORT_B, GPIO_PIN_7,	-1,					-1,				0U, -1, -1 },
	{ "PB16",			GPIO_PORT_B, GPIO_PIN_16,	-1,					-1,				0U, -1, -1 },
	{ "PB17",			GPIO_PORT_B, GPIO_PIN_17,	-1,					-1,				0U, -1, -1 },
	{ "PB18",			GPIO_PORT_B, GPIO_PIN_18,	-1,					-1,				0U, -1, -1 },
	// not a pin, just additional ADC channel
	{ "VBAT",			-1,			 -1,			ADC_CHANNEL_VBAT,	-1,				0U, -1, -1 },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]) - 1;

#endif

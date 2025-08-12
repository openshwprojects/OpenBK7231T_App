#ifdef PLATFORM_RTL8710B

#include "../hal_pinmap_realtek.h"

rtlPinMapping_t g_pins[] = {
	{ "PA0",		PA_0,	NULL, NULL },
	{ "PA5",		PA_5,	NULL, NULL },
	{ "PA6",		PA_6,	NULL, NULL },
	{ "PA7",		PA_7,	NULL, NULL },
	{ "PA8",		PA_8,	NULL, NULL },
	{ "PA9",		PA_9,	NULL, NULL },
	{ "PA10",		PA_10,	NULL, NULL },
	{ "PA11",		PA_11,	NULL, NULL },
	{ "PA12",		PA_12,	NULL, NULL },
	{ "PA14",		PA_14,	NULL, NULL },
	{ "PA15",		PA_15,	NULL, NULL },
	{ "PA18 (RX0)",	PA_18,	NULL, NULL },
	{ "PA19 (ADC1)",PA_19,	NULL, NULL },
	{ "PA22",		PA_22,	NULL, NULL },
	{ "PA23 (TX0)",	PA_23,	NULL, NULL },
	{ "PA29 (RX2)",	PA_29,	NULL, NULL },
	{ "PA30 (TX2)",	PA_30,	NULL, NULL },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	rtlPinMapping_t* pin = g_pins + index;
	switch(pin->pin)
	{
		case PA_0:
		case PA_5:
		case PA_12:
		case PA_14:
		case PA_15:
		case PA_22:
		case PA_23:
		case PA_29:
		case PA_30:
			return 1;
		default: return 0;
	}
}

#endif // PLATFORM_RTL8710B

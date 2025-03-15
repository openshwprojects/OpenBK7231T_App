#ifdef PLATFORM_RTL87X0C

#include "../hal_generic_realtek.h"

rtlPinMapping_t g_pins[] = {
	{ "PA0 (RX1)",	PA_0,	NULL, NULL },
	{ "PA1 (TX1)",	PA_1,	NULL, NULL },
	{ "PA2 (RX1)",	PA_2,	NULL, NULL },
	{ "PA3 (TX1)",	PA_3,	NULL, NULL },
	{ "PA4",		PA_4,	NULL, NULL },
	{ "PA5",		PA_5,	NULL, NULL },
	{ "PA6",		PA_6,	NULL, NULL },
	{ "PA7",		PA_7,	NULL, NULL },
	{ "PA8",		PA_8,	NULL, NULL },
	{ "PA9",		PA_9,	NULL, NULL },
	{ "PA10",		PA_10,	NULL, NULL },
	{ "PA11",		PA_11,	NULL, NULL },
	{ "PA12",		PA_12,	NULL, NULL },
	{ "PA13 (RX0)",	PA_13,	NULL, NULL },
	{ "PA14 (TX0)",	PA_14,	NULL, NULL },
	{ "PA15 (RX2)",	PA_15,	NULL, NULL },
	{ "PA16 (TX2)",	PA_16,	NULL, NULL },
	{ "PA17",		PA_17,	NULL, NULL },
	{ "PA18",		PA_18,	NULL, NULL },
	{ "PA19",		PA_19,	NULL, NULL },
	{ "PA20",		PA_20,	NULL, NULL },
	{ "PA21",		PA_21,	NULL, NULL },
	{ "PA22",		PA_22,	NULL, NULL },
	{ "PA23",		PA_23,	NULL, NULL },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index > 6 && index < 11) return 0;
	return 1;
}

#endif // PLATFORM_RTL87X0C

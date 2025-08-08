#ifdef PLATFORM_RTL8720E

#include "../hal_pinmap_realtek.h"

rtlPinMapping_t g_pins[] = {
	{ "PA0",			PA_0,	NULL, NULL },
	{ "PA1",			PA_1,	NULL, NULL },
	{ "PA2",			PA_2,	NULL, NULL },
	{ "PA3",			PA_3,	NULL, NULL },
	{ "PA4",			PA_4,	NULL, NULL },
	{ "PA5",			PA_5,	NULL, NULL },
	{ "PA6",			PA_6,	NULL, NULL },
	{ "PA7",			PA_7,	NULL, NULL },
	{ "PA8",			PA_8,	NULL, NULL },
	{ "PA9",			PA_9,	NULL, NULL },
	{ "PA10",			PA_10,	NULL, NULL },
	{ "PA11",			PA_11,	NULL, NULL },
	{ "PA12",			PA_12,	NULL, NULL },
	{ "PA13",			PA_13,	NULL, NULL },
	{ "PA14",			PA_14,	NULL, NULL },
	{ "PA15",			PA_15,	NULL, NULL },
	{ "PA16",			PA_16,	NULL, NULL },
	{ "PA17",			PA_17,	NULL, NULL },
	{ "PA18",			PA_18,	NULL, NULL },
	{ "PA19 (LOG_RX)",	PA_19,	NULL, NULL },
	{ "PA20 (LOG_TX)",	PA_20,	NULL, NULL },
	{ "PA21",			PA_21,	NULL, NULL },
	{ "PA22",			PA_22,	NULL, NULL },
	{ "PA23",			PA_23,	NULL, NULL },
	{ "PA24",			PA_24,	NULL, NULL },
	{ "PA25",			PA_25,	NULL, NULL },
	{ "PA26",			PA_26,	NULL, NULL },
	{ "PA27",			PA_27,	NULL, NULL },
	{ "PA28",			PA_28,	NULL, NULL },
	{ "PA29",			PA_29,	NULL, NULL },
	{ "PA30",			PA_30,	NULL, NULL },
	{ "PA31",			PA_31,	NULL, NULL },
	{ "PB0",			PB_0,	NULL, NULL },
	{ "PB1",			PB_1,	NULL, NULL },
	{ "PB2",			PB_2,	NULL, NULL },
	{ "PB3",			PB_3,	NULL, NULL },
	{ "PB4",			PB_4,	NULL, NULL },
	{ "PB5",			PB_5,	NULL, NULL },
	{ "PB6",			PB_6,	NULL, NULL },
	{ "PB7",			PB_7,	NULL, NULL },
	{ "PB8",			PB_8,	NULL, NULL },
	{ "PB9",			PB_9,	NULL, NULL },
	{ "PB10",			PB_10,	NULL, NULL },
	{ "PB11",			PB_11,	NULL, NULL },
	{ "PB12",			PB_12,	NULL, NULL },
	{ "PB13",			PB_13,	NULL, NULL },
	{ "PB14",			PB_14,	NULL, NULL },
	{ "PB15",			PB_15,	NULL, NULL },
	{ "PB16",			PB_16,	NULL, NULL },
	{ "PB17",			PB_17,	NULL, NULL },
	{ "PB18",			PB_18,	NULL, NULL },
	{ "PB19",			PB_19,	NULL, NULL },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	return 1;
}

#endif // PLATFORM_RTL8720E

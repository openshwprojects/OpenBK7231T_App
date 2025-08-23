#ifdef PLATFORM_RTL8710A

#include "../hal_pinmap_realtek.h"

// commented out pins are those not wired out on RTL8711AM
rtlPinMapping_t g_pins[] = {
	//{ "PA0",		PA_0,	NULL, NULL },
	//{ "PA1",		PA_1,	NULL, NULL },
	//{ "PA2",		PA_2,	NULL, NULL },
	{ "PA3",		PA_3,	NULL, NULL },
	{ "PA4",		PA_4,	NULL, NULL },
	{ "PA5",		PA_5,	NULL, NULL },
	{ "PA6",		PA_6,	NULL, NULL },
	{ "PA7",		PA_7,	NULL, NULL },

	{ "PB0 (L_TX)",	PB_0,	NULL, NULL },
	{ "PB1 (L_RX)",	PB_1,	NULL, NULL },
	{ "PB2",		PB_2,	NULL, NULL },
	{ "PB3",		PB_3,	NULL, NULL },
	//{ "PB4",		PB_4,	NULL, NULL },
	//{ "PB5",		PB_5,	NULL, NULL },
	//{ "PB6",		PB_6,	NULL, NULL },
	//{ "PB7",		PB_7,	NULL, NULL },

	{ "PC0",		PC_0,	NULL, NULL },
	{ "PC1",		PC_1,	NULL, NULL },
	{ "PC2",		PC_2,	NULL, NULL },
	{ "PC3",		PC_3,	NULL, NULL },
	{ "PC4",		PC_4,	NULL, NULL },
	{ "PC5",		PC_5,	NULL, NULL },
	//{ "PC6",		PC_6,	NULL, NULL },
	//{ "PC7",		PC_7,	NULL, NULL },
	//{ "PC8",		PC_8,	NULL, NULL },
	//{ "PC9",		PC_9,	NULL, NULL },

	//{ "PD0",		PD_0,	NULL, NULL },
	//{ "PD1",		PD_1,	NULL, NULL },
	//{ "PD2",		PD_2,	NULL, NULL },
	//{ "PD3",		PD_3,	NULL, NULL },
	//{ "PD4",		PD_4,	NULL, NULL },
	//{ "PD5",		PD_5,	NULL, NULL },
	//{ "PD6",		PD_6,	NULL, NULL },
	//{ "PD7",		PD_7,	NULL, NULL },
	//{ "PD8",		PD_8,	NULL, NULL },
	//{ "PD9",		PD_9,	NULL, NULL },

	{ "PE0 (JTAG)",	PE_0,	NULL, NULL }, // JTAG, cannot be used until sys_jtag_off()
	{ "PE1 (JTAG)",	PE_1,	NULL, NULL }, // JTAG, cannot be used until sys_jtag_off()
	{ "PE2 (JTAG)",	PE_2,	NULL, NULL }, // JTAG, cannot be used until sys_jtag_off()
	{ "PE3 (JTAG)",	PE_3,	NULL, NULL }, // JTAG, cannot be used until sys_jtag_off()
	{ "PE4 (JTAG)",	PE_4,	NULL, NULL }, // JTAG, cannot be used until sys_jtag_off()
	//{ "PE5",		PE_5,	NULL, NULL },
	//{ "PE6",		PE_6,	NULL, NULL },
	//{ "PE7",		PE_7,	NULL, NULL },
	//{ "PE8",		PE_8,	NULL, NULL },
	//{ "PE9",		PE_9,	NULL, NULL },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	rtlPinMapping_t* pin = g_pins + index;
	switch(pin->pin)
	{
		case PC_0:
		case PC_1:
		case PC_2:
		case PC_3:
		case PE_0:
		case PE_1:
		case PE_2:
		case PE_3:
			return 1;
		default: return 0;
	}
}

#endif // PLATFORM_RTL8710A

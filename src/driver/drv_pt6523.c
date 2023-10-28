
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

byte pt_inh = 10;
byte pt_ce = 11;
byte pt_clk = 24;
byte pt_di = 8;
byte g_screen[15];
byte g_symbols[5];
byte g_address = 130;

void PT6523_shiftOut(byte *data, int totalBytes) {
	for (int j = 0; j < totalBytes; j++) {
		byte val = data[j];
		for (int i = 0; i < 8; i++) {
			//if (bitOrder == LSBFIRST)
			//HAL_PIN_SetOutputValue(pt_di, !!(val & (1 << i)));
			//else
			HAL_PIN_SetOutputValue(pt_di, !!(val & (1 << ((8 - 1) - i))));

			HAL_PIN_SetOutputValue(pt_clk, 1);
			HAL_PIN_SetOutputValue(pt_clk, 0);
		}
	}
}

void PT6523_Print() {
	HAL_PIN_SetOutputValue(pt_ce, 0);
	// Address Data (A1- A8)
	PT6523_shiftOut(&g_address,1);
	HAL_PIN_SetOutputValue(pt_ce, 1);

	// Character Segment Data (D1- D120) 15 Byte
	PT6523_shiftOut(g_screen, sizeof(g_screen));

	// Symbol Segment Data (D121 - D156 & DR, SC, BU, X) 5 Byte
	PT6523_shiftOut(g_symbols, sizeof(g_symbols));

	HAL_PIN_SetOutputValue(pt_ce, 0);
}

void PT6523_Init()
{
	HAL_PIN_Setup_Output(pt_inh);
	HAL_PIN_SetOutputValue(pt_inh, 1);
	HAL_PIN_Setup_Output(pt_ce);
	HAL_PIN_SetOutputValue(pt_ce, 0);
	HAL_PIN_Setup_Output(pt_clk);
	HAL_PIN_SetOutputValue(pt_clk, 0);
	HAL_PIN_Setup_Output(pt_di);
	HAL_PIN_SetOutputValue(pt_di, 0);
}


void PT6523_RunFrame()
{
	for (int i = 0; i < sizeof(g_screen); i++) {
		g_screen[i] = rand();
	}
	for (int i = 0; i < sizeof(g_symbols); i++) {
		// Can't do rand() here, neither ff, because it turns off the display sometimes
		// Maybe there is some specal turn-off bit here?
		g_symbols[i] = 0x0;// ff;// rand();
	}
	PT6523_Print();
}





// PT6523 LCD driver
// Based on GPL work of by Firat SOYGÜL, 20 Aralik 2017
#include "../obk_config.h"

#if ENABLE_DRIVER_PT6523

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
#include <math.h>

#include "drv_pt6523_font.h"

byte pt_inh = 10;
byte pt_ce = 11;
byte pt_clk = 24;
byte pt_di = 8;
byte g_screen[15];
byte g_symbols[5];
byte g_address = 130;
byte g_volumeStart = 0;
byte g_volumeEnd = 8;
byte g_volumeDir = 1;
bool g_symbolDisc;

void PT6523_AnimateVolume(int levelValue) {
	float convertedLevelValue =
		ceil((levelValue - g_volumeStart) /
			ceil((g_volumeEnd - g_volumeStart) / 7));
	int _volumeLevel;
	if (!g_volumeDir) {
		g_symbols[3] = 0b11111100;
		g_symbols[3] <<= 7 - (int)convertedLevelValue;
		// BIT_SET_TO(g_symbols[1], 7, _iconRock);
		_volumeLevel = (int)convertedLevelValue;
		if (_volumeLevel == 0) {
			BIT_SET_TO(g_symbols[2], 0, 0);
		}
		else {
			BIT_SET_TO(g_symbols[2], 0, 1);
		}
	} else {
		g_symbols[3] = 0b11111100;
		g_symbols[2] = 0b00000001;
		g_symbols[3] >>= 7 - (int)convertedLevelValue;
		g_symbols[2] >>= 7 - (int)convertedLevelValue;
		// BIT_SET_TO(g_symbols[1], 7, _iconRock);
		_volumeLevel = (int)convertedLevelValue;
		if (_volumeLevel == 0) {
			BIT_SET_TO(g_symbols[3], 2, 0);
		}
		else {
			BIT_SET_TO(g_symbols[3], 2, 1);
		}
	}
}

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

void PT6523_Refresh() {
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

int loop = 0;
int _containerSize = 16;
byte g_container[17];
void PT6523_SetLetters() {
	byte j = 1;
	int _n = 0;
	for (int i = 0; i <= 14; i += 2) {
		g_screen[i] = (g_container[_n] << j) | (g_container[_n + 1] >> (8 - j));
		if (i < 2) {
			g_screen[i + 1] = g_container[_n + 1] << j;
		}
		else if (i >= _containerSize) {
			g_screen[i] = g_container[_n + 1];
			g_screen[i + 1] = 0;
		}
		else {
			g_screen[i + 1] =
				(g_container[_n + 1] << j) | (g_container[_n + 2] >> (7 - j));
		}
		if ((_n + 2) > _containerSize) {
			_n = 0;
			g_screen[i + 1] =
				g_container[_containerSize] << j | g_container[0] >> (7 - j);
		}
		else {
			_n += 2;
		}
		j++;
	}
}
void PT6523_ClearString() {
	memset(g_container, 0, sizeof(g_container));
}
void PT6523_DrawString(char gk[], int startOfs) {
	int d = startOfs*2;
	for (int i = startOfs; i < 8; i++) {
		int c = gk[i- startOfs] - 32;
		if (c >= 0 && c <= 94) {
			g_container[d] = pt_character14SEG[c][0];
			g_container[d + 1] = pt_character14SEG[c][1];
		}
		else {
			g_container[d] = 0;
			g_container[d + 1] = 0;
		}
		d += 2;
	}
	PT6523_SetLetters();
}
void PT6523_RunFrame()
{
#if 0
	for (int i = 0; i < sizeof(g_screen); i++) {
		g_screen[i] = rand();
	}
	for (int i = 0; i < sizeof(g_symbols); i++) {
		// Can't do rand() here, neither ff, because it turns off the display sometimes
		// Maybe there is some specal turn-off bit here?
		g_symbols[i] = 0x0;// ff;// rand();
	}
	PT6523_DrawString("OpenBK  ");
	PT6523_AnimateVolume(loop);
	g_symbolDisc = loop % 2;
	BIT_SET_TO(g_screen[7], 3, g_symbolDisc);
	loop++;
	loop %= 8;
	PT6523_Refresh();
#endif
}


#endif // ENABLE_DRIVER_PT6523




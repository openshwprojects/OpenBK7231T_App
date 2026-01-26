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


static softI2C_t g_softI2C;

int ssd1306_addr = 0x3C;

#define SSD1306_CMD  0x00
#define SSD1306_DATA 0x40

static byte ssd1306_test = 1;

static void SSD1306_WriteCmd(byte c) {
	Soft_I2C_Start(&g_softI2C, ssd1306_addr);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_CMD);
	Soft_I2C_WriteByte(&g_softI2C, c);
	Soft_I2C_Stop(&g_softI2C);
}

static void SSD1306_WriteData(byte d) {
	Soft_I2C_Start(&g_softI2C, ssd1306_addr);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_DATA);
	Soft_I2C_WriteByte(&g_softI2C, d);
	Soft_I2C_Stop(&g_softI2C);
}

void SSD1306_Fill(byte v) {
	int i;
	for (i = 0; i < 512; i++)
		SSD1306_WriteData(v);
}

// startDriver SSD1306 16 20 0x3C
// backlog stopdriver SSD1306 ; startDriver SSD1306 16 20 0x3C
void SSD1306_Init() {

	g_softI2C.pin_clk = Tokenizer_GetPin(1, 16);
	g_softI2C.pin_data = Tokenizer_GetPin(2, 20);
	ssd1306_addr = Tokenizer_GetArgIntegerDefault(3, 0x3C);

	Soft_I2C_PreInit(&g_softI2C);

	SSD1306_WriteCmd(0xAE);
	SSD1306_WriteCmd(0xD5);
	SSD1306_WriteCmd(0x80);
	SSD1306_WriteCmd(0xA8);
	SSD1306_WriteCmd(0x1F);
	SSD1306_WriteCmd(0xD3);
	SSD1306_WriteCmd(0x00);
	SSD1306_WriteCmd(0x40);
	SSD1306_WriteCmd(0x8D);
	SSD1306_WriteCmd(0x14);
	SSD1306_WriteCmd(0x20);
	SSD1306_WriteCmd(0x00);
	SSD1306_WriteCmd(0xA1);
	SSD1306_WriteCmd(0xC8);
	SSD1306_WriteCmd(0xDA);
	SSD1306_WriteCmd(0x02);
	SSD1306_WriteCmd(0x81);
	SSD1306_WriteCmd(0x8F);
	SSD1306_WriteCmd(0xD9);
	SSD1306_WriteCmd(0xF1);
	SSD1306_WriteCmd(0xDB);
	SSD1306_WriteCmd(0x40);
	SSD1306_WriteCmd(0xA4);
	SSD1306_WriteCmd(0xA6);
	SSD1306_WriteCmd(0xAF);

	SSD1306_Fill(0x00);
}
void SSD1306_SetOn(bool b) {
	if (b) {
		SSD1306_WriteCmd(0xAF);
	}
	else {
		SSD1306_WriteCmd(0xAE);
	}
}
void SSD1306_SetPos(byte x, byte page) {
	SSD1306_WriteCmd(0xB0 | page);
	SSD1306_WriteCmd(0x00 | (x & 0x0F));
	SSD1306_WriteCmd(0x10 | (x >> 4));
}

void SSD1306_DrawRect(byte x, byte y, byte w, byte h, byte fill) {
	byte px, py;
	byte page_start = y >> 3;
	byte page_end = (y + h - 1) >> 3;

	for (py = page_start; py <= page_end; py++) {
		SSD1306_SetPos(x, py);
		for (px = 0; px < w; px++) {

			byte mask = 0x00;

			if (fill) {
				mask = 0xFF;
			}
			else {
				if (py == page_start || py == page_end)
					mask = 0xFF;
				else if (px == 0 || px == w - 1)
					mask = 0xFF;
			}

			SSD1306_WriteData(mask);
		}
	}
}
void SSD1306_OnEverySecond() {
	SSD1306_SetOn(true);
	if (ssd1306_test) {
		SSD1306_Fill(0x00);
	}
	else {
		SSD1306_DrawRect(0, 0, 30, 15, 0);
		SSD1306_DrawRect(40, 10, 50, 20, 1);
	}
	ssd1306_test = !ssd1306_test;
}

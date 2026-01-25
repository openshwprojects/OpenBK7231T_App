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

#define SSD1306_ADDR 0x3C
#define SSD1306_CMD  0x00
#define SSD1306_DATA 0x40

static byte ssd1306_on = 1;

static void SSD1306_WriteCmd(byte c) {
	Soft_I2C_Start(&g_softI2C, SSD1306_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_CMD);
	Soft_I2C_WriteByte(&g_softI2C, c);
	Soft_I2C_Stop(&g_softI2C);
}

static void SSD1306_WriteData(byte d) {
	Soft_I2C_Start(&g_softI2C, SSD1306_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_DATA);
	Soft_I2C_WriteByte(&g_softI2C, d);
	Soft_I2C_Stop(&g_softI2C);
}

void SSD1306_Clear(void) {
	int i;
	for (i = 0; i < 512; i++) SSD1306_WriteData(0x00);
}

void SSD1306_Init() {

	g_softI2C.pin_clk = 16;
	g_softI2C.pin_data = 20;

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

	SSD1306_Clear();
}

void SSD1306_OnEverySecond() {

	if (ssd1306_on) {
		SSD1306_WriteCmd(0xAE);
	}
	else {
		SSD1306_WriteCmd(0xAF);
	}
	ssd1306_on = !ssd1306_on;
}

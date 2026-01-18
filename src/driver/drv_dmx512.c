/*

// #define RS485_SE_PIN 19
// digitalWrite(RS485_SE_PIN, HIGH);
setPinRole 19 AlwaysHigh

// #define RS485_EN_PIN 17
// digitalWrite(RS485_EN_PIN, HIGH);
setPinRole 17 AlwaysHigh

// #define PIN_5V_EN 16
// digitalWrite(PIN_5V_EN, HIGH);
setPinRole 16 AlwaysHigh

// UART TX (for RS485) on 22
startDriver DMX 22
SM16703P_Init 50 RGBW
startDriver PixelAnim

*/
#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_uart.h"
#include "../hal/hal_generic.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#if ENABLE_DRIVER_DMX

#include "drv_leds_shared.h"
#include "drv_local.h"

#define DMX_CHANNELS_SIZE 512
#define DMX_BUFFER_SIZE (DMX_CHANNELS_SIZE+1)

static byte *g_dmxBuffer;
static int dmx_pixelCount = 0;
static int dmx_pixelSize = 3;

int dmx_pin = 22;

void HAL_UART_Flush(void);
void HAL_SetBaud(uint32_t baud);
void DMX_Show() {
	if (g_dmxBuffer == 0) {
		return;
	}
#if !WINDOWS
	for (int i = 0; i < 2; i++)
#endif
	{
#if WINDOWS
		// BREAK: pull TX low manually
		HAL_PIN_Setup_Output(dmx_pin);
		HAL_PIN_SetOutputValue(dmx_pin, 0);
		HAL_Delay_us(120); // ≥88µs
		HAL_PIN_SetOutputValue(dmx_pin, 1);
		HAL_Delay_us(12); // MAB ≥8µs
		HAL_UART_Init(250000, 2, false, dmx_pin, -1);
#else
#define DMX_BREAK_DURATION_MICROS 88
		uint32_t breakBaud = 1000000 * 8 / DMX_BREAK_DURATION_MICROS;
		HAL_SetBaud(breakBaud);
		HAL_UART_SendByte(0);
		HAL_UART_Flush();
		HAL_SetBaud(250000);
#endif

		// restore UART and send DMX data
		for (int i = 0; i < DMX_BUFFER_SIZE; i++) {
			HAL_UART_SendByte(g_dmxBuffer[i]);
		}
		//Serial485.begin(250000, SERIAL_8N2, RS485_RX_PIN, RS485_TX_PIN);
		////Serial485.write(dmxBuffer, sizeof(dmxBuffer));
		//Serial485.flush();
	}
}

byte DMX_GetByte(uint32_t idx) {
	if (idx >= DMX_CHANNELS_SIZE)
		return 0;
	return g_dmxBuffer[1 + idx];
}
void DMX_setByte(int idx, byte color) {
	if (idx >= DMX_CHANNELS_SIZE)
		return;
	g_dmxBuffer[1 + idx] = color;
}

void DMX_SetLEDCount(int pixel_count, int pixel_size) {
	dmx_pixelCount = pixel_count;
	dmx_pixelSize = pixel_size;
}

void DMX_Init() {
//	dmx_pin = Tokenizer_GetArgIntegerDefault(1, dmx_pin);
	dmx_pin = Tokenizer_GetPin(1, dmx_pin);

	g_dmxBuffer = (byte*)malloc(DMX_BUFFER_SIZE);
	memset(g_dmxBuffer, 0, DMX_BUFFER_SIZE);
	ledStrip_t ws_export;
	ws_export.apply = DMX_Show;
	ws_export.getByte = DMX_GetByte;
	ws_export.setByte = DMX_setByte;
	ws_export.setLEDCount = DMX_SetLEDCount;

	LEDS_InitShared(&ws_export);

	HAL_UART_Init(250000, 2, false, dmx_pin, -1);
}
void DMX_OnEverySecond() {
}


void DMX_Shutdown() {
	if (g_dmxBuffer) {
		free(g_dmxBuffer);
		g_dmxBuffer = 0;
	}
	LEDS_ShutdownShared();
}
#endif





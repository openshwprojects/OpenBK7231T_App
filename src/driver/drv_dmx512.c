#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
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

int dmx_pin = 0;

void DMX_Show() {
	// BREAK: pull TX low manually
	HAL_PIN_Setup_Output(dmx_pin);
	HAL_PIN_SetOutputValue(dmx_pin, 0);
	HAL_Delay_us(120); // ≥88µs
	HAL_PIN_SetOutputValue(dmx_pin, 1);
	HAL_Delay_us(12); // MAB ≥8µs

	// restore UART and send DMX data
	HAL_UART_Init(250000, 2, true);
	for (int i = 0; i < DMX_BUFFER_SIZE; i++) {
		HAL_UART_SendByte(g_dmxBuffer[i]);
	}
	//Serial485.begin(250000, SERIAL_8N2, RS485_RX_PIN, RS485_TX_PIN);
	////Serial485.write(dmxBuffer, sizeof(dmxBuffer));
	//Serial485.flush();
}

byte DMX_GetByte(uint32_t idx) {
	if (idx >= DMX_CHANNELS_SIZE)
		return 0;
	return g_dmxBuffer[1 + idx];
}
void DMX_setByte(int idx, byte color) {
	if (idx >= DMX_CHANNELS_SIZE)
		return 0;
	g_dmxBuffer[1 + idx] = color;
}

void DMX_SetLEDCount(int pixel_count, int pixel_size) {
	dmx_pixelCount = pixel_count;
	dmx_pixelSize = pixel_size;
}

void DMX_Init() {
	g_dmxBuffer = (byte*)malloc(DMX_BUFFER_SIZE);
	memset(g_dmxBuffer, 0, DMX_BUFFER_SIZE);
	ledStrip_t ws_export;
	ws_export.apply = DMX_Show;
	ws_export.getByte = DMX_GetByte;
	ws_export.setByte = DMX_setByte;
	ws_export.setLEDCount = DMX_SetLEDCount;

	LEDS_InitShared(&ws_export);
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





#pragma once

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

int DS1820_getTemp();
void DS1820_driver_Init();
void DS1820_OnEverySecond();
void DS1820_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#define DEVICE_DISCONNECTED_C -127

// some more needed to support multiple devices and GPIOs
#if (DS1820full)
#define DS18B20MAX	10			// max numbe of sensors
#define DS18B20namel	20			// length of description
#define DS18B20MAX_GPIOS 2			// max GPIOs with sensors

bool ds18b20_used_channel(int ch);
int http_fn_cfg_ds18b20(http_request_t* request);
/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
	/* *INDENT-ON* */

	void ds18b20_init(int GPIO);

	bool ds18b20_isConnected(const uint8_t *deviceAddress, uint8_t *scratchPad);
	bool ds18b20_readScratchPad(const uint8_t *deviceAddress, uint8_t *scratchPad);
	bool ds18b20_select(const uint8_t *address);
	bool ds18b20_isAllZeros(const uint8_t * const scratchPad);
	bool isConversionComplete();

	float ds18b20_getTempC(const uint8_t *deviceAddress);

	void reset_search();
	bool search(uint8_t *newAddr, bool search_mode, int Pin);

	/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif


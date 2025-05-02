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

// some more needed to support multiple devices and GPIOs
#if (DS1820full)
#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_RAW -7040
#define DS18B20MAX	10			// max numbe of sensors
#define DS18B20namel	20			// length of description
#define DS18B20MAX_GPIOS 2			// max GPIOs with sensors

typedef uint8_t DeviceAddress[8];		// we need to distinguish sensors by their address

bool ds18b20_used_channel(int ch);
int http_fn_cfg_ds18b20(http_request_t* request);
/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
	/* *INDENT-ON* */

	void ds18b20_init(int GPIO);

#define ds18b20_send ds18b20_write
#define ds18b20_send_byte ds18b20_write_byte
#define ds18b20_RST_PULSE ds18b20_reset

//	bool ds18b20_setResolution(const DeviceAddress tempSensorAddresses[], int numAddresses, uint8_t newResolution);
	bool ds18b20_isConnected(const DeviceAddress *deviceAddress, uint8_t *scratchPad);
//	void ds18b20_writeScratchPad(const DeviceAddress *deviceAddress, const uint8_t *scratchPad);
	bool ds18b20_readScratchPad(const DeviceAddress *deviceAddress, uint8_t *scratchPad);
	void ds18b20_select(const DeviceAddress *address);
//	uint8_t ds18b20_crc8(const uint8_t *addr, uint8_t len);
	bool ds18b20_isAllZeros(const uint8_t * const scratchPad);
	bool isConversionComplete();
//	uint16_t millisToWaitForConversion();

	float ds18b20_getTempC(const DeviceAddress *deviceAddress);
	int16_t calculateTemperature(const DeviceAddress *deviceAddress, uint8_t* scratchPad);
//	float ds18b20_get_temp(void);

	void reset_search();
	bool search(uint8_t *newAddr, bool search_mode, int Pin);

	/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif


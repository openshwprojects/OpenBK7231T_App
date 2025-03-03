/*
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef DS18B20_H_  
#define DS18B20_H_

#define noInterrupts() 
#define interrupts() 

#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_F -196.6
#define DEVICE_DISCONNECTED_RAW -7040
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define pgm_read_byte(addr)   (*(const unsigned char *)(addr))
#define DS18B20MAX	10
#define DS18B20namel	20
#define DS18B20MAX_GPIOS 2

typedef uint8_t DeviceAddress[8];
typedef uint8_t ScratchPad[9];
typedef struct {
  DeviceAddress array[DS18B20MAX];
  uint8_t index;
  char name[DS18B20MAX][DS18B20namel];
  float lasttemp[DS18B20MAX];
  unsigned short last_read[DS18B20MAX];
  short channel[DS18B20MAX];
  short GPIO[DS18B20MAX];

} devicesArray;

bool ds18b20_used_channel(int ch);


void DS18B20_OnEverySecond();
void DS18B20_AppendInformationToHTTPIndexPage(http_request_t* request);
int http_fn_cfg_ds18b20(http_request_t* request);
void DS18B20_driver_Init();

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
	/* *INDENT-ON* */

	void ds18b20_init(int GPIO);

#define ds18b20_send ds18b20_write
#define ds18b20_send_byte ds18b20_write_byte
#define ds18b20_RST_PULSE ds18b20_reset

	void ds18b20_write(char bit);
	unsigned char ds18b20_read(void);
	void ds18b20_write_byte(char data);
	unsigned char ds18b20_read_byte(void);
	unsigned char ds18b20_reset(void);

	bool ds18b20_setResolution(const DeviceAddress tempSensorAddresses[], int numAddresses, uint8_t newResolution);
	bool ds18b20_isConnected(const DeviceAddress *deviceAddress, uint8_t *scratchPad);
	void ds18b20_writeScratchPad(const DeviceAddress *deviceAddress, const uint8_t *scratchPad);
	bool ds18b20_readScratchPad(const DeviceAddress *deviceAddress, uint8_t *scratchPad);
	void ds18b20_select(const DeviceAddress *address);
	uint8_t ds18b20_crc8(const uint8_t *addr, uint8_t len);
	bool ds18b20_isAllZeros(const uint8_t * const scratchPad);
	bool isConversionComplete();
	uint16_t millisToWaitForConversion();

	void ds18b20_requestTemperatures();
	float ds18b20_getTempF(const DeviceAddress *deviceAddress);
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

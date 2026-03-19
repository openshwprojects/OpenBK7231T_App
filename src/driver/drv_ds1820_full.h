#pragma once


void DS1820_full_driver_Init();
void DS1820_full_OnEverySecond();
void DS1820_full_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#define DEVICE_DISCONNECTED_C -127

// some defines needed to support multiple devices and GPIOs
#define DS18B20MAX	10			// max numbe of sensors
#define DS18B20namel	20			// length of description
#define DS18B20MAX_GPIOS 2			// max GPIOs with sensors


// prototypes
bool ds18b20_used_channel(int ch);
int http_fn_cfg_ds18b20(http_request_t* request);
bool ds18b20_isConnected(const uint8_t *deviceAddress, uint8_t *scratchPad);
bool ds18b20_readScratchPad(const uint8_t *deviceAddress, uint8_t *scratchPad);
bool ds18b20_select(const uint8_t *address);
bool ds18b20_isAllZeros(const uint8_t * const scratchPad);
float ds18b20_getTempC(const uint8_t *deviceAddress);
bool isConversionComplete();
void reset_search();
bool search(uint8_t *newAddr, bool search_mode, int Pin);
char *DS1820_full_jsonSensors();

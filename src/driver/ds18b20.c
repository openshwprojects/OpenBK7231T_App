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
#include "../obk_config.h"
#if (ENABLE_DRIVER_DS18B20)
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

#include "ds18b20.h"
#include "OneWire_common.h"

static int ds18_conversionPeriod = 15;	// time between refreshs of temperature
static int ds18_count = 0;		// detected number of devices
static int errcount = 0;
static int lastconv; 			// secondsElapsed on last successfull reading
static uint8_t dsread = 0;
static int Pin;
static devicesArray ds18b20devices;

// OneWire commands
#define GETTEMP			0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define SKIPROM			0xCC  // Command to address all devices on the bus
#define SELECTDEVICE	0x55  // Command to address all devices on the bus
#define COPYSCRATCH     0x48  // Copy scratchpad to EEPROM
#define READSCRATCH     0xBE  // Read from scratchpad
#define WRITESCRATCH    0x4E  // Write to scratchpad
#define RECALLSCRATCH   0xB8  // Recall from EEPROM to scratchpad
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition
// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8
// DSROM FIELDS
#define DSROM_FAMILY    0
#define DSROM_CRC       7
// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820[%i] - " fmt, Pin, ##__VA_ARGS__)


uint8_t DS_GPIO;	// the actual GPIO used (changes in case we have multiple GPIOs defined ...)
uint8_t init = 0;
uint8_t bitResolution = 12;
uint8_t devices = 0;
uint8_t DS18B20GPIOS[DS18B20MAX_GPIOS];
DeviceAddress ROM_NO;
uint8_t LastDiscrepancy;
uint8_t LastFamilyDiscrepancy;
bool LastDeviceFlag;

/// Sends one bit to bus
void ds18b20_write(char bit) {
	if (bit & 1) {
		HAL_PIN_Setup_Output(DS_GPIO);
		noInterrupts();
		HAL_PIN_SetOutputValue(DS_GPIO, 0);
		OWusleepshort(6); 	// was usleep(6);
		HAL_PIN_Setup_Input(DS_GPIO);	// release bus
		OWusleepmed(64);		// was usleep(64);
		interrupts();
	}
	else {
		HAL_PIN_Setup_Output(DS_GPIO);
		noInterrupts();
		HAL_PIN_SetOutputValue(DS_GPIO, 0);
		OWusleepmed(60);		// was usleep(60);
		HAL_PIN_Setup_Input(DS_GPIO);	// release bus
		OWusleepshort(10);		// was usleep(10);
		interrupts();
	}
}

// Reads one bit from bus
unsigned char ds18b20_read(void) {
	unsigned char value = 0;
	HAL_PIN_Setup_Output(DS_GPIO);
	noInterrupts();
	HAL_PIN_SetOutputValue(DS_GPIO, 0);
	OWusleepshort(6);		// was usleep(6);
	HAL_PIN_Setup_Input(DS_GPIO);
	OWusleepshort(9);		// was usleep(9);
	value = HAL_PIN_ReadDigitalInput(DS_GPIO);
	OWusleepmed(55);		// was usleep(55);
	interrupts();
	return (value);
}
// Sends one byte to bus
void ds18b20_write_byte(char data) {
	unsigned char i;
	unsigned char x;
	for (i = 0; i < 8; i++) {
		x = data >> i;
		x &= 0x01;
		ds18b20_write(x);
	}
	OWusleepmed(100);		// was usleep(100);
}
// Reads one byte from bus
unsigned char ds18b20_read_byte(void) {
	unsigned char i;
	unsigned char data = 0;
	for (i = 0; i < 8; i++)
	{
		if (ds18b20_read()) data |= 0x01 << i;
		OWusleepshort(15);		// was usleep(15);
	}
	return(data);
}
// Sends reset pulse
unsigned char ds18b20_reset(void) {
/*	unsigned char presence;
	HAL_PIN_Setup_Output(DS_GPIO);
	noInterrupts();
	HAL_PIN_SetOutputValue(DS_GPIO, 0);
	OWusleeplong(480);		// was usleep(480);
	HAL_PIN_SetOutputValue(DS_GPIO, 1);
	HAL_PIN_Setup_Input(DS_GPIO);
	OWusleepmed(70);		// was usleep(70);
	presence = (HAL_PIN_ReadDigitalInput(DS_GPIO) == 0);
	OWusleeplong(410);		// was usleep(410);
	interrupts();
	return presence;
*/
	return (unsigned char)OWReset(DS_GPIO) ;
}

bool ds18b20_setResolution(const DeviceAddress tempSensorAddresses[], int numAddresses, uint8_t newResolution) {
	bool success = false;
	// handle the sensors with configuration register
	newResolution = constrain(newResolution, 9, 12);
	uint8_t newValue = 0;
	ScratchPad scratchPad;
	// loop through each address
	for (int i = 0; i < numAddresses; i++) {
		// we can only update the sensor if it is connected
		// no need to check for GPIO of device here, ds18b20_readScratchPad() inside of ds18b20_isConnected() will do so and return flase, if not
		// it will also make sure, DS_GPIO is set correctly for this sensor
		if (ds18b20_isConnected((const DeviceAddress*)tempSensorAddresses[i], scratchPad)) {
			switch (newResolution) {
			case 12:
				newValue = TEMP_12_BIT;
				break;
			case 11:
				newValue = TEMP_11_BIT;
				break;
			case 10:
				newValue = TEMP_10_BIT;
				break;
			case 9:
			default:
				newValue = TEMP_9_BIT;
				break;
			}
			// if it needs to be updated we write the new value
			if (scratchPad[CONFIGURATION] != newValue) {
				scratchPad[CONFIGURATION] = newValue;
				ds18b20_writeScratchPad((const DeviceAddress*)tempSensorAddresses[i], scratchPad);
			}
			// done
			success = true;
		}
	}
	return success;
}


bool ds18b20_getGPIO(DeviceAddress devaddr,uint8_t *GPIO)
{
	int i=0;
	for (i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device
			DS1820_LOG(DEBUG, "Found device at index %i - GPIO=%i", i,ds18b20devices.GPIO[i]);
			*GPIO=ds18b20devices.GPIO[i];
			return true;
		}
	}
	return false;
};

void ds18b20_writeScratchPad(const DeviceAddress *deviceAddress, const uint8_t *scratchPad) {
	if ( ! ds18b20_getGPIO(deviceAddress, &DS_GPIO ) ) return; 
	OWReset(DS_GPIO);
	ds18b20_select(deviceAddress);
	OWWriteByte(DS_GPIO,WRITESCRATCH);
	OWWriteByte(DS_GPIO,scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
	OWWriteByte(DS_GPIO,scratchPad[LOW_ALARM_TEMP]); // low alarm temp
	OWWriteByte(DS_GPIO,scratchPad[CONFIGURATION]);
	OWReset(DS_GPIO);
}

bool ds18b20_readScratchPad(const DeviceAddress *deviceAddress, uint8_t* scratchPad) {
//	if ( ! ds18b20_getGPIO(deviceAddress, &DS_GPIO ) ) return false;
	if ( ! ds18b20_getGPIO(deviceAddress, &DS_GPIO ) ) {
		DS1820_LOG(DEBUG, "No GPIO found for device - DS_GPIO=%i", DS_GPIO);
		return false;
	}
	// send the reset command and fail fast
	DS1820_LOG(DEBUG, "GPIO found for device - DS_GPIO=%i", DS_GPIO);
	int b = OWReset(DS_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS_GPIO);
		return false;
	}
	ds18b20_select(deviceAddress);
	OWWriteByte(DS_GPIO, READSCRATCH);
	// Read all registers in a simple loop
	// byte 0: temperature LSB
	// byte 1: temperature MSB
	// byte 2: high alarm temp
	// byte 3: low alarm temp
	// byte 4: DS18B20 & DS1822: configuration register
	// byte 5: internal use & crc
	// byte 6: DS18B20 & DS1822: store for crc
	// byte 7: DS18B20 & DS1822: store for crc
	// byte 8: SCRATCHPAD_CRC
	for (uint8_t i = 0; i < 9; i++) {
		scratchPad[i] = OWReadByte(DS_GPIO);
	}
	b =  OWReset(DS_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS_GPIO);
		return false;
	}
	return (b == 1);
}

void ds18b20_select(const DeviceAddress *address) {
	if ( ! ds18b20_getGPIO(address, &DS_GPIO ) ) return;
	uint8_t i;
	OWWriteByte(DS_GPIO, SELECTDEVICE);           // Choose ROM
	for (i = 0; i < 8; i++) OWWriteByte(DS_GPIO, ((uint8_t *)address)[i]);
}

void ds18b20_requestTemperatures() {
	OWReset(DS_GPIO);
	OWWriteByte(DS_GPIO,SKIPROM);
	OWWriteByte(DS_GPIO,GETTEMP);
	//unsigned long start = esp_timer_get_time() / 1000ULL;
	//while (!isConversionComplete() && ((esp_timer_get_time() / 1000ULL) - start < millisToWaitForConversion())) vPortYield();
}

bool isConversionComplete() {
	return DS1820TConversionDone(DS_GPIO);
}

uint16_t millisToWaitForConversion() {
	switch (bitResolution) {
	case 9:
		return 94;
	case 10:
		return 188;
	case 11:
		return 375;
	default:
		return 750;
	}
}

bool ds18b20_isConnected(const DeviceAddress *deviceAddress, uint8_t *scratchPad) {
	// no need to check for GPIO of device here, ds18b20_readScratchPad() will do so and return flase, if not
	bool b = ds18b20_readScratchPad(deviceAddress, scratchPad);
	DS1820_LOG(DEBUG, "readScratchPad returned %i - Crc8CQuick=%i / scratchPad[SCRATCHPAD_CRC]=%i",b,Crc8CQuick(scratchPad, 8),scratchPad[SCRATCHPAD_CRC]);
	return b && !ds18b20_isAllZeros(scratchPad) && (Crc8CQuick(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}

bool ds18b20_isAllZeros(const uint8_t * const scratchPad) {
	for (size_t i = 0; i < 9; i++) {
		if (scratchPad[i] != 0) {
			return false;
		}
	}
	return true;
}

float ds18b20_getTempF(const DeviceAddress *deviceAddress) {
	ScratchPad scratchPad;
	if (ds18b20_isConnected(deviceAddress, scratchPad)) {
		int16_t rawTemp = calculateTemperature(deviceAddress, scratchPad);
		if (rawTemp <= DEVICE_DISCONNECTED_RAW)
			return DEVICE_DISCONNECTED_F;
		// C = RAW/128
		// F = (C*1.8)+32 = (RAW/128*1.8)+32 = (RAW*0.0140625)+32
		return ((float)rawTemp * 0.0140625f) + 32.0f;
	}
	return DEVICE_DISCONNECTED_F;
}

float ds18b20_getTempC(const DeviceAddress *deviceAddress) {
	ScratchPad scratchPad;
	if (ds18b20_isConnected(deviceAddress, scratchPad)) {
		DS1820_LOG(DEBUG, "reading Device Temperature");
		int16_t rawTemp = calculateTemperature(deviceAddress, scratchPad);
		if (rawTemp <= DEVICE_DISCONNECTED_RAW)
			return DEVICE_DISCONNECTED_C;
		// C = RAW/128
		// F = (C*1.8)+32 = (RAW/128*1.8)+32 = (RAW*0.0140625)+32
		return (float)rawTemp / 128.0f;
	} else DS1820_LOG(DEBUG, "Device not connected");
	return DEVICE_DISCONNECTED_C;
}

// reads scratchpad and returns fixed-point temperature, scaling factor 2^-7
int16_t calculateTemperature(const DeviceAddress *deviceAddress, uint8_t* scratchPad) {
	int16_t fpTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 11) | (((int16_t)scratchPad[TEMP_LSB]) << 3);
	return fpTemperature;
}

void ds18b20_init(int GPIO) {
	DS_GPIO = GPIO;
	//gpio_pad_select_gpio(DS_GPIO);
	init = 1;
	register_testus();
}

//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void reset_search() {
	devices = 0;
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = false;
	LastFamilyDiscrepancy = 0;
	for (int i = 7; i >= 0; i--) {
		ROM_NO[i] = 0;
	}
}
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search

bool search(uint8_t *newAddr, bool search_mode, int Pin) {
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number;
	bool search_result;
	uint8_t id_bit, cmp_id_bit;

	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = false;

	// if the last call was not the last one
	if (!LastDeviceFlag) {
		// 1-Wire reset
		if (!OWReset(Pin)) {
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = false;
			LastFamilyDiscrepancy = 0;
			return false;
		}

		// issue the search command
		if (search_mode == true) {
			OWWriteByte(Pin,0xF0);   // NORMAL SEARCH
		}
		else {
			OWWriteByte(Pin,0xEC);   // CONDITIONAL SEARCH
		}

		// loop to do the search
		do {
			// read a bit and its complement
			id_bit = OWReadBit(Pin);
			cmp_id_bit = OWReadBit(Pin);

			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1)) {
				break;
			}
			else {
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit) {
					search_direction = id_bit;  // bit write value for search
				}
				else {
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy) {
						search_direction = ((ROM_NO[rom_byte_number]
							& rom_byte_mask) > 0);
					}
					else {
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);
					}
					// if 0 was picked then record its position in LastZero
					if (search_direction == 0) {
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

				// serial number search direction write bit
				OWWriteBit(Pin,search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0) {
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while (rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65)) {
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0) {
				LastDeviceFlag = true;
			}
			search_result = true;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0]) {
		devices = 0;
		LastDiscrepancy = 0;
		LastDeviceFlag = false;
		LastFamilyDiscrepancy = 0;
		search_result = false;
	}
	else {
		for (int i = 0; i < 8; i++) {
			newAddr[i] = ROM_NO[i];
		}
		devices++;
	}
	return search_result;
}

// add a new device into devicesArray
void insertArray(devicesArray *a, DeviceAddress devaddr) {
	if (ds18_count >= DS18B20MAX){
		bk_printf("insertArray ERROR:Arry is full, can't add device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ",
			devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7]);
//			(unsigned int)devaddr[0],(unsigned int)devaddr[1],(unsigned int)devaddr[2],(unsigned int)devaddr[3],(unsigned int)devaddr[4],(unsigned int)devaddr[5],(unsigned int)devaddr[6],(unsigned int)devaddr[7]);
		return;
	}
	bk_printf("insertArray - ds18_count=%i  -- adding device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ",ds18_count,
		devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7]);
//		(unsigned int)devaddr[0],(unsigned int)devaddr[1],(unsigned int)devaddr[2],(unsigned int)devaddr[3],(unsigned int)devaddr[4],(unsigned int)devaddr[5],(unsigned int)devaddr[6],(unsigned int)devaddr[7]);

	for (int i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device, no need to reenter
			a->GPIO[i]=DS_GPIO; 	// just to be sure - maybe device is on other GPIO now?!?
			bk_printf("device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X was allready present - just (re-)setting GPIO to %i",
		devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7],DS_GPIO);
			return;
		}
	}

	for (int i = 0; i < 8; i++) {
		a->array[ds18_count][i] = devaddr[i];
	}
	sprintf(a->name[ds18_count],"Sensor %i",ds18_count);
	a->lasttemp[ds18_count] = -127;
	a->last_read[ds18_count] = 0;
	a->channel[ds18_count] = -1;
	a->GPIO[ds18_count]=DS_GPIO;
	ds18_count++;
}

// search DS18B20 devices on one GPIO pin
int DS18B20_fill_devicelist(int Pin)
{
	DeviceAddress devaddr;
	int ret=0;
#if WINDOWS
	// For Windows add some "fake" sensors with increasing addresses
	// 28 FF AA BB CC DD EE 01, 28 FF AA BB CC DD EE 02, ...
	devaddr[0]=0x28; devaddr[1]=0xFF; devaddr[2]=0xAA; devaddr[3]=0xBB;devaddr[4]=0xCC;devaddr[5]=0xDD;devaddr[6]=0xEE;
	while (ds18_count < 1+(DS18B20MAX/4) ){
		ret++;
		devaddr[7]=ret;
		bk_printf("found device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ",
			devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7]);
		insertArray(&ds18b20devices,devaddr);
		ret++;
	}
#else
	reset_search();
	while (search(devaddr,1,Pin) && ds18_count < DS18B20MAX ){
		ret++;
		bk_printf("found device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ",
			devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7]);
		insertArray(&ds18b20devices,devaddr);
		ret++;
	}
#endif
	return ret;
};

// a sensor should have a human readable name to distinguish sensors
int DS18B20_set_devicename(DeviceAddress devaddr,char *name)
{
	int i=0;
	for (i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device
			if (strlen(name)<DS18B20namel) sprintf(ds18b20devices.name[i],name);
			return 1;
		}
	}
	bk_printf("didn't find device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - inserting",
			devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7]);
	insertArray(&ds18b20devices,devaddr);
	// if device was inserted (array not full) now the new device is the last one at position ds18_count-1
	if (! memcmp(devaddr,ds18b20devices.array[ds18_count-1],8)){	// found device
			if (strlen(name)<DS18B20namel) sprintf(ds18b20devices.name[i],name);
			return 1;
	}
	return 0;
};

// a sensor can have a channel assigned to send temparatures via MQTT
int DS18B20_set_channel(DeviceAddress devaddr,int c)
{
	int i=0;
	for (i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device
			ds18b20devices.channel[i]=c;
			return 1;
		}
	}
	bk_printf("didn't find device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - inserting",
			devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7]);
	insertArray(&ds18b20devices,devaddr);
	// if device was inserted (array not full) now the new device is the last one at position ds18_count-1
	if (! memcmp(devaddr,ds18b20devices.array[ds18_count-1],8)){	// found device
			ds18b20devices.channel[ds18_count-1]=c;
			return 1;
	}
	return 0;
};

// convert a string with sensor address to "DeviceAddress"
int devstr2DeviceAddr(DeviceAddress *devaddr, char *dev){
	char *p=dev;
	DeviceAddress daddr;
	int s;
	
#if PLATFORM_W600 || PLATFORM_LN882H || PLATFORM_RTL87X0C		
// this platforms won't allow sscanf of %hhx, so we need to use %x/%X and hence we need temporary unsigned ints ...
// test and add new platforms if needed
	unsigned int t[8];
	s = sscanf(dev,"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", &t[0],&t[1],&t[2],&t[3],&t[4],&t[5],&t[6],&t[7]);
	for (int i=0; i<8; i++) daddr[i]=(uint8_t)t[i];
#else
	s = sscanf(dev,"0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx",
		&daddr[0],&daddr[1],&daddr[2],&daddr[3],&daddr[4],&daddr[5],&daddr[6],&daddr[7]);
#endif
	if ( s!=8 ) {
		bk_printf("devstr2DeviceAddr -  conversion failed (converted %i)",s);
		return 0;
	}
	memcpy(*devaddr,daddr,8);
	return 1;
}

commandResult_t CMD_DS18B20_setsensor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if(Tokenizer_GetArgsCount()<=1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	const char *dev = Tokenizer_GetArg(0);
	const char *name = Tokenizer_GetArg(1);
	DeviceAddress devaddr;
	if (devstr2DeviceAddr(&devaddr,dev)){
		DS18B20_set_devicename(devaddr,name);
		if (Tokenizer_GetArgsCount() >= 2 && Tokenizer_IsArgInteger(2)){
			DS18B20_set_channel(devaddr,Tokenizer_GetArgInteger(2));
		}

		return CMD_RES_OK;
	}
	else return CMD_RES_ERROR;
}

// startDriver DS18B20 [conversionPeriod (seconds) - default 15]
void DS18B20_driver_Init()
{
		ds18_count=0;
		reset_search();
//bk_printf("DS18B20_driver_Init() ... \r\n");
	int i,j=0;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
//	bk_printf(" ... i=%i",i);
		if ((g_cfg.pins.roles[i] == IOR_DS1820_IO) && ( j < DS18B20MAX_GPIOS)){
//		bk_printf(" ... i=%i + j=%i ",i,j);
			DS18B20GPIOS[j++]=i;
			ds18b20_init(i);	// will set  DS_GPIO to i;
//		bk_printf(" ...DS18B20_fill_devicelist(%i)\r\n",i);
			DS18B20_fill_devicelist(i);

		}
	}
	// fill unused "pins" with 99 as sign for unused
	for (;j<DS18B20MAX_GPIOS;j++) DS18B20GPIOS[j]=99;
		bk_printf("DS18B20_driver_Init() after GPIO-fill ... \r\n");
	ds18_conversionPeriod = Tokenizer_GetArgIntegerDefault(1, 15);
	lastconv = 0;

	//cmddetail:{"name":"DS18B20_setsensor","args":"DS18B20-Addr name [channel]",
	//cmddetail:"descr":"Sets a name [and optional channel] to a DS18B20 sensor by sensors address",
	//cmddetail:"fn":"NULL);","file":"driver/ds18b20.c","requires":"",
	//cmddetail:"examples":"DS18B20_setsensor \"0x28 0x01 0x02 0x03 0x04 0x05 0x06 0x07\" \"kitchen\" 2"}
	CMD_RegisterCommand("DS18B20_setsensor", CMD_DS18B20_setsensor, NULL);

};

void DS18B20_AppendInformationToHTTPIndexPage(http_request_t* request)

{
	hprintf255(request, "<h5>DS18B20 devices detected/configured: %i</h5><table><th width='25'>Name</th>"
		"<th width='38'> &nbsp; Address </th><th width='10'> Temp </th><th width='10'> read </th>",ds18_count);
	for (int i=0; i < ds18_count; i++) {
		char tmp[50];
		if (ds18b20devices.lasttemp[i] > -127){
			sprintf(tmp,"%0.2f</td><td>%i s ago",(ds18b20devices.lasttemp[i]),ds18b20devices.last_read[i]);
		}
		else {
			 sprintf(tmp, " -- </td><td> --");
		}
		hprintf255(request, "<tr><td>%s</td>"
		"<td> &nbsp; %02X %02X %02X %02X %02X %02X %02X %02X</td>"
		"<td>%s</td></tr>",ds18b20devices.name[i],
		ds18b20devices.array[i][0],ds18b20devices.array[i][1],ds18b20devices.array[i][2],ds18b20devices.array[i][3],
		ds18b20devices.array[i][4],ds18b20devices.array[i][5],ds18b20devices.array[i][6],ds18b20devices.array[i][7],
		tmp);
	}
	hprintf255(request, "</table>");
}

#include "../httpserver/http_fns.h"

int http_fn_cfg_ds18b20(http_request_t* request)
{
	char tmp[64], tmp2[64];
	int g_changes = 0;

	for (int i=0; i < ds18_count; i++) { 
		sprintf(tmp2,"ds1820name%i",i);
		if (http_getArg(request->url, tmp2, tmp, sizeof(tmp))) {
			DS18B20_set_devicename(ds18b20devices.array[i],tmp);
		}
		sprintf(tmp2,"ds1820chan%i",i);
		if (http_getArg(request->url, tmp2, tmp, sizeof(tmp))) {
			int c=atoi(tmp);
			DS18B20_set_channel(ds18b20devices.array[i],c);
		}

	}

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "DS18B20");


//	poststr_h2(request, "Here you can configure DS18B20 sensors detected or configured");
	hprintf255(request, "<h2>Here you can configure DS18B20 sensors detected or cinfigured</h2><h5>Configure DS18B20 devices detected</h5><form action='/cfg_ds18b20'>");
	hprintf255(request, "<table><tr><th width='38'>Sensor Adress</th><th width='25'> &nbsp; Name </th><th width='10'>Channel</th></tr>");
	for (int i=0; i < ds18_count; i++) {
		hprintf255(request, "<tr><td id='a%i'>0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X</td>"
		"<td>&nbsp; <input name='ds1820name%i' id='ds1820name%i' value='%s'></td>"
		"<td>&nbsp; <input name='ds1820chan%i' id='ds1820chan%i' value='%i'></td></tr>",i,
		ds18b20devices.array[i][0],ds18b20devices.array[i][1],ds18b20devices.array[i][2],ds18b20devices.array[i][3],
		ds18b20devices.array[i][4],ds18b20devices.array[i][5],ds18b20devices.array[i][6],ds18b20devices.array[i][7],
		i,i,ds18b20devices.name[i],
		i,i,ds18b20devices.channel[i]);
	}
	hprintf255(request, "</table>");
		

	poststr(request, "<br><input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? ')\"></form> ");
	hprintf255(request, "<script> function gen(){v='backlog startDriver DS18B20 '; for (i=0; i<%i; i++) { ",ds18_count);
	poststr(request, "v+='; DS18B20_setsensor ' + '\"' + getElement('a'+i).innerHTML + '\" \"' + getElement('ds1820name'+i).value + '\" \"' + getElement('ds1820chan'+i).value + '\" '} return v; };</script>");
	poststr(request,"<br><br><input type='button' value='generate backlog command for config' onclick='t=document.getElementById(\"text\"); t.value=gen(); t.hidden=false;'><textarea id='text' hidden '> </textarea><p>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;	
	
	
	
	
	
}

bool ds18b20_used_channel(int ch) {
	for (int i=0; i < ds18_count; i++) {
		if (ds18b20devices.channel[i] == ch) {
				return true;
		}
	}
	return false;
};

void DS18B20_OnEverySecond()
{
	// for now just find the pin used
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, 99);
	uint8_t scratchpad[9], crc;
	bk_printf("DS18B20_OnEverySecond ... ");
	DS1820_LOG(DEBUG, "DS18B20_OnEverySecond ...");
	if(Pin != 99) 	// so there is at least one Pin defined
	{
//		if (ds18_count == 0) DS18B20_fill_devicelist();
		// only if pin is set
		// request temp if conversion was requested two seconds after request
		// if (dsread == 1 && g_secondsElapsed % 5 == 2) {
		// better if we don't use parasitic power, we can check if conversion is ready
		bk_printf(".. Pin %i found! dsread=%i  -- isConversionComplete()=%i\r\n",Pin,dsread,isConversionComplete());
		DS1820_LOG(INFO, ".. Pin %i found! dsread=%i  -- isConversionComplete()=%i\r\n",Pin,dsread,isConversionComplete());
		if(dsread == 1 
#if WINDOWS
){

			bk_printf("Reading temperature from fake DS18B20 sensor(s)\r\n");
			for (int i=0; i < ds18_count; i++) {
				ds18b20devices.last_read[i] += 1 ;
				errcount = 0;
				float t = 20.0 + 0.1*(g_secondsElapsed%20 -10);
					ds18b20devices.lasttemp[i] = t;
					ds18b20devices.last_read[i] = 0;
					if (ds18b20devices.channel[i]>=0) CHANNEL_Set(ds18b20devices.channel[i], t, CHANNEL_SET_FLAG_SILENT);
					lastconv = g_secondsElapsed;
			}
			dsread=0;
#else
		&& isConversionComplete())
		{
			float t = -127;
			bk_printf("Reading temperature from %i DS18B20 sensor(s)\r\n",ds18_count);
			DS1820_LOG(INFO, "Reading temperature from %i DS18B20 sensor(s)\r\n",ds18_count);
			for (int i=0; i < ds18_count; i++) {
				ds18b20devices.last_read[i] += 1 ;
				errcount = 0;
				t = -127;
				while ( t == -127 && errcount++ < 5){
					t = ds18b20_getTempC((const DeviceAddress*)ds18b20devices.array[i]);
					bk_printf("Device %i (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X) reported %0.2f\r\n",i,
						ds18b20devices.array[i][0],ds18b20devices.array[i][1],ds18b20devices.array[i][2],ds18b20devices.array[i][3],
						ds18b20devices.array[i][4],ds18b20devices.array[i][5],ds18b20devices.array[i][6],ds18b20devices.array[i][7],t);
				}
				if (t != -127){
					ds18b20devices.lasttemp[i] = t;
					ds18b20devices.last_read[i] = 0;
					if (ds18b20devices.channel[i]>=0) CHANNEL_Set(ds18b20devices.channel[i], t, CHANNEL_SET_FLAG_SILENT);
					lastconv = g_secondsElapsed;
				} else{
					if (ds18b20devices.last_read[i] > 60) {
						bk_printf("No temperature read for over 60 seconds for"
							" device %i (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)! Setting to -127°C!\r\n",i,
							ds18b20devices.array[i][0],ds18b20devices.array[i][1],ds18b20devices.array[i][2],
							ds18b20devices.array[i][3],ds18b20devices.array[i][4],ds18b20devices.array[i][5],
							ds18b20devices.array[i][6],ds18b20devices.array[i][7]);
						ds18b20devices.lasttemp[i] = -127;
						dsread=0;
					}
				}

			}
			dsread=0;
#endif		
		}
		else{
			bk_printf("No tepms read! dsread=%i  -- isConversionComplete()=%i\r\n",dsread,isConversionComplete());
			DS1820_LOG(INFO, "No tepms read! dsread=%i  -- isConversionComplete()=%i\r\n",dsread,isConversionComplete());
			for (int i=0; i < ds18_count; i++) {
				ds18b20devices.last_read[i] += 1 ;
			}
			if(dsread == 0 && (g_secondsElapsed % ds18_conversionPeriod == 0 || lastconv == 0))
			{
				ds18b20_requestTemperatures();
				dsread = 1;
				errcount = 0;
			}
		}
	}else {bk_printf("No Pin found\r\n");DS1820_LOG(INFO, "No Pin found\r\n"); }
}
#endif

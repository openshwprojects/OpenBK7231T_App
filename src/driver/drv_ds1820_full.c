#include "../obk_config.h"

#if (ENABLE_DRIVER_DS1820_FULL)

#include "drv_ds1820_common.h"
#include "drv_ds1820_full.h"

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

#define DEVSTR		"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
#define DEV2STR(T)	T[0],T[1],T[2],T[3],T[4],T[5],T[6],T[7]
#define SPSTR		DEVSTR " 0x%02X "
#define SP2STR(T)	DEV2STR(T),T[8]



static uint8_t dsread = 0;
static int Pin = -1;
//static int t = -127;
static int errcount = 0;
static int lastconv; // secondsElapsed on last successfull reading
static uint8_t ds18_family = 0;
static int ds18_conversionPeriod = 0;

typedef uint8_t ScratchPad[9];
typedef uint8_t DeviceAddress[8];		// we need to distinguish sensors by their address

typedef struct {
  DeviceAddress array[DS18B20MAX];
  uint8_t index;
  char name[DS18B20MAX][DS18B20namel];
  float lasttemp[DS18B20MAX];
  unsigned short last_read[DS18B20MAX];
  short channel[DS18B20MAX];
  short GPIO[DS18B20MAX];

} DS1820devices;

static int ds18_count = 0;		// detected number of devices

uint8_t DS18B20_GPIO;	// the actual GPIO used (changes in case we have multiple GPIOs defined ...)

DeviceAddress ROM_NO;

static DS1820devices ds18b20devices;
uint8_t DS18B20GPIOS[DS18B20MAX_GPIOS];
uint8_t devices = 0;


uint8_t LastDiscrepancy;
uint8_t LastFamilyDiscrepancy;
bool LastDeviceFlag;

#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820 - " fmt, ##__VA_ARGS__)

// prototypes
int devstr2DeviceAddr(uint8_t *devaddr, const char *dev);
bool ds18b20_getGPIO(const uint8_t* devaddr,uint8_t *GPIO);
bool ds18b20_readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad);



bool ds18b20_writeScratchPad(const uint8_t *deviceAddress, const uint8_t *scratchPad) {
	if ( ! ds18b20_getGPIO(deviceAddress, &DS18B20_GPIO ) ) {
		DS1820_LOG(DEBUG, "No GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
		return false;
	}
	DS1820_LOG(DEBUG, "GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
	// send the reset command and fail fast
	int b = OWReset(DS18B20_GPIO);
	if (b == 0) {
//		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
	ds18b20_select(deviceAddress);
	OWWriteByte(DS18B20_GPIO,WRITE_SCRATCHPAD);
	OWWriteByte(DS18B20_GPIO,scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
	OWWriteByte(DS18B20_GPIO,scratchPad[LOW_ALARM_TEMP]); // low alarm temp
	OWWriteByte(DS18B20_GPIO,scratchPad[CONFIGURATION]);
	DS1820_LOG(DEBUG, "Wrote scratchpad " SPSTR , SP2STR(scratchPad));
	return (bool)OWReset(DS18B20_GPIO);
}



static commandResult_t Cmd_DS18B20_SetResolution(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int arg = Tokenizer_GetArgInteger(0);
	if (arg > 12 || arg < 9)
		return CMD_RES_BAD_ARGUMENT;
	// arg can be 9 10 11 12
	// the newValue shall be 9:0x1F 10:0x3F 11:0x5F 12:0x7F
	// ignoring the "0x0F" an lookig just at the first digit, 
	// we can easily see this is 1+(arg-9)*2
	// so this can be written as 
	uint8_t newValue = 0x1F | (arg-9)<<5 ;
	DS1820_LOG(DEBUG, "DS1820_SetResolution: newValue=0x%02X ", newValue);	
	ScratchPad scratchPad;
	if (Tokenizer_GetArgsCount() > 1){
		const char *dev = Tokenizer_GetArg(1);
		DeviceAddress devaddr={0};
		if (! devstr2DeviceAddr(devaddr,(const char*)dev)){
			DS1820_LOG(ERROR, "DS1820_SetResolution: device not found");
			return CMD_RES_BAD_ARGUMENT;
		}
		if (devaddr[0] != 0x28) {
			DS1820_LOG(ERROR, "DS1820_SetResolution not supported by sensor family 0x%02X",devaddr[0]);
			return CMD_RES_UNKNOWN_COMMAND;
		}

		if (ds18b20_isConnected((uint8_t*)devaddr, scratchPad)){
			// if it needs to be updated we write the new value
			DS1820_LOG(DEBUG, "Read scratchpad " SPSTR , SP2STR(scratchPad));
			if (scratchPad[CONFIGURATION] != newValue) {
				scratchPad[CONFIGURATION] = newValue;
				DS1820_LOG(DEBUG, "Going to write scratchpad " SPSTR , SP2STR(scratchPad));
				if (! ds18b20_writeScratchPad((uint8_t*)devaddr, scratchPad)){
//					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad for sensor " DEVSTR ,DEV2STR((uint8_t*)devaddr));
					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad for sensor " DEVSTR ,DEV2STR(devaddr));
				}
			}
		}
		else {
					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to read scratchpad!");	
		}
	 }
	 else {
		for (int i=0; i < ds18_count; i++) {
//			DS1820_LOG(DEBUG, "DS1820_SetResolution: setting sensor " DEVSTR ,DEV2STR((uint8_t*)ds18b20devices.array[i]));
			DS1820_LOG(DEBUG, "DS1820_SetResolution: setting sensor " DEVSTR ,DEV2STR(ds18b20devices.array[i]));
			if (ds18b20devices.array[i][0] != 0x28) {
				DS1820_LOG(ERROR, "DS1820_SetResolution not supported by sensor family 0x%02X",ds18b20devices.array[i][0]);
				return CMD_RES_UNKNOWN_COMMAND;
			}
			if (ds18b20_isConnected((uint8_t*)ds18b20devices.array[i], scratchPad)) {
				if (scratchPad[CONFIGURATION] != newValue) {
					scratchPad[CONFIGURATION] = newValue;
					if (! ds18b20_writeScratchPad((uint8_t*)ds18b20devices.array[i], scratchPad)){
//						DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad for sensor " DEVSTR ,DEV2STR((uint8_t*)ds18b20devices.array[i]));
						DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad for sensor " DEVSTR ,DEV2STR(ds18b20devices.array[i]));
					}
				}
			}
			else {
//				DS1820_LOG(ERROR, "DS1820_SetResolution: couldn't connect to sensor " DEVSTR ,DEV2STR((uint8_t*)ds18b20devices.array[i]));
				DS1820_LOG(ERROR, "DS1820_SetResolution: couldn't connect to sensor " DEVSTR ,DEV2STR(ds18b20devices.array[i]));
			}
		}
	}


	//temperature conversion was interrupted
	dsread = 0;

	return CMD_RES_OK;
}

void ds18b20_requestConvertT(int GPIO) {
	OWReset(GPIO);
	OWWriteByte(GPIO,SKIP_ROM);
	OWWriteByte(GPIO,CONVERT_T);
}




// use same code to read scatchpad and test CRC in one
// parameter beside scratchPad to fill is
//	 only GPIO pin for single DS1820
//	 sensor address for multiple devices
bool ds18b20_readScratchPad(const uint8_t *deviceAddress, uint8_t* scratchPad) {
	if ( ! ds18b20_getGPIO(deviceAddress, &DS18B20_GPIO ) ) {
		DS1820_LOG(DEBUG, "No GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
		return false;
	}
	DS1820_LOG(DEBUG, "GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
	// send the reset command and fail fast
	int b = OWReset(DS18B20_GPIO);
	if (b == 0) {
//		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
	ds18b20_select(deviceAddress);
	OWWriteByte(DS18B20_GPIO, READ_SCRATCHPAD);
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
		scratchPad[i] = OWReadByte(DS18B20_GPIO);
	}
	uint8_t crc = Crc8CQuick(scratchPad, 8);
	if(crc != scratchPad[8])
	{
		DS1820_LOG(ERROR, "Read CRC=%x != calculated:%x (errcount=%i)", scratchPad[8], crc, errcount);
		DS1820_LOG(ERROR, "Scratchpad Data Read: " SPSTR,
			SP2STR(scratchPad));

		return false;
	}

	return OWReset(DS18B20_GPIO);
}


float ds18b20_Scratchpat2TempC(const ScratchPad scratchPad, uint8_t family) {
	int16_t raw = (int16_t)scratchPad[TEMP_MSB] << 8 | (int16_t)scratchPad[TEMP_LSB];
	if(family == 0x10)
	{
		// DS18S20 or old DS1820
		int16_t dT = 128 * (scratchPad[7] - scratchPad[6]);
		dT /= scratchPad[7];
		raw = 64 * (raw & 0xFFFE) - 32 + dT;
		DS1820_LOG(DEBUG, "family=%x, raw=%i, count_remain=%i, count_per_c=%i, dT=%i", family, raw, scratchPad[6], scratchPad[7], dT);
	}
	else
	{ // DS18B20
	
		// Bits 6 and 5 of CFG byte will contain R1 R0 representing the resolution:
		// 00	 9 bits
		// 01 	10 bits
		// 10	11 bits
		// 11	12 bits
		//
		// might be read as "how many from the last three bits are valid? 0 for 9bits, all 3 for 12 bits
		// we want the unused bits to be 0, so we need to mask out ~ R1R0 bits ( 0 for 12bit ... 3 for 9 bit)
		// so here's what we do:
		// get bits 6 and five ( & 0x60) and push them 5 bits right --> so we have 0 0 0 0 0 0 R1 R0 --> decimal 0 (for 9 bits) to 3 (for 12 bts)
		// we want to mask the bits, so substract from 3 --> decimal 3 (=3-0 for 9 bits) to 0 (=3-3 for 12 bits)
		// to mask out / delete x bits we use 		& ((1 << x ) -1)
		//  
		uint8_t maskbits = 3 - ((scratchPad[4] & 0x60) >> 5) ;	// maskbits will be number of bit to zero out
		raw = raw & ~ ( (1 << maskbits) -1) ;
//		DS1820_LOG(DEBUG, "maskbits=%x, raw=%i", maskbits, raw);
		raw = raw << 3; // multiply by 8
		DS1820_LOG(DEBUG, "ds18b20_Scratchpat2TempC: family=%x, raw=%i (%i bit resolution)", family, raw, 12 - maskbits );
	}

	// Raw is t * 128
/*
	t = (raw / 128) * 100; // Whole degrees
	int frac = (raw % 128) * 100 / 128; // Fractional degrees
	t += t > 0 ? frac : -frac;
*/
//	t = (raw * 100) / 128;	// no need to calc fractional, if we can simply get 100 * temp in one line
	if (raw <= DEVICE_DISCONNECTED_C * 128)
		return DEVICE_DISCONNECTED_C;
	// C = RAW/128
	return (float)raw / 128.0f;
}


bool ds18b20_used_channel(int ch) {
	for (int i=0; i < ds18_count; i++) {
		if (ds18b20devices.channel[i] == ch) {
				return true;
		}
	}
	return false;
};

bool ds18b20_getGPIO(const uint8_t* devaddr,uint8_t *GPIO)
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

bool ds18b20_select(const uint8_t *address) {
	if ( ! ds18b20_getGPIO(address, &DS18B20_GPIO ) ) return false;
	uint8_t i;
	OWWriteByte(DS18B20_GPIO, SELECT_DEVICE);           // Choose ROM
	for (i = 0; i < 8; i++) OWWriteByte(DS18B20_GPIO, ((uint8_t *)address)[i]);
	return true;
}

bool ds18b20_isAllZeros(const uint8_t * const scratchPad) {
	for (size_t i = 0; i < 9; i++) {
		if (scratchPad[i] != 0) {
			return false;
		}
	}
	return true;
}

bool ds18b20_isConnected(const uint8_t *deviceAddress, uint8_t *scratchPad) {
	// no need to check for GPIO of device here, ds18b20_readScratchPad() will do so and return false, if not
	bool b = ds18b20_readScratchPad((const uint8_t *)deviceAddress, scratchPad);
	DS1820_LOG(DEBUG, "ds18b20_readScratchPad returned %i - Crc8CQuick=%i / scratchPad[SCRATCHPAD_CRC]=%i",b,Crc8CQuick(scratchPad, 8),scratchPad[SCRATCHPAD_CRC]);
	return b && !ds18b20_isAllZeros(scratchPad) && (Crc8CQuick(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}


float ds18b20_getTempC(const uint8_t *dev) {
	ScratchPad scratchPad;
	if (ds18b20_isConnected((const uint8_t *)dev, scratchPad)) {
		return ds18b20_Scratchpat2TempC(scratchPad, (uint8_t)dev[0]);	// deviceAddress[0] equals to "family" (0x28 for DS18B20)
	} else DS1820_LOG(DEBUG, "Device not connected");
	return DEVICE_DISCONNECTED_C;
}


bool isConversionComplete() {
	return DS1820TConversionDone(DS18B20_GPIO);
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
			OWWriteByte(Pin,SEARCH_ROM);   // search all devices
		}
		else {
			OWWriteByte(Pin,ALARM_SEARCH);   // search for devices with alarm only
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




// add a new device into DS1820devices
void insertArray(DS1820devices *a, DeviceAddress devaddr) {
	if (ds18_count >= DS18B20MAX){
		bk_printf("insertArray ERROR:Arry is full, can't add device " DEVSTR " ",
			DEV2STR(devaddr));
		return;
	}
	bk_printf("insertArray - ds18_count=%i  -- adding device " DEVSTR " ",ds18_count,
		DEV2STR(devaddr));

	for (int i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device, no need to reenter
			a->GPIO[i]=DS18B20_GPIO; 	// just to be sure - maybe device is on other GPIO now?!?
			bk_printf("device " DEVSTR " was allready present - just (re-)setting GPIO to %i",
		DEV2STR(devaddr),DS18B20_GPIO);
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
	a->GPIO[ds18_count]=DS18B20_GPIO;
	ds18_count++;
}

// search DS18B20 devices on one GPIO pin
int DS18B20_fill_devicelist(int Pin)
{
	DeviceAddress devaddr={0};
	int ret=0;
#if WINDOWS
	// For Windows add some "fake" sensors with increasing addresses
	// 28 FF AA BB CC DD EE 01, 28 FF AA BB CC DD EE 02, ...
	devaddr[0]=0x28; devaddr[1]=0xFF; devaddr[2]=0xAA; devaddr[3]=0xBB;devaddr[4]=0xCC;devaddr[5]=0xDD;devaddr[6]=0xEE;
	while (ds18_count < 1+(DS18B20MAX/2) ){
		devaddr[7]=++ret;
		bk_printf("found device " DEVSTR " ",
			DEV2STR(devaddr));
		insertArray(&ds18b20devices,devaddr);
	}
#else
	reset_search();
	while (search(devaddr,1,Pin) && ds18_count < DS18B20MAX ){
		bk_printf("found device " DEVSTR" ",
			DEV2STR(devaddr));
		insertArray(&ds18b20devices,devaddr);
		ret++;
	}
#endif
	return ret;
};

// a sensor must have a GPIO assigned
int DS18B20_set_deviceGPIO(DeviceAddress devaddr,const int pin)
{
	int i=0;
	for (i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device
			ds18b20devices.GPIO[i]=pin;
			return 1;
		}
	}
	bk_printf("didn't find device " DEVSTR " - inserting",
			DEV2STR(devaddr));
	insertArray(&ds18b20devices,devaddr);
	// if device was inserted (array not full) now the new device is the last one at position ds18_count-1
	if (! memcmp(devaddr,ds18b20devices.array[ds18_count-1],8)){	// found device
			ds18b20devices.GPIO[i]=pin;
			return 1;
	}
	return 0;
};


// a sensor should have a human readable name to distinguish sensors
int DS18B20_set_devicename(DeviceAddress devaddr,const char *name)
{
	int i=0;
	for (i=0; i < ds18_count; i++) {
		if (! memcmp(devaddr,ds18b20devices.array[i],8)){	// found device
			if (strlen(name)<DS18B20namel) sprintf(ds18b20devices.name[i],name);
			return 1;
		}
	}
	bk_printf("didn't find device " DEVSTR " - inserting",
			DEV2STR(devaddr));
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
	bk_printf("didn't find device " DEVSTR " - inserting",
			DEV2STR(devaddr));
	insertArray(&ds18b20devices,devaddr);
	// if device was inserted (array not full) now the new device is the last one at position ds18_count-1
	if (! memcmp(devaddr,ds18b20devices.array[ds18_count-1],8)){	// found device
			ds18b20devices.channel[ds18_count-1]=c;
			return 1;
	}
	return 0;
};


// Helper to check for a valid hex character
int is_valid_hex_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// convert a string with sensor address to "DeviceAddress"
// valid input is 
// "0x28 0x01 0x02 0x03 0x04 0x05 0x06 0x07" or
// "2801020304050607"
// additional whitespace is ignored, also any input after 8 bytes (warning is printed)
int devstr2DeviceAddr(uint8_t *devaddr, const char *dev){
    // Clear the address array
    memset(devaddr, 0, 8);
    int count = 0; // Count of valid hex values read
    char hex_pair[3] = {0}; // Buffer to hold two hex digits

    for (int i = 0; dev[i] != '\0'; i++) {
        // Skip whitespace
        if (isspace((unsigned char)dev[i])) {
            continue;
        }
        if (count >= 8) {	// we already found 8 address byte, but input is not empty !
		DS1820_LOG(WARN, "devstr2DeviceAddr: WARNING additional input after 8th address byte ('%s') is ignored. Using only %.*s",&dev[i],i,dev);
		return count;
        }
        // Check for "0x" or "0X"
        if (dev[i] == '0' && (dev[i + 1] == 'x' || dev[i + 1] == 'X')) {
            i++; // Skip both '0' and 'x'
            // Read the next two hex digits
            if (is_valid_hex_char(dev[i + 1]) && is_valid_hex_char(dev[i + 2])) {
                hex_pair[0] = dev[i + 1];
                hex_pair[1] = dev[i + 2];
                devaddr[count++] = (uint8_t)strtol(hex_pair, NULL, 16); // Convert to uint8_t
                i += 2; // advance the two hex digits
            } else {
                return -1; // Error: Invalid hex sequence
            }
        } else if (is_valid_hex_char(dev[i])) {
            // If a valid hex character is found, read the next character
            if (is_valid_hex_char(dev[i + 1])) {
                hex_pair[0] = dev[i];
                hex_pair[1] = dev[i + 1];
                devaddr[count++] = (uint8_t)strtol(hex_pair, NULL, 16); // Convert to uint8_t
                i++; // Move to the next hex character
            } else {
                return -1; // Error: Incomplete hex pair
            }
        } else {
            return -1; // Error: Invalid character found
        } 
    }
    // Return the number of valid hex values read
    return count; // Success
}

commandResult_t CMD_DS18B20_setsensor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if(Tokenizer_GetArgsCount()<=1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	const char *dev = Tokenizer_GetArg(0);
//	int gpio = Tokenizer_IsArgInteger(1) ? Tokenizer_GetArgInteger(1) : HAL_PIN_Find(Tokenizer_GetArg(1));
	int gpio = Tokenizer_GetPin(1,-1);
	if (gpio < 0) {
		DS1820_LOG(ERROR, "DS18B20_setsensor: failed to find GPIO for '%s' (returned index: %i)",Tokenizer_GetArg(1),gpio);
		return CMD_RES_ERROR;
	}
	const char *name = Tokenizer_GetArg(2);
	DeviceAddress devaddr={0};
	if (devstr2DeviceAddr(devaddr,(const char*)dev)){
		DS18B20_set_deviceGPIO(devaddr,gpio);
		DS18B20_set_devicename(devaddr,name);
		if (Tokenizer_GetArgsCount() >= 3 && Tokenizer_IsArgInteger(3)){
			DS18B20_set_channel(devaddr,Tokenizer_GetArgInteger(3));
		}

		return CMD_RES_OK;
	}
	else return CMD_RES_ERROR;
}


void scan_sensors(){
	ds18_count=0;
	reset_search();
	int i,j=0;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if ((g_cfg.pins.roles[i] == IOR_DS1820_IO) && ( j < DS18B20MAX_GPIOS)){
			DS18B20GPIOS[j++]=i;
			DS18B20_GPIO = i ;	// set DS18B20_GPIO to i
			DS18B20_fill_devicelist(i);
		}
	}
	// fill unused "pins" with 99 as sign for unused
	for (;j<DS18B20MAX_GPIOS;j++) DS18B20GPIOS[j]=99;
}

commandResult_t CMD_DS18B20_scansensors(const void *context, const char *cmd, const char *args, int cmdFlags) {
	scan_sensors();
	return CMD_RES_OK;
}


// startDriver DS1820_full [conversionPeriod (seconds) - default 15]
void DS1820_full_driver_Init()
{
	ds18_conversionPeriod = Tokenizer_GetArgIntegerDefault(1, 15);
	lastconv = 0;
	dsread = 0;
	ds18_family = 0;
	scan_sensors();

	//cmddetail:{"name":"DS1820_FULL_SetResolution","args":"[int]",
	//cmddetail:"descr":"Sets resolution for connected DS1820 sensor (9/10/11/12 bits)",
	//cmddetail:"fn":"Cmd_DS18B20_SetResolution","file":"driver/drv_ds1820_full.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DS1820_FULL_SetResolution", Cmd_DS18B20_SetResolution, NULL);
	//cmddetail:{"name":"DS1820_FULL_setsensor","args":"DS18B20-Addr name [channel]",
	//cmddetail:"descr":"Sets a name [and optional channel] to a DS18B20 sensor by sensors address",
	//cmddetail:"fn":"CMD_DS18B20_setsensor","file":"driver/drv_ds1820_full.c","requires":"",
	//cmddetail:"examples":"DS1820_FULL_setsensor \"0x28 0x01 0x02 0x03 0x04 0x05 0x06 0x07\" \"kitchen\" 2"}
	CMD_RegisterCommand("DS1820_FULL_setsensor", CMD_DS18B20_setsensor, NULL);
	//cmddetail:{"name":"DS1820_FULL_scansensors","args":"-",
	//cmddetail:"descr":"(Re-)Scan all GPIOs defined for DS1820 for sensors",
	//cmddetail:"fn":"CMD_DS18B20_scansensors","file":"driver/drv_ds1820_full.c","requires":"",
	//cmddetail:"examples":"DS1820_FULL_scansensors"}
	CMD_RegisterCommand("DS1820_FULL_scansensors", CMD_DS18B20_scansensors, NULL);

	// no need to discover the "family" we know the address, the first byte is the family

#if ENABLE_DS1820_TEST_US	
	init_TEST_US();
#endif


};


/*
		printer(request, "\"DS1820\":");
		// following check will clear NaN values
		printer(request, "{");
		printer(request, "\"Temperature\": %.1f,", chan_val1);
		// close ENERGY block
		printer(request, "},");

*/

static char *jsonSensor_reststr = NULL;
char *DS1820_full_jsonSensors(){
	if (ds18_count <= 0 ) return NULL;
	if (jsonSensor_reststr!=NULL) free(jsonSensor_reststr); 
	// {"DS18B20-<id>":{"Name":"<name - DS18B20namel>","Id":"0102030405060708","Temperature": <temp -127,00>},
	// 123456789012 123456789012  +    DS18B20namel  1234567890123456789012345678901234567890       1234567890        
	// length of str: 12 + 12 + DS18B20namel + 40 + 10 --> 74 + DS18B20namel  --> use 75 + DS18B20namel


	// Tasmota style:
	// {"DS18B20-XX":{"Id":"010203040506","Temperature":-XXX,X},
	// 123456789012345678901234567890123456789012345678901234567
	//          10        20        30        40        50
	// length of str: 57  --> use 60
	// Tasmota-ID:
	// middle 6 bytes of 8 byte ROM-Address in reverse order:
	//  ROM=2801020304050607  --> Id=060504030201
	//
	
	//	        char address[17];
	//        for (uint32_t j = 0; j < 6; j++) {
	//          sprintf(address+2*j, "%02X", ds18x20_sensor[index].address[6-j]);  // Skip sensor type and crc
	//        }


// for extended style	
//	int size = (75 + DS18B20namel) * ds18_count;

// for "plain" Tasmota style
	int size = 60 * ds18_count;


	char *str = (char *)malloc(size * sizeof(char));
	if (str == NULL) {
        	DS1820_LOG(ERROR, "Could not allocate memory for sensor string!!");
        	return NULL; // string allocation failed
        }
        
	str[0] = 0;
	for (int i=0; i < ds18_count; i++) {
// full extension - complete ROM address + name 
//		sprintf(tmp, "\"DS18B20-%i\":{\"Name\":\"%s\",\"Id\":\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"Temperature\": %.1f},",i,ds18b20devices.name[i],DEV2STR(ds18b20devices.array[i]),ds18b20devices.lasttemp[i]);


// extended Tasmoty style: Name + Tasmota-Id
/*
		char tmp[75 + DS18B20namel];
		sprintf(tmp, "\"DS18B20-%i\":{\"Name\":\"%s\",\"Id\":\"%02X%02X%02X%02X%02X%02X\",\"Temperature\": %.1f},",i+1,ds18b20devices.name[i],
			ds18b20devices.array[i][6],
			ds18b20devices.array[i][5],
			ds18b20devices.array[i][4],
			ds18b20devices.array[i][3],
			ds18b20devices.array[i][2],
			ds18b20devices.array[i][1],
			ds18b20devices.lasttemp[i]);
*/

// "Plain" Tasmota style - only ID 
		char tmp[60]; 
		sprintf(tmp, "\"DS18B20-%i\":{\"Id\":\"%02X%02X%02X%02X%02X%02X\",\"Temperature\": %.1f},",i+1,
			ds18b20devices.array[i][6],
			ds18b20devices.array[i][5],
			ds18b20devices.array[i][4],
			ds18b20devices.array[i][3],
			ds18b20devices.array[i][2],
			ds18b20devices.array[i][1],
			ds18b20devices.lasttemp[i]);
		strncat(str, tmp, size - strlen(str) - 1); // Concatenate to the main string
	}
        jsonSensor_reststr = str;
	return jsonSensor_reststr;
}

void DS1820_full_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState){
		hprintf255(request, "<h5>DS18B20 devices detected/configured: %i</h5>",ds18_count);
		return;
	}
		
	if (ds18_count > 0 ){
		hprintf255(request, "DS18B20 devices<table><th width='25'>Name</th>"
			"<th width='38'> &nbsp; Address </th><th width='8'> Pin </th><th width='6'> CH </th><th width='10'> Temp </th><th width='10'> read </th>",ds18_count);
		for (int i=0; i < ds18_count; i++) {
			const char * pinalias=NULL; 
			char gpioname[10];
			char tmp[50];
			pinalias = HAL_PIN_GetPinNameAlias(ds18b20devices.GPIO[i]);
			if (! pinalias ) {
				sprintf(gpioname,"GPIO %u",ds18b20devices.GPIO[i]);
				pinalias = gpioname;
			}
			if (ds18b20devices.lasttemp[i] > -127){
				sprintf(tmp,"%0.2f</td><td>%i s ago",(ds18b20devices.lasttemp[i]),ds18b20devices.last_read[i]);
			}
			else {
				 sprintf(tmp, " -- </td><td> --");
			}
			int ch=ds18b20devices.channel[i];
			char chan[5]=" - ";
			if (ch >=0) sprintf(chan,"%i",(uint8_t)ch);
			hprintf255(request, "<tr><td>%s</td>"
			"<td> &nbsp; %02X %02X %02X %02X %02X %02X %02X %02X</td>"
			"<td>%s</td><td>%s</td><td>%s</td></tr>",ds18b20devices.name[i],
			DEV2STR(ds18b20devices.array[i]),
			pinalias, chan, tmp);
		}
		hprintf255(request, "</table>");
	}
}

#include "../httpserver/http_fns.h"

int http_fn_cfg_ds18b20(http_request_t* request)
{
	char tmp[64], tmp2[64];

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
	hprintf255(request, "<h2>Configure DS18B20 Sensors</h2><form action='/cfg_ds18b20' onsubmit='return chkchann()&&sc();'>");
	hprintf255(request, "<table><tr><th width='38'>Sensor Address</th><th width='4'>Pin</th><th width='25'> &nbsp; Name </th><th width='5'>Channel</th></tr>");
	for (int i=0; i < ds18_count; i++) {
		int ch=ds18b20devices.channel[i];
		char chan[5]={0};
		if (ch >=0) sprintf(chan,"%i",(uint8_t)ch);
		hprintf255(request, "<tr><td id='a%i'>"DEVSTR"</td>"
		"<td id='pin%i'> %i</th>"
		"<td>&nbsp; <input name='ds1820name%i' id='ds1820name%i' value='%s'></td>",i,
		DEV2STR(ds18b20devices.array[i]),
		i,ds18b20devices.GPIO[i],
		i,i,ds18b20devices.name[i]);
		hprintf255(request, "<td>&nbsp; <input name='ds1820chan%i' id='ds1820chan%i' value='%s' onchange='chkchann()'></td></tr>",
		i,i,chan);
	}
	hprintf255(request, "</table>");
		

	poststr(request, "<br><input type=\"submit\" value=\"Submit\"></form>");
	poststr(request,"<br>Use multiline command <input type='checkbox' id='CB' onchange='gen()'><br><input type='button' value='generate command for config' onclick='gen(); ct.hidden=false;'><textarea id='text' hidden '> </textarea><p>");
	hprintf255(request, "<script>chanels=document.querySelectorAll('[name*=\"ds1820chan\"]');chanInUse=Array(%i).fill(0);usedCH=[",CHANNEL_MAX);
	for (int j = 0; j < CHANNEL_MAX-1; j++) {
		hprintf255(request,"%i,",(CHANNEL_IsInUse(j)&&!ds18b20_used_channel(j))?1:0);
	}
	hprintf255(request,"%i];",(CHANNEL_IsInUse(CHANNEL_MAX-1)&&!ds18b20_used_channel(CHANNEL_MAX-1))?1:0);
	poststr(request, "function chkchann(){ok=true;chanInUse.fill(0);chanels.forEach((e)=>{((i=parseInt(e.value))>=0)&&(chanInUse[i]++);});chanInUse.forEach((e,i)=>{if((e>0)&&((e>1)||(usedCH[i]!=0))){ok=false;alert('Channel '+i+' already in use!');}});return ok;};");
	poststr(request, "function sc(){chanels.forEach((e) => {(e.value.trim()=='')&&(e.value=-1);})};ct=document.getElementById('text');function gen(){v='';if(getElement('CB').checked){M=0;T='\\n'}else{M=1;T='; ';v+='backlog '};v+='alias sets DS1820_FULL_setsensor'+T+'startDriver DS1820_FULL';");
	hprintf255(request, "for (i=0;i<%i;i++){",ds18_count);
	poststr(request, "v+=T+'sets '+getElement('a'+i).innerHTML.replaceAll(/[ ]*0[xX]/g,'')+' '+getElement('pin'+i).innerHTML+' \"'+getElement('ds1820name'+i).value+'\" '+getElement('ds1820chan'+i).value}ct.value=v;};</script>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;	
}

void DS1820_full_OnEverySecond()
{
	// for now just find the pin used
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, -1);

	// only if (at least one) pin is set
	if(Pin < 0) {
		DS1820_LOG(INFO, "No Pin found\r\n");
		return;
	}
	//Temperature measurement is done in two repeatable steps. 
	// Step 1 - dsread = 0. Sensor requested to do temperature conversion.
	//          That requires some time - 15-100-750ms, depending on sensor family/vendor.
	//          However, time between steps is always one second.
	// Step 2 - dsread = 1. Sensor finished conversion, requesting conversion result.

	// request temp if conversion was requested two seconds after request
	// if (dsread == 1 && g_secondsElapsed % 5 == 2) {
	// better if we don't use parasitic power, we can check if conversion is ready
		DS1820_LOG(DEBUG, ".. Pin %i found! dsread=%i \r\n",Pin,dsread);
		if(dsread == 1){ 
#if WINDOWS
			bk_printf("Reading temperature from fake DS18B20 sensor(s)\r\n");
			for (int i=0; i < ds18_count; i++) {
				ds18b20devices.last_read[i] += 1 ;
				errcount = 0;
				float t_float = 20.0 + i/10.0 + (float)(g_secondsElapsed%100)/100.0;
					ds18b20devices.lasttemp[i] = t_float;
					ds18b20devices.last_read[i] = 0;
					if (ds18b20devices.channel[i]>=0) CHANNEL_Set(ds18b20devices.channel[i], (int)(t_float*100), CHANNEL_SET_FLAG_SILENT);
					lastconv = g_secondsElapsed;
			}
#else	// to #if WINDOWS
			float t_float = -127;
			const char * pinalias; 
			char gpioname[10];
			DS1820_LOG(INFO, "Reading temperature from %i DS18B20 sensor(s)\r\n",ds18_count);
			for (int i=0; i < ds18_count; i++) {
				pinalias = HAL_PIN_GetPinNameAlias(ds18b20devices.GPIO[i]);
				if (! pinalias ) {
					sprintf(gpioname,"GPIO %u",ds18b20devices.GPIO[i]);
					pinalias = gpioname;
				}
				ds18b20devices.last_read[i] += 1 ;
				errcount = 0;
				t_float = -127;
				while ( t_float == -127 && errcount++ < 5){
					t_float = ds18b20_getTempC((const uint8_t*)ds18b20devices.array[i]);
					DS1820_LOG(DEBUG, "Device %i (" DEVSTR ") reported %0.2f\r\n",i,
						DEV2STR(ds18b20devices.array[i]),t_float);
				}
				if (t_float != -127){
					ds18b20devices.lasttemp[i] = t_float;
					ds18b20devices.last_read[i] = 0;
					if (ds18b20devices.channel[i]>=0) CHANNEL_Set(ds18b20devices.channel[i], (int)(t_float*100), CHANNEL_SET_FLAG_SILENT);
					lastconv = g_secondsElapsed;
					DS1820_LOG(INFO, "Sensor " DEVSTR " on %s reported %0.2f\r\n",DEV2STR(ds18b20devices.array[i]),pinalias,t_float);
				} else{
					if (ds18b20devices.last_read[i] > 60) {
						DS1820_LOG(ERROR, "No temperature read for over 60 seconds for"
							" device %i (" DEVSTR " on %s)! Setting to -127°C!\r\n",i,
							DEV2STR(ds18b20devices.array[i]),pinalias);
						ds18b20devices.lasttemp[i] = -127;
						dsread=0;
					}
				}
			}
#endif	// to #if WINDOWS		
			dsread=0;
		}
		else{
//			bk_printf("No tepms read! dsread=%i  -- isConversionComplete()=%i\r\n",dsread,isConversionComplete());
//			DS1820_LOG(INFO, "No tepms read! dsread=%i  -- isConversionComplete()=%i\r\n",dsread,isConversionComplete());
			for (int i=0; i < ds18_count; i++) {
				ds18b20devices.last_read[i] += 1 ;
			}
			if(dsread == 0 && (g_secondsElapsed % ds18_conversionPeriod == 0 || lastconv == 0))
			{
				DS1820_LOG(INFO, "Starting conversion");
				for (int i=0; i<DS18B20MAX_GPIOS; i++){
					ds18b20_requestConvertT(DS18B20GPIOS[i]);
				}
				dsread = 1;
				errcount = 0;
			}
		}


}

#endif // to #if (ENABLE_DRIVER_DS1820_FULL)

#include "drv_ds1820_simple.h"
#include "../hal/hal_generic.h"
#if PLATFORM_ESPIDF
#include "freertos/task.h"
#define noInterrupts() portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;taskENTER_CRITICAL(&mux)
#define interrupts() taskEXIT_CRITICAL(&mux)
#elif LINUX
#define noInterrupts() 
#define interrupts() 
#else
#include <task.h>
#define noInterrupts() taskENTER_CRITICAL()
#define interrupts() taskEXIT_CRITICAL()
#endif

static uint8_t dsread = 0;
static int Pin = -1;
static int t = -127;
static int errcount = 0;
static int lastconv; // secondsElapsed on last successfull reading
static uint8_t ds18_family = 0;
static int ds18_conversionPeriod = 0;

static int DS1820_DiscoverFamily();

#if (DS1820full)
typedef uint8_t DeviceAddress[8];		// we need to distinguish sensors by their address
typedef uint8_t ScratchPad[9];

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

// some OneWire commands
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


#endif

#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820[%i] - " fmt, Pin, ##__VA_ARGS__)

/*

timing numbers and general code idea from

https://www.analog.com/en/resources/technical-articles/1wire-communication-through-software.html

Parameter 	Speed 		Recommended (µs)
A 		Standard 	6
B 		Standard 	64
C 		Standard 	60
D 		Standard 	10
E 		Standard 	9
F 		Standard 	55
G 		Standard 	0
H 		Standard 	480
I 		Standard 	70
J 		Standard 	410

*/

#define OWtimeA	6
#define OWtimeB	64
#define OWtimeC	60
#define OWtimeD	10
#define OWtimeE	9
#define OWtimeF	55
#define OWtimeG	0
#define OWtimeH	480
#define OWtimeI	70
#define OWtimeJ	410

#define READ_ROM 0x33
#define SKIP_ROM 0xCC
#define CONVERT_T 0x44
#define READ_SCRATCHPAD 0xBE
#define WRITE_SCRATCHPAD 0x4E

static int OWReset(int Pin)
{
	int result;

	noInterrupts();

	//HAL_Delay_us(OWtimeG);
	HAL_PIN_Setup_Output(Pin);
	HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
	HAL_Delay_us(OWtimeH);
	HAL_PIN_Setup_Input_Pullup(Pin);  // Releases the bus
	HAL_Delay_us(OWtimeI);
	result = HAL_PIN_ReadDigitalInput(Pin) ^ 0x01; // Sample for presence pulse from slave

	interrupts();

	HAL_Delay_us(OWtimeJ); // Complete the reset sequence recovery
	return result; // Return sample presence pulse result
}

//-----------------------------------------------------------------------------
// Send a 1-Wire write bit. Provide 10us recovery time.
//-----------------------------------------------------------------------------
static void OWWriteBit(int Pin, int bit)
{
	if(bit)
	{
		// Write '1' bit
		HAL_PIN_Setup_Output(Pin);
		noInterrupts();
		HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
		HAL_Delay_us(OWtimeA);
		HAL_PIN_SetOutputValue(Pin, 1); // Releases the bus
		interrupts();	// hope for the best for the following timer and keep CRITICAL as short as possible
		HAL_Delay_us(OWtimeB); // Complete the time slot and 10us recovery
	}
	else
	{
		// Write '0' bit
		HAL_PIN_Setup_Output(Pin);
		noInterrupts();
		HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
		HAL_Delay_us(OWtimeC);
		HAL_PIN_SetOutputValue(Pin, 1); // Releases the bus
		interrupts();	// hope for the best for the following timer and keep CRITICAL as short as possible
		HAL_Delay_us(OWtimeD);
	}
}

//-----------------------------------------------------------------------------
// Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.
//-----------------------------------------------------------------------------
static int OWReadBit(int Pin)
{
	int result;

	noInterrupts();
	HAL_PIN_Setup_Output(Pin);
	HAL_PIN_SetOutputValue(Pin, 0);  // Drives DQ low
	HAL_Delay_us(1);                 // give sensor time to react on start pulse
	HAL_PIN_Setup_Input_Pullup(Pin); // Release the bus
	HAL_Delay_us(OWtimeE);           // give time for bus rise, if not pulled

	result = HAL_PIN_ReadDigitalInput(Pin); // Sample for presence pulse from slave
	interrupts();	// hope for the best for the following timer and keep CRITICAL as short as possible
	HAL_Delay_us(OWtimeF); // Complete the time slot and 10us recovery
	return result;
}

//-----------------------------------------------------------------------------
// Poll if DS1820 temperature conversion is complete
//-----------------------------------------------------------------------------
static int DS1820TConversionDone(int Pin)
{
	// Write '1' bit
	OWWriteBit(Pin, 1);
	// check for '1' - conversion complete (will be '0' else)
	return OWReadBit(Pin);
}


//-----------------------------------------------------------------------------
// Write 1-Wire data byte
//-----------------------------------------------------------------------------
static void OWWriteByte(int Pin, int data)
{
	int loop;

	// Loop to write each bit in the byte, LS-bit first
	for(loop = 0; loop < 8; loop++)
	{
		OWWriteBit(Pin, data & 0x01);

		// shift the data byte for the next bit
		data >>= 1;
	}
}

//-----------------------------------------------------------------------------
// Read 1-Wire data byte and return it
//-----------------------------------------------------------------------------
static int OWReadByte(int Pin)
{
	int loop, result = 0;

	for(loop = 0; loop < 8; loop++)
	{
		// shift the result to get it ready for the next bit
		result >>= 1;

		// if result is one, then set MS bit
		if(OWReadBit(Pin))
			result |= 0x80;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Write a 1-Wire data byte and return the sampled result.
//-----------------------------------------------------------------------------
static int OWTouchByte(int Pin, int data)
{
	int loop, result = 0;

	for(loop = 0; loop < 8; loop++)
	{
		// shift the result to get it ready for the next bit
		result >>= 1;

		// If sending a '1' then read a bit else write a '0'
		if(data & 0x01)
		{
			if(OWReadBit(Pin))
				result |= 0x80;
		}
		else
			OWWriteBit(Pin, 0);

		// shift the data byte for the next bit
		data >>= 1;
	}
	return result;
}

// quicker CRC with lookup table
// based on code found here: https://community.st.com/t5/stm32-mcus-security/use-stm32-crc-to-compare-ds18b20-crc/m-p/333749/highlight/true#M4690
// Dallas 1-Wire CRC Test App -
//  x^8 + x^5 + x^4 + 1 0x8C (0x131)

static uint8_t Crc8CQuick(uint8_t* Buffer, uint8_t Size)
{
	// Nibble table for polynomial 0x8C
	static const uint8_t CrcTable[] =
	{
		0x00,0x9D,0x23,0xBE,0x46,0xDB,0x65,0xF8,
		0x8C,0x11,0xAF,0x32,0xCA,0x57,0xE9,0x74
	};
	uint8_t Crc = 0x00;
	while(Size--)
	{
		Crc ^= *Buffer++; // Apply Data    
		Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F]; // Two rounds of 4-bits
		Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F];
	}
	return(Crc);
}

int DS1820_getTemp()
{
	return t;
}


#if ENABLE_DS1820_TEST_US
// will allow testus <pin> <us between tests> <us val 1> <us val 2> .... 
commandResult_t CMD_OW_testus(const void *context, const char *cmd, const char *args, int cmdFlags) {
   Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
   int tests= Tokenizer_GetArgsCount()-2;   // first two are pin and pause-value
   if(Tokenizer_GetArgsCount()<=3) {
      return CMD_RES_NOT_ENOUGH_ARGUMENTS;
   }
#define MAXUSTESTS 10
   int testvals[MAXUSTESTS];
   int pin = Tokenizer_GetArgInteger(0);
   int pause = Tokenizer_GetArgInteger(1);
   if (tests > MAXUSTESTS){
      tests = MAXUSTESTS;
      DS1820_LOG(INFO, "testus -  Warning, will only do the first %i tests!\r\n",tests);
   } 
   for (int i=0; i<tests; i++){
      testvals[i]=Tokenizer_GetArgInteger(2+i);
   }
   DS1820_LOG(DEBUG, "testus - pin=%i pause=%i tests=%i ...",pin,pause,tests);
   for (int i=0; i<tests; i++){
      DS1820_LOG(DEBUG, "test %i value=%i ...",i,testvals[i]);
   }
   DS1820_LOG(DEBUG, "\r\n starting tests ...");

   HAL_PIN_SetOutputValue(pin, 1);
   HAL_PIN_Setup_Output(pin);
   HAL_PIN_SetOutputValue(pin, 1);
   // at least on BK7238 HAL_PIN_Setup_Output(pin) will set pin LOW first, even if HAL_PIN_SetOutputValue(pin, 1); was called before
   // so to have a clear defined tes, do a long delay before the actual tests (we could also ignore the first change, but if it's different on other platforms...)
   vTaskDelay(200);
   for (int i=0; i<tests; i++){
      noInterrupts();
      HAL_PIN_SetOutputValue(pin, 0);
      HAL_Delay_us(testvals[i]);
      HAL_PIN_SetOutputValue(pin, 1);
      interrupts();
      HAL_Delay_us(pause);
   }
   DS1820_LOG(DEBUG, "... tests done\r\n");
   return CMD_RES_OK;
}
#endif


static commandResult_t Cmd_SetResolution(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int arg = Tokenizer_GetArgInteger(0);
	if (arg > 12 || arg < 9)
		return CMD_RES_BAD_ARGUMENT;

	if (ds18_family != 0x28) {
		DS1820_LOG(ERROR, "DS1820_SetResolution not supported by sensor");
		return CMD_RES_UNKNOWN_COMMAND;
	}

	uint8_t cfg = arg;
	cfg = cfg - 9;
	cfg = cfg * 32;
	cfg |= 0x1F;

	if(OWReset(Pin) == 0)
	{
		DS1820_LOG(ERROR, "WriteScratchpad Reset failed");
		return CMD_RES_ERROR;
	}

	OWWriteByte(Pin, SKIP_ROM);
	OWWriteByte(Pin, WRITE_SCRATCHPAD); //Write Scratchpad command
	OWWriteByte(Pin, 0x7F); //TH
	OWWriteByte(Pin, 0x80); //TL
	OWWriteByte(Pin, cfg);  //CFG

	//temperature conversion was interrupted
	dsread = 0;

	return CMD_RES_OK;
}


#if (DS1820full)

bool ds18b20_used_channel(int ch) {
	for (int i=0; i < ds18_count; i++) {
		if (ds18b20devices.channel[i] == ch) {
				return true;
		}
	}
	return false;
};

// reads scratchpad and returns fixed-point temperature, scaling factor 2^-7
int16_t calculateTemperature(const DeviceAddress *deviceAddress, uint8_t* scratchPad) {
	int16_t fpTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 11) | (((int16_t)scratchPad[TEMP_LSB]) << 3);
	return fpTemperature;
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

bool ds18b20_readScratchPad(const DeviceAddress *deviceAddress, uint8_t* scratchPad) {
//	if ( ! ds18b20_getGPIO(deviceAddress, &DS18B20_GPIO ) ) return false;
	if ( ! ds18b20_getGPIO(deviceAddress, &DS18B20_GPIO ) ) {
		DS1820_LOG(DEBUG, "No GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
		return false;
	}
	// send the reset command and fail fast
	DS1820_LOG(DEBUG, "GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
	int b = OWReset(DS18B20_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
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
	b =  OWReset(DS18B20_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
	return (b == 1);
}

void ds18b20_select(const DeviceAddress *address) {
	if ( ! ds18b20_getGPIO(address, &DS18B20_GPIO ) ) return;
	uint8_t i;
	OWWriteByte(DS18B20_GPIO, SELECTDEVICE);           // Choose ROM
	for (i = 0; i < 8; i++) OWWriteByte(DS18B20_GPIO, ((uint8_t *)address)[i]);
}

bool ds18b20_isAllZeros(const uint8_t * const scratchPad) {
	for (size_t i = 0; i < 9; i++) {
		if (scratchPad[i] != 0) {
			return false;
		}
	}
	return true;
}

bool ds18b20_isConnected(const DeviceAddress *deviceAddress, uint8_t *scratchPad) {
	// no need to check for GPIO of device here, ds18b20_readScratchPad() will do so and return flase, if not
	bool b = ds18b20_readScratchPad(deviceAddress, scratchPad);
	DS1820_LOG(DEBUG, "readScratchPad returned %i - Crc8CQuick=%i / scratchPad[SCRATCHPAD_CRC]=%i",b,Crc8CQuick(scratchPad, 8),scratchPad[SCRATCHPAD_CRC]);
	return b && !ds18b20_isAllZeros(scratchPad) && (Crc8CQuick(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
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


void ds18b20_init(int GPIO) {
	DS18B20_GPIO = GPIO;
	//gpio_pad_select_gpio(DS18B20_GPIO);
}

void ds18b20_requestTemperatures() {
	OWReset(DS18B20_GPIO);
	OWWriteByte(DS18B20_GPIO,SKIP_ROM);
	OWWriteByte(DS18B20_GPIO,CONVERT_T);
	//unsigned long start = esp_timer_get_time() / 1000ULL;
	//while (!isConversionComplete() && ((esp_timer_get_time() / 1000ULL) - start < millisToWaitForConversion())) vPortYield();
}



// add a new device into DS1820devices
void insertArray(DS1820devices *a, DeviceAddress devaddr) {
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
			a->GPIO[i]=DS18B20_GPIO; 	// just to be sure - maybe device is on other GPIO now?!?
			bk_printf("device 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X was allready present - just (re-)setting GPIO to %i",
		devaddr[0],devaddr[1],devaddr[2],devaddr[3],devaddr[4],devaddr[5],devaddr[6],devaddr[7],DS18B20_GPIO);
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


void scan_sensors(){
	ds18_count=0;
	reset_search();
//bk_printf("DS18B20_driver_Init() ... \r\n");
	int i,j=0;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
//	bk_printf(" ... i=%i",i);
		if ((g_cfg.pins.roles[i] == IOR_DS1820_IO) && ( j < DS18B20MAX_GPIOS)){
//		bk_printf(" ... i=%i + j=%i ",i,j);
			DS18B20GPIOS[j++]=i;
			ds18b20_init(i);	// will set  DS18B20_GPIO to i;
//		bk_printf(" ...DS18B20_fill_devicelist(%i)\r\n",i);
			DS18B20_fill_devicelist(i);

		}
	}
	// fill unused "pins" with 99 as sign for unused
	for (;j<DS18B20MAX_GPIOS;j++) DS18B20GPIOS[j]=99;
		bk_printf("scan_sensor() after GPIO-fill ... \r\n");
}

commandResult_t CMD_DS18B20_scansensors(const void *context, const char *cmd, const char *args, int cmdFlags) {
	scan_sensors();
	return CMD_RES_OK;
}


#endif		// to #if DS1820full



// startDriver DS1820 [conversionPeriod (seconds) - default 15]
void DS1820_driver_Init()
{
	ds18_conversionPeriod = Tokenizer_GetArgIntegerDefault(1, 15);
	lastconv = 0;
	dsread = 0;
	ds18_family = 0;

	//cmddetail:{"name":"DS1820_SetResolution","args":"[int]",
	//cmddetail:"descr":"Sets resolution for connected DS1820 sensor (9/10/11/12 bits)",
	//cmddetail:"fn":"Cmd_SetResolution","file":"drv/drv_ds1820_simple.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DS1820_SetResolution", Cmd_SetResolution, NULL);
#if ENABLE_DS1820_TEST_US	
	//cmddetail:{"name":"testus","args":"pin <pause in us> <testval 1 in us> [<testval n in us>...]",
	//cmddetail:"descr":"tests usleep on given pin ",
	//cmddetail:"fn":"NULL);","file":"driver/drv_ds1820_simple.c","requires":"",
	//cmddetail:"examples":"testus 11 5 2 4 6 10 20 50 100 200 500"}
	CMD_RegisterCommand("testus", CMD_OW_testus, NULL);
#endif

#if (DS1820full)
	scan_sensors();
	//cmddetail:{"name":"DS18B20_setsensor","args":"DS18B20-Addr name [channel]",
	//cmddetail:"descr":"Sets a name [and optional channel] to a DS18B20 sensor by sensors address",
	//cmddetail:"fn":"NULL);","file":"driver/drv_ds1802_simple.c","requires":"",
	//cmddetail:"examples":"DS18B20_setsensor \"0x28 0x01 0x02 0x03 0x04 0x05 0x06 0x07\" \"kitchen\" 2"}
	CMD_RegisterCommand("DS18B20_setsensor", CMD_DS18B20_setsensor, NULL);

	//cmddetail:{"name":"DS18B20_scansensors","args":"-",
	//cmddetail:"descr":"(Re-)Scan all GPIOs defined for DS1820 for sensors",
	//cmddetail:"fn":"NULL);","file":"driver/drv_ds1802_simple.c","requires":"",
	//cmddetail:"examples":"DS18B20_setsensor \"0x28 0x01 0x02 0x03 0x04 0x05 0x06 0x07\" \"kitchen\" 2"}
	CMD_RegisterCommand("DS18B20_scansensors", CMD_DS18B20_scansensors, NULL);

	// no need to discover the "family" we know the address, the first byte is the family

#else
	//Find PIN and check device so DS1820_SetResolution could be used in autoexec.bat
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, -1);
	if (Pin >= 0)
		DS1820_DiscoverFamily();
#endif
};

#if (DS1820full)
void DS1820_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState){
		hprintf255(request, "<h5>DS18B20 devices detected/configured: %i</h5>",ds18_count);
		return;
	}
		
	if (ds18_count > 0 ){
		hprintf255(request, "DS18B20 devices<table><th width='25'>Name</th>"
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
	hprintf255(request, "<script> function gen(){v='backlog startDriver DS1820 '; for (i=0; i<%i; i++) { ",ds18_count);
	poststr(request, "v+='; DS18B20_setsensor ' + '\"' + getElement('a'+i).innerHTML + '\" \"' + getElement('ds1820name'+i).value + '\" \"' + getElement('ds1820chan'+i).value + '\" '} return v; };</script>");
	poststr(request,"<br><br><input type='button' value='generate backlog command for config' onclick='t=document.getElementById(\"text\"); t.value=gen(); t.hidden=false;'><textarea id='text' hidden '> </textarea><p>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;	
	
	
	
	
	
}

#else
void DS1820_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState){
		return;
	}
	hprintf255(request, "<h5>DS1820 Temperature: %.2f C (read %i secs ago)</h5>", (float)t / 100, g_secondsElapsed - lastconv);
}

static int DS1820_DiscoverFamily()
{
	if(!OWReset(Pin))
	{
		DS1820_LOG(DEBUG, "Discover Reset failed");
		return 0;
	}

	// Read ROM
	uint8_t ROM[8];
	OWWriteByte(Pin, READ_ROM);
	for(int i = 0; i < 8; i++)
	{
		ROM[i] = OWReadByte(Pin);
	}

	// Check CRC
	uint8_t crc = Crc8CQuick(ROM, 7);
	if(crc != ROM[7])
	{
		// This might mean bad signal integrity or multiple 1-wire devices on the bus
		DS1820_LOG(DEBUG, "Discover CRC failed (CRC=%x != calculated:%x)", ROM[7], crc);
		return 0;
	}

	// Check family
	uint8_t family = ROM[0];
	if(family == 0x10 || family == 0x28)
	{
		ds18_family = family;
		DS1820_LOG(INFO, "Discover Family - discovered %x", family);
		return 1;
	}
	else
	{
		DS1820_LOG(DEBUG, "Discover Family %x not supported", family);
		return 0;
	}
}

void singleDS1820temp(){
	uint8_t scratchpad[9], crc;
	int16_t raw;
	if (!DS1820TConversionDone(Pin))
			return;

	if(OWReset(Pin) == 0)
	{
		DS1820_LOG(ERROR, "Read Reset failed");
		return;
	}
	OWWriteByte(Pin, SKIP_ROM);
	OWWriteByte(Pin, READ_SCRATCHPAD);
	for(int i = 0; i < 9; i++)
	{
		scratchpad[i] = OWReadByte(Pin);
	}
	crc = Crc8CQuick(scratchpad, 8);
	if(crc != scratchpad[8])
	{
		DS1820_LOG(ERROR, "Read CRC=%x != calculated:%x (errcount=%i)", scratchpad[8], crc, errcount);
		DS1820_LOG(ERROR, "Scratchpad Data Read: %x %x %x %x %x %x %x %x %x",
			scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3], scratchpad[4],
			scratchpad[5], scratchpad[6], scratchpad[7], scratchpad[8]);

		errcount++;
		if(errcount > 5) dsread = 0; // retry afer 5 failures
			
		return;
	}

	raw = (scratchpad[1] << 8) | scratchpad[0];

	if(ds18_family == 0x10)
	{
		// DS18S20 or old DS1820
		int16_t dT = 128 * (scratchpad[7] - scratchpad[6]);
		dT /= scratchpad[7];
		raw = 64 * (raw & 0xFFFE) - 32 + dT;
		DS1820_LOG(DEBUG, "family=%x, raw=%i, count_remain=%i, count_per_c=%i, dT=%i", ds18_family, raw, scratchpad[6], scratchpad[7], dT);
	}
	else
	{ // DS18B20
		uint8_t cfg = scratchpad[4] & 0x60;
		if(cfg == 0x00) raw = raw & ~7;      // 9 bit resolution, 93.75 ms
		else if(cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if(cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
		raw = raw << 3; // multiply by 8
		DS1820_LOG(DEBUG, "family=%x, raw=%i, cfg=%x (%i bit resolution)", ds18_family, raw, cfg, 9 + (cfg) / 32);
	}

	// Raw is t * 128
	t = (raw / 128) * 100; // Whole degrees
	int frac = (raw % 128) * 100 / 128; // Fractional degrees
	t += t > 0 ? frac : -frac;

	dsread = 0;
	lastconv = g_secondsElapsed;
	CHANNEL_Set(g_cfg.pins.channels[Pin], t, CHANNEL_SET_FLAG_SILENT);
	DS1820_LOG(INFO, "Temp=%i.%02i", (int)t / 100, (int)t % 100);
	
	return;
}

#endif 
void DS1820_OnEverySecond()
{
	

	// for now just find the pin used
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, -1);

	// only if (at least one) pin is set
	if(Pin < 0) {
		bk_printf("No Pin found\r\n");DS1820_LOG(INFO, "No Pin found\r\n");
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
#if (DS1820full)
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
#else	// to #if WINDOWS
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
					DS1820_LOG(INFO, "Device %i (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X) reported %0.2f\r\n",i,
						ds18b20devices.array[i][0],ds18b20devices.array[i][1],ds18b20devices.array[i][2],ds18b20devices.array[i][3],
						ds18b20devices.array[i][4],ds18b20devices.array[i][5],ds18b20devices.array[i][6],ds18b20devices.array[i][7],t);
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
#endif	// to #if WINDOWS		
		}
		else{
//			bk_printf("No tepms read! dsread=%i  -- isConversionComplete()=%i\r\n",dsread,isConversionComplete());
//			DS1820_LOG(INFO, "No tepms read! dsread=%i  -- isConversionComplete()=%i\r\n",dsread,isConversionComplete());
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

#else
	if(dsread == 1)
	{
		singleDS1820temp();
	}
	
	if(g_secondsElapsed % ds18_conversionPeriod == 0 || lastconv == 0) //dsread == 0
	{
		if(ds18_family == 0)
		{
			int discovered = DS1820_DiscoverFamily();
			if(!discovered)
			{
				lastconv = -1; // reset lastconv to avoid immediate retry
				DS1820_LOG(ERROR, "Family not discovered");
				return;
			}
		}
		if(OWReset(Pin) == 0)
		{
			lastconv = -1; // reset lastconv to avoid immediate retry
			DS1820_LOG(ERROR, "Reset failed");
#if DS1820_DEBUG
			// if device is not found, maybe "usleep" is not working as expected
			// lets do usleepds() with numbers 50.000 and 100.00
			// if all is well, it should take 50ms and 100ms
			// if not, we need to "calibrate" the loop
			int tempsleep = 5000;
			portTickType actTick = portTICK_RATE_MS * xTaskGetTickCount();
			usleepds(tempsleep);
			int duration = (int)(portTICK_RATE_MS * xTaskGetTickCount() - actTick);

			DS1820_LOG(DEBUG, "usleepds(%i) took %i ms ", tempsleep, duration);

			tempsleep = 100000;
			actTick = portTICK_RATE_MS * xTaskGetTickCount();
			usleepds(tempsleep);
			duration = (int)(portTICK_RATE_MS * xTaskGetTickCount() - actTick);

			DS1820_LOG(DEBUG, "usleepds(%i) took %i ms ", tempsleep, duration);

			if(duration < 95 || duration > 105)
			{
				// calc a new factor for usleepds
				DS1820_LOG(ERROR, "usleepds duration divergates - proposed factor to adjust usleepds %f ", (float)100 / duration);
			}
#endif	// #if DS1820_DEBUG
			return;
		}

		DS1820_LOG(INFO, "Starting conversion");
		OWWriteByte(Pin, SKIP_ROM);
		OWWriteByte(Pin, CONVERT_T);
		
		

		errcount = 0;
		dsread = 1;
	}
#endif	// to #if DS1820full

}

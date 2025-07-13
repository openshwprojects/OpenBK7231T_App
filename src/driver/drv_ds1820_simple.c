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


#define DEVSTR		"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
#define DEV2STR(T)	T[0],T[1],T[2],T[3],T[4],T[5],T[6],T[7]
#define SPSTR		DEVSTR " 0x%02X "
#define SP2STR(T)	DEV2STR(T),T[8]



// some OneWire commands
#define SEARCH_ROM		0xF0	// to identify the ROM codes of all slave devices on the bus
#define ALARM_SEARCH		0xEC	// all devices with an alarm condition (as SEARCH_ROM except only slaves with a set alarm flag will respond)
#define READ_ROM		0x33	// Only single device! Read 64-bit ROM code without using the Search ROM procedure
#define SKIP_ROM		0xCC	// Ignore ROM adress / address all devices
#define CONVERT_T		0x44	// take temperature reading and copy it to scratchpad
#define READ_SCRATCHPAD		0xBE	// Read scratchpad
#define WRITE_SCRATCHPAD	0x4E	// Write scratchpad
#define SELECT_DEVICE		0x55	// (Match ROM) address a single devices on the bus
/*
//unused for now
#define COPY_SCRATCHPAD	0x48		// Copy scratchpad to EEPROM
#define RECALL_E2		0xB8	// Recall from EEPROM to scratchpad
#define READ_POWER_SUPPLY	0xB4	// Determine if device needs parasite power
*/

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
// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit



static uint8_t dsread = 0;
static int Pin = -1;
static int t = -127;
static int errcount = 0;
static int lastconv; // secondsElapsed on last successfull reading
static uint8_t ds18_family = 0;
static int ds18_conversionPeriod = 0;

static int DS1820_DiscoverFamily();

typedef uint8_t ScratchPad[9];


#if (DS1820full)
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



#endif


#if (DS1820full)
#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820 - " fmt, ##__VA_ARGS__)
#else
#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820[%i] - " fmt, Pin, ##__VA_ARGS__)
#endif


// prototypes
#if (DS1820full)
int devstr2DeviceAddr(uint8_t *devaddr, const char *dev);
bool ds18b20_getGPIO(const uint8_t* devaddr,uint8_t *GPIO);
bool ds18b20_readSratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad);
#else
bool ds18b20_readSratchPad(uint8_t DS18B20_GPIO, uint8_t* scratchPad);
#endif



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

#if (DS1820full)
bool ds18b20_writeScratchPad(const uint8_t *deviceAddress, const uint8_t *scratchPad) {
	if ( ! ds18b20_getGPIO(deviceAddress, &DS18B20_GPIO ) ) {
		DS1820_LOG(DEBUG, "No GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
		return false;
	}
	DS1820_LOG(DEBUG, "GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
#else
bool ds18b20_writeScratchPad(uint8_t DS18B20_GPIO, const uint8_t* scratchPad) {
#endif
	// send the reset command and fail fast
	int b = OWReset(DS18B20_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
#if (DS1820full)
	ds18b20_select(deviceAddress);
#else
	OWWriteByte(DS18B20_GPIO, SKIP_ROM);
#endif
	OWWriteByte(DS18B20_GPIO,WRITE_SCRATCHPAD);
	OWWriteByte(DS18B20_GPIO,scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
	OWWriteByte(DS18B20_GPIO,scratchPad[LOW_ALARM_TEMP]); // low alarm temp
	OWWriteByte(DS18B20_GPIO,scratchPad[CONFIGURATION]);
	DS1820_LOG(DEBUG, "Wrote scratchpad " SPSTR , SP2STR(scratchPad));
	b = OWReset(DS18B20_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
	return true;
}



static commandResult_t Cmd_SetResolution(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int arg = Tokenizer_GetArgInteger(0);
	if (arg > 12 || arg < 9)
		return CMD_RES_BAD_ARGUMENT;
	uint8_t newValue = 0;
	switch (arg) {
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
		DS1820_LOG(DEBUG, "DS1820_SetResolution: newValue=0x%02X ", newValue);	

#if ! (DS1820full)
	if (ds18_family != 0x28) {
		DS1820_LOG(ERROR, "DS1820_SetResolution not supported by sensor");
		return CMD_RES_UNKNOWN_COMMAND;
	}
#endif
	ScratchPad scratchPad;
#if (DS1820full)
	if (Tokenizer_GetArgsCount() > 1){
		const char *dev = Tokenizer_GetArg(1);
		DeviceAddress devaddr={0};
		if (! devstr2DeviceAddr(devaddr,(const char*)dev)){
			DS1820_LOG(ERROR, "DS1820_SetResolution: device not found");
			return CMD_RES_BAD_ARGUMENT;
		}
		if (ds18b20_isConnected((uint8_t*)devaddr, scratchPad)){
#else
	if (ds18b20_readSratchPad(Pin, scratchPad)) {
#endif
			// if it needs to be updated we write the new value
			DS1820_LOG(DEBUG, "Read scratchpad " SPSTR , SP2STR(scratchPad));
			if (scratchPad[CONFIGURATION] != newValue) {
				scratchPad[CONFIGURATION] = newValue;
				DS1820_LOG(DEBUG, "Going to write scratchpad " SPSTR , SP2STR(scratchPad));

#if (DS1820full)
				if (! ds18b20_writeScratchPad((uint8_t*)devaddr, scratchPad)){
//					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad for sensor " DEVSTR ,DEV2STR((uint8_t*)devaddr));
					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad for sensor " DEVSTR ,DEV2STR(devaddr));
				}
#else
				if (! ds18b20_writeScratchPad(Pin, scratchPad)) {
					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to write scratchpad!");
				}
#endif
			}
		}
		else {
					DS1820_LOG(ERROR, "DS1820_SetResolution: failed to read scratchpad!");	
		}
#if (DS1820full)
	 }
	 else {
		for (int i=0; i < ds18_count; i++) {
//			DS1820_LOG(DEBUG, "DS1820_SetResolution: setting sensor " DEVSTR ,DEV2STR((uint8_t*)ds18b20devices.array[i]));
			DS1820_LOG(DEBUG, "DS1820_SetResolution: setting sensor " DEVSTR ,DEV2STR(ds18b20devices.array[i]));
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
#endif
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
#if (DS1820full)
bool ds18b20_readSratchPad(const uint8_t *deviceAddress, uint8_t* scratchPad) {
	if ( ! ds18b20_getGPIO(deviceAddress, &DS18B20_GPIO ) ) {
		DS1820_LOG(DEBUG, "No GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
		return false;
	}
	DS1820_LOG(DEBUG, "GPIO found for device - DS18B20_GPIO=%i", DS18B20_GPIO);
#else
bool ds18b20_readSratchPad(uint8_t DS18B20_GPIO, uint8_t* scratchPad) {
#endif
	// send the reset command and fail fast
	int b = OWReset(DS18B20_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
#if (DS1820full)
	ds18b20_select(deviceAddress);
#else
	OWWriteByte(DS18B20_GPIO, SKIP_ROM);
#endif
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

	b =  OWReset(DS18B20_GPIO);
	if (b == 0) {
		DS1820_LOG(DEBUG, "Resetting GPIO %i failed", DS18B20_GPIO);
		return false;
	}
	return (b == 1);
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
		// we whant to mask the bits, so substract from 3 --> decimal 3 (=3-0 for 9 bits) to 0 (=3-3 for 12 bits)
		// to mask out / delete x bits we use 		& ((1 << x ) -1)
		//  
		uint8_t maskbits = 3 - ((scratchPad[4] & 0x60) >> 5) ;	// maskbits will be number of bit to zero out
		raw = raw & ~ ( (1 << maskbits) -1) ;
//		DS1820_LOG(DEBUG, "maskbits=%x, raw=%i", maskbits, raw);
		raw = raw << 3; // multiply by 8
		DS1820_LOG(DEBUG, "ds18b20_Scratchpat2TempC: family=%x, raw=%i (%i bit resolution)", family, raw, 12 - maskbits );
	}

	// Raw is t * 128
	t = (raw / 128) * 100; // Whole degrees
	int frac = (raw % 128) * 100 / 128; // Fractional degrees
	t += t > 0 ? frac : -frac;

	if (t <= DEVICE_DISCONNECTED_C)
		return DEVICE_DISCONNECTED_C;
	// C = RAW/128
	return (float)raw / 128.0f;
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
	// no need to check for GPIO of device here, ds18b20_readSratchPad() will do so and return false, if not
	bool b = ds18b20_readSratchPad((const uint8_t *)deviceAddress, scratchPad);
	DS1820_LOG(DEBUG, "ds18b20_readSratchPad returned %i - Crc8CQuick=%i / scratchPad[SCRATCHPAD_CRC]=%i",b,Crc8CQuick(scratchPad, 8),scratchPad[SCRATCHPAD_CRC]);
	return b && !ds18b20_isAllZeros(scratchPad) && (Crc8CQuick(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}


float ds18b20_getTempC(const uint8_t *dev) {
	ScratchPad scratchPad;
	if (ds18b20_isConnected((const DeviceAddress *)dev, scratchPad)) {
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
	while (ds18_count < 1+(DS18B20MAX/4) ){
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

// convert a string with sensor address to "DeviceAddress"
int devstr2DeviceAddr(uint8_t *devaddr, const char *dev){
	char *p=dev;
	DeviceAddress daddr={0};
	int s;
#if PLATFORM_W600 || PLATFORM_LN882H || PLATFORM_RTL87X0C	
// this platforms won't allow sscanf of %hhx, so we need to use %x/%X and hence we need temporary unsigned ints ...
// test and add new platforms if needed
	unsigned int t[8];
	s = sscanf(dev,DEVSTR, &t[0],&t[1],&t[2],&t[3],&t[4],&t[5],&t[6],&t[7]);
	for (int i=0; i<8; i++) daddr[i]=(uint8_t)t[i];
#else
	s = sscanf(dev,"0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx",
		&daddr[0],&daddr[1],&daddr[2],&daddr[3],&daddr[4],&daddr[5],&daddr[6],&daddr[7]);
#endif
//	DS1820_LOG(DEBUG, "devstr2DeviceAddr: After sscanf - DevAddr=" DEVSTR, DEV2STR(daddr));

	if ( s!=8 ) {
		bk_printf("devstr2DeviceAddr -  conversion failed (converted %i)",s);
		DS1820_LOG(ERROR, "devstr2DeviceAddr: conversion failed (converted %i)",s);
		return 0;
	}
	memcpy(devaddr,daddr,8);
	return 1;
}

commandResult_t CMD_DS18B20_setsensor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if(Tokenizer_GetArgsCount()<=1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	const char *dev = Tokenizer_GetArg(0);
	int gpio = Tokenizer_IsArgInteger(1) ? Tokenizer_GetArgInteger(1) : HAL_PIN_Find(Tokenizer_GetArg(1));
	if (gpio < 0) {
		DS1820_LOG(ERROR, "DS18B20_setsensor: failed to find GPIO for '%s' (as int: %i)",Tokenizer_GetArg(1),gpio);
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
	//cmddetail:"examples":"DS18B20_scansensors"}
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
			DEV2STR(ds18b20devices.array[i]),
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
	hprintf255(request, "<h2>Configure DS18B20 sensors</h2><form action='/cfg_ds18b20'>");
	hprintf255(request, "<table><tr><th width='38'>Sensor Adress</th><th width='4'>Pin</th><th width='25'> &nbsp; Name </th><th width='5'>Channel</th></tr>");
	for (int i=0; i < ds18_count; i++) {
		hprintf255(request, "<tr><td id='a%i'>"DEVSTR"</td>"
		"<td id='pin%i'> %i</th>"
		"<td>&nbsp; <input name='ds1820name%i' id='ds1820name%i' value='%s'></td>"
		"<td>&nbsp; <input name='ds1820chan%i' id='ds1820chan%i' value='%i'></td></tr>",i,
		DEV2STR(ds18b20devices.array[i]),
		i,ds18b20devices.GPIO[i],
		i,i,ds18b20devices.name[i],
		i,i,ds18b20devices.channel[i]);
	}
	hprintf255(request, "</table>");
		

	poststr(request, "<br><input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? ')\"></form> ");
	hprintf255(request, "<script> function gen(){v='backlog startDriver DS1820 '; for (i=0; i<%i; i++) { ",ds18_count);
	poststr(request, "v+='; DS18B20_setsensor ' + '\"' + getElement('a'+i).innerHTML + '\" '  + getElement('pin'+i).innerHTML + ' \"' + getElement('ds1820name'+i).value + '\" ' + getElement('ds1820chan'+i).value} return v; };</script>");
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

	if (! ds18b20_readSratchPad(Pin, scratchpad)){
		errcount++;
		if(errcount > 5) dsread = 0; // retry afer 5 failures
			
		return;
	}

	float temp = ds18b20_Scratchpat2TempC(scratchpad, ds18_family);	
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
		DS1820_LOG(DEBUG, ".. Pin %i found! dsread=%i \r\n",Pin,dsread);
		if(dsread == 1){ 
#if WINDOWS
			bk_printf("Reading temperature from fake DS18B20 sensor(s)\r\n");
			for (int i=0; i < ds18_count; i++) {
				ds18b20devices.last_read[i] += 1 ;
				errcount = 0;
				float t_float = 20.0 + 0.1*(g_secondsElapsed%20 -10);
					ds18b20devices.lasttemp[i] = t_float;
					ds18b20devices.last_read[i] = 0;
					if (ds18b20devices.channel[i]>=0) CHANNEL_Set(ds18b20devices.channel[i], (int)(t_float*10), CHANNEL_SET_FLAG_SILENT);
					lastconv = g_secondsElapsed;
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
					if (ds18b20devices.channel[i]>=0) CHANNEL_Set(ds18b20devices.channel[i], t, CHANNEL_SET_FLAG_SILENT);
					lastconv = g_secondsElapsed;
					DS1820_LOG(INFO, "Sensor " DEVSTR " on %s reported %0.2f\r\n",DEV2STR(ds18b20devices.array[i]),pinalias,t);
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
		ds18b20_requestConvertT(Pin);
		errcount = 0;
		dsread = 1;
	}
#endif	// to #if DS1820full

}

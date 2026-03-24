#include "../obk_config.h"

#if (ENABLE_DRIVER_DS1820_FULL) || (ENABLE_DRIVER_DS1820)


#include "drv_ds1820_common.h"
#include "../hal/hal_generic.h"
#include "../hal/hal_pins.h"
#if PLATFORM_ESPIDF
#include "freertos/task.h"
#define noInterrupts() portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;taskENTER_CRITICAL(&mux)
#define interrupts() taskEXIT_CRITICAL(&mux)
#elif LINUX || WINDOWS
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


#if ENABLE_DS1820_TEST_US	
	static bool testus_initialized = false;
#endif

int OWReset(int Pin)
{
	int result;

// ESP32 will crash if interrupts are disabled for > 480us
#ifndef CONFIG_IDF_TARGET_ESP32
	noInterrupts();
#endif

	//HAL_Delay_us(OWtimeG);
	HAL_PIN_Setup_Output(Pin);
	HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
	HAL_Delay_us(OWtimeH);
	HAL_PIN_Setup_Input_Pullup(Pin);  // Releases the bus
	HAL_Delay_us(OWtimeI);
	result = HAL_PIN_ReadDigitalInput(Pin) ^ 0x01; // Sample for presence pulse from slave

#ifndef CONFIG_IDF_TARGET_ESP32
	interrupts();
#endif

	HAL_Delay_us(OWtimeJ); // Complete the reset sequence recovery
	return result; // Return sample presence pulse result
}

//-----------------------------------------------------------------------------
// Send a 1-Wire write bit. Provide 10us recovery time.
//-----------------------------------------------------------------------------
void OWWriteBit(int Pin, int bit)
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
int OWReadBit(int Pin)
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
int DS1820TConversionDone(int Pin)
{
	// Write '1' bit
	OWWriteBit(Pin, 1);
	// check for '1' - conversion complete (will be '0' else)
	return OWReadBit(Pin);
}


//-----------------------------------------------------------------------------
// Write 1-Wire data byte
//-----------------------------------------------------------------------------
void OWWriteByte(int Pin, int data)
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
int OWReadByte(int Pin)
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
/*
//-----------------------------------------------------------------------------
// Write a 1-Wire data byte and return the sampled result.
//-----------------------------------------------------------------------------
int OWTouchByte(int Pin, int data)
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
*/

// quicker CRC with lookup table
// based on code found here: https://community.st.com/t5/stm32-mcus-security/use-stm32-crc-to-compare-ds18b20-crc/m-p/333749/highlight/true#M4690
// Dallas 1-Wire CRC Test App -
//  x^8 + x^5 + x^4 + 1 0x8C (0x131)

uint8_t Crc8CQuick(uint8_t* Buffer, uint8_t Size)
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


#if ENABLE_DS1820_TEST_US && ! WINDOWS
// will allow testus <pin> <us between tests> <us val 1> <us val 2> .... 
commandResult_t CMD_OW_testus(const void *context, const char *cmd, const char *args, int cmdFlags) {
   Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
   int tests= Tokenizer_GetArgsCount()-2;   // first two are pin and pause-value
   if(Tokenizer_GetArgsCount()<=3) {
      return CMD_RES_NOT_ENOUGH_ARGUMENTS;
   }
#define MAXUSTESTS 10
   int testvals[MAXUSTESTS];
//   int pin = Tokenizer_GetArgInteger(0);
   int pin = Tokenizer_GetPin(0,-1);
   if (pin==-1) return CMD_RES_BAD_ARGUMENT;
   int pause = Tokenizer_GetArgInteger(1);
   if (tests > MAXUSTESTS){
      tests = MAXUSTESTS;
      ADDLOG_ERROR(LOG_FEATURE_CMD, "testus -  Warning, will only do the first %i tests!\r\n",tests);
   } 
   for (int i=0; i<tests; i++){
      testvals[i]=Tokenizer_GetArgInteger(2+i);
   }
   ADDLOG_DEBUG(LOG_FEATURE_CMD, "testus - pin=%i pause=%i tests=%i ...",pin,pause,tests);
   for (int i=0; i<tests; i++){
      ADDLOG_DEBUG(LOG_FEATURE_CMD, "test %i value=%i ...",i,testvals[i]);
   }
   ADDLOG_DEBUG(LOG_FEATURE_CMD, "\r\n starting tests ...");

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
   ADDLOG_DEBUG(LOG_FEATURE_CMD, "... tests done\r\n");
   return CMD_RES_OK;
}
#endif

void init_TEST_US(){
#if ENABLE_DS1820_TEST_US && ! WINDOWS	
	//cmddetail:{"name":"testus","args":"pin <pause in us> <testval 1 in us> [<testval n in us>...]",
	//cmddetail:"descr":"tests usleep on given pin ",
	//cmddetail:"fn":"CMD_OW_testus","file":"driver/drv_ds1820_common.c","requires":"",
	//cmddetail:"examples":"testus 11 5 2 4 6 10 20 50 100 200 500"}
	if (! testus_initialized) CMD_RegisterCommand("testus", CMD_OW_testus, NULL);
	testus_initialized = true;
#endif
}

#endif // to #if (ENABLE_DRIVER_DS1820_FULL) || (ENABLE_DRIVER_DS1820)

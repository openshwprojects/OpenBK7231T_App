#include "../obk_config.h"
#if (ENABLE_DRIVER_DS1820) || (ENABLE_DRIVER_DS18B20)

#include "OneWire_common.h"
#if PLATFORM_ESPIDF
#include "freertos/task.h"
#define noInterrupts() portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;taskENTER_CRITICAL(&mux)
#define interrupts() taskEXIT_CRITICAL(&mux)
#elif WINDOWS
	// no task.h
#define noInterrupts() {}
#define interrupts() {}
void vTaskDelay(void) {} 
#else
#include <task.h>
#define noInterrupts() taskENTER_CRITICAL()
#define interrupts() taskEXIT_CRITICAL()
#endif

// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"

// usleep adopted from DHT driver
void OWusleep(int r)
{
#ifdef WIN32
	// not possible on Windows port
#elif PLATFORM_BEKEN && ! PLATFORM_BK7238
	float adj = 1;
	if(g_powersave) adj = 1.5;
	usleep((17 * r * adj) / 10); // "1" is to fast and "2" to slow, 1.7 seems better than 1.5 (only from observing readings, no scope involved)
#else
	HAL_Delay_us(r);
#endif
}


// taken from drv_ds1820_simple -- own code below
// add some "special timing" for Beken - works w/o and with powerSave 1 for me
void OWusleepshort(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#if PLATFORM_BEKEN && ! PLATFORM_BK7238
	int newr = r / (3 * g_powersave + 1);		// devide by 4 if powerSave set to 1
	for(volatile int i = 0; i < newr; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		//__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop");
	}

#else
	OWusleep(r);
#endif
}

void OWusleepmed(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#if PLATFORM_BEKEN && ! PLATFORM_BK7238
	int newr = 10 * r / (10 + 5 * g_powersave);		// devide by 1.5 powerSave set to 1
	for(volatile int i = 0; i < newr; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}

#else
	OWusleep(r);
#endif
}

void OWusleeplong(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#if PLATFORM_BEKEN && ! PLATFORM_BK7238
	int newr = 10 * r / (10 + 5 * g_powersave);		// devide by 1.5 powerSave set to 1
	for(volatile int i = 0; i < newr; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		//		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop");	// 5
	}

#else
	OWusleep(r);
#endif
}


/*

// add some "special timing" for Beken - works w/o and with powerSave 1 for me
void OWusleepshort(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#if PLATFORM_BEKEN
	int newr = r / (3 * g_powersave + 1);		// devide by 4 if powerSave set to 1
	for(volatile int i = 0; i < newr; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		//__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop");
	}

#else
	OWusleep(r);
#endif
}


void OWusleepmed(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#if PLATFORM_BEKEN
	int newr = 10 * r / (10 + 5 * g_powersave);		// devide by 1.5 powerSave set to 1
	for(volatile int i = 0; i < newr; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}

#else
	OWusleep(r);
#endif
}

void OWusleeplong(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#if PLATFORM_BEKEN
	int newr = 10 * r / (10 + 5 * g_powersave);		// devide by 1.5 powerSave set to 1
	for(volatile int i = 0; i < newr; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		//		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop");	// 5
	}

#else
	OWusleep(r);
#endif
}
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

int OWReset(int Pin)
{
	int result;

	//usleep(OWtimeG);
	HAL_PIN_Setup_Output(Pin);
	HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
	OWusleeplong(OWtimeH);
	HAL_PIN_SetOutputValue(Pin, 1); // Releases the bus
	OWusleepmed(OWtimeI);
	HAL_PIN_Setup_Input(Pin);
	result = HAL_PIN_ReadDigitalInput(Pin) ^ 0x01; // Sample for presence pulse from slave
	OWusleeplong(OWtimeJ); // Complete the reset sequence recovery
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
		OWusleepshort(OWtimeA);
		HAL_PIN_SetOutputValue(Pin, 1); // Releases the bus
		interrupts();	// hope for the best for the following timer and keep CRITICAL as short as possible
		OWusleepmed(OWtimeB); // Complete the time slot and 10us recovery
	}
	else
	{
		// Write '0' bit
		HAL_PIN_Setup_Output(Pin);
		noInterrupts();
		HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
		OWusleepmed(OWtimeC);
		HAL_PIN_SetOutputValue(Pin, 1); // Releases the bus
		interrupts();	// hope for the best for the following timer and keep CRITICAL as short as possible
		OWusleepshort(OWtimeD);
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
	HAL_PIN_SetOutputValue(Pin, 0); // Drives DQ low
	OWusleepshort(OWtimeA);
	HAL_PIN_SetOutputValue(Pin, 1); // Releases the bus
	OWusleepshort(OWtimeE);
	HAL_PIN_Setup_Input(Pin);
	result = HAL_PIN_ReadDigitalInput(Pin); // Sample for presence pulse from slave
	interrupts();	// hope for the best for the following timer and keep CRITICAL as short as possible
	OWusleepmed(OWtimeF); // Complete the time slot and 10us recovery
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

// will need testus <pin> <us between tests> <us val 1> <us val 2> .... 
commandResult_t CMD_OW_testus(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	int tests= Tokenizer_GetArgsCount()-2;	// first two are pin and pause-value
	if(Tokenizer_GetArgsCount()<=1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
#define MAXUSTESTS 10
	int testvals[MAXUSTESTS];
	int pin = Tokenizer_GetArgInteger(1);
	int pause = Tokenizer_GetArgInteger(2);
	if (tests > MAXUSTESTS) tests = MAXUSTESTS; 
	for (int i=0; i<tests; i++){
		testvals[i]=Tokenizer_GetArgInteger(3+i);
	}
	for (int i=0; i<tests; i++){
		HAL_PIN_SetOutputValue(pin, 0);
		noInterrupts();
		HAL_PIN_SetOutputValue(pin, 1);
		OWusleep(pause);
		HAL_PIN_SetOutputValue(pin, 0);
		OWusleep(testvals[i]);
		HAL_PIN_SetOutputValue(pin, 1);
		interrupts();
	}
	return CMD_RES_OK;
}

void register_testus(){
	//cmddetail:{"name":"testus","args":"pin <pause in us> <testval 1 in us> [<testval n in us>...]",
	//cmddetail:"descr":"tests usleep on given pin ",
	//cmddetail:"fn":"NULL);","file":"driver/OneWire_common.c","requires":"",
	//cmddetail:"examples":"testus 11 5 2 4 6 10 20 50 100 200 500"}
	CMD_RegisterCommand("testus", CMD_OW_testus, NULL);
}


#endif


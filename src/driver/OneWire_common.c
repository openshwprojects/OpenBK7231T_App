#include "OneWire_common.h"

// usleep adopted from DHT driver
void OWusleep(int r)
{
#ifdef WIN32
	// not possible on Windows port
#elif PLATFORM_BEKEN
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

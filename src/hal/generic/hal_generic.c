#include "../hal_generic.h"

void __attribute__((weak)) HAL_RebootModule()
{

}

void __attribute__((weak)) HAL_Delay_us(int delay)
{
	for(volatile int i = 0; i < delay; i++)
	{
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
}

void __attribute__((weak)) HAL_Configure_WDT()
{

}

void __attribute__((weak)) HAL_Run_WDT()
{

}

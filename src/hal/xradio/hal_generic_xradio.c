#if PLATFORM_XRADIO

#include "driver/chip/hal_util.h"
#include "driver/chip/hal_wdg.h"

void HAL_RebootModule() 
{
	HAL_WDG_Reboot();
}

void HAL_Delay_us(int delay)
{
	HAL_UDelay(delay);
}

void HAL_Configure_WDT()
{
	WDG_InitParam param;
#if PLATFORM_XR809
	param.event = WDG_EVT_RESET;
	param.timeout = WDG_TIMEOUT_16SEC;
	param.resetCycle = WDG_DEFAULT_RESET_CYCLE;
#else
	param.hw.event = WDG_EVT_RESET;
	param.hw.resetCpuMode = WDG_RESET_CPU_CORE;
	param.hw.timeout = WDG_TIMEOUT_16SEC;
	param.hw.resetCycle = WDG_DEFAULT_RESET_CYCLE;
#endif
	HAL_WDG_Init(&param);
	HAL_WDG_Start();
}

void HAL_Run_WDT()
{
	HAL_WDG_Feed();
}

#endif // PLATFORM_XR809
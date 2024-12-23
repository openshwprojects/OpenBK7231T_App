#ifdef PLATFORM_LN882H

#include "hal/hal_wdt.h"
#include "utils/reboot_trace/reboot_trace.h"

extern void ln_block_delayus(uint32_t us);

void HAL_RebootModule() 
{
	ln_chip_reboot();
}

void HAL_Configure_WDT()
{
	/* Watchdog initialization */
	wdt_init_t_def wdt_init;
	memset(&wdt_init, 0, sizeof(wdt_init));
	wdt_init.wdt_rmod = WDT_RMOD_1;         // When equal to 0, the counter is reset directly when it overflows; when equal to 1, an interrupt is generated first when the counter overflows, and if it overflows again, it resets.
	wdt_init.wdt_rpl = WDT_RPL_32_PCLK;     // Set the reset delay time
	wdt_init.top = WDT_TOP_VALUE_9;         //wdt cnt value = 0x1FFFF   Time = 4.095 s
	hal_wdt_init(WDT_BASE, &wdt_init);

	/* Configure watchdog interrupt */
	NVIC_SetPriority(WDT_IRQn, 4);
	NVIC_EnableIRQ(WDT_IRQn);

	/* Enable watchdog */
	hal_wdt_en(WDT_BASE, HAL_ENABLE);
}

void HAL_Run_WDT()
{
	hal_wdt_cnt_restart(WDT_BASE);
}

void HAL_Delay_us(int delay)
{
	ln_block_delayus(delay);
}

#endif // PLATFORM_LN882H

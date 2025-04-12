#include "../../new_common.h"
#include "../hal_generic.h"
#include <BkDriverWdg.h>

#include "../../beken378/driver/include/arm_arch.h"
#include "../../beken378/driver/pwm/bk_timer.h"
#include "../../beken378/driver/include/bk_timer_pub.h"
#include "../../beken378/func/include/fake_clock_pub.h"

// from wlan_ui.c
void bk_reboot(void);
extern bool g_powersave;

void HAL_RebootModule() 
{
	bk_reboot();
}

#if !defined TIMER0_2_READ_VALUE && defined TIMERR0_2_READ_VALUE
//typo in sdk
#define TIMER0_2_READ_VALUE TIMERR0_2_READ_VALUE
#endif

static uint32_t getTicksCount() {
	uint32_t timeout = 0;
	REG_WRITE(TIMER0_2_READ_CTL, (CAL_TIMER_ID << 2) | 1);
	while (REG_READ(TIMER0_2_READ_CTL) & 1) {
		timeout++;
		if (timeout > (120 * 1000)){
			bk_printf("ERROR - timeout in getTicksCount()\r\n");
			return 0;
		}
	}
	return REG_READ(TIMER0_2_READ_VALUE);
}

#if !defined CFG_XTAL_FREQUENCE
//26MHz, as per bk7231t_os/beken378/driver/sys_ctrl/sys_ctrl.c
#define CFG_XTAL_FREQUENCE 26000000
#endif

//ONE_CAL_TIME - from fake_clock.c
#define ONE_CAL_TIME       15000
#define TICKS_PER_US       (CFG_XTAL_FREQUENCE / 1000 / 1000)
#define US_PER_OVERFLOW    (ONE_CAL_TIME * 1000)
// 26 ticks per us * 15 000 000 us per overflow
#define TICKS_PER_OVERFLOW (TICKS_PER_US * US_PER_OVERFLOW)

//https://github.com/libretiny-eu/libretiny
void HAL_Delay_us(int delay) {
#if PLATFORM_BK7238
	uint64_t m = (uint64_t)fclk_get_tick();
	if(delay)
	{
		uint64_t e = (m + (delay*RT_TICK_PER_SECOND/1000/1000));
		if(m > e)
		{ //overflow
			while((uint64_t)fclk_get_tick() > e){}
		}
		while((uint64_t)fclk_get_tick() < e){}
	}
#else
	// 2us with gpio_output() while switch GPIO pins.
	// This is platform-specific, so put it here.
	// us-range delays are for bitbang in most cases.
	uint32_t startTick = getTicksCount();
	if (delay > 1 && startTick >0){
		/* startTick2 accounts for the case where the timer counter overflows */

		uint32_t failed_getTicks = 0;
		uint32_t startTick2 = startTick - TICKS_PER_OVERFLOW;

		uint32_t delayTicks = TICKS_PER_US * delay;
		while (1) {
			uint32_t t = getTicksCount();
			if (t == 0) failed_getTicks ++;	
			if (failed_getTicks > 1) {
				bk_printf("ERROR in HAL_Delay_us() - too many timeouts for getTicksCount()\r\n");
				break;
			}	
			if ((t - startTick >= delayTicks) && // normal case
			    (t - startTick2 >= delayTicks))  // counter overflow case
				break;
		}

	}
#endif
}

void HAL_Configure_WDT()
{
	bk_wdg_initialize(10000);
}

void HAL_Run_WDT()
{
	bk_wdg_reload();
}

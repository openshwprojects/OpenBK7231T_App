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
	if (CAL_TIMER_ID >2) bk_printf("ERROR - in getTicksCount() CAL_TIMER_ID = %i\r\n",CAL_TIMER_ID);
	REG_WRITE(TIMER0_2_READ_CTL, (CAL_TIMER_ID << 2) | 1);
	while (REG_READ(TIMER0_2_READ_CTL) & 1) {
		timeout++;
		if (timeout > (120 * 1000)){
			bk_printf("ERROR - timeout in getTicksCount()\r\n");
			return BK_TIMER_FAILURE;
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
	usleep(delay);
#else
	// 2us with gpio_output() while switch GPIO pins.
	// This is platform-specific, so put it here.
	// us-range delays are for bitbang in most cases.
	uint32_t startTick = getTicksCount();
	if (delay > 1 && startTick != BK_TIMER_FAILURE ){
		/* startTick2 accounts for the case where the timer counter overflows */

		uint32_t failed_getTicks = 0;
		uint32_t startTick2 = startTick - TICKS_PER_OVERFLOW;

		uint32_t delayTicks = TICKS_PER_US * delay;
		while (1) {
			uint32_t t = getTicksCount();
			if (t == BK_TIMER_FAILURE) failed_getTicks ++;	
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

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

#if ! (PLATFORM_BK7252 || PLATFORM_BK7238)
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

#endif // #if ! (PLATFORM_BK7252 || PLATFORM_BK7238)

// https://github.com/libretiny-eu/libretiny
// not working on BK7238
// not building for PLATFORM_BK7525
void HAL_Delay_us(int delay) {
#if (PLATFORM_BK7252)
	float adj = 1;
	if(g_powersave) adj = 1.5;
	// use original code for non BK7381 - I cant test
	// from feedback by divadiow 1.7 (17/10) is too low by 1.3 to 4.2 - let's try 2.5 * 1.7 ~ 4.25 --> I'll take 4.3
	// next feedack: I think we need to use different factors for higher values
	// from feedback I tried these
	if (delay >= 50) usleep((69*delay)/10);
	else if (delay >= 20) usleep((54*delay)/10);
	else if (delay >= 5) usleep((52*delay)/10);
	else usleep((43 * delay * adj) / 10); //  don't know if adjusting is needed, leave it for now
#elif (PLATFORM_BK7238) 
	int adj = 100;
	// use 3 for values >=10, 4 for < 10
	//
	// use 2.7 for values >=50
	// use 3.5 for values < 50 but > 10
	// use 5 for values <= 10
	if(g_powersave) { 
		if (delay >= 50) adj = 270;
		else if (delay > 10) adj = 350;
		else adj = 500;
	}
	
	// starting over again for BK7238
	// read factors between 5 and 6,7 - using 6 seems a good start
	// trying with 6.4 - higher values are off else
	usleep(((64000/adj)*delay)/100);
#else
	uint32_t startTick = getTicksCount();
	if (delay > 1 && startTick != BK_TIMER_FAILURE ){		// be sure, timer works
		uint32_t endTicks = startTick + TICKS_PER_US * delay;	// end tick value after delay
		// 
		// there are three possible cases: 
		//	1. a delay with overflow 
		//	2. a delay surely without overflow
		//	3. a delay with a possible overflow
		// 
		// Case 1. with overflow:
		//	quite "easy":
		//		wait for overflow, then wait until counter > calculated end value (after overflow)
		if (endTicks >= TICKS_PER_OVERFLOW){
			while (getTicksCount() > startTick) {};	// until overflow happens, the actual count is > than start
			endTicks -= TICKS_PER_OVERFLOW;		// calculate endTicks (subtracting maximum value
			while (getTicksCount() < endTicks) {};	// wait, until end counter is reached
			
		} 
		else {
		// 
		//	For the other two cases we need to define a "safe distance" away from overflow
		//		if the end value is lower, we define it as "safe" - there can be no overflow when testing 
		//	 
		
			uint32_t safeTick = TICKS_PER_OVERFLOW - (TICKS_PER_US * 10);	// an end value "10 us away" from overflow should be safe 

		// Case 2. surely no overflow:
		//	very easy:
		//		since end value is a "safe distance" away from overflow we can simply wait, until end value is reached 
		
			if (endTicks < safeTick){			// so end value is "safe" distance away from overflow
				while (getTicksCount() < endTicks) {};	// just wait, until end counter is reached
			}
			else {
			
		// Case 3. the tricky one: end value is "near" to overflow - so we can't simply wait, until end value is reached, 
		//	because we might miss the check, were counter is > endTicks and the next test is after an overflow and (since the value returend is way below our end point) we missed the end condition.
		//	So we need two checks: is the value still below endTicks AND was there no overflow (we test, if the value is still > startTick)
		// 
		// 	to make the "expensive" tests as low as possible, devide the test:
		//		first test "simple" inside the safe boundaries
		//		then do the test for end value and check, if overflow happened   

				while (getTicksCount() < safeTick) {};	// "simple" wait, until safeTick is reached
				uint32_t t = getTicksCount();
				while ( t < endTicks && t > startTick){	// when "near" overflow, check if endTicks is reached or overflow happened (then t < startTick)
					t = getTicksCount();
				};

			}
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

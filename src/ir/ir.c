#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include "bk_timer_pub.h"

#include <ctype.h>

static unsigned long counter = 0;

#if PLATFORM_BK7231T

#include "include.h"
#include "arm_arch.h"
#include "bk_timer_pub.h"
// from apps\OpenBK7231T_App\src\ir
// to apps\OpenBK7231T_App\sdk\OpenBK7231T\platforms\bk7231t\bk7231t_os\beken378\driver\pwm

#include "../../sdk/OpenBK7231T/platforms/bk7231t/bk7231t_os/beken378/driver/pwm/bk_timer.h"
// or #include "../../../../platforms\bk7231t\bk7231t_os\beken378\driver\pwm\bk_timer.h"
#include "drv_model_pub.h"
#include "intc_pub.h"
#include "icu_pub.h"
#include "uart_pub.h"

static void DRV_IR_ISR(UINT8 t){
    counter++;
}

extern void (*p_TIMER_Int_Handler[TIMER_CHANNEL_NO])(UINT8);

void DRV_IR_Test(){

    counter = 0;
    UINT32 chan = BKTIMER0;
    UINT32 periodus = 50;
    UINT32 div = 1;

	ADDLOG_INFO(LOG_FEATURE_CMD, "Before timer config chan %d, period %dus, div %d:", chan, periodus, div);
    UINT32 period = REG_READ(REG_TIMERCTLA_PERIOD_ADDR(chan));
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test hw period %u", period);

    UINT32 value = REG_READ(TIMER0_2_CTL);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test tmr2ctrl 0x%8X", value);
    UINT32 mask = (TIMERCTLA_CLKDIV_MASK << TIMERCTLA_CLKDIV_POSI);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test tmr2mask 0x%8X", mask);
    UINT32 divr = (value & TIMERCTLA_CLKDIV_MASK) << TIMERCTLA_CLKDIV_POSI;
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test tmr2period@26mhz %d", divr);

    UINT32 intr = REG_READ(TIMER0_2_CTL);
    intr = (intr >> chan);
    intr &= 1;
    ADDLOG_INFO(LOG_FEATURE_CMD, "ir test interupt enb is %d", intr);

    if (p_TIMER_Int_Handler[chan]){
        ADDLOG_INFO(LOG_FEATURE_CMD, "ir test interupt handler already set to %u", p_TIMER_Int_Handler[chan]);
    } else {
        ADDLOG_INFO(LOG_FEATURE_CMD, "ir test interupt handler null %u", p_TIMER_Int_Handler[chan]);
    }

    // note: Timer1 seems to have prescale of 16.
    // timer0 works, but is it in use?

    timer_param_t params = {
        chan,
        div, // div
        periodus, // us
        DRV_IR_ISR
    };
    //GLOBAL_INT_DECLARATION();

	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test routine 1");

    bk_timer_init();

	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test routine 2");

    //UINT32 status = 0;
    //GLOBAL_INT_DISABLE();

    //GLOBAL_INT_RESTORE();


    UINT32 res;
    res = sddev_control(TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM_US, &params);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test routine initres %u", res);
    res = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &chan);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test routine enbres %u", res);


	ADDLOG_INFO(LOG_FEATURE_CMD, "After timer config:");
    period = REG_READ(REG_TIMERCTLA_PERIOD_ADDR(chan));
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test hw period %u", period);

    value = REG_READ(TIMER0_2_CTL);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test tmr2ctrl 0x%8X", value);
    mask = (TIMERCTLA_CLKDIV_MASK << TIMERCTLA_CLKDIV_POSI);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test tmr2mask 0x%8X", mask);
    divr = (value & TIMERCTLA_CLKDIV_MASK) << TIMERCTLA_CLKDIV_POSI;
	ADDLOG_INFO(LOG_FEATURE_CMD, "ir test tmr2div %d", div);
    intr = REG_READ(TIMER0_2_CTL);
    intr = (intr >> chan);
    intr &= 1;
    ADDLOG_INFO(LOG_FEATURE_CMD, "ir test interupt enb is %d", intr);

    if (p_TIMER_Int_Handler[chan]){
        ADDLOG_INFO(LOG_FEATURE_CMD, "ir test interupt handler set to %u", p_TIMER_Int_Handler[chan]);
    } else {
        ADDLOG_INFO(LOG_FEATURE_CMD, "ir test interupt handler null %u", p_TIMER_Int_Handler[chan]);
    }
    
}
#else
// stub if not OpenBK7231T
void DRV_IR_Test(){
}
#endif

void DRV_IR_Print(){
    if (counter){
        ADDLOG_INFO(LOG_FEATURE_CMD, "IR counter: %u", counter);
    }
}


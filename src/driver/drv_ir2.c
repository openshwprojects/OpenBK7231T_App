// freeze
#include "drv_local.h"
#include "../new_common.h"

#if PLATFORM_BEKEN


#include "include.h"
#include "arm_arch.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include "../cmnds/cmd_public.h"
#include "bk_timer_pub.h"
#include "drv_model_pub.h"

#include <gpio_pub.h>
//#include "pwm.h"
#include "pwm_pub.h"

#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/func/user_driver/BkDriverPwm.h"
#include "../../beken378/func/user_driver/BkDriverI2c.h"
#include "../../beken378/driver/i2c/i2c1.h"
#include "../../beken378/driver/gpio/gpio.h"


#endif

static UINT32 ir_chan = BKTIMER0;
static UINT32 ir_div = 1;
static UINT32 ir_periodus = 50;
int txpin = 26;

int *times;
int maxTimes;
int *cur;
int *stop;
int myPeriodUs;


void Send_Init() {

	cur = times;
	stop = times;
	while (1) {
		*stop = atoi("xx");
		stop++;
	}
}
void Send_Tick() {
	*cur -= myPeriodUs;
	if (*cur <= 0) {
		cur++;
		if (cur == stop) {
			// done
		}
	}
}


static commandResult_t CMD_IR2_Test1(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int pwmIndex = PIN_GetPWMIndexForPinIndex(txpin);
	// is this pin capable of PWM?
	if (pwmIndex != -1) {
		uint32_t pwmfrequency = 38000;
		uint32_t period = (26000000 / pwmfrequency);
		uint32_t duty = period / 2;
#if PLATFORM_BK7231N
		// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
		bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty, 0, 0);
#else
		bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty);
#endif
		bk_pwm_start((bk_pwm_t)pwmIndex);
	}

	return CMD_RES_OK;
}
int state = 0;
void Test2_ISR(UINT8 t) {
	state = !state;
	bk_gpio_output(txpin, state);
}
static commandResult_t CMD_IR2_Test2(const void* context, const char* cmd, const char* args, int cmdFlags) {
	bk_gpio_config_output(txpin);

	Tokenizer_TokenizeString(args, 0);

	myPeriodUs = Tokenizer_GetArgIntegerDefault(0, 50);

	timer_param_t params = {
	 (unsigned char)ir_chan,
	 (unsigned char)ir_div, // div
	 myPeriodUs, // us
	 Test2_ISR
	};
	//GLOBAL_INT_DECLARATION();


	UINT32 res;
	// test what error we get with an invalid command
	res = sddev_control((char *)TIMER_DEV_NAME, -1, 0);

	if (res == 1) {
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"bk_timer already initialised");
	}
	else {
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"bk_timer driver not initialised?");
		if ((int)res == -5) {
			ADDLOG_INFO(LOG_FEATURE_IR, (char *)"bk_timer sddev not found - not initialised?");
			return;
		}
		return;
	}


	//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer init");
	// do not need to do this
	//bk_timer_init();
	//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer init done");
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"will ir timer setup %u", res);
	res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM_US, &params);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer setup %u", res);
	res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer enabled %u", res);
	return CMD_RES_OK;
}
void DRV_IR2_Init() {
	CMD_RegisterCommand("Test1", CMD_IR2_Test1, NULL);
	CMD_RegisterCommand("Test2", CMD_IR2_Test2, NULL);



}




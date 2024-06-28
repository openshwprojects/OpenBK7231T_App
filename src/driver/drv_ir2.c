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
#include "../../beken378/driver/pwm/pwm.h"
#define CFG_SOC_NAME SOC_BK7231N
#include "../../beken378/driver/pwm/pwm_new.h"

#endif

static UINT32 ir_chan = BKTIMER0;
static UINT32 ir_div = 1;
static UINT32 ir_periodus = 50;
int txpin = 26;

int times[512];
int maxTimes = 512;
int *cur;
int *stop;
int myPeriodUs;
int curTime = 0;
int state = 0;

void Send_Init() {

	float frequency = 38000;
	float duty_cycle = 0.330000f;
	stop = times;
	const char *input = "x";
	// parse string like 10,12,432,432,432,432,432
	char *token = strtok(input, ",");
	while (token) {
		if (stop - times < maxTimes) {
			*stop = atoi(token);
			stop++;
		}
		else {
			break;
		}
		token = strtok(NULL, ",");
	}
	cur = times;
}

void Send_Tick() {
	if (cur == 0)
		return;
	curTime += myPeriodUs;
	int tg = *cur;
	if (tg <= curTime) {
		curTime -= tg;
		state = !state;
#if PLATFORM_BK7231N
		//bk_pwm_update_param((bk_pwm_t)pwmIndex, pwmperiod, duty, 0, 0);
#else
		//bk_pwm_update_param((bk_pwm_t)pwmIndex, pwmperiod, duty);
#endif

		cur++;
		if (cur == stop) {
			// done
			cur = 0;
		}
	}
}

int pwmIndex = -1;
uint32 period;
static commandResult_t CMD_IR2_Test1(const void* context, const char* cmd, const char* args, int cmdFlags) {
	pwmIndex = PIN_GetPWMIndexForPinIndex(txpin);
	// is this pin capable of PWM?
	if (pwmIndex != -1) {
		uint32_t pwmfrequency = 38000;
		period = (26000000 / pwmfrequency);
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
volatile UINT32 *gpio_cfg_addr;
void Test3_ISR(UINT8 t) {
	UINT32 reg_val = REG_READ(gpio_cfg_addr);
	reg_val ^= GCFG_OUTPUT_BIT;
	REG_WRITE(gpio_cfg_addr, reg_val);
}
static commandResult_t CMD_IR2_Test3(const void* context, const char* cmd, const char* args, int cmdFlags) {
	bk_gpio_config_output(txpin);

	int id = txpin;

	if (id >= GPIO32)
		id += 16;
	gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);

	Tokenizer_TokenizeString(args, 0);

	myPeriodUs = Tokenizer_GetArgIntegerDefault(0, 50);

	timer_param_t params = {
	 (unsigned char)ir_chan,
	 (unsigned char)ir_div, // div
	 myPeriodUs, // us
	 Test3_ISR
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

UINT8 current_channel;
UINT8 current_group;
static void pwm_set_duty_cycle(UINT8 ucChannel, UINT32 u32DutyCycle)
{
	UINT32 value;

	if (ucChannel < 2)
	{
		current_group = 0;
		current_channel = ucChannel;
	}
	else  if (ucChannel < 4)
	{
		current_group = 1;
		current_channel = ucChannel - 2;
	}
	else
	{
		current_group = 2;
		current_channel = ucChannel - 4;
	}

	//check last opreation work
	value = REG_READ(REG_PWM_GROUP_CTRL_ADDR(current_group));
	if (value & (PWM_GROUP_PWM_CFG_UPDATA_MASK(current_channel)))
	{

	}

	if (current_channel == 0)
	{
		REG_WRITE(REG_GROUP_PWM0_T1_ADDR(current_group), u32DutyCycle);
	}
	else
	{
		REG_WRITE(REG_GROUP_PWM1_T1_ADDR(current_group), u32DutyCycle);
	}

	// bit 7/15 :cfg_updata enable
	value = REG_READ(REG_PWM_GROUP_CTRL_ADDR(current_group));
	value |= PWM_GROUP_PWM_CFG_UPDATA_MASK(current_channel);
	REG_WRITE(REG_PWM_GROUP_CTRL_ADDR(current_group), value);

}

static commandResult_t CMD_IR2_TestDuty(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);

	float fduty = Tokenizer_GetArgFloatDefault(0, 0.5f);

	uint32 duty_cycle = period * fduty;

#if 1
	pwm_set_duty_cycle(pwmIndex, duty_cycle);
	///REG_WRITE(REG_APB_BK_PWMn_DC_ADDR(pwmIndex), duty_cycle);
#else
#define REG_APB_BK_PWMn_CNT_ADDR(n)         (PWM_BASE + 0x08 + 2 * 0x04 * (n))
	UINT32 value;
	value = (((UINT32)duty_cycle & 0x0000FFFF) << 16)
		+ ((UINT32)period & 0x0000FFFF);
	REG_WRITE(REG_APB_BK_PWMn_CNT_ADDR(pwmIndex), value);
#endif
	return CMD_RES_OK;
}

void DRV_IR2_Init() {
	CMD_RegisterCommand("Test1", CMD_IR2_Test1, NULL);
	CMD_RegisterCommand("Test2", CMD_IR2_Test2, NULL);
	CMD_RegisterCommand("Test3", CMD_IR2_Test3, NULL);
	CMD_RegisterCommand("TestDuty", CMD_IR2_TestDuty, NULL);



}

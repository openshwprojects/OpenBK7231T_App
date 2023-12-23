#include "../obk_config.h"


#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include <math.h>

#if PLATFORM_BK7231N
#include <BkDriverPwm.h>
#endif

int PIN_GetPWMIndexForPinIndex(int pin);


int myDuty(int value, int pwmfrequency) {
	//uint32_t value_upscaled = value * 10.0f; //Duty cycle 0...100 -> 0...1000
	uint32_t period = (26000000 / pwmfrequency); //TODO: Move to global variable and set in init func so it does not have to be recalculated every time...
	uint32_t duty = (value / 100.0 * period); //No need to use upscaled variable
	return duty;
}
static commandResult_t CMD_PWMG_Setup(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	int err;
	// OSStatus bk_pwm_group_initialize(bk_pwm_t pwm1,  bk_pwm_t pwm2, 
	// uint32_t frequency, uint32_t duty_cycle1, uint32_t duty_cycle2, uint32_t dead_band);
	int p1 = 6; // PWM0
	int p2 = 7; // PWM1
	int pwm1 = PIN_GetPWMIndexForPinIndex(p1);
	int pwm2 = PIN_GetPWMIndexForPinIndex(p2);
	int freq = 1000;
	int duty1 = (Tokenizer_GetArgIntegerDefault(0, 20));
	int duty2 = (Tokenizer_GetArgIntegerDefault(1, 20));
	int dead = (Tokenizer_GetArgIntegerDefault(2, 10));

#if PLATFORM_BK7231N
	err = bk_pwm_group_mode_disable(pwm1);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "bk_pwm_group_mode_disable %i", err);
	err = bk_pwm_group_initialize(pwm1, pwm2, freq, duty1, duty2, dead);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "bk_pwm_group_initialize %i", err);
	err = bk_pwm_group_mode_enable(pwm1);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "bk_pwm_group_mode_enable %i", err);
#endif

	return CMD_RES_OK;
}
// backlog startDriver PWMG; PWMG_Setup 300 200 10
// backlog startDriver PWMG; PWMG_Setup 100 500 10
void PWMG_Init() {

	CMD_RegisterCommand("PWMG_Setup", CMD_PWMG_Setup, NULL);
}




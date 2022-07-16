#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"

#include "BkDriverTimer.h"
#include "BkDriverGpio.h"
#include "sys_timer.h"
#include "gw_intf.h"

// HLW8012 aka BL0937

#define ELE_HW_TIME 1
#define HW_TIMER_ID 0


// Those can be set by Web page pins configurator
// The below are default values for Mycket smart socket
int GPIO_HLW_SEL = 24; // pwm4
int GPIO_HLW_CF = 7;
int GPIO_HLW_CF1 = 8;

bool g_sel = true;
uint32_t res_v = 0;
uint32_t res_c = 0;
uint32_t res_p = 0;
float calib_v = 0.13253012048f;
float calib_p = 1.5f;
float calib_c = 0.0118577075f;

volatile uint32_t g_vc_pulses = 0;
volatile uint32_t g_p_pulses = 0;

void HlwCf1Interrupt(unsigned char pinNum) {  // Service Voltage and Current
	g_vc_pulses++;
}
void HlwCfInterrupt(unsigned char pinNum) {  // Service Power
	g_p_pulses++;
}
int BL0937_PowerSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realPower;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	realPower = atof(args);
	calib_p = realPower / res_p;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realPower, calib_p);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return 0;
}
int BL0937_PowerRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	calib_p = atof(args);
	return 0;
}
int BL0937_CurrentRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	calib_c = atof(args);
	return 0;
}
int BL0937_VoltageRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	calib_v = atof(args);
	return 0;
}
int BL0937_VoltageSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realV;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	realV = atof(args);
	calib_v = realV / res_v;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realV, calib_v);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}

	return 0;
}
int BL0937_CurrentSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realI;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	realI = atof(args);
	calib_c = realI / res_c;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realI, calib_c);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return 0;
}
void BL0937_Init() {

	// if not found, this will return the already set value
	GPIO_HLW_SEL = PIN_FindPinIndexForRole(IOR_BL0937_SEL,GPIO_HLW_SEL);
	GPIO_HLW_CF = PIN_FindPinIndexForRole(IOR_BL0937_CF,GPIO_HLW_CF);
	GPIO_HLW_CF1 = PIN_FindPinIndexForRole(IOR_BL0937_CF1,GPIO_HLW_CF1);


	HAL_PIN_Setup_Output(GPIO_HLW_SEL);
    HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF1);

    gpio_int_enable(GPIO_HLW_CF1,IRQ_TRIGGER_FALLING_EDGE,HlwCf1Interrupt);

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF);
    gpio_int_enable(GPIO_HLW_CF,IRQ_TRIGGER_FALLING_EDGE,HlwCfInterrupt);

	CMD_RegisterCommand("PowerSet","",BL0937_PowerSet, "Sets current power value for calibration", NULL);
	CMD_RegisterCommand("VoltageSet","",BL0937_VoltageSet, "Sets current V value for calibration", NULL);
	CMD_RegisterCommand("CurrentSet","",BL0937_CurrentSet, "Sets current I value for calibration", NULL);
	CMD_RegisterCommand("PREF","",BL0937_PowerRef, "Sets the calibration multiplier", NULL);
	CMD_RegisterCommand("VREF","",BL0937_VoltageRef, "Sets the calibration multiplier", NULL);
	CMD_RegisterCommand("IREF","",BL0937_CurrentRef, "Sets the calibration multiplier", NULL);
}
void BL0937_RunFrame() {
	float final_v;
	float final_c;
	float final_p;


	if(g_sel) {
		res_v = g_vc_pulses;
		g_sel = false;
	} else {
		res_c = g_vc_pulses;
		g_sel = true;
	}
    HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);
	g_vc_pulses = 0;

	res_p = g_p_pulses;
	g_p_pulses = 0;

	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Voltage pulses %i, current %i, power %i\n", res_v, res_c, res_p);

	final_v = res_v * calib_v;
	final_c = res_c * calib_c;
	final_p = res_p * calib_p;
#if 0
	{
		char dbg[128];
		sprintf(dbg,"Voltage %f, current %f, power %f\n", final_v, final_c, final_p);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
#endif
	BL_ProcessUpdate(final_v,final_c,final_p);
}


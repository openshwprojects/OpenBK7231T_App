#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"

#if PLATFORM_BEKEN

#include "BkDriverTimer.h"
#include "BkDriverGpio.h"
#include "sys_timer.h"
#include "gw_intf.h"

// HLW8012 aka BL0937

#define ELE_HW_TIME 1
#define HW_TIMER_ID 0
#else

#endif


// Those can be set by Web page pins configurator
// The below are default values for Mycket smart socket
int GPIO_HLW_SEL = 24; // pwm4
int GPIO_HLW_CF = 7;
int GPIO_HLW_CF1 = 8;

//The above three actually are pin indices. For W600 the actual gpio_pins are different.
unsigned int GPIO_HLW_CF_pin;
unsigned int GPIO_HLW_CF1_pin;

bool g_sel = true;
uint32_t res_v = 0;
uint32_t res_c = 0;
uint32_t res_p = 0;
float BL0937_VREF = 0.13253012048f;
float BL0937_PREF = 1.5f;
float BL0937_CREF = 0.0118577075f;
float BL0937_PMAX = 3680.0f;
float last_p = 0.0f;

volatile uint32_t g_vc_pulses = 0;
volatile uint32_t g_p_pulses = 0;
static portTickType pulseStamp;

#if PLATFORM_W600

static void HlwCf1Interrupt(void* context) {
	tls_clr_gpio_irq_status(GPIO_HLW_CF1_pin);
	g_vc_pulses++;
}
static void HlwCfInterrupt(void* context) {
	tls_clr_gpio_irq_status(GPIO_HLW_CF_pin);
	g_p_pulses++;
}

#else

void HlwCf1Interrupt(unsigned char pinNum) {  // Service Voltage and Current
	g_vc_pulses++;
}
void HlwCfInterrupt(unsigned char pinNum) {  // Service Power
	g_p_pulses++;
}

#endif

commandResult_t BL0937_PowerSet(const void* context, const char* cmd, const char* args, int cmdFlags) {
	float realPower;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	realPower = atof(args);
	BL0937_PREF = realPower / res_p;

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER,BL0937_PREF);

	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"PowerSet: you gave %f, set ref to %f\n", realPower, BL0937_PREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return CMD_RES_OK;
}

commandResult_t BL0937_PowerMax(const void *context, const char *cmd, const char *args, int cmdFlags) {
    float maxPower;

    if(args==0||*args==0) {
        addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    maxPower = atof(args);
    if ((maxPower>200.0) && (maxPower<7200.0f))
    {
        BL0937_PMAX = maxPower;
        // UPDATE: now they are automatically saved
        CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER_MAX, BL0937_PMAX);           
        {
            char dbg[128];
            snprintf(dbg, sizeof(dbg),"PowerMax: set max to %f\n", BL0937_PMAX);
            addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
        }
    }
    return CMD_RES_OK;
}

commandResult_t BL0937_PowerRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BL0937_PREF = atof(args);

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER,BL0937_PREF);

	return CMD_RES_OK;
}
commandResult_t BL0937_CurrentRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BL0937_CREF = atof(args);

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT,BL0937_CREF);

	return CMD_RES_OK;
}
commandResult_t BL0937_VoltageRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BL0937_VREF = atof(args);

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE,BL0937_VREF);

	return CMD_RES_OK;
}
commandResult_t BL0937_VoltageSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realV;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	realV = atof(args);
	BL0937_VREF = realV / res_v;

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE,BL0937_VREF);

	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"VoltageSet: you gave %f, set ref to %f\n", realV, BL0937_VREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}

	return CMD_RES_OK;
}
commandResult_t BL0937_CurrentSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realI;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	realI = atof(args);
	BL0937_CREF = realI / res_c;

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT,BL0937_CREF);

	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"CurrentSet: you gave %f, set ref to %f\n", realI, BL0937_CREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return CMD_RES_OK;
}

void BL0937_Shutdown_Pins()
{
#if PLATFORM_W600
	tls_gpio_irq_disable(GPIO_HLW_CF1_pin);
	tls_gpio_irq_disable(GPIO_HLW_CF_pin);
#elif PLATFORM_BEKEN
	gpio_int_disable(GPIO_HLW_CF1);
	gpio_int_disable(GPIO_HLW_CF);
#endif
}

void BL0937_Init_Pins() {
	// if not found, this will return the already set value
	GPIO_HLW_SEL = PIN_FindPinIndexForRole(IOR_BL0937_SEL, GPIO_HLW_SEL);
	GPIO_HLW_CF = PIN_FindPinIndexForRole(IOR_BL0937_CF, GPIO_HLW_CF);
	GPIO_HLW_CF1 = PIN_FindPinIndexForRole(IOR_BL0937_CF1, GPIO_HLW_CF1);

#if PLATFORM_W600
	GPIO_HLW_CF1_pin = HAL_GetGPIOPin(GPIO_HLW_CF1);
	GPIO_HLW_CF_pin = HAL_GetGPIOPin(GPIO_HLW_CF);
	//printf("GPIO_HLW_CF=%d GPIO_HLW_CF1=%d\n", GPIO_HLW_CF, GPIO_HLW_CF1);
	//printf("GPIO_HLW_CF1_pin=%d GPIO_HLW_CF_pin=%d\n", GPIO_HLW_CF1_pin, GPIO_HLW_CF_pin);
#endif

	// UPDATE: now they are automatically saved
	BL0937_VREF = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, BL0937_VREF);
	BL0937_PREF = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, BL0937_PREF);
	BL0937_CREF = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, BL0937_CREF);
	BL0937_PMAX = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER_MAX, BL0937_PMAX);

	HAL_PIN_Setup_Output(GPIO_HLW_SEL);
	HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF1);

#if PLATFORM_W600
	tls_gpio_isr_register(GPIO_HLW_CF1_pin, HlwCf1Interrupt, NULL);
	tls_gpio_irq_enable(GPIO_HLW_CF1_pin, WM_GPIO_IRQ_TRIG_FALLING_EDGE);
#elif PLATFORM_BEKEN
	gpio_int_enable(GPIO_HLW_CF1, IRQ_TRIGGER_FALLING_EDGE, HlwCf1Interrupt);
#endif

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF);

#if PLATFORM_W600
	tls_gpio_isr_register(GPIO_HLW_CF_pin, HlwCfInterrupt, NULL);
	tls_gpio_irq_enable(GPIO_HLW_CF_pin, WM_GPIO_IRQ_TRIG_FALLING_EDGE);
#elif PLATFORM_BEKEN
	gpio_int_enable(GPIO_HLW_CF, IRQ_TRIGGER_FALLING_EDGE, HlwCfInterrupt);
#endif

	g_vc_pulses = 0;
	g_p_pulses = 0;
	pulseStamp = xTaskGetTickCount();
}
void BL0937_Init() 
{
    BL_Shared_Init();

	//cmddetail:{"name":"PowerSet","args":"",
	//cmddetail:"descr":"Sets current power value for calibration",
	//cmddetail:"fn":"BL0937_PowerSet","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PowerSet",BL0937_PowerSet, NULL);
	//cmddetail:{"name":"VoltageSet","args":"",
	//cmddetail:"descr":"Sets current V value for calibration",
	//cmddetail:"fn":"BL0937_VoltageSet","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("VoltageSet",BL0937_VoltageSet, NULL);
	//cmddetail:{"name":"CurrentSet","args":"",
	//cmddetail:"descr":"Sets current I value for calibration",
	//cmddetail:"fn":"BL0937_CurrentSet","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("CurrentSet",BL0937_CurrentSet, NULL);
	//cmddetail:{"name":"PREF","args":"",
	//cmddetail:"descr":"Sets the calibration multiplier",
	//cmddetail:"fn":"BL0937_PowerRef","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PREF",BL0937_PowerRef, NULL);
	//cmddetail:{"name":"VREF","args":"",
	//cmddetail:"descr":"Sets the calibration multiplier",
	//cmddetail:"fn":"BL0937_VoltageRef","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("VREF",BL0937_VoltageRef, NULL);
	//cmddetail:{"name":"IREF","args":"",
	//cmddetail:"descr":"Sets the calibration multiplier",
	//cmddetail:"fn":"BL0937_CurrentRef","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("IREF",BL0937_CurrentRef, NULL);
	//cmddetail:{"name":"PowerMax","args":"[limit]",
	//cmddetail:"descr":"Sets Maximum power value measurement limiter",
	//cmddetail:"fn":"BL0937_PowerMax","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("PowerMax",BL0937_PowerMax, NULL);

	BL0937_Init_Pins();
}

void BL0937_RunFrame()
{
	float final_v;
	float final_c;
	float final_p;
	bool bNeedRestart;
	portTickType ticksElapsed;

	bNeedRestart = false;
	if (GPIO_HLW_SEL != PIN_FindPinIndexForRole(IOR_BL0937_SEL, GPIO_HLW_SEL)) {
		bNeedRestart = true;
	}
	if (GPIO_HLW_CF != PIN_FindPinIndexForRole(IOR_BL0937_CF, GPIO_HLW_CF)) {
		bNeedRestart = true;
	}
	if (GPIO_HLW_CF1 != PIN_FindPinIndexForRole(IOR_BL0937_CF1, GPIO_HLW_CF1)) {
		bNeedRestart = true;
	}

	ticksElapsed = (xTaskGetTickCount() - pulseStamp);

#if PLATFORM_BEKEN
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
#else

#endif

#if 1
	if (bNeedRestart) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "BL0937 pins have changed, will reset the interrupts");

		BL0937_Shutdown_Pins();
		BL0937_Init_Pins();
#if PLATFORM_BEKEN
		GLOBAL_INT_RESTORE();
#else

#endif
		return;
	}


#endif
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
#if PLATFORM_BEKEN
    GLOBAL_INT_RESTORE();
#else

#endif

	pulseStamp = xTaskGetTickCount();
	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Voltage pulses %i, current %i, power %i\n", res_v, res_c, res_p);

	final_v = res_v * BL0937_VREF;
	final_v *= (float)ticksElapsed;
	final_v /= (1000.0f / (float)portTICK_PERIOD_MS);

	final_c = res_c * BL0937_CREF;
	final_c *= (float)ticksElapsed;
	final_c /= (1000.0f / (float)portTICK_PERIOD_MS);

	final_p = res_p * BL0937_PREF;
	final_p *= (float)ticksElapsed;
	final_p /= (1000.0f / (float)portTICK_PERIOD_MS);

    /* patch to limit max power reading, filter random reading errors */
    if (final_p > BL0937_PMAX)
    {
        /* MAX value breach, use last value */
        {
            char dbg[128];
            snprintf(dbg, sizeof(dbg),"Power reading: %f exceeded MAX limit: %f, Last: %f\n", final_p, BL0937_PMAX, last_p);
            addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
        }
        final_p = last_p;
    } else {
        /* Valid value save for next time */
        last_p = final_p;
    }    
#if 0
	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"Voltage %f, current %f, power %f\n", final_v, final_c, final_p);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
#endif
	BL_ProcessUpdate(final_v,final_c,final_p);
}


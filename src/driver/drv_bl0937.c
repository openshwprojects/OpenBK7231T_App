// HLW8012 aka BL0937
#include "drv_bl0937.h"
#include "../obk_config.h"
#include "../hal/hal_pins.h"
#include "../new_pins.h"

#if ENABLE_DRIVER_BL0937

//dummy
#include <math.h>

#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_pwrCal.h"
#include "drv_uart.h"

#define DEFAULT_VOLTAGE_CAL 0.13253012048f
#define DEFAULT_CURRENT_CAL 0.0118577075f
#define DEFAULT_POWER_CAL 1.5f

// Those can be set by Web page pins configurator
// The below are default values for Mycket smart socket
int GPIO_HLW_SEL = 24; // pwm4
bool g_invertSEL = false;
int GPIO_HLW_CF = 7;
int GPIO_HLW_CF1 = 8;

bool g_sel = true;
uint32_t res_v = 0;
uint32_t res_c = 0;
uint32_t res_p = 0;
float BL0937_PMAX = 3680.0f;
float last_p = 0.0f;

volatile uint32_t g_vc_pulses = 0;
volatile uint32_t g_p_pulses = 0;
static portTickType pulseStamp;


void HlwCf1Interrupt(int pinNum)
{
	g_vc_pulses++;
}
void HlwCfInterrupt(int pinNum)
{
	g_p_pulses++;
}

commandResult_t BL0937_PowerMax(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	float maxPower;

	if(args == 0 || *args == 0)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	maxPower = atof(args);
	if((maxPower > 200.0) && (maxPower < 7200.0f))
	{
		BL0937_PMAX = maxPower;
		// UPDATE: now they are automatically saved
		CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER_MAX, BL0937_PMAX);
		{
			char dbg[128];
			snprintf(dbg, sizeof(dbg), "PowerMax: set max to %f\n", BL0937_PMAX);
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
		}
	}
	return CMD_RES_OK;
}

void BL0937_Shutdown_Pins()
{
	HAL_DetachInterrupt(GPIO_HLW_CF);
	HAL_DetachInterrupt(GPIO_HLW_CF1);
}

void BL0937_Init_Pins()
{
	int tmp;

	// if not found, this will return the already set value
	tmp = PIN_FindPinIndexForRole(IOR_BL0937_SEL_n, -1);
	if(tmp != -1)
	{
		g_invertSEL = true;
		GPIO_HLW_SEL = tmp;
	}
	else
	{
		g_invertSEL = false;
		GPIO_HLW_SEL = PIN_FindPinIndexForRole(IOR_BL0937_SEL, GPIO_HLW_SEL);
	}
	GPIO_HLW_CF = PIN_FindPinIndexForRole(IOR_BL0937_CF, GPIO_HLW_CF);
	GPIO_HLW_CF1 = PIN_FindPinIndexForRole(IOR_BL0937_CF1, GPIO_HLW_CF1);

	BL0937_PMAX = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER_MAX, BL0937_PMAX);

	HAL_PIN_Setup_Output(GPIO_HLW_SEL);
	HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF1);
	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF);

	HAL_AttachInterrupt(GPIO_HLW_CF, INTERRUPT_STUB, HlwCfInterrupt);
	HAL_AttachInterrupt(GPIO_HLW_CF1, INTERRUPT_STUB, HlwCf1Interrupt);

	g_vc_pulses = 0;
	g_p_pulses = 0;
	pulseStamp = xTaskGetTickCount();
}

void BL0937_Init(void)
{
	BL_Shared_Init();

	PwrCal_Init(PWR_CAL_MULTIPLY, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
		DEFAULT_POWER_CAL);

	//cmddetail:{"name":"PowerMax","args":"[MaxPowerInW]",
	//cmddetail:"descr":"Sets the maximum power limit for BL measurement used to filter incorrect values",
	//cmddetail:"fn":"BL0937_PowerMax","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PowerMax", BL0937_PowerMax, NULL);

	BL0937_Init_Pins();
}

void BL0937_RunEverySecond(void)
{
	float final_v;
	float final_c;
	float final_p;
	bool bNeedRestart;
	portTickType ticksElapsed;
	portTickType xPassedTicks;

	bNeedRestart = false;
	if(g_invertSEL)
	{
		if(GPIO_HLW_SEL != PIN_FindPinIndexForRole(IOR_BL0937_SEL_n, GPIO_HLW_SEL))
		{
			bNeedRestart = true;
		}
	}
	else
	{
		if(GPIO_HLW_SEL != PIN_FindPinIndexForRole(IOR_BL0937_SEL, GPIO_HLW_SEL))
		{
			bNeedRestart = true;
		}
	}
	if(GPIO_HLW_CF != PIN_FindPinIndexForRole(IOR_BL0937_CF, GPIO_HLW_CF))
	{
		bNeedRestart = true;
	}
	if(GPIO_HLW_CF1 != PIN_FindPinIndexForRole(IOR_BL0937_CF1, GPIO_HLW_CF1))
	{
		bNeedRestart = true;
	}


#if PLATFORM_BEKEN
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
#else

#endif

#if 1
	if(bNeedRestart)
	{
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
	if(g_sel)
	{
		if(g_invertSEL)
		{
			res_c = g_vc_pulses;
		}
		else
		{
			res_v = g_vc_pulses;
		}
		g_sel = false;
	}
	else
	{
		if(g_invertSEL)
		{
			res_v = g_vc_pulses;
		}
		else
		{
			res_c = g_vc_pulses;
		}
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
	xPassedTicks = xTaskGetTickCount();
	ticksElapsed = (xPassedTicks - pulseStamp);
	pulseStamp = xPassedTicks;
	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Voltage pulses %i, current %i, power %i\n", res_v, res_c, res_p);

	PwrCal_Scale(res_v, res_c, res_p, &final_v, &final_c, &final_p);

	final_v *= (1000.0f / (float)portTICK_PERIOD_MS);
	final_v /= (float)ticksElapsed;

	final_c *= (1000.0f / (float)portTICK_PERIOD_MS);
	final_c /= (float)ticksElapsed;

	final_p *= (1000.0f / (float)portTICK_PERIOD_MS);
	final_p /= (float)ticksElapsed;

	/* patch to limit max power reading, filter random reading errors */
	if(final_p > BL0937_PMAX)
	{
		/* MAX value breach, use last value */
		{
			char dbg[128];
			snprintf(dbg, sizeof(dbg), "Power reading: %f exceeded MAX limit: %f, Last: %f\n", final_p, BL0937_PMAX, last_p);
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
		}
		final_p = last_p;
	}
	else
	{
		/* Valid value save for next time */
		last_p = final_p;
	}
#if 0
	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg), "Voltage %f, current %f, power %f\n", final_v, final_c, final_p);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
	}
#endif
	BL_ProcessUpdate(final_v, final_c, final_p, NAN, NAN);
}

// close ENABLE_DRIVER_BL0937
#endif


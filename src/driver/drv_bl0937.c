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

//#define DEFAULT_VOLTAGE_CAL 0.13253012048f
//#define DEFAULT_CURRENT_CAL 0.0118577075f
//#define DEFAULT_POWER_CAL 1.5f

#define DEFAULT_VOLTAGE_CAL 0.15813353251f 	//Vref=1.218, R1=6*330kO, R2=1kO, K=15397
#define DEFAULT_CURRENT_CAL 0.01287009447f 	//Vref=1.218, Rs=1mO, K=94638
#define DEFAULT_POWER_CAL 1.72265706655f 	//Vref=1.218, R1=6*330kO, R2=1kO, Rs=1mO, K=1721506

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

volatile uint32_t g_v_pulses = 0, g_c_pulses = 0;
volatile portTickType g_ticksElapsed_v=0, g_ticksElapsed_c=0;
volatile uint32_t g_p_pulses = 0, g_p_pulsesprev = 0;
volatile portTickType g_pulseStampPrev_v=0, g_pulseStampPrev_c=0, g_pulseStampPrev_p=0;
static int g_sht_secondsUntilNextMeasurement = 1, g_sht_secondsBetweenMeasurements = 20;


void HlwCf1Interrupt(int pinNum)
{
//		g_vc_pulses++;
	if (g_sel) {
		g_v_pulses++;
	} else {
		g_c_pulses++;
	}
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

//	g_vc_pulses = 0;
	g_v_pulses = 0;
	g_c_pulses = 0;
	g_p_pulses = 0;
	g_pulseStampPrev_v = xTaskGetTickCount();
	g_pulseStampPrev_c = g_pulseStampPrev_v;
	g_pulseStampPrev_p = g_pulseStampPrev_v;

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
	float freq_v_mhz, freq_c_mhz, freq_p_mhz;
	bool bNeedRestart;
//	portTickType ticksElapsed, ticksElapsed_p;
	portTickType ticksElapsed_p;
//	portTickType xPassedTicks;
	portTickType pulseStampNow;
	float power_cal_cur;
	float current_cal_cur;
	float voltage_cal_cur;
	
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
// V and I measurement must be done/changed every second
 	pulseStampNow = xTaskGetTickCount();
	if (g_p_pulses >= g_p_pulsesprev) {
		res_p = g_p_pulses-g_p_pulsesprev;
	} else {
		res_p = (0xFFFFFFFF - g_p_pulsesprev) + g_p_pulses + 1;
	}
	if ( g_sht_secondsUntilNextMeasurement %1 < 1 || res_p > 2) { //|| res_c > 0
		if( (g_sel && g_invertSEL) || (!g_sel && !g_invertSEL))	{
			//res_c = g_vc_pulses;
			res_c = g_c_pulses;
//			g_ticksElapsed_c = ticksElapsed;
			if (pulseStampNow >= g_pulseStampPrev_c) {
				g_ticksElapsed_c = pulseStampNow-g_pulseStampPrev_c;
			} else {
				g_ticksElapsed_c = (0xFFFFFFFF - g_pulseStampPrev_c) + pulseStampNow + 1;
			}
			g_pulseStampPrev_c = pulseStampNow;
			g_c_pulses=0;
		} else if( (g_sel && !g_invertSEL) || (!g_sel && g_invertSEL)) {
//			res_v = g_vc_pulses;
			res_v = g_v_pulses;
			if (pulseStampNow >= g_pulseStampPrev_v) {
				g_ticksElapsed_v = pulseStampNow-g_pulseStampPrev_v;
			} else {
				g_ticksElapsed_v = (0xFFFFFFFF - g_pulseStampPrev_v) + pulseStampNow + 1;
			}
			g_pulseStampPrev_v = pulseStampNow;
			g_v_pulses=0;
//			g_ticksElapsed_v = ticksElapsed;
		}
		g_sel=!g_sel;
	}

	HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);
	//g_vc_pulses = 0;
	
	#if PLATFORM_BEKEN
		GLOBAL_INT_RESTORE();
	#else
	
	#endif

	if (g_sht_secondsUntilNextMeasurement <= 0 || res_p > 8) {
//change to calc with overflow, assume always 32bit 
//		xPassedTicks = xTaskGetTickCount();
//		ticksElapsed = (xPassedTicks - pulseStamp);
//		pulseStamp = xPassedTicks;
		if (pulseStampNow >= g_pulseStampPrev_p) {
			ticksElapsed_p = pulseStampNow-g_pulseStampPrev_p;
		} else {
			ticksElapsed_p = (0xFFFFFFFF - g_pulseStampPrev_p) + pulseStampNow + 1;
		}

		//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Voltage pulses %i, current %i, power %i\n", res_v, res_c, res_p);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Volt pulses %i / (ts %i) vticks %i, current %i /  (ts %i) cticks %i, power %i / pticks %i (prev %i) tsnow %i \n"
			, res_v,  g_pulseStampPrev_v, g_ticksElapsed_v, res_c,  g_pulseStampPrev_c, g_ticksElapsed_c, res_p, ticksElapsed_p, g_pulseStampPrev_p, pulseStampNow);
	
		//do not reset, calc considering overflow
		g_p_pulsesprev = g_p_pulses;
		g_pulseStampPrev_p = pulseStampNow;
		

//		PwrCal_Scale(res_v, res_c, res_p, &final_v, &final_c, &final_p);

//		voltage_cal = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, default_voltage_cal);
//	    current_cal = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, default_current_cal);
//	    power_cal = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, default_power_cal);
		voltage_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, DEFAULT_VOLTAGE_CAL);
	    current_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, DEFAULT_CURRENT_CAL);
	    power_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, DEFAULT_POWER_CAL);
	
		//Vref=1.218, R1=6*330kO, R2=1kO, K=15397
		//Vref=1.218, Rs=1mO, K=94638
		//Vref=1.218, R1=6*330kO, R2=1kO, Rs=1mO, K=1721506
	
//		final_v = (float)res_v * (1000.0f / (float)portTICK_PERIOD_MS);
//		final_v /= (float)g_ticksElapsed_v;
		freq_v_mhz = (float)res_v / (float)g_ticksElapsed_v;
		freq_v_mhz /= (1000.0f / (float)portTICK_PERIOD_MS);
//		final_v = (float)freq_v_mhz;
		final_v = freq_v_mhz * 1.218f;
	//	final_v /= 15397.0f;	
		final_v /= 15397.0f;
		final_v *= (330.0f*6.0f + 1.0f) / 1.0f;

	//	final_v *= voltage_cal_cur; 
	
//		final_c = (float)res_v * (1000.0f / (float)portTICK_PERIOD_MS);
//		final_c /= (float)g_ticksElapsed_c;
		freq_c_mhz = (float)res_c / (float)g_ticksElapsed_c;
		freq_c_mhz /= (1000.0f / (float)portTICK_PERIOD_MS);
//		final_c = (float)freq_c_mhz;
		final_c = freq_c_mhz * 1.218f;
//		final_c /= 94638.0f;	
		final_c /= 94638.0f;
		final_c *= 1000.0f; // to Amps from mA	

	//final_c *= current_cal_cur;		
			
//		final_p = (float)res_p * (1000.0f /  (float)portTICK_PERIOD_MS);
//		final_p /= (float)ticksElapsed_p;
		freq_p_mhz = (float)res_p / (float)ticksElapsed_p;
		freq_p_mhz /= (1000.0f / (float)portTICK_PERIOD_MS);
//		final_p = (float)freq_p_mhz;
		final_p = freq_p_mhz * 1.218f;
		final_p *= 1.218f;
		final_p /= 1721506.0f;
		final_p *= ((330.0f*6.0f + 1.0f) / 1.0f);	
		final_p /= 1000.0f; 
	
	//	final_p *= power_cal_cur;

		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"v202511062255 Scaled v %.1f (vf %.1f) c %.4f (cf %.3f) p %.2f  (pf %.2f) . Scalefact v %.6f c %.6f p %.6f tick_period_ms %.2f"
			,final_v, freq_v_mhz, final_c, freq_c_mhz, final_p, freq_p_mhz, voltage_cal_cur, current_cal_cur, power_cal_cur, (float)portTICK_PERIOD_MS);

		if (voltage_cal_cur != DEFAULT_VOLTAGE_CAL && power_cal_cur != DEFAULT_POWER_CAL) {
			float v_cal2p=DEFAULT_VOLTAGE_CAL/voltage_cal_cur;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"Scaled v %.1f c %.4f p %.2f (/v_cal2p %.6f=%.2f), pcal %f\n", final_v, final_c, final_p, v_cal2p, final_p / v_cal2p, power_cal_cur, portTICK_PERIOD_MS);
			final_p /= v_cal2p;
		}

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
		g_sht_secondsUntilNextMeasurement = g_sht_secondsBetweenMeasurements;
	}
	if (g_sht_secondsUntilNextMeasurement > 0) {
		g_sht_secondsUntilNextMeasurement--;
	}

}

// close ENABLE_DRIVER_BL0937
#endif


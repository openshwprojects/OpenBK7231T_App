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
volatile portTickType g_pulseStampStart_v=0, g_pulseStampStart_c=0, g_pulseStampStart_p=0;
#define PULSESTAMPDEBUG 0
#if PULSESTAMPDEBUG>0
volatile portTickType g_pulseStampEnd_v=0, g_pulseStampEnd_c=0;
#endif
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
	g_pulseStampStart_v = xTaskGetTickCount();
	g_pulseStampStart_c = g_pulseStampStart_v;
	g_pulseStampStart_p = g_pulseStampStart_v;

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
	float freq_v, freq_c, freq_p; //range during calculation quite dynamic
//	bool valid_v=false, valid_c=false, valid_p=false;
	bool g_sel_change=false;
	bool bNeedRestart;
//	portTickType ticksElapsed, ticksElapsed_p;
	portTickType ticksElapsed_p;
//	portTickType xPassedTicks;
	portTickType pulseStampNow;	
#if PULSESTAMPDEBUG>0
	portTickType pulseStamp_g_sel_change=0;
#endif
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
	if ((g_sel && !g_invertSEL) || (!g_sel && g_invertSEL)) {
//	if ( g_sel ) {		
		if ( g_v_pulses>200 )  { //valid reading
#if PULSESTAMPDEBUG>0
			g_pulseStampEnd_v = pulseStampNow;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %i gsel=%i vp %i tsv %i %i \n"
				, pulseStampNow, g_sel, g_v_pulses, g_pulseStampStart_v, g_pulseStampEnd_v);
#endif
			if (pulseStampNow >= g_pulseStampStart_v) {
				g_ticksElapsed_v = pulseStampNow-g_pulseStampStart_v;
			} else {
				g_ticksElapsed_v = (0xFFFFFFFF - g_pulseStampStart_v) + pulseStampNow + 1;
			}
			res_v = g_v_pulses;
			g_v_pulses=0;
//			valid_v=true;
		} else {
//			res_v = 0;
//			valid_v=false;
		}
//		g_sel=false;
		g_pulseStampStart_c = pulseStampNow;
		g_sel_change=true;
	} else if( (g_sel && g_invertSEL) || (!g_sel && !g_invertSEL))	{
//	} else {
		if ( g_c_pulses>3 || g_sht_secondsUntilNextMeasurement <= 0 )  { //reading high enough or max sample time
#if PULSESTAMPDEBUG>0
			g_pulseStampEnd_c = pulseStampNow;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %i gsel=%i vc %i tsc %i %i \n"
				, pulseStampNow, g_sel, g_c_pulses, g_pulseStampStart_c, g_pulseStampEnd_c);
#endif
			if (pulseStampNow >= g_pulseStampStart_c) {
				g_ticksElapsed_c = pulseStampNow-g_pulseStampStart_v;
			} else {
				g_ticksElapsed_c = (0xFFFFFFFF - g_pulseStampStart_c) + pulseStampNow + 1;
			}
			res_c = g_c_pulses;
			g_c_pulses=0;
//			valid_c=true;
//			g_sel=true;
			g_pulseStampStart_v = pulseStampNow;
			g_sel_change=true;
		} else {
//			res_c = 0;
//			valid_c=false;
			g_sel_change=false;
		}
	} 
#if PULSESTAMPDEBUG>0
 	addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %i gsel=%i g_sel_change %i \n"
		, pulseStampNow, g_sel, g_sel_change);
#endif

	if ( g_sel_change) {
		g_sel = !g_sel;
		HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);
#if PULSESTAMPDEBUG>0
		pulseStamp_g_sel_change= pulseStampNow;
#endif
	}
	
	#if PLATFORM_BEKEN
		GLOBAL_INT_RESTORE();
	#else
	
	#endif

	if (g_p_pulses >= g_p_pulsesprev) {
		res_p = g_p_pulses-g_p_pulsesprev;
	} else {
		res_p = (0xFFFFFFFF - g_p_pulsesprev) + g_p_pulses + 1;
	}

	if (res_p > 5 || g_sht_secondsUntilNextMeasurement <= 0 ) {
		if (pulseStampNow >= g_pulseStampStart_p) {
			ticksElapsed_p = pulseStampNow-g_pulseStampStart_p;
		} else {
			ticksElapsed_p = (0xFFFFFFFF - g_pulseStampStart_p) + pulseStampNow + 1;
		}

#if PULSESTAMPDEBUG>0
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"Volt pulses %i / (tsend %i) vticks %i, current %i /  (tsend %i) cticks %i, power %i / pticks %i (prev %i) tsnow %i gselchg %i\n"
			, res_v, g_pulseStampEnd_v, g_ticksElapsed_v
			, res_c, g_pulseStampEnd_c, g_ticksElapsed_c
			, res_p, ticksElapsed_p, g_pulseStampStart_p, pulseStampNow, pulseStamp_g_sel_change);
#else
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"Volt pulses %i / (tsstrt %i) vticks %i, current %i /  (tsstrt %i) cticks %i, power %i / pticks %i (prev %i) tsnow %i \n"
			, res_v, g_pulseStampStart_v, g_ticksElapsed_v
			, res_c, g_pulseStampStart_c, g_ticksElapsed_c
			, res_p, ticksElapsed_p, g_pulseStampStart_p, pulseStampNow);
#endif	
		//do not reset, calc considering overflow
		g_p_pulsesprev = g_p_pulses;
		g_pulseStampStart_p = pulseStampNow;

		// at reference design (6x330kO / 1kO), 1mO, [lower with Tuya Plug (~2.030MO)]
		//frequency V: 80..250V = 550.5 .. 1595.3 HZ [-50Hz], 10mV=0.063812 [0.062241] Hz
		//frequency C: .001 .. 20A = 0.077700 1553,99 Hz, 0.1mA=0.007770 Hz
		//frequency P:  @80V: 0.08 .. 1600W = 0.046862 ..  937.237 Hz, 0,1W=0,058577 [0.057135] Hz
		//frequency P: @250V: 0.25 .. 5000W = 0.146443 .. 2928.866 Hz, 0,1W=0,058577 [0.057135] Hz

		voltage_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, DEFAULT_VOLTAGE_CAL);
	    current_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, DEFAULT_CURRENT_CAL);
	    power_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, DEFAULT_POWER_CAL);
	
		//Vref=1.218, R1=6*330kO, R2=1kO, K=15397
		//Vref=1.218, Rs=1mO, K=94638
		//Vref=1.218, R1=6*330kO, R2=1kO, Rs=1mO, K=1721506
	
		if (g_ticksElapsed_v > 0) {
			freq_v = (float)res_v * (1000.0f / (float)portTICK_PERIOD_MS);
			freq_v /= (float)g_ticksElapsed_v;
/*			final_v = freq_v * 1.218f;
			final_v /= 15397.0f;
			final_v *= (330.0f*6.0f + 1.0f) / 1.0f; //voltage divider
*/
		} else {
			final_v = 11.1;	
			freq_v = 9999;
		}
	
		if (g_ticksElapsed_c > 0) {
			freq_c = (float)res_c * (1000.0f / (float)portTICK_PERIOD_MS);
			freq_c /= (float)g_ticksElapsed_c;
/*			final_c = freq_c * 1.218f;
			final_c /= 94638.0f;
			final_c *= 1000.0f; // Rs=	
*/
		} else {
			final_c = 22.222f;	
			freq_c = 9999;
		}
		if (ticksElapsed_p > 0) {
			freq_p = (float)res_p * (1000.0f / (float)portTICK_PERIOD_MS);	
			freq_p /= (float)ticksElapsed_p;
			// accurate volt measurements is easier --> apply to power automatically
			// inaccuracy because of voltage divider deviation (fe.g. from datasheet) while V and P freq calc use same inputV to chip V(v)
			if (voltage_cal_cur != DEFAULT_VOLTAGE_CAL && power_cal_cur == DEFAULT_POWER_CAL) {
				float v_cal2p=DEFAULT_VOLTAGE_CAL/voltage_cal_cur;
				addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"Scaled v %.1f c %.4f p %.2f (/v_cal2p %.6f=%.2f), pcal %f\n", final_v, final_c, final_p, v_cal2p, final_p / v_cal2p, power_cal_cur, portTICK_PERIOD_MS);
				final_p /= v_cal2p;
			}
/*			final_p = freq_p * 1.218f;
			final_p *= 1.218f;
			final_p /= 1721506.0f;
			final_p *= ((330.0f*6.0f + 1.0f) / 1.0f);	
			final_p /= 1000.0f; 
*/	
		} else {
			final_p = 9999.99f;	
			freq_p = 9999;
		}	
		PwrCal_Scale((int)(freq_v*100), (float)(freq_c*1000), (int)(freq_p*1000), &final_v, &final_c, &final_p);

		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Scaled v %.1f (vf %.2f) c %.4f (cf %.3f) p %.2f  (pf %3f) . Scalefact v %.6f c %.6f p %.6f "
			,final_v, freq_v, final_c, freq_c, final_p, freq_p, voltage_cal_cur, current_cal_cur, power_cal_cur);
		
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

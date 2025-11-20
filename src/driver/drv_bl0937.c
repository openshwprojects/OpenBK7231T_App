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

#define DEFAULT_VOLTAGE_CAL 1.581335325E-02f 	//factor 10 for precision (resolution should be ~16mV) Vref=1.218, R1=6*330kO, R2=1kO, K=15397
#define DEFAULT_VOLTAGE_FREQMULTIPLY 10.0f
#define DEFAULT_CURRENT_CAL 1.287009447E-02f	//no factor because scale uses float Vref=1.218, Rs=1mO, K=94638
#define DEFAULT_CURRENT_FREQMULTIPLY 1.0f
#define DEFAULT_POWER_CAL 	1.707145397E-03f 	//factor 1000 for precision (resolution should be ~1.75mW) Vref=1.218, R1=6*330kO, R2=1kO, Rs=1mO, K=1721506
#define DEFAULT_POWER_FREQMULTIPLY 1000.0f

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
static portTickType g_ticksElapsed_v=1, g_ticksElapsed_c=1; //initial value > 0 to enable calculation at first run
volatile uint32_t g_p_pulses = 0, g_p_pulsesprev = 0;
static portTickType g_pulseStampStart_v=0, g_pulseStampStart_c=0, g_pulseStampStart_p=0;
#define PULSESTAMPDEBUG 0
#if PULSESTAMPDEBUG>0
volatile portTickType g_pulseStampEnd_v=0, g_pulseStampEnd_c=0;
#endif

#define NEW_UPDATE_CYCLES 0
#if NEW_UPDATE_CYCLES>0
	static uint32_t g_minPulsesV = 200, g_minPulsesC = 3, g_minPulsesP = 5; 
	static uint32_t g_bl_secUntilNextCalc= 1, g_bl_secForceNextCalc = 30, g_bl_secMinNextCalc = 3; //at 230V minintervall/Pdiff 15s=0.115W, 7.5s=0.23W, 
#else
	static uint32_t g_bl_secUntilNextCalc= 0, g_bl_secForceNextCalc = 0, g_bl_secMinNextCalc = 0; //at 230V minintervall/Pdiff 15s=0.115W, 7.5s=0.23W, 3s=0.575W, 1.5s=1.15W, 1s=1.73W
	static uint32_t g_minPulsesV = 0, g_minPulsesC = 0, g_minPulsesP = 0;	// keep behaviour for compatibility
#endif

static float g_freqmultiplierV=DEFAULT_VOLTAGE_FREQMULTIPLY, g_freqmultiplierP=DEFAULT_POWER_FREQMULTIPLY; //not needed for C because float used
static float g_p_forceonroc=0.0f; 
static int g_forceonroc_gtlim=-1;
static float g_p_prevsec=0;
static uint32_t g_p_pulsesprevsec = 0;

//static	uint8_t cyclecnt=0, cyclecnt_prev=0;
static portTickType g_pulseStampTestPrev=0;

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

commandResult_t BL0937_cmdPowerMax(const void* context, const char* cmd, const char* args, int cmdFlags)
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

commandResult_t BL0937_cmdIntervalCPMinMax(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_IntervalCPMinMax: not enough arguments, current values %lu %lu."
			, g_bl_secMinNextCalc, g_bl_secForceNextCalc);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	int minCycleTime = Tokenizer_GetArgInteger(0);
	int maxCycleTime = Tokenizer_GetArgInteger(1);
	if( minCycleTime < 1 || maxCycleTime < minCycleTime || maxCycleTime > 900 || minCycleTime > maxCycleTime)
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_IntervalCPMinMax: minimum 1 second, maximum 900 seconds allowed, min %i between max %i (keep current %i %i)."
			, minCycleTime, maxCycleTime, g_bl_secMinNextCalc, g_bl_secForceNextCalc);
		return CMD_RES_BAD_ARGUMENT;
	}
	else
	{
		g_bl_secMinNextCalc = minCycleTime;
		int alreadyWaitedSec=g_bl_secForceNextCalc-g_bl_secUntilNextCalc;
		if (alreadyWaitedSec > maxCycleTime) {
			g_bl_secUntilNextCalc = 2;
		} else {
			g_bl_secUntilNextCalc = maxCycleTime - alreadyWaitedSec;
		}
		g_bl_secForceNextCalc = maxCycleTime;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_IntervalCPMinMax: one cycle will have min %i and max %i seconds. Remaining to max cycle time %i"
		, g_bl_secMinNextCalc, g_bl_secForceNextCalc, g_bl_secUntilNextCalc);

	return CMD_RES_OK;
}

commandResult_t BL0937_cmdMinPulsesVCP(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_MinPulsesVCP: not enough arguments, current values %lu %lu %lu."
			, g_minPulsesV, g_minPulsesC, g_minPulsesP);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_MinPulsesVCP: min pulses VCP %i %i %i (old=%i %i %i), one P-pulse equals ~0.474..0.486 mWh"
		, Tokenizer_GetArgInteger(0), Tokenizer_GetArgInteger(1), Tokenizer_GetArgInteger(2)
		, g_minPulsesV, g_minPulsesC, g_minPulsesP);
	g_minPulsesV = Tokenizer_GetArgInteger(0);
	g_minPulsesC = Tokenizer_GetArgInteger(1);
	g_minPulsesP = Tokenizer_GetArgInteger(2);

	return CMD_RES_OK;
}

commandResult_t BL0937_cmdScalefactorMultiply(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	float voltage_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, DEFAULT_VOLTAGE_CAL);
	float current_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, DEFAULT_CURRENT_CAL);
	float power_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, DEFAULT_POWER_CAL);

	float multiplyscale_v = (float)BL0937_utlGetDigitFactor((float)DEFAULT_VOLTAGE_CAL, voltage_cal_cur);
	float multiplyscale_c = (float)BL0937_utlGetDigitFactor((float)DEFAULT_CURRENT_CAL, current_cal_cur);
	float multiplyscale_p = (float)BL0937_utlGetDigitFactor((float)DEFAULT_POWER_CAL, power_cal_cur);

//	float multipliereaddrv=0, multipliereaddrc=0, multipliereaddrp=0;
//	float multiplyscale_v = (float)BL0937_utlGetDigitFactor((float)DEFAULT_VOLTAGE_CAL, voltage_cal_cur, &multipliereaddrv);
//	float multiplyscale_c = (float)BL0937_utlGetDigitFactor((float)DEFAULT_CURRENT_CAL, current_cal_cur, &multipliereaddrc);
//	float multiplyscale_p = (float)BL0937_utlGetDigitFactor((float)DEFAULT_POWER_CAL, power_cal_cur, &multipliereaddrp);


	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: not enough arguments to change. Current (default) values V %E (%E) C %E (%E) P %E (%E), recommended factors %f %f %f \n"
			, voltage_cal_cur, DEFAULT_VOLTAGE_CAL, current_cal_cur, DEFAULT_CURRENT_CAL, power_cal_cur, DEFAULT_POWER_CAL
			, multiplyscale_v, multiplyscale_c, multiplyscale_p );
//		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: not enough arguments to change. Current values %E %E %E, recommended factors %f (%f) %f (%f) %f (%f) \n"
//			, voltage_cal_cur, current_cal_cur, power_cal_cur, 1.0f/multiplyscale_v, multipliereaddrv
//			, 1.0f/multiplyscale_c, multipliereaddrc, 1.0f/multiplyscale_p, multipliereaddrp);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;

		return CMD_RES_OK;
	}

	float factor_v = (float)Tokenizer_GetArgFloat(0);
	float factor_c = (float)Tokenizer_GetArgFloat(1);
	float factor_p = (float)Tokenizer_GetArgFloat(2);

	ADDLOG_DEBUG(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: V %E * %f C  %E * %f P %E * %f \n"
		, voltage_cal_cur, factor_v, current_cal_cur, factor_c, power_cal_cur, factor_p);
	voltage_cal_cur *= factor_v;
	current_cal_cur *= factor_c;
	power_cal_cur *= factor_p;
	ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: write new scale factors to CFG %E %E %E\n"
		, voltage_cal_cur, current_cal_cur, power_cal_cur);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, voltage_cal_cur);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, current_cal_cur);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, power_cal_cur);	

	//new scale factors are only updated on next reboot, trigger it here for uninterrupted usage
	PwrCal_Init(PWR_CAL_MULTIPLY, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
	DEFAULT_POWER_CAL);

	return CMD_RES_OK;
}

commandResult_t BL0937_cmdForceOnPwrROC(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ForceOnPwrROC: not enough arguments, current roc recognition %f W/s.", (float)g_p_forceonroc);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_p_pulsesprevsec = g_p_pulses; //update to prevent misleading error message
	g_p_forceonroc=(float)Tokenizer_GetArgInteger(0);
	ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ForceOnPwrROC: %f W/s", (float)g_p_forceonroc);
	return CMD_RES_OK;
}


uint32_t BL0937_utlDiffCalcU32(uint32_t lowval, uint32_t highval)
{

//c++ automatically handles overflow, but we want to be explicit here
	uint32_t diff;
	if (highval >= lowval) {
		diff = highval-lowval;
	} else {
		diff = (0xFFFFFFFFUL - lowval) + highval + 1;
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"diff low %lu high %lu = %lu\n", lowval, highval, diff, highval-lowval);
	}

//	return diff;
	return highval-lowval;
}

//static float BL0937_utlGetDigitFactor(float val1, float val2, float * factor10)
/*
static float BL0937_utlGetDigitFactor(float val1, float val2)
{
	//returns factor of 10 to multiply val2 to get close to val1 (to compare/move decimal point)
	float factor = 1.0f;
//	* factor10 = 1.0f;
	
	float div=1.0f;
	if (val1 > 0.0f && val2 > 0.0f) {
//	float div = ( abs((float)val1) / abs((float)val2));
		if (val1 > val2) {
			div = val1 / val2;
			while (div >= 10.0f) {
				div /= 10.0f;
				factor *= 10.0f;
//				* factor10 *= 10;
			}
		} else {
			div = val2 / val1;
			while (div >= 1.0f) {
				div /= 10.0f;
				factor /= 10.0f;
//				* factor10 /= 10;
			}
		}

	} 

	return factor;
}
*/
static float BL0937_utlGetDigitFactor(float val1, float val2)
{
	//returns factor of 10 to multiply val2 to get close to val1 (to compare/move decimal point)
	float factor = 1.0f;
//	* factor10 = 1.0f;
	int digits1=0, digits2=0;

	digits1 = (int)log10f(val1);
	if (val1 < 1.0f) {
//		digits1 -= 1;
	}
	digits2 = (int)log10f(val2);
	if (val2 < 1.0f) {
//		digits2 -= 1;
	}
	factor = powf(10.0f, (float)(digits1 - digits2));
	return factor;
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
	CMD_RegisterCommand("PowerMax", BL0937_cmdPowerMax, NULL);

	//cmddetail:{"name":"BL0937_IntervalCPMinMax","args":"[MaxIntervalSeconds]",
	//cmddetail:"descr":"Sets the min and max interval for power and current readings (calculation). 
	//	Max setting applies to low power loads, because frequency of BL0937 below 1Hz", 
	//	min increases resolution (at 230V: 1sec max ~1.7W, 30sec ~0.115W)
	//cmddetail:"fn":"BL0937_IntervalCPMinMax","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL0937_IntervalCPMinMax", BL0937_cmdIntervalCPMinMax, NULL);

	//cmddetail:{"name":"BL0937_MinPulsesVCP","args":"[MinPulsesVCP]",
	//cmddetail:"descr":"Sets the minimum pulses for voltage, current and power calculations 
	//	(at low V C P values). Limited by BL0937_IntervalCPMinMax",
	//cmddetail:"fn":"BL0937_MinPulsesVCP","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL0937_MinPulsesVCP", BL0937_cmdMinPulsesVCP, NULL);

	//cmddetail:{"name":"BL0937_ScalefactorMultiply","args":"[VoltageFactor][CurrentFactor][PowerFactor]",
	//cmddetail:"descr":"Multiplies the scale factors for voltage, current and power measurements (for migration)",
	//cmddetail:"fn":"BL0937_ScalefactorMultiply","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL0937_ScalefactorMultiply", BL0937_cmdScalefactorMultiply, NULL);

	//cmddetail:{"name":"BL0937_ForceOnPwrROC","args":"[rate of change in W/sec]",
	//cmddetail:"descr":"Define rate of change of power to force update even if min time or pulses not reached",
	//cmddetail:"fn":"ForceOnPwrROC","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL0937_ForceOnPwrROC", BL0937_cmdForceOnPwrROC, NULL);

//MinPulsesVCP defines minimum level of changes to be measured (similar to hysteresis)
//One pulse equals 0.474207 mWh
//MinIntervalCPMax defines kind of averaging for low power levels

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
		if ( g_v_pulses >= g_minPulsesV ) { //|| g_bl_secUntilNextCalc >= g_bl_secForceNextCalc -1  )  { extended interval reserved for current
#if PULSESTAMPDEBUG>0
			g_pulseStampEnd_v = pulseStampNow;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %lu gsel=%i vp %lu tsv %lu %lu \n"
				, pulseStampNow, g_sel, g_v_pulses, g_pulseStampStart_v, g_pulseStampEnd_v);
#endif
			g_ticksElapsed_v = BL0937_utlDiffCalcU32(g_pulseStampStart_v, pulseStampNow);
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
		if ( (g_c_pulses >= g_minPulsesC && (g_bl_secUntilNextCalc <= g_bl_secForceNextCalc - g_bl_secMinNextCalc)) 
				|| g_bl_secUntilNextCalc <= 1 )  { //reading high enough or max sample time
#if PULSESTAMPDEBUG>0
			g_pulseStampEnd_c = pulseStampNow;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %lu gsel=%i vc %lu tsc %lu %lu \n"
				, pulseStampNow, g_sel, g_c_pulses, g_pulseStampStart_c, g_pulseStampEnd_c);
#endif
			g_ticksElapsed_c = BL0937_utlDiffCalcU32(g_pulseStampStart_c, pulseStampNow);
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
 	addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %lu gsel=%i g_sel_change %i \n"
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
	res_p = BL0937_utlDiffCalcU32(g_p_pulsesprev, g_p_pulses);

	voltage_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, DEFAULT_VOLTAGE_CAL);
	current_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, DEFAULT_CURRENT_CAL);
	power_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, DEFAULT_POWER_CAL);

#define SCALECOMPATIBILITYFIX 1
#if SCALECOMPATIBILITYFIX>0
	// adjust scale factors to match old calculation method (multiplication factor in calibration value)
/*
	if (voltage_cal_cur < 0.01f ) {
		g_freqmultiplierV=10;
	} else if (voltage_cal_cur < 0.01f ) {
		g_freqmultiplierV=1;
	}
	if (power_cal_cur < 0.01f ) {
		g_freqmultiplierP=100;
	} else {
		g_freqmultiplierP=1;
	}
*/
	g_freqmultiplierV = DEFAULT_VOLTAGE_FREQMULTIPLY;
	g_freqmultiplierV *= BL0937_utlGetDigitFactor((float)DEFAULT_VOLTAGE_CAL, voltage_cal_cur);
	g_freqmultiplierP = DEFAULT_POWER_FREQMULTIPLY;
	g_freqmultiplierP *= BL0937_utlGetDigitFactor((float)DEFAULT_POWER_CAL, power_cal_cur);

#endif

//--> force on pwr roc
//	uint32_t pulsediff_p_prevsec = BL0937_utlDiffCalcU32(g_p_pulsesprevsec, g_p_pulses);
//	uint32_t freq_p_lastsec = BL0937_utlDiffCalcU32(g_p_pulsesprevsec, g_p_pulses) * (1000 / portTICK_PERIOD_MS);
//	freq_p_lastsec /= (1000 / portTICK_PERIOD_MS);
//accuracy of function call all every second should be enough, otherwise other port tick store and calc required
	float p_roc = 0.0f;
	float p_thissec = -9999.99f;
	if (g_p_forceonroc > 0) {
		uint freq_p_thissec = (uint)BL0937_utlDiffCalcU32(g_p_pulsesprevsec, g_p_pulses);
		if (freq_p_thissec > 10000) {
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"p roc detection invalid freq %i p_pulses prevsec %i now %i]\n"
				,freq_p_thissec, g_p_prevsec, g_p_pulses);
		} else {
			p_thissec = ((float)freq_p_thissec * ((float)g_freqmultiplierP * power_cal_cur));
			p_roc = p_thissec - g_p_prevsec;
			g_p_prevsec = p_thissec;		
		}
		g_p_pulsesprevsec=g_p_pulses;
		if( fabs(p_roc) >= g_p_forceonroc) {
			/*force for two cycles otherwise in case of
				change to low power last value kept a while, 
				positive change first value is an average 
					-->optionally change to report higher value immediately? */		
			g_forceonroc_gtlim =  2; 
		} else {
			g_forceonroc_gtlim--;
		}
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER,"power prev %.2f / now %.2f (%d Hz * %f * %.3E) -> roc=%f [limit=%.2f, force=%i]\n"
			, g_p_prevsec, p_thissec, freq_p_thissec, (float)g_freqmultiplierP, power_cal_cur, p_roc, g_p_forceonroc, g_forceonroc_gtlim);
	} else {
		g_forceonroc_gtlim=0;
	}
//<-- force on pwr roc

	if ( (res_p >= g_minPulsesP  && (g_bl_secUntilNextCalc <= g_bl_secForceNextCalc - g_bl_secMinNextCalc))
			|| g_bl_secUntilNextCalc <= 0 || g_forceonroc_gtlim >0 ) {
				portTickType g_pulseStampTest= pulseStampNow*10000;
				uint32_t difftestov=0;
				uint32_t difftest=0;
				if (g_pulseStampTest < g_pulseStampTestPrev)
				{
					difftestov = (0xFFFFFFFFUL - g_pulseStampTestPrev) + g_pulseStampTest + 1;
					difftest=g_pulseStampTest-g_pulseStampTestPrev;
					addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"tick overflow low %lu high %lu = %lu %lu\n"
						, g_pulseStampTest, g_pulseStampTestPrev, difftestov, difftest);
				}
				g_pulseStampTestPrev= g_pulseStampTest;
				
		ticksElapsed_p = BL0937_utlDiffCalcU32(g_pulseStampStart_p, pulseStampNow);
#if PULSESTAMPDEBUG>0
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER,"Volt pulses %lu / (tsend %lu) vticks %lu, current %lu /  (tsend %lu) cticks %lu, power %lu / pticks %lu (prev %lu) tsnow %lu gselchg %lu\n"
			, res_v, g_pulseStampEnd_v, g_ticksElapsed_v
			, res_c, g_pulseStampEnd_c, g_ticksElapsed_c
			, res_p, ticksElapsed_p, g_pulseStampStart_p, pulseStampNow, pulseStamp_g_sel_change);
#else
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER,"Volt pulses %lu / (tsstrt %lu) vticks %lu, current %lu /  (tsstrt %lu) cticks %lu, power %lu / pticks %lu (prev %lu) tsnow %lu \n"
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
		//frequency P:  @80V: 0.08 .. 1600W = 0.046862 ..  937.237 Hz, 0.1W=0.058577 [0.057135] Hz
		//frequency P: @250V: 0.25 .. 5000W = 0.146443 .. 2928.866 Hz, 0.1W=0.058577 [0.057135] Hz


		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "Scalefactor default/used [frequency multiplier]: v %E [%f] c %E [1] p %E [%f], usedscalefactor v %E c %E p %E \n"
			, DEFAULT_VOLTAGE_CAL/voltage_cal_cur, (float)g_freqmultiplierV, DEFAULT_CURRENT_CAL/current_cal_cur
			, DEFAULT_POWER_CAL/power_cal_cur, (float)g_freqmultiplierP, voltage_cal_cur, current_cal_cur, power_cal_cur);


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
			freq_v = 99999;
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
			freq_c = 99999;
		}
		if (ticksElapsed_p > 0) {
			freq_p = (float)res_p * (1000.0f / (float)portTICK_PERIOD_MS);	
			freq_p /= (float)ticksElapsed_p;
			// accurate volt measurements is easier --> apply to power automatically
			// inaccuracy because of voltage divider deviation (fe.g. from datasheet) while V and P freq calc use same inputV to chip V(v)
			if (voltage_cal_cur != DEFAULT_VOLTAGE_CAL && power_cal_cur == DEFAULT_POWER_CAL) {
				float v_cal2p=DEFAULT_VOLTAGE_CAL/voltage_cal_cur;
				addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"P autoscale with volt ratio, consider change p_cal by cmd BL0937_ScalefactorMultiply 1 1 %.6f\n"
					, v_cal2p);
				freq_p /= v_cal2p;
			}
/*			final_p = freq_p * 1.218f;
			final_p *= 1.218f;
			final_p /= 1721506.0f;
			final_p *= ((330.0f*6.0f + 1.0f) / 1.0f);	
			final_p /= 1000.0f; 
*/	
		} else {
			final_p = 9999.99f;	
			freq_p = 99999;
		}	
//		PwrCal_Scale((int)(freq_v*100), (float)(freq_c*1000), (int)(freq_p*1000), &final_v, &final_c, &final_p);
		freq_v *= (float)g_freqmultiplierV;
		freq_p *= (float)g_freqmultiplierP;
		PwrCal_Scale((int)freq_v, (float)freq_c, (int)freq_p, &final_v, &final_c, &final_p);

		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"Scaled v %.2f (vf %.3f / %.3f) c %.5f (cf %.3f / 1) p %.5f  (pf %.3f / %.3f (rocforce=%i, p last sec %.2f)) \n"
			,final_v, freq_v, (float)g_freqmultiplierV, final_c, freq_c, final_p, freq_p, (float)g_freqmultiplierP, g_forceonroc_gtlim, p_thissec);
		
		if (g_forceonroc_gtlim > 1 && final_p < ( p_thissec - g_p_forceonroc/2 ) ) { 
			final_p = p_thissec;
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
		g_bl_secUntilNextCalc = g_bl_secForceNextCalc;
	}
	if (g_bl_secUntilNextCalc > 0) {
		g_bl_secUntilNextCalc--;
	}
/*
	cyclecnt_prev = cyclecnt;
	cyclecnt++;
	uint8_t diffc;
	if (cyclecnt < cyclecnt_prev) {
		//overflow
		diffc = (0xFF - cyclecnt_prev) + cyclecnt + 1;
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"pwrcalc counter overflow cur %lu prev %lu diff %lu prev-cur %lu\n"
			, cyclecnt, cyclecnt_prev, diffc, cyclecnt - cyclecnt_prev);
	} else {
		diffc = cyclecnt - cyclecnt_prev;
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"pwrcalc counter cur %lu prev %lu diff %lu\n"
			, cyclecnt, cyclecnt_prev, diffc);
	}
*/}
// close ENABLE_DRIVER_BL0937
#endif

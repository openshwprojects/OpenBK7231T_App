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

#define DEFAULT_VOLTAGE_CAL 1.581335325E-03f 	//factor 100 for precision Vref=1.218, R1=6*330kO, R2=1kO, K=15397
//#define DEFAULT_CURRENT_CAL 1.287009447E-05f	//factor 1000 for precision Vref=1.218, Rs=1mO, K=94638
#define DEFAULT_CURRENT_CAL 1.287009447E-02f	//factor 1000 maybe not necessary because scale uses float for C for precision Vref=1.218, Rs=1mO, K=94638
#define DEFAULT_POWER_CAL 	1.707145397E-03f 	//factor 1000 for precision Vref=1.218, R1=6*330kO, R2=1kO, Rs=1mO, K=1721506

#define MINPULSES_VOLTAGE 200
#define MINPULSES_CURRENT 3
#define MINPULSES_POWER 5

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
static portTickType g_ticksElapsed_v=0, g_ticksElapsed_c=0;
volatile uint32_t g_p_pulses = 0, g_p_pulsesprev = 0;
static portTickType g_pulseStampStart_v=0, g_pulseStampStart_c=0, g_pulseStampStart_p=0;
#define PULSESTAMPDEBUG 0
#if PULSESTAMPDEBUG>0
volatile portTickType g_pulseStampEnd_v=0, g_pulseStampEnd_c=0;
#endif
static uint32_t g_bl_secUntilNextCalc= 1, g_bl_secForceNextCalc = 30, g_bl_secMinNextCalc = 3; //at 230V minintervall/Pdiff 15s=0.115W, 7.5s=0.23W, 3s=0.575W, 1.5s=1.15W, 1s=1.73W
static int g_minPulsesV = MINPULSES_VOLTAGE, g_minPulsesC = MINPULSES_CURRENT, g_minPulsesP = MINPULSES_POWER;
static int g_factorScaleV=100, g_factorScaleC=1000, g_factorScaleP=1000;

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
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_IntervalCPMinMax: not enough arguments, current values %i %i."
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

	ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_IntervalCPMinMax: one cycle will have min %i and max %i seconds. Remaining current cycle %i"
		, g_bl_secMinNextCalc, g_bl_secForceNextCalc, g_bl_secUntilNextCalc);

	return CMD_RES_OK;
}

commandResult_t BL0937_cmdMinPulsesVCP(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_MinPulsesVCP: not enough arguments, current values %i %i %i."
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
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: not enough arguments, current values %E %E %E."
			, g_minPulsesV, g_minPulsesC, g_minPulsesP);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: min pulses VCP %E %E %E (old=%E %E %E)"
		, Tokenizer_GetArgInteger(0), Tokenizer_GetArgInteger(1), Tokenizer_GetArgInteger(2)
		, voltage_cal_cur, current_cal_cur, power_cal_cur);
	voltage_cal_cur *= Tokenizer_GetArgInteger(0);
	current_cal_cur *= Tokenizer_GetArgInteger(1);
	power_cal_cur *= Tokenizer_GetArgInteger(2);	
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, voltage_cal_cur);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, current_cal_cur);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, power_cal_cur);	
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
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %i gsel=%i vp %i tsv %i %i \n"
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
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"now %i gsel=%i vc %i tsc %i %i \n"
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
	res_p = BL0937_utlDiffCalcU32(g_p_pulsesprev, g_p_pulses);

	if ( (res_p >= g_minPulsesP  && (g_bl_secUntilNextCalc <= g_bl_secForceNextCalc - g_bl_secMinNextCalc))
			|| g_bl_secUntilNextCalc <= 0 ) {
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
				
		ticksElapsed_p = BL0937_utlDiffCalcU32(g_pulseStampStart_p*10000, pulseStampNow*10000); //test overflow
		ticksElapsed_p = BL0937_utlDiffCalcU32(g_pulseStampStart_p, pulseStampNow);
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

#define SCALECOMPATIBILITYFIX 1
#if SCALECOMPATIBILITYFIX>0
		// adjust scale factors to match old calculation method (multiplication factor in calibration value)
		
		if (voltage_cal_cur < 0.01f ) {
			g_factorScaleV=100;
		} else if (voltage_cal_cur < 0.01f ) {
			g_factorScaleV=1;
		}
		if (current_cal_cur < 0.0001f ) {
			g_factorScaleC=1000;
		} else {
			g_factorScaleC=1;
		}
		if (power_cal_cur < 0.01f ) {
			g_factorScaleP=1000;
		} else {
			g_factorScaleP=1;
		}
#endif

		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "Scalefactor default/used [raw2float multiplier]: v %f [%d] c %f [%d] p %f [%d], usedv %E c %E p %E v %E c %E p %E \n"
			, DEFAULT_VOLTAGE_CAL/voltage_cal_cur,g_factorScaleV, DEFAULT_CURRENT_CAL/current_cal_cur, g_factorScaleC
			, DEFAULT_POWER_CAL/power_cal_cur, g_factorScaleP, voltage_cal_cur, current_cal_cur, power_cal_cur);


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
				addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"Scaled v %.1f c %.4f p %.2f (/v_cal2p %.6f=%.2f), pcal %f\n"
					, final_v, final_c, final_p, v_cal2p, final_p / v_cal2p, power_cal_cur, portTICK_PERIOD_MS);
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
		freq_v *= (float)g_factorScaleV;
		freq_c *= (float)g_factorScaleC;
		freq_p *= (float)g_factorScaleP;
		PwrCal_Scale((int)freq_v, (float)freq_c, (int)freq_p, &final_v, &final_c, &final_p);

		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Scaled v %.2f (vf %.3f / %i) c %.6f (cf %.3f / %i) p %.6f  (pf %.3f / %i) \n"
			,final_v, freq_v, g_factorScaleV, final_c, freq_c, g_factorScaleC, final_p, freq_p, g_factorScaleP);
		
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
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"pwrcalc counter overflow cur %i prev %i diff %i prev-cur %i\n"
			, cyclecnt, cyclecnt_prev, diffc, cyclecnt - cyclecnt_prev);
	} else {
		diffc = cyclecnt - cyclecnt_prev;
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER,"pwrcalc counter cur %i prev %i diff %i\n"
			, cyclecnt, cyclecnt_prev, diffc);
	}
*/}
// close ENABLE_DRIVER_BL0937
#endif

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

#include "../mqtt/new_mqtt.h"

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

volatile unsigned long g_v_pulses = 0, g_c_pulses = 0;
static portTickType g_ticksElapsed_v=1, g_ticksElapsed_c=1; //initial value > 0 to enable calculation at first run
volatile unsigned long g_p_pulses = 0, g_p_pulsesprev = 0;
static portTickType g_pulseStampStart_v=0, g_pulseStampStart_c=0, g_pulseStampStart_p=0;
#define PULSESTAMPDEBUG 0
#if PULSESTAMPDEBUG>0
volatile portTickType g_pulseStampEnd_v=0, g_pulseStampEnd_c=0;
#endif

#define NEW_UPDATE_CYCLES 0
#if NEW_UPDATE_CYCLES>0
	static unsigned long g_minPulsesV = 200, g_minPulsesC = 3, g_minPulsesP = 5; 
	static unsigned long g_bl_secUntilNextCalc= 1, g_bl_secForceNextCalc = 30, g_bl_secMinNextCalc = 3; //at 230V minintervall/Pdiff 15s=0.115W, 7.5s=0.23W, 
#else
	static unsigned long g_bl_secUntilNextCalc= 0, g_bl_secForceNextCalc = 0, g_bl_secMinNextCalc = 0; //at 230V minintervall/Pdiff 15s=0.115W, 7.5s=0.23W, 3s=0.575W, 1.5s=1.15W, 1s=1.73W
	static unsigned long g_minPulsesV = 0, g_minPulsesC = 0, g_minPulsesP = 0;	// keep behaviour for compatibility
#endif

static portTickType g_pulseStampTestPrev=0;

static float g_freqmultiplierV=DEFAULT_VOLTAGE_FREQMULTIPLY, g_freqmultiplierP=DEFAULT_POWER_FREQMULTIPLY; //not needed for C because float used
static float g_p_forceonroc=0.0f; //pwr change within last second
static float g_p_forceonpwr=0.0f; //pwr change within current cycle
static int g_forceonroc_gtlim=-1;
static int g_forceonpwr_gtlim=-1;
static float g_p_prevsec=0;
static unsigned long g_p_pulsesprevsec = 0;

static int g_enable_mqtt_on_cmd = 1;
static int g_enable_sendtimestamps = 0;

static int g_v_avg_res=0;
static int g_v_avg_ticks=0;
static int g_v_avg_count=0;
static int g_c_avg_res=0;
static int g_c_avg_ticks=0;
static int g_c_avg_count=0;

#define TIME_CHECK_COMPARE_NTP 1
#if TIME_CHECK_COMPARE_NTP > 0
#include "drv_ntp.h"
#include <time.h>

//time_t g_ntpTime;
struct tm *ltm;
int_fast8_t g_ntp_hourlast=-1;
int g_diff_ntp_secel=0;
int g_sfreqcalcdone=0;
int_fast8_t g_sfreqcalc_ntphour_last=-1;
time_t g_sfreqcalc_ntpTime_last;
unsigned long g_sfreqcalc_secelap_last=0;
float g_scale_samplefreq=1;
#endif

#define CMD_SEND_VAL_MQTT 1

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
	const char cmdName[] = "PowerMax";
	float maxPower;
	int argok=-2;

	if(args == 0 || *args == 0)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "This command needs one argument");
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
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
		argok=1;
	}
#if CMD_SEND_VAL_MQTT > 0
	if (g_enable_mqtt_on_cmd>0)	{
		char curvalstr[8]; 
		if ( (float)maxPower < 10000.0f ) { //ensure there is no strlen overflow
			sprintf(curvalstr, "%.2f", (float)maxPower);
		} else {
			strcpy(curvalstr, "invalid");
		}
		MQTT_PublishMain_StringString(cmdName, curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);
	}
#endif
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}

commandResult_t BL0937_cmdIntervalCPMinMax(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	const char cmdName[] = "BL0937_IntervalCPMinMax";
	int argok=0;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_GetArgsCount()<2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: not enough arguments, current values %lu %lu. \n", g_secondsElapsed, cmdName
			, g_bl_secMinNextCalc, g_bl_secForceNextCalc);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
	} else {
		int minCycleTime = Tokenizer_GetArgInteger(0);
		int maxCycleTime = Tokenizer_GetArgInteger(1);
		if( minCycleTime < 0 || maxCycleTime < minCycleTime || maxCycleTime > 900 || minCycleTime > maxCycleTime) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: minimum 0 second, maximum 900 seconds allowed, min %i between max %i (keep current %i %i). \n", g_secondsElapsed, cmdName
				, minCycleTime, maxCycleTime, g_bl_secMinNextCalc, g_bl_secForceNextCalc);
	//		return CMD_RES_BAD_ARGUMENT;
			argok=-2;
		} else 	{
			g_bl_secMinNextCalc = minCycleTime;
			int alreadyWaitedSec=g_bl_secForceNextCalc-g_bl_secUntilNextCalc;
			if (alreadyWaitedSec > maxCycleTime || g_secondsElapsed < 30) { //if used in startup command (first 30 sec)
				g_bl_secUntilNextCalc = 2;
			} else {
				g_bl_secUntilNextCalc = maxCycleTime - alreadyWaitedSec;
			}
			g_bl_secForceNextCalc = maxCycleTime;
			argok=1;
		}
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: one cycle will have min %i and max %i seconds. Remaining to max cycle time %i \n", g_secondsElapsed, cmdName
			, g_bl_secMinNextCalc, g_bl_secForceNextCalc, g_bl_secUntilNextCalc);
	}
#if CMD_SEND_VAL_MQTT > 0
	if (g_enable_mqtt_on_cmd>0)	{
		char curvalstr[24]; //10+1 char per uint32
		sprintf(curvalstr, "%lu %lu", g_bl_secMinNextCalc, g_bl_secForceNextCalc);
		MQTT_PublishMain_StringString(cmdName, curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);
	}
#endif
//	return CMD_RES_OK;
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}

commandResult_t BL0937_cmdMinPulsesVCP(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	const char cmdName[] = "BL0937_MinPulsesVCP";
	int argok=0;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_GetArgsCount()<3) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: not enough arguments, current values %lu %lu %lu.", g_secondsElapsed, cmdName
			, g_minPulsesV, g_minPulsesC, g_minPulsesP);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: min pulses VCP %i %i %i (old=%i %i %i), one P-pulse equals ~0.474..0.486 mWh \n", g_secondsElapsed, cmdName
			, Tokenizer_GetArgInteger(0), Tokenizer_GetArgInteger(1), Tokenizer_GetArgInteger(2)
			, g_minPulsesV, g_minPulsesC, g_minPulsesP);
		g_minPulsesV = Tokenizer_GetArgInteger(0);
		g_minPulsesC = Tokenizer_GetArgInteger(1);
		g_minPulsesP = Tokenizer_GetArgInteger(2);
		argok=1;
	}
#if CMD_SEND_VAL_MQTT > 0
	if (g_enable_mqtt_on_cmd>0)	{
		char curvalstr[36]; //10+1 char per uint32
		sprintf(curvalstr, "%lu %lu %lu", g_minPulsesV, g_minPulsesC, g_minPulsesP);
		MQTT_PublishMain_StringString(cmdName, curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);
	}
#endif
//	return CMD_RES_OK;
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}

commandResult_t BL0937_cmdScalefactorMultiply(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	const char cmdName[] = "BL0937_ScalefactorMultiply";
	int argok=0;
	float voltage_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, DEFAULT_VOLTAGE_CAL);
	float current_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, DEFAULT_CURRENT_CAL);
	float power_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, DEFAULT_POWER_CAL);

	float multiplyscale_v = (float)BL0937_utlGetDigitFactor(voltage_cal_cur, (float)DEFAULT_VOLTAGE_CAL);
	float multiplyscale_c = (float)BL0937_utlGetDigitFactor(current_cal_cur, (float)DEFAULT_CURRENT_CAL);
	float multiplyscale_p = (float)BL0937_utlGetDigitFactor(power_cal_cur, (float)DEFAULT_POWER_CAL);

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
//	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3))
	if(Tokenizer_GetArgsCount()<3) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: not enough arguments to change. Current (default) values V %E (%E) C %E (%E) P %E (%E), recommended factors %f %f %f \n", g_secondsElapsed, cmdName
			, voltage_cal_cur, DEFAULT_VOLTAGE_CAL, current_cal_cur, DEFAULT_CURRENT_CAL, power_cal_cur, DEFAULT_POWER_CAL
			, multiplyscale_v, multiplyscale_c, multiplyscale_p );
//		ADDLOG_INFO(LOG_FEATURE_CMD, "BL0937_ScalefactorMultiply: not enough arguments to change. Current values %E %E %E, recommended factors %f (%f) %f (%f) %f (%f) \n"
//			, voltage_cal_cur, current_cal_cur, power_cal_cur, 1.0f/multiplyscale_v, multipliereaddrv
//			, 1.0f/multiplyscale_c, multipliereaddrc, 1.0f/multiplyscale_p, multipliereaddrp);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
	} else {
		float factor_v = (float)Tokenizer_GetArgFloat(0);
		float factor_c = (float)Tokenizer_GetArgFloat(1);
		float factor_p = (float)Tokenizer_GetArgFloat(2);

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "ts %5d %s: V %E * %f C  %E * %f P %E * %f \n", g_secondsElapsed, cmdName
			, voltage_cal_cur, factor_v, current_cal_cur, factor_c, power_cal_cur, factor_p);
		voltage_cal_cur *= factor_v;
		current_cal_cur *= factor_c;
		power_cal_cur *= factor_p;
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: write new scale factors to CFG %E %E %E\n", g_secondsElapsed, cmdName
			, voltage_cal_cur, current_cal_cur, power_cal_cur);
		CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, voltage_cal_cur);
		CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, current_cal_cur);
		CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, power_cal_cur);	

		//new scale factors are only updated on next reboot, trigger it here for uninterrupted usage
		PwrCal_Init(PWR_CAL_MULTIPLY, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
		DEFAULT_POWER_CAL);
		argok=1;
	}
#if CMD_SEND_VAL_MQTT > 0
	if (g_enable_mqtt_on_cmd>0)	{
		char curvalstr[64]; 
		sprintf(curvalstr, "%.6E %.6E %.6E", voltage_cal_cur, current_cal_cur, power_cal_cur);
		MQTT_PublishMain_StringString("ScalefactorsVCP", curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);
	}
#endif

//	return CMD_RES_OK;
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}

commandResult_t BL0937_cmdForceOnPwrROC(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	const char cmdName[] = "BL0937_ForceOnPwrROC";
	int argok=0;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_GetArgsCount()<2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: not enough arguments given, current limits roc %f W/s | pwr %f Wcycle.\n", g_secondsElapsed, cmdName
			, (float)g_p_forceonroc, (float)g_p_forceonpwr);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
	} else {
		g_p_forceonroc=(float)Tokenizer_GetArgInteger(0);
		g_p_forceonpwr=(float)Tokenizer_GetArgInteger(1);
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: %f W/s %f Wcycle", g_secondsElapsed, cmdName
			, (float)g_p_forceonroc, (float)g_p_forceonpwr);
		argok=1;
	}
#if CMD_SEND_VAL_MQTT > 0
	if (g_enable_mqtt_on_cmd>0)	{
		char curvalstr[16]; 
		if ( (float)g_p_forceonroc < 10000.0f && (float)g_p_forceonpwr < 10000.0f) { //ensure there is no strlen overflow
			sprintf(curvalstr, "%.2f %.2f", (float)g_p_forceonroc, (float)g_p_forceonpwr);
		} else {
			strcpy(curvalstr, "invalid");
		}
		MQTT_PublishMain_StringString(cmdName, curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);
	}
#endif
//	return CMD_RES_OK;
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}

#if TIME_CHECK_COMPARE_NTP > 0
commandResult_t cmdSendTimestamps(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	const char cmdName[] = "SendTimestamps";
	int argok=0;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_GetArgsCount()<1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: not enough arguments to change, current val = %i\n", g_enable_sendtimestamps);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
	} else {
		g_enable_sendtimestamps=(int)Tokenizer_GetArgInteger(0);
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: %i", g_secondsElapsed, cmdName, g_enable_sendtimestamps);
		argok=1;
	}
	char curvalstr[12]; 
	sprintf(curvalstr, "%i", g_enable_sendtimestamps);
	MQTT_PublishMain_StringString(cmdName, curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);

//	return CMD_RES_OK;
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}
#endif

#if CMD_SEND_VAL_MQTT > 0
commandResult_t cmdEnabeMQTTOnCommand(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	const char cmdName[] = "EnabeMQTTOnCommand";
	int argok=0;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_GetArgsCount()<1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: not enough arguments to change, current val = %d\n", g_enable_mqtt_on_cmd);
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		argok=-1;
	} else {
		g_enable_mqtt_on_cmd=(int)Tokenizer_GetArgInteger(0);
		ADDLOG_INFO(LOG_FEATURE_CMD, "ts %5d %s: %i", g_secondsElapsed, cmdName, g_enable_mqtt_on_cmd);
		argok=1;
	}
	char curvalstr[12]; 
	sprintf(curvalstr, "%i", g_enable_mqtt_on_cmd);
	MQTT_PublishMain_StringString(cmdName, curvalstr, OBK_PUBLISH_FLAG_QOS_ZERO);

//	return CMD_RES_OK;
	return (argok>0)?CMD_RES_OK:((argok>=-1)?CMD_RES_NOT_ENOUGH_ARGUMENTS:CMD_RES_BAD_ARGUMENT);
}
#endif

uint32_t BL0937_utlDiffCalcU32(uint32_t lowval, uint32_t highval)
{
//c++ automatically handles overflow, but we want to be explicit here
	uint32_t diff;
	if (highval >= lowval) {
		diff = highval-lowval;
	} else {
		diff = (0xFFFFFFFFUL - lowval) + highval + 1;
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d diffcalc low %lu high %lu = %lu\n", g_secondsElapsed, lowval, highval, diff, highval-lowval);
	}

//	return diff;
	return highval-lowval;
}

static float BL0937_utlGetDigitFactor(float val1, float val2)
{
	//returns factor of 10 to multiply val2 to get close to val1 (to compare/move decimal point)
	float factor = 1.0f;
//	* factor10 = 1.0f;
	int digits1=0, digits2=0;


	digits1 = (int)(log10f(val1));	
	if ( val1 < 1.0f) {
		digits1--;
	}
	digits2 = (int)(log10f(val2)+0);	
	if (val2 < 1.0f) {
		digits2--;
	}
	factor = powf(10.0f, (float)(digits2 - digits1));
//	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d getfactor val1 %E val2 %E digits %d %d factor %f\n", g_secondsElapsed
//		, val1, val2, digits1, digits2, factor);
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
	//  note: mqtt interval / VCPPublishInterval and threshod setting may delay publish additionally
	//  OR logic with P from MinPulsesVCP (max time or min P pulses)
	//cmddetail:"fn":"BL0937_IntervalCPMinMax","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL0937_IntervalCPMinMax", BL0937_cmdIntervalCPMinMax, NULL);

	//cmddetail:{"name":"BL0937_MinPulsesVCP","args":"[MinPulsesVCP]",
	//cmddetail:"descr":"Sets the minimum pulses for voltage, current and power calculations 
	//	(at low V C P values). Limited by BL0937_IntervalCPMinMax",
	//  V and C used for switch between V/C measure, 
	//  P used for OR logic with BL0937_IntervalCPMinMax to update (max time or min P pulses)
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

#if TIME_CHECK_COMPARE_NTP > 0
	//cmddetail:{"name":"SendTimestamps","args":"[0/1]",
	//cmddetail:"descr":"Enable/disable sending timestamps with measurements",
	//cmddetail:"fn":"cmdSendTimestamps","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SendTimestamps", cmdSendTimestamps, NULL);
#endif

#if CMD_SEND_VAL_MQTT > 0
	//cmddetail:{"name":"EnabeMQTTOnCommand","args":"[0/1]",
	//cmddetail:"descr":"Enable MQTT message with new (current) values after command",
	//cmddetail:"fn":"cmdEnabeMQTTOnCommand","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("EnabeMQTTOnCommand", cmdEnabeMQTTOnCommand, NULL);
#endif

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
	float freq_v_avg, freq_c_avg; 
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
 	pulseStampNow = xTaskGetTickCount();

	voltage_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, DEFAULT_VOLTAGE_CAL);
	current_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, DEFAULT_CURRENT_CAL);
	power_cal_cur = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, DEFAULT_POWER_CAL);

#define SCALECOMPATIBILITYFIX 1
#if SCALECOMPATIBILITYFIX>0
	// adjust scale factors to match old calculation method (multiplication factor in calibration value)
	g_freqmultiplierV = DEFAULT_VOLTAGE_FREQMULTIPLY;
	g_freqmultiplierV *= BL0937_utlGetDigitFactor(voltage_cal_cur, (float)DEFAULT_VOLTAGE_CAL);
	g_freqmultiplierP = DEFAULT_POWER_FREQMULTIPLY;
	g_freqmultiplierP *= BL0937_utlGetDigitFactor(power_cal_cur, (float)DEFAULT_POWER_CAL);

#endif

	//--> force on pwr roc
//	uint32_t pulsediff_p_prevsec = BL0937_utlDiffCalcU32(g_p_pulsesprevsec, g_p_pulses);
//	uint32_t freq_p_lastsec = BL0937_utlDiffCalcU32(g_p_pulsesprevsec, g_p_pulses) * (1000 / portTICK_PERIOD_MS);
//	freq_p_lastsec /= (1000 / portTICK_PERIOD_MS);
//accuracy of function call every second should be enough, otherwise other port tick store and calc required
	float p_roc = 0.0f;
	float p_thissec = -9999.99f;
	if (g_p_forceonroc > 0) {
		unsigned long freq_p_thissec = (unsigned long)BL0937_utlDiffCalcU32(g_p_pulsesprevsec, g_p_pulses);
		if (freq_p_thissec > 10000) {
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ts %5d p roc detection invalid freq %i p_pulses prevsec %i now %i]\n", g_secondsElapsed
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
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d power prev %.2f / now %.2f (%d Hz * %f * %.3E) -> roc=%.1f [limit=%.2f, force=%i sec2next=%d], p_pulsestot %d\n", g_secondsElapsed
			, g_p_prevsec, p_thissec, freq_p_thissec, (float)g_freqmultiplierP, power_cal_cur, p_roc, g_p_forceonroc, g_forceonroc_gtlim, g_bl_secUntilNextCalc, res_p);
	} else {
		g_forceonroc_gtlim=0;
	}
//<-- force on pwr roc

//--> force on pwr within cycle
	float p_cycle = 0.0f;
//	float p_diff = 0.0f;
	float freq_p_cycle = 0.0f;
	if (g_p_forceonpwr > 0) {
		uint32_t ticksElapsed_p_cycle=BL0937_utlDiffCalcU32(g_pulseStampStart_p, pulseStampNow);
		if (ticksElapsed_p_cycle > 0) {
			freq_p_cycle= (float)res_p * (1000.0f / (float)portTICK_PERIOD_MS);	
			freq_p_cycle /= (float)ticksElapsed_p_cycle;		
			p_cycle = ((float)freq_p_cycle * ((float)g_freqmultiplierP * power_cal_cur));
		} 
		if (freq_p_cycle > 10000 || p_cycle > BL0937_PMAX) {
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ts %5d p cycle detection invalid freq %i p_pulses prevsec %i now %i]\n", g_secondsElapsed
				,freq_p_cycle, last_p, g_p_pulses);
		} else {
//			p_diff = (p_cycle > last_p)? (p_cycle - last_p) : (last_p - p_cycle);
//			p_diff = p_cycle - last_p;
			if( fabs(p_cycle - last_p) >= g_p_forceonpwr) {
				g_forceonpwr_gtlim =  1; 
			} else {
				g_forceonpwr_gtlim--;
			}
			addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d power prev %.2f / cycle %.2f [limit=%.2f, force=%i sec2next=%d], p_pulsestot %d\n", g_secondsElapsed
				, last_p, p_cycle, freq_p_cycle, g_p_forceonpwr, g_forceonpwr_gtlim, g_bl_secUntilNextCalc, res_p);
		}
	} else {
		g_forceonroc_gtlim=0;
	}

	res_p = BL0937_utlDiffCalcU32(g_p_pulsesprev, g_p_pulses);
	//<-- force on pwr roc

	//check if P will cause update
	int p_update = 0;
	if ( (res_p >= g_minPulsesP  && (g_bl_secUntilNextCalc <= g_bl_secForceNextCalc - g_bl_secMinNextCalc))
			|| g_bl_secUntilNextCalc <= 0 || g_forceonroc_gtlim > 0 || g_forceonpwr_gtlim > 0) {
		p_update = 1;
	}


// V and I measurement must be done/changed every second
	if ((g_sel && !g_invertSEL) || (!g_sel && g_invertSEL)) {
//	if ( g_sel ) {
		if ( g_v_pulses >= g_minPulsesV ) {
#if PULSESTAMPDEBUG>0
			g_pulseStampEnd_v = pulseStampNow;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d now %lu gsel=%i vp %lu tsv %lu %lu \n", g_secondsElapsed
				, pulseStampNow, g_sel, g_v_pulses, g_pulseStampStart_v, g_pulseStampEnd_v);
#endif
			g_ticksElapsed_v = BL0937_utlDiffCalcU32(g_pulseStampStart_v, pulseStampNow);
			res_v = g_v_pulses;
	#define VOLT_CURR_AVG 1
	#if VOLT_CURR_AVG>0
			g_v_avg_res += res_v;
			g_v_avg_ticks += g_ticksElapsed_v;
			g_v_avg_count++;
	#endif
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
//		if ( (g_c_pulses >= g_minPulsesC && (g_bl_secUntilNextCalc <= g_bl_secForceNextCalc - g_bl_secMinNextCalc)) 
//				|| g_bl_secUntilNextCalc <= 1 )  { //reading high enough or max sample time
		// doesn't switch if min pulses too high
		if ( (g_c_pulses >= g_minPulsesC && (g_bl_secUntilNextCalc <= (g_bl_secForceNextCalc - g_bl_secMinNextCalc) ) )
		 || g_bl_secUntilNextCalc <= 1 || p_update > 0 )  { //reading high enough and min cycle timeor max sample time, reserve one cycle for volt
#if PULSESTAMPDEBUG>0
			g_pulseStampEnd_c = pulseStampNow;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d now %lu gsel=%i vc %lu tsc %lu %lu \n", g_secondsElapsed
				, pulseStampNow, g_sel, g_c_pulses, g_pulseStampStart_c, g_pulseStampEnd_c);
#endif
			g_ticksElapsed_c = BL0937_utlDiffCalcU32(g_pulseStampStart_c, pulseStampNow);
			res_c = g_c_pulses;
	#define VOLT_CURR_AVG 1
	#if VOLT_CURR_AVG>0
			g_c_avg_res += res_c;
			g_c_avg_ticks += g_ticksElapsed_c;
			g_c_avg_count++;
	#endif
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
 	addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d now %lu gsel=%i g_sel_change %i \n", g_secondsElapsed
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

//	if ( (res_p >= g_minPulsesP  && (g_bl_secUntilNextCalc <= g_bl_secForceNextCalc - g_bl_secMinNextCalc))
//			|| g_bl_secUntilNextCalc <= 0 || g_forceonroc_gtlim > 0 || g_forceonpwr_gtlim > 0) {
	if ( p_update > 0 || g_bl_secUntilNextCalc <= 0 ) {
				portTickType g_pulseStampTest= pulseStampNow*10000;
				unsigned long difftestov=0;
				unsigned long difftest=0;
				if (g_pulseStampTest < g_pulseStampTestPrev)
				{
					difftestov = (0xFFFFFFFFUL - g_pulseStampTestPrev) + g_pulseStampTest + 1;
					difftest=g_pulseStampTest-g_pulseStampTestPrev;
					addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ts %5d tick overflow low %lu high %lu = %lu %lu\n", g_secondsElapsed
						, g_pulseStampTest, g_pulseStampTestPrev, difftestov, difftest);
				}
				g_pulseStampTestPrev= g_pulseStampTest;
				
		ticksElapsed_p = BL0937_utlDiffCalcU32(g_pulseStampStart_p, pulseStampNow);
#if PULSESTAMPDEBUG>0
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d Volt pulses %lu / (tsend %lu) vticks %lu, current %lu /  (tsend %lu) cticks %lu, power %lu / pticks %lu (prev %lu) tsnow %lu gselchg %lu\n", g_secondsElapsed
			, res_v, g_pulseStampEnd_v, g_ticksElapsed_v
			, res_c, g_pulseStampEnd_c, g_ticksElapsed_c
			, res_p, ticksElapsed_p, g_pulseStampStart_p, pulseStampNow, pulseStamp_g_sel_change);
#else
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d Volt pulses %lu / (tsstrt %lu) vticks %lu, current %lu /  (tsstrt %lu) cticks %lu, power %lu / pticks %lu (prev %lu) tsnow %lu \n", g_secondsElapsed
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


		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d Scalefactor default/used [frequency multiplier]: v %E [%f] c %E [1] p %E [%f], usedscalefactor v %E c %E p %E \n", g_secondsElapsed
			, DEFAULT_VOLTAGE_CAL/voltage_cal_cur, (float)g_freqmultiplierV, DEFAULT_CURRENT_CAL/current_cal_cur
			, DEFAULT_POWER_CAL/power_cal_cur, (float)g_freqmultiplierP, voltage_cal_cur, current_cal_cur, power_cal_cur);
	#if VOLT_CURR_AVG>0
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d Volt avg pulses %lu / vticks %lu, current avg pulses %lu /  cticks %lu\n", g_secondsElapsed
			, g_v_avg_res, g_v_avg_ticks
			, g_c_avg_res, g_c_avg_ticks);
	#endif

		//Vref=1.218, R1=6*330kO, R2=1kO, K=15397
		//Vref=1.218, Rs=1mO, K=94638
		//Vref=1.218, R1=6*330kO, R2=1kO, Rs=1mO, K=1721506
	
		if (g_ticksElapsed_v > 0) {
			freq_v = (float)res_v * (1000.0f / (float)portTICK_PERIOD_MS);
			freq_v /= (float)g_ticksElapsed_v;
	#if VOLT_CURR_AVG>0
//			if (g_v_avg_count > 0) {
				freq_v_avg = (float)g_v_avg_res * (1000.0f / (float)portTICK_PERIOD_MS);
				freq_v_avg /= (float)g_v_avg_ticks;
//				g_v_avg_count = 0;
//			} else {
//				freq_v_avg = freq_v;
//			}
	#endif
/*			final_v = freq_v * 1.218f;
			final_v /= 15397.0f;
			final_v *= (330.0f*6.0f + 1.0f) / 1.0f; //voltage divider
*/
		} else {
			final_v = 11.1;	
			freq_v = 99999;
			freq_v_avg = freq_v;
		}
	
		if (g_ticksElapsed_c > 0) {
			freq_c = (float)res_c * (1000.0f / (float)portTICK_PERIOD_MS);
			freq_c /= (float)g_ticksElapsed_c;
	#if VOLT_CURR_AVG>0
//			if (g_c_avg_count > 0) {
				freq_c_avg = (float)g_c_avg_res * (1000.0f / (float)portTICK_PERIOD_MS);
				freq_c_avg /= (float)g_c_avg_ticks;
//				g_c_avg_count = 0;
//			} else {
//				freq_c_avg = freq_c;
//			}
	#endif
/*			final_c = freq_c * 1.218f;
			final_c /= 94638.0f;
			final_c *= 1000.0f; // Rs=	
*/
		} else {
			final_c = 22.222f;	
			freq_c = 99999;
			freq_c_avg = freq_c;
		}
		if (ticksElapsed_p > 0) {
			freq_p = (float)res_p * (1000.0f / (float)portTICK_PERIOD_MS);	
			freq_p /= (float)ticksElapsed_p;
			// accurate volt measurements is easier --> apply to power automatically
			// inaccuracy because of voltage divider deviation (fe.g. from datasheet) while V and P freq calc use same inputV to chip V(v)
			if (voltage_cal_cur != DEFAULT_VOLTAGE_CAL && power_cal_cur == DEFAULT_POWER_CAL) {
				float v_cal2p=DEFAULT_VOLTAGE_CAL/voltage_cal_cur;
				addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d P autoscale with volt ratio, consider change p_cal by cmd BL0937_ScalefactorMultiply 1 1 %.6f\n", g_secondsElapsed
					, v_cal2p);
				freq_p *= (float)g_freqmultiplierV;
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
		freq_p *= (float)g_freqmultiplierP;
		freq_v *= (float)g_freqmultiplierV;
#if VOLT_CURR_AVG>0
//		freq_v_avg = (g_v_avg_count > 0) ? (freq_v_avg * (float)g_freqmultiplierV) : freq_v;
//		freq_c_avg = (g_c_avg_count > 0) ? freq_c_avg : freq_c;
		PwrCal_Scale((int)((g_v_avg_count > 0) ? (freq_v_avg * (float)g_freqmultiplierV) : freq_v)
			, (float)((g_c_avg_count > 0) ? freq_c_avg : freq_c), (int)freq_p, &final_v, &final_c, &final_p);
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d Scaled v %.2f (vf %.3f [%.3f x %i] / %.3f) c %.5f (cf %.3f [%.3f x %i] / 1) p %.5f  (pf %.3f / %.3f \n", g_secondsElapsed
			,final_v, freq_v, freq_v_avg, g_v_avg_count, (float)g_freqmultiplierV, final_c, freq_c, freq_c_avg, g_c_avg_count, final_p, freq_p, (float)g_freqmultiplierP);
		if (g_v_avg_count > 0) { //just for testing, reset only if at least one sample error with ROC
			g_v_avg_res = 0;
			g_v_avg_ticks = 0;
			g_v_avg_count = 0;
		}
		if (g_c_avg_count > 0) { //just for testing, reset only if at least one sample
			g_c_avg_res = 0;
			g_c_avg_ticks = 0;
			g_c_avg_count = 0;
		}
#else
		PwrCal_Scale((int)freq_v, (float)freq_c, (int)freq_p, &final_v, &final_c, &final_p);
		addLogAdv(LOG_DEBUG, LOG_FEATURE_ENERGYMETER, "ts %5d Scaled v %.2f (vf %.3f / %.3f) c %.5f (cf %.3f / 1) p %.5f  (pf %.3f / %.3f \n", g_secondsElapsed
			,final_v, freq_v, (float)g_freqmultiplierV, final_c, freq_c, final_p, freq_p, (float)g_freqmultiplierP);
#endif

		
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

#if TIME_CHECK_COMPARE_NTP > 0
	struct tm* ltm = gmtime(&g_ntpTime);
	#define NTPTIMEOFFSET 1763850000
	if (g_enable_sendtimestamps>0 && NTP_IsTimeSynced()) {
		if ( g_sfreqcalc_ntphour_last <  3600) {
			g_sfreqcalc_secelap_last = g_ntpTime;
			g_sfreqcalc_ntphour_last = g_secondsElapsed;
		}

//		g_ntpTime = (time_t)NTP_GetCurrentTime();
		ltm = gmtime(&g_ntpTime);
		if (g_ntp_hourlast != ltm->tm_hour ) {
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ts %5d ntpts %d cur diff (with offset NTPTIMEOFFSET %d) %d\n", g_secondsElapsed
				, g_ntpTime, NTPTIMEOFFSET, (g_ntpTime - g_secondsElapsed - NTPTIMEOFFSET));
			
//				char valueStr[16];
//				sprintf(valueStr, "%f", f);
			MQTT_PublishMain_StringInt("timechk_ntptime", (int)g_ntpTime, OBK_PUBLISH_FLAG_QOS_ZERO);
			MQTT_PublishMain_StringInt("timechk_secelapsed", (int)g_secondsElapsed, OBK_PUBLISH_FLAG_QOS_ZERO);
			MQTT_PublishMain_StringInt("timechk_diff_ntp_secelapsed", (int)(g_ntpTime - g_secondsElapsed), OBK_PUBLISH_FLAG_QOS_ZERO);
			MQTT_PublishMain_StringInt("timechk_pulseStampNow", pulseStampNow, OBK_PUBLISH_FLAG_QOS_ZERO);
			MQTT_PublishMain_StringInt("timechk_diff_ntp_pulsestamp", (int)(g_ntpTime - ( (pulseStampNow * portTICK_PERIOD_MS) / 1000 )), OBK_PUBLISH_FLAG_QOS_ZERO);
			g_ntp_hourlast = ltm->tm_hour;
			#define SAMPLEFREQCALC_EVERY_X_HOUR 2			
			int secntpdiffday=(int)(g_ntpTime - g_sfreqcalc_ntpTime_last);
			if ( secntpdiffday >= ((SAMPLEFREQCALC_EVERY_X_HOUR-0 * 3600) - 60) ) {
				if ( g_sfreqcalcdone < 1 && ( SAMPLEFREQCALC_EVERY_X_HOUR != 24 || 4 == ltm->tm_hour) ) { //specific hour if every 24hours
					int secelapdiffday=(int)(g_secondsElapsed - g_sfreqcalc_secelap_last);
					//MQTT_PublishMain_StringInt("timechk_sfreqclc diff_secelapsed_ntp", (int)( secelapdiffday - secntpdiffday), OBK_PUBLISH_FLAG_QOS_ZERO);
					g_scale_samplefreq=secntpdiffday / secelapdiffday;
					//MQTT_PublishMain_StringFloat("timechk_samplefreqscale", (float)(g_scale_samplefreq), 8, OBK_PUBLISH_FLAG_QOS_ZERO);
					g_sfreqcalc_ntpTime_last = g_ntpTime;
					g_sfreqcalc_secelap_last = g_secondsElapsed;
					g_sfreqcalc_ntphour_last = ltm->tm_hour;
					g_sfreqcalcdone = 1;
				} else {
					g_sfreqcalcdone = 0;
				}
			}
		} else {
		}
	}
#endif
}
// close ENABLE_DRIVER_BL0937
#endif

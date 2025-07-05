#include "drv_pwrCal.h"

#include <math.h>
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"

static pwr_cal_type_t cal_type;

static float voltage_cal = 1;
static float current_cal = 1;
static float power_cal = 1;

static int latest_raw_voltage;
static float latest_raw_current;
static int latest_raw_power;

//#define PWRCAL_DEBUG

static commandResult_t Calibrate(const char *cmd, const char *args, float raw,
                                 float *cal, int cfg_index) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    float real = Tokenizer_GetArgFloat(0);
	if (real == 0.0f) {
        ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "%s",
                     CMD_GetResultString(CMD_RES_BAD_ARGUMENT));
        return CMD_RES_BAD_ARGUMENT;
    }
#define VERY_SMALL_VAL 0.001f
	if (raw > -VERY_SMALL_VAL && raw < VERY_SMALL_VAL) {
		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Calibration incorrect - connect load first.");
		return CMD_RES_ERROR;
	}


    *cal = (cal_type == PWR_CAL_MULTIPLY ? real / raw : raw / real);
    if (isnan(*cal) || ((*cal) > -VERY_SMALL_VAL && (*cal) < VERY_SMALL_VAL))
    {
        ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Calibration incorrect - value is zero");
        return CMD_RES_ERROR;
    }
    CFG_SetPowerMeasurementCalibrationFloat(cfg_index, *cal);

#ifdef PWRCAL_DEBUG
    ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "%s: you gave %f, set ref to %f\n",
                cmd, real, *cal);
#endif
    return CMD_RES_OK;
}

static commandResult_t CalibrateVoltage(const void *context, const char *cmd,
                                        const char *args, int cmdFlags) {
    return Calibrate(cmd, args, latest_raw_voltage, &voltage_cal,
                     CFG_OBK_VOLTAGE);
}

static commandResult_t CalibrateCurrent(const void *context, const char *cmd,
                                        const char *args, int cmdFlags) {
    return Calibrate(cmd, args, latest_raw_current, &current_cal,
                     CFG_OBK_CURRENT);
}

static commandResult_t CalibratePower(const void *context, const char *cmd,
                                      const char *args, int cmdFlags) {
    return Calibrate(cmd, args, latest_raw_power, &power_cal, CFG_OBK_POWER);
}

static float Scale(float raw, float cal) {
    return (cal_type == PWR_CAL_MULTIPLY ? raw * cal : raw / cal);
}


static commandResult_t CalibrateCmd(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	int argc = Tokenizer_GetArgsCount();

	if (argc == 0) {
		// Print current calibration floats
		ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "Calibrate values: Voltage=%.6f Current=%.6f Power=%.6f",
			voltage_cal, current_cal, power_cal);
		return CMD_RES_OK;
	}

	if (argc != 3) {
		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "%s requires exactly 3 float arguments or none",
			cmd);
		return CMD_RES_BAD_ARGUMENT;
	}

	float v = Tokenizer_GetArgFloat(0);
	float c = Tokenizer_GetArgFloat(1);
	float p = Tokenizer_GetArgFloat(2);

#define VERY_SMALL_VAL 0.001f
	if (fabsf(v) < VERY_SMALL_VAL || fabsf(c) < VERY_SMALL_VAL || fabsf(p) < VERY_SMALL_VAL) {
		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "%s: calibration values must be non-zero", cmd);
		return CMD_RES_BAD_ARGUMENT;
	}

	// Save new calibration values to config and update variables
	voltage_cal = v;
	current_cal = c;
	power_cal = p;

	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, voltage_cal);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, current_cal);
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, power_cal);

	ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "%s: Calibration updated to Voltage=%.6f Current=%.6f Power=%.6f",
		cmd, voltage_cal, current_cal, power_cal);

	return CMD_RES_OK;
}

void PwrCal_Init(pwr_cal_type_t type, float default_voltage_cal,
                 float default_current_cal, float default_power_cal) {
    cal_type = type;

    voltage_cal = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE,
                                                          default_voltage_cal);
    current_cal = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT,
                                                          default_current_cal);
    power_cal = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER,
                                                        default_power_cal);

	//cmddetail:{"name":"VoltageSet","args":"Voltage",
	//cmddetail:"descr":"Measure the real voltage with an external, reliable power meter and enter this voltage via this command to calibrate. The calibration is automatically saved in the flash memory.",
	//cmddetail:"fn":"NULL);","file":"driver/drv_pwrCal.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("VoltageSet", CalibrateVoltage, NULL);
	//cmddetail:{"name":"CurrentSet","args":"Current",
	//cmddetail:"descr":"Measure the real Current with an external, reliable power meter and enter this Current via this command to calibrate. The calibration is automatically saved in the flash memory.",
	//cmddetail:"fn":"NULL);","file":"driver/drv_pwrCal.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("CurrentSet", CalibrateCurrent, NULL);
	//cmddetail:{"name":"PowerSet","args":"Power",
	//cmddetail:"descr":"Measure the real Power with an external, reliable power meter and enter this Power via this command to calibrate. The calibration is automatically saved in the flash memory.",
	//cmddetail:"fn":"NULL);","file":"driver/drv_pwrCal.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("PowerSet", CalibratePower, NULL);


	CMD_RegisterCommand("Calibrate", CalibrateCmd, NULL); 
}

void PwrCal_Scale(int raw_voltage, float raw_current, int raw_power,
                  float *real_voltage, float *real_current, float *real_power) {
    latest_raw_voltage = raw_voltage;
    latest_raw_current = raw_current;
    latest_raw_power = raw_power;

    *real_voltage = Scale(raw_voltage, voltage_cal);
    *real_current = Scale(raw_current, current_cal);
    *real_power = Scale(raw_power, power_cal);
}

float PwrCal_ScalePowerOnly(int raw_power) {
    return Scale(raw_power, power_cal);
}

#include "drv_pwrCal.h"

#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"

static pwr_cal_type_t cal_type;

static float voltage_cal = 1;
static float current_cal = 1;
static float power_cal = 1;

static int latest_raw_voltage;
static int latest_raw_current;
static int latest_raw_power;

static uint8_t GetValidFloatArg(const char *args, float *farg) {
    if (args == 0 || *args == 0) {
        ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "This command needs one argument");
        return 0;
    }

    float val = atof(args);
    if (val == 0.0f) {
        ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "Argument must not be 0.0");
        return 0;
    }

    *farg = val;
    return 1;
}

static commandResult_t Calibrate(const char *cmd, const char *args, int raw,
                                 float *cal, int cfg_index) {
    float real;
    if (!GetValidFloatArg(args, &real))
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;

    *cal = (cal_type == PWR_CAL_MULTIPLY ? real / raw : raw / real);

    CFG_SetPowerMeasurementCalibrationFloat(cfg_index, *cal);

    ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "%s: you gave %f, set ref to %f\n",
                cmd, real, *cal);
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

static float Scale(int raw, float cal) {
    return (cal_type == PWR_CAL_MULTIPLY ? raw * cal : raw / cal);
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
}

void PwrCal_Scale(int raw_voltage, int raw_current, int raw_power,
                  float *real_voltage, float *real_current, float *real_power) {
    latest_raw_voltage = raw_voltage;
    latest_raw_current = raw_current;
    latest_raw_power = raw_power;

    *real_voltage = Scale(raw_voltage, voltage_cal);
    *real_current = Scale(raw_current, current_cal);
    *real_power = Scale(raw_power, power_cal);
}

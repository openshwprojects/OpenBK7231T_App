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

static commandResult_t SetVoltageCal(const void *context, const char *cmd,
                                     const char *args, int cmdFlags) {
    if (!GetValidFloatArg(args, &voltage_cal))
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;

    CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, voltage_cal);
    return CMD_RES_OK;
}

static commandResult_t SetCurrentCal(const void *context, const char *cmd,
                                     const char *args, int cmdFlags) {
    if (!GetValidFloatArg(args, &current_cal))
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;

    CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, current_cal);
    return CMD_RES_OK;
}

static commandResult_t SetPowerCal(const void *context, const char *cmd,
                                   const char *args, int cmdFlags) {
    if (!GetValidFloatArg(args, &power_cal))
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;

    CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, power_cal);
    return CMD_RES_OK;
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

    CMD_RegisterCommand("VoltageSet", CalibrateVoltage, NULL);
    CMD_RegisterCommand("CurrentSet", CalibrateCurrent, NULL);
    CMD_RegisterCommand("PowerSet", CalibratePower, NULL);
    CMD_RegisterCommand("VREF", SetVoltageCal, NULL);
    CMD_RegisterCommand("IREF", SetCurrentCal, NULL);
    CMD_RegisterCommand("PREF", SetPowerCal, NULL);
}

void PwrCal_Scale(int raw_voltage, int raw_current, int raw_power,
                  float *real_voltage, float *real_current, float *real_power) {
    *real_voltage = Scale(raw_voltage, voltage_cal);
    *real_current = Scale(raw_current, current_cal);
    *real_power = Scale(raw_power, power_cal);
}

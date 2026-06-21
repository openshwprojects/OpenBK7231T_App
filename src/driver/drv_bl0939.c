#include "drv_bl0939.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_BL0939SPI

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../cmnds/cmd_public.h"
#include "../hal/hal_flashVars.h"
#include "../hal/hal_ota.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_generic.h"
#include "../httpserver/hass.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "drv_public.h"

#define BL0939_SW_SPI_DELAY_US            1
#define BL0939_SPI_CMD_READ               0x55
#define BL0939_SPI_CMD_WRITE              0xA5

#define BL0939_REG_IA_FAST_RMS            0x00
#define BL0939_REG_IA_RMS                 0x04
#define BL0939_REG_IB_RMS                 0x05
#define BL0939_REG_V_RMS                  0x06
#define BL0939_REG_IB_FAST_RMS            0x07
#define BL0939_REG_A_WATT                 0x08
#define BL0939_REG_B_WATT                 0x09
#define BL0939_REG_CFA_CNT                0x0A
#define BL0939_REG_CFB_CNT                0x0B
#define BL0939_REG_A_CORNER               0x0C
#define BL0939_REG_B_CORNER               0x0D
#define BL0939_REG_TPS1                   0x0E
#define BL0939_REG_TPS2                   0x0F
#define BL0939_REG_MODE                   0x18
#define BL0939_REG_SOFT_RESET             0x19
#define BL0939_REG_USR_WRPROT             0x1A
#define BL0939_USR_WRPROT_DISABLE         0x55

#define BL0939_DEFAULT_VOLTAGE_CAL        15188.0f
#define BL0939_DEFAULT_CURRENT_A_CAL      251210.0f
#define BL0939_DEFAULT_CURRENT_B_CAL      251210.0f
#define BL0939_DEFAULT_POWER_A_CAL        598.0f
#define BL0939_DEFAULT_POWER_B_CAL        598.0f

#define BL0939_SAVE_COUNTER               3600
#define BL0939_INVALID_CF_CNT             0xFFFFFFFFu

#define BL0939_MQTT_PUBLISH_INTERVAL      10

typedef struct {
    uint32_t ia_rms;
    uint32_t ib_rms;
    uint32_t v_rms;
    int32_t a_watt;
    int32_t b_watt;
    uint32_t cfa_cnt;
    uint32_t cfb_cnt;
    uint32_t a_corner;
    uint32_t b_corner;
} BL0939_RawData_t;

typedef struct {
    float voltage;
    float current_a;
    float current_b;
    float power_a;
    float power_b;
    float apparent_a;
    float apparent_b;
    float pf_a;
    float pf_b;
} BL0939_UpdateData_t;

typedef enum {
    BL0939_CHANNEL_A = 0,
    BL0939_CHANNEL_B = 1,
} BL0939_PhaseChannel_t;

static BL0939_RawData_t last_raw;
static BL0939_UpdateData_t last_update;
static ENERGY_DATA energy_acc_a = { .Import = 0, .Export = 0 };
static ENERGY_DATA energy_acc_b = { .Import = 0, .Export = 0 };
static int save_count_down = BL0939_SAVE_COUNTER;
static uint32_t checksum_errors;
static uint32_t read_errors;
static uint32_t prev_cfa_cnt = BL0939_INVALID_CF_CNT;
static uint32_t prev_cfb_cnt = BL0939_INVALID_CF_CNT;
static int bl0939_pin_sclk = -1;
static int bl0939_pin_mosi = -1;
static int bl0939_pin_miso = -1;
static int bl0939_ready;
static int mqtt_publish_count_down;

static float cal_voltage;
static float cal_current_a;
static float cal_current_b;
static float cal_power_a;
static float cal_power_b;

typedef enum {
    BL0939_SENSOR_VOLTAGE = 0,
    BL0939_SENSOR_CURRENT_A,
    BL0939_SENSOR_POWER_A,
    BL0939_SENSOR_APPARENT_A,
    BL0939_SENSOR_IMPORT_A,
    BL0939_SENSOR_EXPORT_A,
    BL0939_SENSOR_PF_A,
    BL0939_SENSOR_CURRENT_B,
    BL0939_SENSOR_POWER_B,
    BL0939_SENSOR_APPARENT_B,
    BL0939_SENSOR_IMPORT_B,
    BL0939_SENSOR_EXPORT_B,
    BL0939_SENSOR_PF_B,
    BL0939_SENSOR_IMPORT_TOTAL,
    BL0939_SENSOR_EXPORT_TOTAL,
    BL0939_SENSOR_COUNT
} BL0939_SensorId_t;

typedef struct {
    const char *topic;
    const char *title;
    const char *device_class;
    const char *unit;
    const char *state_class;
    int decimals;
} BL0939_SensorDef_t;

static const BL0939_SensorDef_t g_bl0939_sensors[] = {
    { "bl0939_voltage",      "BL0939 Voltage",             "voltage",      "V",   "measurement",      2 },
    { "bl0939_current_a",    "BL0939 Current A",           "current",      "A",   "measurement",      3 },
    { "bl0939_power_a",      "BL0939 Power A",             "power",        "W",   "measurement",      2 },
    { "bl0939_apparent_a",   "BL0939 Apparent Power A",    "apparent_power","VA",  "measurement",      2 },
    { "bl0939_import_a",     "BL0939 Energy Import A",     "energy",       "kWh", "total_increasing", 4 },
    { "bl0939_export_a",     "BL0939 Energy Export A",     "energy",       "kWh", "total_increasing", 4 },
    { "bl0939_pf_a",         "BL0939 Power Factor A",      "power_factor", "",    "measurement",      3 },
    { "bl0939_current_b",    "BL0939 Current B",           "current",      "A",   "measurement",      3 },
    { "bl0939_power_b",      "BL0939 Power B",             "power",        "W",   "measurement",      2 },
    { "bl0939_apparent_b",   "BL0939 Apparent Power B",    "apparent_power","VA",  "measurement",      2 },
    { "bl0939_import_b",     "BL0939 Energy Import B",     "energy",       "kWh", "total_increasing", 4 },
    { "bl0939_export_b",     "BL0939 Energy Export B",     "energy",       "kWh", "total_increasing", 4 },
    { "bl0939_pf_b",         "BL0939 Power Factor B",      "power_factor", "",    "measurement",      3 },
    { "bl0939_import_total", "BL0939 Energy Import Total", "energy",       "kWh", "total_increasing", 4 },
    { "bl0939_export_total", "BL0939 Energy Export Total", "energy",       "kWh", "total_increasing", 4 },
};

static int32_t BL0939_Int24ToInt32(uint32_t val) {
    val &= 0x00FFFFFF;
    if (val & 0x00800000) {
        val |= 0xFF000000;
    }
    return (int32_t)val;
}

static int32_t BL0939_CalcSignedCounterDelta(uint32_t now, uint32_t previous) {
    int32_t diff = (int32_t)((now & 0x00FFFFFF) - (previous & 0x00FFFFFF));
    if (diff > 0x007FFFFF) {
        diff -= 0x01000000;
    } else if (diff < -0x00800000) {
        diff += 0x01000000;
    }
    return diff;
}

static float BL0939_SafeDivide(float raw, float cal) {
    if (cal > -0.000001f && cal < 0.000001f) {
        return 0.0f;
    }
    return raw / cal;
}

static void BL0939_LoadCalibration(void) {
    cal_voltage = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE, BL0939_DEFAULT_VOLTAGE_CAL);
    cal_current_a = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT, BL0939_DEFAULT_CURRENT_A_CAL);
    cal_power_a = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER, BL0939_DEFAULT_POWER_A_CAL);
    cal_current_b = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KIA, BL0939_DEFAULT_CURRENT_B_CAL);
    cal_power_b = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KIB, BL0939_DEFAULT_POWER_B_CAL);
}

static void BL0939_SaveStats(int force) {
    if (OTA_GetProgress() != -1) {
        ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "BL0939: OTA in progress, skipping energy save");
        return;
    }

    if (!force) {
        save_count_down--;
        if (save_count_down > 0) {
            return;
        }
    }

    save_count_down = BL0939_SAVE_COUNTER;
    ENERGY_DATA *data[2] = { &energy_acc_a, &energy_acc_b };
    HAL_FlashVars_SaveEnergy(data, 2);
}

static void BL0939_RestoreStats(void) {
    energy_acc_a.Import = 0;
    energy_acc_a.Export = 0;
    energy_acc_b.Import = 0;
    energy_acc_b.Export = 0;
    HAL_FlashVars_GetEnergy(&energy_acc_a, ENERGY_CHANNEL_A);
    HAL_FlashVars_GetEnergy(&energy_acc_b, ENERGY_CHANNEL_B);
}

static void BL0939_SPI_Delay(void) {
#if BL0939_SW_SPI_DELAY_US > 0
    HAL_Delay_us(BL0939_SW_SPI_DELAY_US);
#endif
}

static void BL0939_SPI_SetClock(int value) {
    HAL_PIN_SetOutputValue(bl0939_pin_sclk, value ? 1 : 0);
}

static void BL0939_SPI_SetMosi(int value) {
    HAL_PIN_SetOutputValue(bl0939_pin_mosi, value ? 1 : 0);
}

static int BL0939_SPI_GetMiso(void) {
    return HAL_PIN_ReadDigitalInput(bl0939_pin_miso) ? 1 : 0;
}

static uint8_t BL0939_SPI_TransferByte(uint8_t out) {
    uint8_t in = 0;
    int bit;

    for (bit = 7; bit >= 0; bit--) {
        BL0939_SPI_SetClock(0);
        BL0939_SPI_SetMosi((out >> bit) & 1);
        BL0939_SPI_Delay();
        BL0939_SPI_SetClock(1);
        BL0939_SPI_Delay();
        in = (uint8_t)((in << 1) | BL0939_SPI_GetMiso());
        BL0939_SPI_SetClock(0);
        BL0939_SPI_Delay();
    }

    return in;
}

static int BL0939_LoadPins(void) {
    bl0939_pin_sclk = PIN_FindPinIndexForRole(IOR_BL0939_SCLK, -1);
    bl0939_pin_mosi = PIN_FindPinIndexForRole(IOR_BL0939_MOSI, -1);
    bl0939_pin_miso = PIN_FindPinIndexForRole(IOR_BL0939_MISO, -1);

    if (bl0939_pin_sclk < 0 || bl0939_pin_mosi < 0 || bl0939_pin_miso < 0) {
        ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER,
                     "BL0939SPI requires BL0939_SCLK, BL0939_MOSI and BL0939_MISO pin roles");
        return 0;
    }
    return 1;
}

static void BL0939_SPI_SetupPins(void) {
    HAL_PIN_Setup_Output(bl0939_pin_sclk);
    HAL_PIN_Setup_Output(bl0939_pin_mosi);
    HAL_PIN_Setup_Input(bl0939_pin_miso);
    BL0939_SPI_SetClock(0);
    BL0939_SPI_SetMosi(0);
}

static int BL0939_SPI_ReadReg(uint8_t reg, uint32_t *val) {
    uint8_t recv[4];

    BL0939_SPI_TransferByte(BL0939_SPI_CMD_READ);
    BL0939_SPI_TransferByte(reg);
    recv[0] = BL0939_SPI_TransferByte(0x00);
    recv[1] = BL0939_SPI_TransferByte(0x00);
    recv[2] = BL0939_SPI_TransferByte(0x00);
    recv[3] = BL0939_SPI_TransferByte(0x00);

    uint8_t checksum = BL0939_SPI_CMD_READ + reg + recv[0] + recv[1] + recv[2];
    checksum ^= 0xFF;
    if (recv[3] != checksum) {
        checksum_errors++;
        ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
                    "BL0939: bad checksum reg=%02X got=%02X wanted=%02X bytes=%02X %02X %02X",
                    reg, recv[3], checksum, recv[0], recv[1], recv[2]);
        return -1;
    }

    *val = ((uint32_t)recv[0] << 16) | ((uint32_t)recv[1] << 8) | recv[2];
    return 0;
}

static int BL0939_SPI_WriteReg(uint8_t reg, uint32_t val) {
    uint8_t send[6];

    send[0] = BL0939_SPI_CMD_WRITE;
    send[1] = reg;
    send[2] = (val >> 16) & 0xFF;
    send[3] = (val >> 8) & 0xFF;
    send[4] = val & 0xFF;
    send[5] = (send[0] + send[1] + send[2] + send[3] + send[4]) ^ 0xFF;

    BL0939_SPI_TransferByte(send[0]);
    BL0939_SPI_TransferByte(send[1]);
    BL0939_SPI_TransferByte(send[2]);
    BL0939_SPI_TransferByte(send[3]);
    BL0939_SPI_TransferByte(send[4]);
    BL0939_SPI_TransferByte(send[5]);
    return 0;
}

static int BL0939_ReadRaw(BL0939_RawData_t *data) {
    uint32_t tmp;
    int errors = 0;

    if (BL0939_SPI_ReadReg(BL0939_REG_V_RMS, &data->v_rms) < 0) errors++;
    if (BL0939_SPI_ReadReg(BL0939_REG_IA_RMS, &data->ia_rms) < 0) errors++;
    if (BL0939_SPI_ReadReg(BL0939_REG_IB_RMS, &data->ib_rms) < 0) errors++;
    if (BL0939_SPI_ReadReg(BL0939_REG_A_WATT, &tmp) < 0) errors++; else data->a_watt = BL0939_Int24ToInt32(tmp);
    if (BL0939_SPI_ReadReg(BL0939_REG_B_WATT, &tmp) < 0) errors++; else data->b_watt = BL0939_Int24ToInt32(tmp);
    if (BL0939_SPI_ReadReg(BL0939_REG_CFA_CNT, &data->cfa_cnt) < 0) errors++;
    if (BL0939_SPI_ReadReg(BL0939_REG_CFB_CNT, &data->cfb_cnt) < 0) errors++;
    if (BL0939_SPI_ReadReg(BL0939_REG_A_CORNER, &data->a_corner) < 0) errors++;
    if (BL0939_SPI_ReadReg(BL0939_REG_B_CORNER, &data->b_corner) < 0) errors++;

    if (errors) {
        read_errors++;
    }
    return errors ? -1 : 0;
}

static void BL0939_SetEnergyStat(BL0939_PhaseChannel_t channel, float import_kwh, float export_kwh) {
    ENERGY_DATA *data = channel == BL0939_CHANNEL_B ? &energy_acc_b : &energy_acc_a;
    data->Import = import_kwh;
    data->Export = export_kwh;
    BL0939_SaveStats(1);
}

static float BL0939_GetSensorValue(BL0939_SensorId_t id) {
    switch (id) {
    case BL0939_SENSOR_VOLTAGE:
        return last_update.voltage;
    case BL0939_SENSOR_CURRENT_A:
        return last_update.current_a;
    case BL0939_SENSOR_POWER_A:
        return last_update.power_a;
    case BL0939_SENSOR_APPARENT_A:
        return last_update.apparent_a;
    case BL0939_SENSOR_IMPORT_A:
        return energy_acc_a.Import;
    case BL0939_SENSOR_EXPORT_A:
        return energy_acc_a.Export;
    case BL0939_SENSOR_PF_A:
        return last_update.pf_a;
    case BL0939_SENSOR_CURRENT_B:
        return last_update.current_b;
    case BL0939_SENSOR_POWER_B:
        return last_update.power_b;
    case BL0939_SENSOR_APPARENT_B:
        return last_update.apparent_b;
    case BL0939_SENSOR_IMPORT_B:
        return energy_acc_b.Import;
    case BL0939_SENSOR_EXPORT_B:
        return energy_acc_b.Export;
    case BL0939_SENSOR_PF_B:
        return last_update.pf_b;
    case BL0939_SENSOR_IMPORT_TOTAL:
        return energy_acc_a.Import + energy_acc_b.Import;
    case BL0939_SENSOR_EXPORT_TOTAL:
        return energy_acc_a.Export + energy_acc_b.Export;
    default:
        return 0.0f;
    }
}

static void BL0939_PublishMQTT(int force) {
#if ENABLE_MQTT
    int i;

    if (!force) {
        mqtt_publish_count_down--;
        if (mqtt_publish_count_down > 0) {
            return;
        }
    }

    mqtt_publish_count_down = BL0939_MQTT_PUBLISH_INTERVAL;
    for (i = 0; i < BL0939_SENSOR_COUNT; i++) {
        const BL0939_SensorDef_t *def = &g_bl0939_sensors[i];
        MQTT_PublishMain_StringFloat(def->topic, BL0939_GetSensorValue((BL0939_SensorId_t)i), def->decimals, OBK_PUBLISH_FLAG_QOS_ZERO);
    }
#endif
}

static void BL0939_AddCounterEnergy(int32_t delta, float power_cal, ENERGY_DATA *energy) {
    if (delta == 0) {
        return;
    }

    // BL0939 CFA_CNT/CFB_CNT are signed/algebraic energy pulse counters.
    // Use the same counter-to-Wh scaling shape as the existing BL0942 driver, but preserve sign.
    // The power calibration coefficient remains the user-facing calibration control for each CT channel.
    float energy_wh = fabsf(BL0939_SafeDivide((float)delta, power_cal)) * 1638.4f * 256.0f / 3600.0f;
    float energy_kwh = energy_wh / 1000.0f;

    if (delta > 0) {
        energy->Import += energy_kwh;
    } else {
        energy->Export += energy_kwh;
    }
}

static void BL0939_ScaleAndUpdate(const BL0939_RawData_t *data) {
    last_update.voltage = BL0939_SafeDivide((float)data->v_rms, cal_voltage);
    last_update.current_a = BL0939_SafeDivide((float)data->ia_rms, cal_current_a);
    last_update.current_b = BL0939_SafeDivide((float)data->ib_rms, cal_current_b);
    last_update.power_a = BL0939_SafeDivide((float)data->a_watt, cal_power_a);
    last_update.power_b = BL0939_SafeDivide((float)data->b_watt, cal_power_b);

    last_update.apparent_a = last_update.voltage * last_update.current_a;
    last_update.apparent_b = last_update.voltage * last_update.current_b;
    last_update.pf_a = last_update.apparent_a <= 0.001f ? 1.0f : last_update.power_a / last_update.apparent_a;
    last_update.pf_b = last_update.apparent_b <= 0.001f ? 1.0f : last_update.power_b / last_update.apparent_b;

    if (last_update.pf_a > 1.0f) last_update.pf_a = 1.0f;
    if (last_update.pf_a < -1.0f) last_update.pf_a = -1.0f;
    if (last_update.pf_b > 1.0f) last_update.pf_b = 1.0f;
    if (last_update.pf_b < -1.0f) last_update.pf_b = -1.0f;

    if (prev_cfa_cnt != BL0939_INVALID_CF_CNT) {
        int32_t cfa_delta = BL0939_CalcSignedCounterDelta(data->cfa_cnt, prev_cfa_cnt);
        int32_t cfb_delta = BL0939_CalcSignedCounterDelta(data->cfb_cnt, prev_cfb_cnt);
        BL0939_AddCounterEnergy(cfa_delta, cal_power_a, &energy_acc_a);
        BL0939_AddCounterEnergy(cfb_delta, cal_power_b, &energy_acc_b);
        ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "BL0939: cf delta A=%ld B=%ld", (long)cfa_delta, (long)cfb_delta);
    }
    prev_cfa_cnt = data->cfa_cnt;
    prev_cfb_cnt = data->cfb_cnt;

    BL0939_PublishMQTT(0);
}

static commandResult_t BL0939_CalibrateFloatValue(const char *cmd, float real, float raw, int cfg_index, float *cal) {
    if (real > -0.000001f && real < 0.000001f) {
        ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "BL0939: calibration value cannot be zero");
        return CMD_RES_BAD_ARGUMENT;
    }
    if (raw > -0.001f && raw < 0.001f) {
        ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "BL0939: raw calibration value is zero; connect load first");
        return CMD_RES_ERROR;
    }

    *cal = raw / real;
    CFG_SetPowerMeasurementCalibrationFloat(cfg_index, *cal);
    ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "BL0939: %s set calibration to %f", cmd, *cal);
    return CMD_RES_OK;
}

static commandResult_t BL0939_CalibrateFloat(const char *cmd, const char *args, float raw, int cfg_index, float *cal) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    return BL0939_CalibrateFloatValue(cmd, Tokenizer_GetArgFloat(0), raw, cfg_index, cal);
}

static commandResult_t BL0939_CalibrateVoltage(const void *context, const char *cmd, const char *args, int cmdFlags) {
    return BL0939_CalibrateFloat(cmd, args, (float)last_raw.v_rms, CFG_OBK_VOLTAGE, &cal_voltage);
}

static commandResult_t BL0939_CalibrateCurrentA(const void *context, const char *cmd, const char *args, int cmdFlags) {
    return BL0939_CalibrateFloat(cmd, args, (float)last_raw.ia_rms, CFG_OBK_CURRENT, &cal_current_a);
}

static commandResult_t BL0939_CalibrateCurrentB(const void *context, const char *cmd, const char *args, int cmdFlags) {
    return BL0939_CalibrateFloat(cmd, args, (float)last_raw.ib_rms, CFG_OBK_RES_KIA, &cal_current_b);
}

static commandResult_t BL0939_CalibratePowerA(const void *context, const char *cmd, const char *args, int cmdFlags) {
    return BL0939_CalibrateFloat(cmd, args, (float)last_raw.a_watt, CFG_OBK_POWER, &cal_power_a);
}

static commandResult_t BL0939_CalibratePowerB(const void *context, const char *cmd, const char *args, int cmdFlags) {
    return BL0939_CalibrateFloat(cmd, args, (float)last_raw.b_watt, CFG_OBK_RES_KIB, &cal_power_b);
}

static int BL0939_IsChannelArgB(const char *arg) {
    return !strcmp(arg, "b") || !strcmp(arg, "B") || !strcmp(arg, "2") || !strcmp(arg, "channel_b");
}

static int BL0939_IsChannelArgA(const char *arg) {
    return !strcmp(arg, "a") || !strcmp(arg, "A") || !strcmp(arg, "1") || !strcmp(arg, "channel_a");
}

static commandResult_t BL0939_CalibrateCurrentAB(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    const char *channel = Tokenizer_GetArg(0);
    float real = Tokenizer_GetArgFloat(1);
    if (BL0939_IsChannelArgA(channel)) {
        return BL0939_CalibrateFloatValue(cmd, real, (float)last_raw.ia_rms, CFG_OBK_CURRENT, &cal_current_a);
    }
    if (BL0939_IsChannelArgB(channel)) {
        return BL0939_CalibrateFloatValue(cmd, real, (float)last_raw.ib_rms, CFG_OBK_RES_KIA, &cal_current_b);
    }
    return CMD_RES_BAD_ARGUMENT;
}

static commandResult_t BL0939_CalibratePowerAB(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    const char *channel = Tokenizer_GetArg(0);
    float real = Tokenizer_GetArgFloat(1);
    if (BL0939_IsChannelArgA(channel)) {
        return BL0939_CalibrateFloatValue(cmd, real, (float)last_raw.a_watt, CFG_OBK_POWER, &cal_power_a);
    }
    if (BL0939_IsChannelArgB(channel)) {
        return BL0939_CalibrateFloatValue(cmd, real, (float)last_raw.b_watt, CFG_OBK_RES_KIB, &cal_power_b);
    }
    return CMD_RES_BAD_ARGUMENT;
}

static commandResult_t BL0939_SetEnergy(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    BL0939_SetEnergyStat(BL0939_CHANNEL_A, Tokenizer_GetArgFloat(0), Tokenizer_GetArgFloat(1));
    BL0939_SetEnergyStat(BL0939_CHANNEL_B, Tokenizer_GetArgFloat(2), Tokenizer_GetArgFloat(3));
    BL0939_PublishMQTT(1);
    return CMD_RES_OK;
}

static commandResult_t BL0939_ClearEnergy(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    const char *channel = Tokenizer_GetArgsCount() >= 1 ? Tokenizer_GetArg(0) : "all";

    if (!strcmp(channel, "a") || !strcmp(channel, "A") || !strcmp(channel, "channel_a")) {
        BL0939_SetEnergyStat(BL0939_CHANNEL_A, 0, 0);
    } else if (!strcmp(channel, "b") || !strcmp(channel, "B") || !strcmp(channel, "channel_b")) {
        BL0939_SetEnergyStat(BL0939_CHANNEL_B, 0, 0);
    } else {
        BL0939_SetEnergyStat(BL0939_CHANNEL_A, 0, 0);
        BL0939_SetEnergyStat(BL0939_CHANNEL_B, 0, 0);
    }
    BL0939_PublishMQTT(1);
    return CMD_RES_OK;
}

static commandResult_t BL0939_ReadRegCmd(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    int reg = Tokenizer_GetArgInteger(0);
    if (reg < 0 || reg > 0xFF) {
        return CMD_RES_BAD_ARGUMENT;
    }
    uint32_t value = 0;
    int result = BL0939_SPI_ReadReg((uint8_t)reg, &value);
    ADDLOG_INFO(LOG_FEATURE_CMD, "BL0939_ReadReg result=%d reg=%02X value=%06lX", result, reg, (unsigned long)value);
    return result < 0 ? CMD_RES_ERROR : CMD_RES_OK;
}

static commandResult_t BL0939_WriteRegCmd(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    int reg = Tokenizer_GetArgInteger(0);
    uint32_t value = (uint32_t)Tokenizer_GetArgInteger(1);
    if (reg < 0 || reg > 0xFF || value > 0xFFFFFF) {
        return CMD_RES_BAD_ARGUMENT;
    }
    int result = BL0939_SPI_WriteReg((uint8_t)reg, value);
    ADDLOG_INFO(LOG_FEATURE_CMD, "BL0939_WriteReg result=%d reg=%02X value=%06lX", result, reg, (unsigned long)value);
    return result < 0 ? CMD_RES_ERROR : CMD_RES_OK;
}

static commandResult_t BL0939_StatusCmd(const void *context, const char *cmd, const char *args, int cmdFlags) {
    ADDLOG_INFO(LOG_FEATURE_CMD,
                "BL0939 Status: V=%.2fV IA=%.3fA IB=%.3fA PA=%.2fW PB=%.2fW ImpA=%.4fkWh ExpA=%.4fkWh ImpB=%.4fkWh ExpB=%.4fkWh csum=%lu read=%lu",
                last_update.voltage,
                last_update.current_a,
                last_update.current_b,
                last_update.power_a,
                last_update.power_b,
                energy_acc_a.Import,
                energy_acc_a.Export,
                energy_acc_b.Import,
                energy_acc_b.Export,
                (unsigned long)checksum_errors,
                (unsigned long)read_errors);
    return CMD_RES_OK;
}

static void BL0939_AddCommands(void) {
    //cmddetail:{"name":"BL0939_Status","args":"","descr":"Print current BL0939 scaled readings, energy counters and SPI error counters.","fn":"BL0939_StatusCmd","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_Status"}
    CMD_RegisterCommand("BL0939_Status", BL0939_StatusCmd, NULL);
    //cmddetail:{"name":"BL0939_SetVoltage","args":"[Voltage]","descr":"Calibrate BL0939 voltage using the currently measured raw voltage and a known real voltage.","fn":"BL0939_CalibrateVoltage","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetVoltage 230.0"}
    CMD_RegisterCommand("BL0939_SetVoltage", BL0939_CalibrateVoltage, NULL);
    CMD_RegisterCommand("VoltageSet", BL0939_CalibrateVoltage, NULL);
    //cmddetail:{"name":"BL0939_SetCurrentA","args":"[Current]","descr":"Calibrate BL0939 channel A current using a known real current in amps.","fn":"BL0939_CalibrateCurrentA","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetCurrentA 1.25"}
    CMD_RegisterCommand("BL0939_SetCurrentA", BL0939_CalibrateCurrentA, NULL);
    //cmddetail:{"name":"BL0939_SetCurrentB","args":"[Current]","descr":"Calibrate BL0939 channel B current using a known real current in amps.","fn":"BL0939_CalibrateCurrentB","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetCurrentB 1.25"}
    CMD_RegisterCommand("BL0939_SetCurrentB", BL0939_CalibrateCurrentB, NULL);
    //cmddetail:{"name":"BL0939_SetCurrent","args":"[a|b] [Current]","descr":"Calibrate one BL0939 current channel using a channel selector and a known real current in amps.","fn":"BL0939_CalibrateCurrentAB","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetCurrent A 1.25"}
    CMD_RegisterCommand("BL0939_SetCurrent", BL0939_CalibrateCurrentAB, NULL);
    CMD_RegisterCommand("CurrentSet", BL0939_CalibrateCurrentAB, NULL);
    //cmddetail:{"name":"BL0939_SetPowerA","args":"[Power]","descr":"Calibrate BL0939 channel A active power using a known real power in watts.","fn":"BL0939_CalibratePowerA","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetPowerA 100.0"}
    CMD_RegisterCommand("BL0939_SetPowerA", BL0939_CalibratePowerA, NULL);
    //cmddetail:{"name":"BL0939_SetPowerB","args":"[Power]","descr":"Calibrate BL0939 channel B active power using a known real power in watts.","fn":"BL0939_CalibratePowerB","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetPowerB 100.0"}
    CMD_RegisterCommand("BL0939_SetPowerB", BL0939_CalibratePowerB, NULL);
    //cmddetail:{"name":"BL0939_SetPower","args":"[a|b] [Power]","descr":"Calibrate one BL0939 active power channel using a channel selector and a known real power in watts.","fn":"BL0939_CalibratePowerAB","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetPower B 100.0"}
    CMD_RegisterCommand("BL0939_SetPower", BL0939_CalibratePowerAB, NULL);
    CMD_RegisterCommand("PowerSet", BL0939_CalibratePowerAB, NULL);
    //cmddetail:{"name":"BL0939_SetEnergy","args":"[ImportA] [ExportA] [ImportB] [ExportB]","descr":"Set BL0939 accumulated import/export kWh counters.","fn":"BL0939_SetEnergy","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_SetEnergy 0 0 0 0"}
    CMD_RegisterCommand("BL0939_SetEnergy", BL0939_SetEnergy, NULL);
    //cmddetail:{"name":"BL0939_ClearEnergy","args":"[a|b|all]","descr":"Clear BL0939 accumulated import/export kWh counters.","fn":"BL0939_ClearEnergy","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_ClearEnergy all"}
    CMD_RegisterCommand("BL0939_ClearEnergy", BL0939_ClearEnergy, NULL);
    //cmddetail:{"name":"BL0939_ReadReg","args":"[Register]","descr":"Read a raw BL0939 register and print the result to the log. Register may be decimal or hexadecimal.","fn":"BL0939_ReadRegCmd","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_ReadReg 0x06"}
    CMD_RegisterCommand("BL0939_ReadReg", BL0939_ReadRegCmd, NULL);
    //cmddetail:{"name":"BL0939_WriteReg","args":"[Register] [Value]","descr":"Write a raw BL0939 register. Register and value may be decimal or hexadecimal; use only for diagnostics or chip setup experiments.","fn":"BL0939_WriteRegCmd","file":"driver/drv_bl0939.c","requires":"ENABLE_DRIVER_BL0939SPI","examples":"BL0939_WriteReg 0x1A 0x55"}
    CMD_RegisterCommand("BL0939_WriteReg", BL0939_WriteRegCmd, NULL);
}

void BL0939_SPI_Init(void) {
    memset(&last_raw, 0, sizeof(last_raw));
    memset(&last_update, 0, sizeof(last_update));
    prev_cfa_cnt = BL0939_INVALID_CF_CNT;
    prev_cfb_cnt = BL0939_INVALID_CF_CNT;
    checksum_errors = 0;
    read_errors = 0;
    bl0939_ready = 0;

    BL0939_LoadCalibration();
    BL0939_RestoreStats();
    BL0939_AddCommands();

    if (!BL0939_LoadPins()) {
        return;
    }

    BL0939_SPI_SetupPins();
    bl0939_ready = 1;
    mqtt_publish_count_down = 0;

    BL0939_SPI_WriteReg(BL0939_REG_USR_WRPROT, BL0939_USR_WRPROT_DISABLE);
    ADDLOG_INFO(LOG_FEATURE_ENERGYMETER,
                "BL0939SPI initialized using bit-bang pins SCLK=P%d MOSI=P%d MISO=P%d",
                bl0939_pin_sclk, bl0939_pin_mosi, bl0939_pin_miso);
}

void BL0939_SPI_Stop(void) {
    if (bl0939_ready) {
        BL0939_SaveStats(1);
        BL0939_SPI_SetClock(0);
        BL0939_SPI_SetMosi(0);
    }
    bl0939_ready = 0;
}

void BL0939_SPI_RunEverySecond(void) {
    BL0939_RawData_t data;
    memset(&data, 0, sizeof(data));

    if (!bl0939_ready) {
        return;
    }

    if (BL0939_ReadRaw(&data) < 0) {
        return;
    }
    last_raw = data;
    BL0939_ScaleAndUpdate(&data);
    BL0939_SaveStats(0);
}

void BL0939_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
    if (bPreState || !bl0939_ready) {
        return;
    }

    if (http_getArgInteger(request->url, "BL0939_ClearEnergy")) {
        char channel[8];
        if (http_getArg(request->url, "channel", channel, sizeof(channel))) {
            BL0939_ClearEnergy(NULL, "BL0939_ClearEnergy", channel, 0);
        }
    }

    poststr(request,
            "<hr><table style='width:100%'>"
            "<tr><td><b>BL0939 Actions</b></td>"
            "<td style='text-align: right;'><button style='background-color:red;' onclick='location.href=\"?BL0939_ClearEnergy=1&channel=A\"'>Clear A</button></td>"
            "<td style='text-align: right;'><button style='background-color:red;' onclick='location.href=\"?BL0939_ClearEnergy=1&channel=B\"'>Clear B</button></td>"
            "</tr></table>");
}

void BL0939_OnHassDiscovery(const char *topic) {
    HassDeviceInfo* dev_info = NULL;
    int i;

    for (i = 0; i < BL0939_SENSOR_COUNT; i++) {
        const BL0939_SensorDef_t *sensor = &g_bl0939_sensors[i];
        char state_topic[64];

        dev_info = hass_init_device_info(CUSTOM_SENSOR, i, NULL, NULL, 0, sensor->topic);
        cJSON_DeleteItemFromObject(dev_info->root, "name");
        cJSON_AddStringToObject(dev_info->root, "name", sensor->title);
        snprintf(state_topic, sizeof(state_topic), "~/%s/get", sensor->topic);
        cJSON_AddStringToObject(dev_info->root, "stat_t", state_topic);
        if (sensor->device_class[0]) {
            cJSON_AddStringToObject(dev_info->root, "dev_cla", sensor->device_class);
        }
        if (sensor->unit[0]) {
            cJSON_AddStringToObject(dev_info->root, "unit_of_meas", sensor->unit);
        }
        if (sensor->state_class[0]) {
            cJSON_AddStringToObject(dev_info->root, "stat_cla", sensor->state_class);
        }
        MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
        hass_free_device_info(dev_info);
    }

    dev_info = hass_init_button_device_info("Clear Energy A", "BL0939_ClearEnergy", "A", HASS_CATEGORY_DIAGNOSTIC);
    MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
    hass_free_device_info(dev_info);

    dev_info = hass_init_button_device_info("Clear Energy B", "BL0939_ClearEnergy", "B", HASS_CATEGORY_DIAGNOSTIC);
    MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
    hass_free_device_info(dev_info);
}

void BL0939_Save_Statistics(void) {
    if (DRV_IsRunning("BL0939SPI") && bl0939_ready) {
        BL0939_SaveStats(1);
    }
}

#endif

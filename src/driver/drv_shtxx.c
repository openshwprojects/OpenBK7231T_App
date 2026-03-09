// Do NOT add new_common.h – it pulls in stdio/stdlib transitively
// and costs 1-3KB on newlib-based toolchains.
#include "../new_pins.h"
#include "../new_cfg.h"        // needed for g_cfg.pins – included explicitly
                               // rather than relying on transitive includes
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "drv_shtxx.h"

// -------------------------------------------------------
// Config table in flash – zero RAM cost.
// hum_scale stays uint8_t (100/125) matching the original;
// the ×10 factor is applied at the single call site in
// SHTXX_Measure / SHTXX_ConvertAndStore, so no struct change.
// -------------------------------------------------------
static const shtxx_cfg_t SHT_g_cfg[2] =
{
    [SHTXX_TYPE_SHT3X] = {
        .rst_cmd      = { 0x30, 0xA2 },
        .rst_len      = 2,
        .rst_delay_ms = 2,
        .meas_cmd     = { 0x24, 0x00 },   // single-shot, high rep., no clock stretch
        .meas_len     = 2,
        .meas_delay_ms= 15,
        .hum_scale    = 100,
        .hum_offset   = 0,
    },
    [SHTXX_TYPE_SHT4X] = {
        .rst_cmd      = { 0x94, 0x00 },   // 2nd byte unused, len=1
        .rst_len      = 1,
        .rst_delay_ms = 1,
        .meas_cmd     = { 0xFD, 0x00 },   // high repeatability, 2nd byte unused
        .meas_len     = 1,
        .meas_delay_ms= 10,
        .hum_scale    = 125,
        .hum_offset   = -6,
    },
};

static shtxx_dev_t g_sensors[SHTXX_MAX_SENSORS];
static uint8_t     g_numSensors = 0;

#ifdef ENABLE_SHT3_EXTENDED_FEATURES
// Single shared string for SHT3x-only guard; avoids one rodata
// entry per call site.
static const char g_onlySht3[] = "SHTXX: SHT3x only.";
#endif

// -------------------------------------------------------
// CRC-8  poly=0x31, init=0xFF, outer loop unrolled.
// -------------------------------------------------------
static bool SHTXX_CRC8(const uint8_t *data, uint8_t expected)
{
    uint8_t crc = 0xFF ^ data[0];
    for(uint8_t bit = 0; bit < 8; bit++)
        crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    crc ^= data[1];
    for(uint8_t bit = 0; bit < 8; bit++)
        crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    return crc == expected;
}

// -------------------------------------------------------
// Send a byte sequence. Inlined at -Os (small, few sites).
// -------------------------------------------------------
static inline void SHTXX_SendCmd(shtxx_dev_t *dev, const uint8_t *cmd, uint8_t len)
{
ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX_SendCmd: Calling Soft_I2C_Start() for addr=0x%02X",dev->i2cAddr);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr);
    for(uint8_t i = 0; i < len; i++)
        Soft_I2C_WriteByte(&dev->i2c, cmd[i]);
    Soft_I2C_Stop(&dev->i2c);
}

// -------------------------------------------------------
// Send command → wait → read 6 bytes → verify both CRCs.
// -------------------------------------------------------
static bool SHTXX_SendCmdReadData(shtxx_dev_t *dev,
                                   const uint8_t *cmd, uint8_t cmd_len,
                                   uint8_t delay, uint8_t *out)
{
ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX_SendCmdReadData: Calling Soft_I2C_Start() for addr=0x%02X",dev->i2cAddr | 1);
    SHTXX_SendCmd(dev, cmd, cmd_len);
    if(delay) rtos_delay_milliseconds(delay);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr | 1);
    Soft_I2C_ReadBytes(&dev->i2c, out, 6);
    Soft_I2C_Stop(&dev->i2c);
    if(!SHTXX_CRC8(&out[0], out[2]) || !SHTXX_CRC8(&out[3], out[5]))
    {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: CRC mismatch.");
        return false;
    }
    return true;
}

// -------------------------------------------------------
// Convert raw words → 0.1-unit integers, store, set channels.
//
// WHEN BOTH FEATURE FLAGS ARE OFF this function has exactly
// one caller (SHTXX_Measure). It is marked static inline so
// the compiler folds it back in at -Os, producing zero
// function-call overhead – identical to the original layout.
//
// WHEN ENABLE_SHT3_EXTENDED_FEATURES IS ON, SHTXX_FetchPeriodic
// also calls it, so the compiler will emit it as a real
// function automatically (the inline is advisory, not forced).
//
// Integer math only – no float.
// Error vs float: <0.02 °C, <0.05 %RH (within sensor spec).
// -------------------------------------------------------
static inline void SHTXX_ConvertAndStore(shtxx_dev_t *dev, const uint8_t *data)
{
    const shtxx_cfg_t *cfg = &SHT_g_cfg[dev->typeIdx];

    uint16_t raw_t = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_h = ((uint16_t)data[3] << 8) | data[4];

    int16_t t10 = (int16_t)((1750u * raw_t + 32767u) / 65535u) - 450 + dev->calTemp;

    // hum_scale stays uint8_t (100/125); ×10 applied here for 0.1 %RH resolution.
    // 10 * 125 * 65535 = 81,918,750 – fits uint32_t without overflow.
    int16_t h10 = (int16_t)((10u * cfg->hum_scale * (uint32_t)raw_h + 32767u) / 65535u)
                  + 10 * (cfg->hum_offset + dev->calHum);
    if(h10 <    0) h10 =    0;
    if(h10 > 1000) h10 = 1000;

    dev->lastTemp  = t10;
    dev->lastHumid = h10;

    if(dev->channel_temp  >= 0) CHANNEL_Set(dev->channel_temp,  t10, 0);
    if(dev->channel_humid >= 0) CHANNEL_Set(dev->channel_humid, h10, 0);

    // Avoid stdlib abs() – single ternary, one NEG on Thumb
    int16_t t_frac = t10 % 10;
    if(t_frac < 0) t_frac = -t_frac;
    ADDLOG_INFO(LOG_FEATURE_SENSOR,
                "SHTXX (SDA=%i SCL=%i): T:%d.%dC H:%d.%d%%",
                dev->i2c.pin_data, dev->i2c.pin_clk,
                t10 / 10, t_frac, h10 / 10, h10 % 10);
}

// -------------------------------------------------------
// One-shot measurement (both types).
// -------------------------------------------------------
static void SHTXX_Measure(shtxx_dev_t *dev)
{
    const shtxx_cfg_t *cfg = &SHT_g_cfg[dev->typeIdx];
    uint8_t data[6];
    if(!SHTXX_SendCmdReadData(dev, cfg->meas_cmd, cfg->meas_len,
                               cfg->meas_delay_ms, data))
    {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: Measurement failed.");
        return;
    }
    SHTXX_ConvertAndStore(dev, data);
}

// -------------------------------------------------------
// Soft-reset, then probe with a real measurement.
// -------------------------------------------------------
static void SHTXX_Initialization(shtxx_dev_t *dev)
{
    const shtxx_cfg_t *cfg = &SHT_g_cfg[dev->typeIdx];
    uint8_t probe[6];
    SHTXX_SendCmd(dev, cfg->rst_cmd, cfg->rst_len);
    rtos_delay_milliseconds(cfg->rst_delay_ms);
    dev->isWorking = SHTXX_SendCmdReadData(dev, cfg->meas_cmd, cfg->meas_len,
                                            cfg->meas_delay_ms, probe);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: SHT%cx init %s.",
                (dev->typeIdx == SHTXX_TYPE_SHT3X) ? '3' : '4',
                dev->isWorking ? "ok" : "failed");
}

// =======================================================
// Serial number + auto-detect (ENABLE_SERIAL_READ)
// =======================================================
#ifdef ENABLE_SERIAL_READ

static bool SHTXX_ReadSerial(shtxx_dev_t *dev)
{
    static const uint8_t sn_cmd[2][2] = { { 0x36, 0x82 }, { 0x89, 0x00 } };
    static const uint8_t sn_len[2]    = { 2, 1 };
    uint8_t data[6];
    SHTXX_SendCmd(dev, sn_cmd[dev->typeIdx], sn_len[dev->typeIdx]);
    rtos_delay_milliseconds(2);
ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX_ReadSerial: Calling Soft_I2C_Start() for addr=0x%02X",dev->i2cAddr | 1);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr | 1);
    Soft_I2C_ReadBytes(&dev->i2c, data, 6);
    Soft_I2C_Stop(&dev->i2c);
    if(!SHTXX_CRC8(&data[0], data[2]) || !SHTXX_CRC8(&data[3], data[5]))
        return false;
    dev->serial = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16)
                | ((uint32_t)data[3] <<  8) |  (uint32_t)data[4];
    return true;
}

static uint8_t SHTXX_DetectType(shtxx_dev_t *dev)
{
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: Detecting (SDA=%i)...", dev->i2c.pin_data);
    dev->typeIdx = SHTXX_TYPE_SHT4X;
    if(SHTXX_ReadSerial(dev)) return SHTXX_TYPE_SHT4X;
    rtos_delay_milliseconds(10);
    dev->typeIdx = SHTXX_TYPE_SHT3X;
    if(SHTXX_ReadSerial(dev)) return SHTXX_TYPE_SHT3X;
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: Detection failed, defaulting SHT3x.");
    return SHTXX_TYPE_SHT3X;
}

// Sensor index resolver – only compiled when ENABLE_SERIAL_READ is on,
// because multi-sensor selection only makes sense when sensors can be
// identified (and when you have multiple sensors you need serial anyway).
// When ENABLE_SERIAL_READ is OFF all commands get context-sensor directly.
static shtxx_dev_t *SHTXX_GetSensorByArg(const char *cmd, int arg_index, bool arg_present)
{
    if(!arg_present) return &g_sensors[0];
    int n = Tokenizer_GetArgInteger(arg_index);
    if(n < 1 || n > (int)g_numSensors)
    {
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, "%s: sensor %i out of range (1..%i)",
                     cmd, n, g_numSensors);
        return NULL;
    }
    return &g_sensors[n - 1];
}

#endif  // ENABLE_SERIAL_READ

// =======================================================
// SHT3x-only extended features (ENABLE_SHT3_EXTENDED_FEATURES)
// =======================================================
#ifdef ENABLE_SHT3_EXTENDED_FEATURES

#define SHTXX_REQUIRE_SHT3X(dev, code) do {             \
    if((dev)->typeIdx != SHTXX_TYPE_SHT3X) {            \
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, g_onlySht3);   \
        return (code);                                   \
    }                                                    \
} while(0)

static void SHTXX_StartPeriodic(shtxx_dev_t *dev, uint8_t msb, uint8_t lsb)
{
    const uint8_t cmd[2] = { msb, lsb };
    SHTXX_SendCmd(dev, cmd, 2);
    dev->periodicActive = true;
}

static void SHTXX_StopPeriodic(shtxx_dev_t *dev)
{
    if(!dev->periodicActive) return;
    static const uint8_t cmd[2] = { 0x30, 0x93 };
    SHTXX_SendCmd(dev, cmd, 2);
    rtos_delay_milliseconds(1);
    dev->periodicActive = false;
}

static void SHTXX_FetchPeriodic(shtxx_dev_t *dev)
{
    static const uint8_t cmd[2] = { 0xE0, 0x00 };
    uint8_t data[6];
    if(!SHTXX_SendCmdReadData(dev, cmd, 2, 0, data))
    {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: Periodic fetch failed.");
        return;
    }
    SHTXX_ConvertAndStore(dev, data);  // second caller – compiler emits real fn
}

static void SHTXX_SetHeater(shtxx_dev_t *dev, bool on)
{
    const uint8_t cmd[2] = { 0x30, on ? 0x6D : 0x66 };
    SHTXX_SendCmd(dev, cmd, 2);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX (SDA=%i): Heater %s",
                dev->i2c.pin_data, on ? "on" : "off");
}

static void SHTXX_ReadStatus(shtxx_dev_t *dev)
{
    static const uint8_t cmd[2] = { 0xF3, 0x2D };
    uint8_t buf[3];
    SHTXX_SendCmd(dev, cmd, 2);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr | 1);
    Soft_I2C_ReadBytes(&dev->i2c, buf, 3);
    Soft_I2C_Stop(&dev->i2c);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX (SDA=%i): Status %02X%02X",
                dev->i2c.pin_data, buf[0], buf[1]);
}

static void SHTXX_ClearStatus(shtxx_dev_t *dev)
{
    static const uint8_t cmd[2] = { 0x30, 0x41 };
    SHTXX_SendCmd(dev, cmd, 2);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX (SDA=%i): Status cleared", dev->i2c.pin_data);
}

// Alert limits – float unavoidable for the register bit-packing.
// These are user-command paths only, never the hot measurement path.
static void SHTXX_ReadAlertReg(shtxx_dev_t *dev, uint8_t reg,
                                float *out_hum, float *out_temp)
{
    const uint8_t cmd[2] = { 0xE1, reg };
    uint8_t data[2];
    SHTXX_SendCmd(dev, cmd, 2);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr | 1);
    Soft_I2C_ReadBytes(&dev->i2c, data, 2);
    Soft_I2C_Stop(&dev->i2c);
    uint16_t w = ((uint16_t)data[0] << 8) | data[1];
    *out_hum  = 100.0f * (w & 0xFE00) / 65535.0f;
    *out_temp = 175.0f * ((uint16_t)(w << 7) / 65535.0f) - 45.0f;
}

static void SHTXX_WriteAlertReg(shtxx_dev_t *dev, uint8_t reg,
                                 float hum, float temp)
{
    if(hum < 0.0f || hum > 100.0f || temp < -45.0f || temp > 130.0f)
    {
        ADDLOG_INFO(LOG_FEATURE_CMD, "SHTXX: Alert value out of range.");
        return;
    }
    uint16_t rawH = (uint16_t)(hum  / 100.0f * 65535.0f);
    uint16_t rawT = (uint16_t)((temp + 45.0f) / 175.0f * 65535.0f);
    uint16_t w    = (rawH & 0xFE00) | ((rawT >> 7) & 0x01FF);
    uint8_t  d[2] = { (uint8_t)(w >> 8), (uint8_t)(w & 0xFF) };
    uint8_t  crc  = 0xFF ^ d[0];
    for(uint8_t b = 0; b < 8; b++) crc = (crc & 0x80) ? ((crc<<1)^0x31) : (crc<<1);
    crc ^= d[1];
    for(uint8_t b = 0; b < 8; b++) crc = (crc & 0x80) ? ((crc<<1)^0x31) : (crc<<1);
    const uint8_t cmd[2] = { 0x61, reg };
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr);
    Soft_I2C_WriteByte(&dev->i2c, cmd[0]);
    Soft_I2C_WriteByte(&dev->i2c, cmd[1]);
    Soft_I2C_WriteByte(&dev->i2c, d[0]);
    Soft_I2C_WriteByte(&dev->i2c, d[1]);
    Soft_I2C_WriteByte(&dev->i2c, crc);
    Soft_I2C_Stop(&dev->i2c);
}

static void SHTXX_GetAllAlerts(shtxx_dev_t *dev)
{
    float tLS, tLC, tHC, tHS, hLS, hLC, hHC, hHS;
    SHTXX_ReadAlertReg(dev, 0x1F, &hHS, &tHS);
    SHTXX_ReadAlertReg(dev, 0x14, &hHC, &tHC);
    SHTXX_ReadAlertReg(dev, 0x09, &hLC, &tLC);
    SHTXX_ReadAlertReg(dev, 0x02, &hLS, &tLS);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX Alert T (LS/LC/HC/HS): %.1f/%.1f/%.1f/%.1f",
                tLS, tLC, tHC, tHS);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX Alert H (LS/LC/HC/HS): %.1f/%.1f/%.1f/%.1f",
                hLS, hLC, hHC, hHS);
}

// --- Extended command handlers ---

commandResult_t SHTXX_CMD_LaunchPer(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int argc = Tokenizer_GetArgsCount();
    uint8_t msb = 0x23, lsb = 0x22;    // 4 mps, high rep (good default)
    shtxx_dev_t *dev;
    if(argc >= 2) {
        msb = (uint8_t)Tokenizer_GetArgInteger(0);
        lsb = (uint8_t)Tokenizer_GetArgInteger(1);
        dev = SHTXX_GetSensorByArg(cmd, 2, argc >= 3);
    } else {
        dev = SHTXX_GetSensorByArg(cmd, 0, argc >= 1);
    }
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_StopPeriodic(dev);
    rtos_delay_milliseconds(25);
    SHTXX_StartPeriodic(dev, msb, lsb);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX (SDA=%i): Periodic 0x%02X/0x%02X",
                dev->i2c.pin_data, msb, lsb);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_FetchPer(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    if(!dev->periodicActive) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: Start periodic first.");
        return CMD_RES_ERROR;
    }
    SHTXX_FetchPeriodic(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_StopPer(const void *context, const char *cmd,
                                   const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_StopPeriodic(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Heater(const void *context, const char *cmd,
                                  const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    int state = Tokenizer_GetArgInteger(0);
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 1, Tokenizer_GetArgsCount() >= 2);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_SetHeater(dev, state != 0);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_GetStatus(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_ReadStatus(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_ClearStatus(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_ClearStatus(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_ReadAlert(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_GetAllAlerts(dev);
    return CMD_RES_OK;
}

// SHTXX_SetAlert <tHS> <tLS> <hHS> <hLS> [sensor]
commandResult_t SHTXX_CMD_SetAlert(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 4, Tokenizer_GetArgsCount() >= 5);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    SHTXX_REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    float tHS = Tokenizer_GetArgFloat(0), tLS = Tokenizer_GetArgFloat(1);
    float hHS = Tokenizer_GetArgFloat(2), hLS = Tokenizer_GetArgFloat(3);
    SHTXX_WriteAlertReg(dev, 0x1D, hHS, tHS);
    SHTXX_WriteAlertReg(dev, 0x16, hHS - 0.5f, tHS - 0.5f);
    SHTXX_WriteAlertReg(dev, 0x0B, hLS + 0.5f, tLS + 0.5f);
    SHTXX_WriteAlertReg(dev, 0x00, hLS, tLS);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX (SDA=%i): Alert T=%.1f/%.1f H=%.1f/%.1f",
                dev->i2c.pin_data, tLS, tHS, hLS, hHS);
    return CMD_RES_OK;
}

#endif  // ENABLE_SHT3_EXTENDED_FEATURES

// =======================================================
// Core command handlers – always compiled.
// When ENABLE_SERIAL_READ is OFF these use context directly
// (same pattern as the original) to avoid linking
// SHTXX_GetSensorByArg and Tokenizer_GetArgInteger for the
// multi-sensor path that is never needed in minimal builds.
// =======================================================

commandResult_t SHTXX_CMD_Calibrate(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

#ifdef ENABLE_SERIAL_READ
    // Multi-sensor: optional 3rd arg selects target sensor
    shtxx_dev_t *dev = SHTXX_GetSensorByArg(cmd, 2, Tokenizer_GetArgsCount() >= 3);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
#else
    // Single-sensor fast path: context IS the device pointer
    shtxx_dev_t *dev = (shtxx_dev_t *)context;
#endif

    dev->calTemp = (int16_t)(Tokenizer_GetArgFloat(0) * 10.0f);
    dev->calHum  = (int8_t) (Tokenizer_GetArgFloat(1) * 10.0f);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Cycle(const void *context, const char *cmd,
                                 const char *args, int cmdFlags)
{
    shtxx_dev_t *dev = (shtxx_dev_t *)context;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    int s = Tokenizer_GetArgInteger(0);
    if(s < 1) { ADDLOG_INFO(LOG_FEATURE_CMD, "SHTXX: Min 1s."); return CMD_RES_BAD_ARGUMENT; }
    dev->secondsBetween = (uint8_t)s;
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Force(const void *context, const char *cmd,
                                 const char *args, int cmdFlags)
{
    shtxx_dev_t *dev = (shtxx_dev_t *)context;
    dev->secondsUntilNext = dev->secondsBetween;
    SHTXX_Measure(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Reinit(const void *context, const char *cmd,
                                  const char *args, int cmdFlags)
{
    shtxx_dev_t *dev = (shtxx_dev_t *)context;
    dev->secondsUntilNext = dev->secondsBetween;
    SHTXX_Initialization(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_AddSensor(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    SHTXX_Init();
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_ListSensors(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
    if(!g_numSensors) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: No sensors.");
        return CMD_RES_OK;
    }
    for(uint8_t i = 0; i < g_numSensors; i++)
    {
        shtxx_dev_t *s = &g_sensors[i];
        int16_t tf = s->lastTemp % 10;
        if(tf < 0) tf = -tf;
#ifdef ENABLE_SERIAL_READ
        ADDLOG_INFO(LOG_FEATURE_SENSOR,
                    "  [%i] SHT%cx SDA=%i SCL=%i sn=%08X T=%d.%dC H=%d.%d%% ch=%i/%i",
                    i, (s->typeIdx==SHTXX_TYPE_SHT3X)?'3':'4',
                    s->i2c.pin_data, s->i2c.pin_clk, s->serial,
                    s->lastTemp/10, tf, s->lastHumid/10, s->lastHumid%10,
                    s->channel_temp, s->channel_humid);
#else
        ADDLOG_INFO(LOG_FEATURE_SENSOR,
                    "  [%i] SHT%cx SDA=%i SCL=%i T=%d.%dC H=%d.%d%% ch=%i/%i",
                    i, (s->typeIdx==SHTXX_TYPE_SHT3X)?'3':'4',
                    s->i2c.pin_data, s->i2c.pin_clk,
                    s->lastTemp/10, tf, s->lastHumid/10, s->lastHumid%10,
                    s->channel_temp, s->channel_humid);
#endif
    }
    return CMD_RES_OK;
}

// -------------------------------------------------------
// startDriver SHTXX [SCL=<pin>] [SDA=<pin>] [chan_t=<ch>]
//                   [chan_h=<ch>] [type=3|4] [address=<hex>]
//
// type=0 or omitted → auto-detect (needs ENABLE_SERIAL_READ).
// address= accepts decimal (68) or "0x45" notation.
// -------------------------------------------------------
void SHTXX_Init()
{
    if(g_numSensors >= SHTXX_MAX_SENSORS) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: Max sensors reached.");
        return;
    }
    shtxx_dev_t *dev = &g_sensors[g_numSensors];

    dev->i2c.pin_clk   = 9;
    dev->i2c.pin_data  = 17;
    dev->channel_temp  = -1;
    dev->channel_humid = -1;
    if(g_numSensors == 0) {
        dev->i2c.pin_clk   = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, dev->i2c.pin_clk);
        dev->i2c.pin_data  = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, dev->i2c.pin_data);
        dev->channel_temp  = g_cfg.pins.channels [dev->i2c.pin_data];
        dev->channel_humid = g_cfg.pins.channels2[dev->i2c.pin_data];
    }

    dev->i2c.pin_clk   = Tokenizer_GetPinEqual("SCL=",    dev->i2c.pin_clk);
    dev->i2c.pin_data  = Tokenizer_GetPinEqual("SDA=",    dev->i2c.pin_data);
    dev->channel_temp  = Tokenizer_GetArgEqualInteger("chan_t=", dev->channel_temp);
    dev->channel_humid = Tokenizer_GetArgEqualInteger("chan_h=", dev->channel_humid);

    dev->secondsBetween   = 10;
    dev->secondsUntilNext = 1;
    dev->calTemp = 0;
    dev->calHum  = 0;

    int typeArg  = Tokenizer_GetArgEqualInteger("type=", 3);
    dev->typeIdx = (typeArg == 4) ? SHTXX_TYPE_SHT4X : SHTXX_TYPE_SHT3X;

    // Address: parse "0x45" without strtol (saves ~400B if not already in fw).
    // Use A/B suffix for backward compat: address=A → 0x44, address=B → 0x45.
    dev->i2cAddr = SHTXX_I2C_ADDR;
    if(dev->typeIdx == SHTXX_TYPE_SHT3X) {
        uint8_t A = (int8_t)(Tokenizer_GetArgEqualInteger("address=", 0x44));
        if (A != SHTXX_I2C_ADDR) dev->i2cAddr = A << 1;
    }

    Soft_I2C_PreInit(&dev->i2c);
    rtos_delay_milliseconds(50);
    
    setPinUsedString(dev->i2c.pin_clk, "SHTXX SCL");
    setPinUsedString(dev->i2c.pin_data, "SHTXX SDA");

#ifdef ENABLE_SERIAL_READ
    dev->serial = 0;
    if(typeArg == 0 || typeArg == 3) {  // explicit type=0 or default=3 → try detect
        dev->typeIdx = SHTXX_DetectType(dev);
    } else {
        SHTXX_ReadSerial(dev);  // type known, just log the serial
    }
#endif

    SHTXX_Initialization(dev);

    if(g_numSensors == 0)
    {
        //cmddetail:{"name":"SHTXX_Calibrate","args":"[DeltaTemp] [DeltaHum] [sensor]",
        //cmddetail:"descr":"Offset calibration. Floats in °C and %RH. Sensor index optional (1-based).",
        //cmddetail:"fn":"SHTXX_CMD_Calibrate","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_Calibrate -1.5 3"}
        CMD_RegisterCommand("SHTXX_Calibrate",   SHTXX_CMD_Calibrate,   dev);

        //cmddetail:{"name":"SHTXX_Cycle","args":"[seconds]",
        //cmddetail:"descr":"Measurement interval in seconds (min 1).",
        //cmddetail:"fn":"SHTXX_CMD_Cycle","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_Cycle 30"}
        CMD_RegisterCommand("SHTXX_Cycle",       SHTXX_CMD_Cycle,       dev);

        //cmddetail:{"name":"SHTXX_Measure","args":"",
        //cmddetail:"descr":"Trigger immediate one-shot measurement.",
        //cmddetail:"fn":"SHTXX_CMD_Force","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_Measure"}
        CMD_RegisterCommand("SHTXX_Measure",     SHTXX_CMD_Force,       dev);

        //cmddetail:{"name":"SHTXX_Reinit","args":"",
        //cmddetail:"descr":"Soft-reset and re-probe.",
        //cmddetail:"fn":"SHTXX_CMD_Reinit","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_Reinit"}
        CMD_RegisterCommand("SHTXX_Reinit",      SHTXX_CMD_Reinit,      dev);

        //cmddetail:{"name":"SHTXX_AddSensor","args":"[SCL=pin] [SDA=pin] [type=3|4] [chan_t=ch] [chan_h=ch]",
        //cmddetail:"descr":"Add an additional SHT sensor on different pins.",
        //cmddetail:"fn":"SHTXX_CMD_AddSensor","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_AddSensor SDA=4 SCL=5 type=3"}
        CMD_RegisterCommand("SHTXX_AddSensor",   SHTXX_CMD_AddSensor,   dev);

        //cmddetail:{"name":"SHTXX_ListSensors","args":"",
        //cmddetail:"descr":"List all registered sensors.",
        //cmddetail:"fn":"SHTXX_CMD_ListSensors","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_ListSensors"}
        CMD_RegisterCommand("SHTXX_ListSensors", SHTXX_CMD_ListSensors, dev);

#ifdef ENABLE_SHT3_EXTENDED_FEATURES
        //cmddetail:{"name":"SHTXX_LaunchPer","args":"[msb] [lsb] [sensor]",
        //cmddetail:"descr":"Start SHT3x periodic capture. Default 0x23/0x22 = 4mps high rep.",
        //cmddetail:"fn":"SHTXX_CMD_LaunchPer","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_LaunchPer\nSHTXX_LaunchPer 0x23 0x22 2"}
        CMD_RegisterCommand("SHTXX_LaunchPer",   SHTXX_CMD_LaunchPer,   dev);

        //cmddetail:{"name":"SHTXX_FetchPer","args":"[sensor]",
        //cmddetail:"descr":"Fetch latest periodic result. SHT3x only.",
        //cmddetail:"fn":"SHTXX_CMD_FetchPer","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_FetchPer"}
        CMD_RegisterCommand("SHTXX_FetchPer",    SHTXX_CMD_FetchPer,    dev);

        //cmddetail:{"name":"SHTXX_StopPer","args":"[sensor]",
        //cmddetail:"descr":"Stop SHT3x periodic capture.",
        //cmddetail:"fn":"SHTXX_CMD_StopPer","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_StopPer"}
        CMD_RegisterCommand("SHTXX_StopPer",     SHTXX_CMD_StopPer,     dev);

        //cmddetail:{"name":"SHTXX_Heater","args":"[0|1] [sensor]",
        //cmddetail:"descr":"Enable/disable heater. SHT3x only.",
        //cmddetail:"fn":"SHTXX_CMD_Heater","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_Heater 1"}
        CMD_RegisterCommand("SHTXX_Heater",      SHTXX_CMD_Heater,      dev);

        //cmddetail:{"name":"SHTXX_GetStatus","args":"[sensor]",
        //cmddetail:"descr":"Read status register. SHT3x only.",
        //cmddetail:"fn":"SHTXX_CMD_GetStatus","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_GetStatus"}
        CMD_RegisterCommand("SHTXX_GetStatus",   SHTXX_CMD_GetStatus,   dev);

        //cmddetail:{"name":"SHTXX_ClearStatus","args":"[sensor]",
        //cmddetail:"descr":"Clear status register. SHT3x only.",
        //cmddetail:"fn":"SHTXX_CMD_ClearStatus","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_ClearStatus"}
        CMD_RegisterCommand("SHTXX_ClearStatus", SHTXX_CMD_ClearStatus, dev);

        //cmddetail:{"name":"SHTXX_ReadAlert","args":"[sensor]",
        //cmddetail:"descr":"Read alert thresholds. SHT3x only.",
        //cmddetail:"fn":"SHTXX_CMD_ReadAlert","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_ReadAlert"}
        CMD_RegisterCommand("SHTXX_ReadAlert",   SHTXX_CMD_ReadAlert,   dev);

        //cmddetail:{"name":"SHTXX_SetAlert","args":"[tHS] [tLS] [hHS] [hLS] [sensor]",
        //cmddetail:"descr":"Set alert thresholds. SHT3x only.",
        //cmddetail:"fn":"SHTXX_CMD_SetAlert","file":"driver/drv_shtxx.c","requires":"",
        //cmddetail:"examples":"SHTXX_SetAlert 40 10 80 20"}
        CMD_RegisterCommand("SHTXX_SetAlert",    SHTXX_CMD_SetAlert,    dev);
#endif
    }

    g_numSensors++;
}

void SHTXX_StopDriver()
{
    for(uint8_t i = 0; i < g_numSensors; i++) {
        shtxx_dev_t *dev = &g_sensors[i];
#ifdef ENABLE_SHT3_EXTENDED_FEATURES
        if(dev->typeIdx == SHTXX_TYPE_SHT3X) SHTXX_StopPeriodic(dev);
#endif
        const shtxx_cfg_t *cfg = &SHT_g_cfg[dev->typeIdx];
        SHTXX_SendCmd(dev, cfg->rst_cmd, cfg->rst_len);
    }
}

void SHTXX_OnEverySecond()
{
    for(uint8_t i = 0; i < g_numSensors; i++) {
        shtxx_dev_t *dev = &g_sensors[i];
        if(dev->secondsUntilNext == 0) {
            if(dev->isWorking) {
#ifdef ENABLE_SHT3_EXTENDED_FEATURES
                if(dev->periodicActive) SHTXX_FetchPeriodic(dev);
                else
#endif
                SHTXX_Measure(dev);
            }
            dev->secondsUntilNext = dev->secondsBetween;
        } else {
            dev->secondsUntilNext--;
        }
    }
}

void SHTXX_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
    if(bPreState) return;
    for(uint8_t i = 0; i < g_numSensors; i++) {
        shtxx_dev_t *dev = &g_sensors[i];
        int16_t tf = dev->lastTemp % 10; if(tf < 0) tf = -tf;
        hprintf255(request,
                   "<h2>SHT%cx[%u] Temp=%d.%dC Humid=%d.%d%%</h2>",
                   (dev->typeIdx==SHTXX_TYPE_SHT3X)?'3':'4', i,
                   dev->lastTemp/10, tf, dev->lastHumid/10, dev->lastHumid%10);
        if(!dev->isWorking)
            hprintf255(request, "WARNING: SHT[%u] init failed – check pins!", i);
        if(dev->channel_humid >= 0 && dev->channel_humid == dev->channel_temp)
            hprintf255(request, "WARNING: SHT[%u] temp/humid share a channel!", i);
    }
}

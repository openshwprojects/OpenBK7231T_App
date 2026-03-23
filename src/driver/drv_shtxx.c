// drv_shtxx.c  –  SHT3x/SHT4x (Sensirion)
//
// startDriver SHTXX [-SDA <pin>] [-SCL <pin>]
//                   [-type <0|3|4>]   			0/omit=auto, 3=SHT3x, 4=SHT4x
//                   [-address <7-bit hex>]
//                   [-chan_t <ch>] [-chan_h <ch>]
//
// Additional sensors:
//   SHTXX_AddSensor -SDA <pin> -SCL <pin> [-family …] [-address …] …

#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "drv_shtxx.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_SHTXX

#define CMD_UNUSED  (void)context; (void)cmdFlags

// -----------------------------------------------------------------------
// Family indices  (0 = auto/sentinel, never stored after init)
// -----------------------------------------------------------------------
#define SHTXX_TYPE_AUTO  0
#define SHTXX_TYPE_SHT3X 3
#define SHTXX_TYPE_SHT4X 4

#ifndef SHTXX_MAX_SENSORS
#  define SHTXX_MAX_SENSORS 4
#endif

#define SHTXX_ADDR_DEFAULT  (0x44 << 1)
#define SHTXX_ADDR_ALT      (0x45 << 1)   // SHT3x only

// -----------------------------------------------------------------------
// Per-sensor state
// -----------------------------------------------------------------------
typedef struct {
    softI2C_t i2c;
    uint8_t   i2cAddr;
    uint8_t   type;             // SHTXX_TYPE_SHT3X or SHTXX_TYPE_SHT4X
    int16_t   calTemp;          // °C  × 10
    int8_t    calHum;           // %RH × 10
    int8_t    channel_temp;
    int8_t    channel_humid;
    uint8_t   secondsBetween;
    uint8_t   secondsUntilNext;
    bool      isWorking;
    int16_t   lastTemp;         // °C  × 10
    int16_t   lastHumid;        // %RH × 10
#ifdef SHTXX_ENABLE_SERIAL_LOG
    uint32_t  serial;
#endif
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES
    bool      periodicActive;
#endif
} shtxx_dev_t;

static shtxx_dev_t g_sensors[SHTXX_MAX_SENSORS];
static uint8_t     g_numSensors = 0;

// -----------------------------------------------------------------------
// I²C primitives
// -----------------------------------------------------------------------
static void I2C_Write(shtxx_dev_t *dev, uint8_t b0, uint8_t b1, uint8_t n)
{
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr);
    Soft_I2C_WriteByte(&dev->i2c, b0);
    if(n >= 2) Soft_I2C_WriteByte(&dev->i2c, b1);
    Soft_I2C_Stop(&dev->i2c);
}
static void I2C_Read(shtxx_dev_t *dev, uint8_t *buf, uint8_t n)
{
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr | 1);
    Soft_I2C_ReadBytes(&dev->i2c, buf, n);
    Soft_I2C_Stop(&dev->i2c);
}

// -----------------------------------------------------------------------
// CRC-8 (poly=0x31, init=0xFF)
// -----------------------------------------------------------------------
static uint8_t SHTXX_CRC8(const uint8_t *data, uint8_t n)
{
    uint8_t crc = 0xFF;
    while(n--) {
        crc ^= *data++;
        for(uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
    return crc;
}

// -----------------------------------------------------------------------
// Send command, wait, read 6 bytes, verify both CRCs
// -----------------------------------------------------------------------
static bool SHTXX_CmdRead6(shtxx_dev_t *dev, uint8_t b0, uint8_t b1,
                            uint8_t cmdlen, uint8_t dly_ms, uint8_t *out)
{
    I2C_Write(dev, b0, b1, cmdlen);
    if(dly_ms) rtos_delay_milliseconds(dly_ms);
    I2C_Read(dev, out, 6);
    if(SHTXX_CRC8(&out[0], 2) != out[2] || SHTXX_CRC8(&out[3], 2) != out[5]) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX CRC fail (SDA=%i)", dev->i2c.pin_data);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------
// Convert raw 6-byte frame → store + log
// -----------------------------------------------------------------------
static void SHTXX_ConvertStore(shtxx_dev_t *dev, const uint8_t *d)
{
    uint16_t raw_t = ((uint16_t)d[0] << 8) | d[1];
    uint16_t raw_h = ((uint16_t)d[3] << 8) | d[4];

    // T = -45 + 175 * raw/65535  →  optimised with >>16
    int16_t t10 = (int16_t)((1750u * (uint32_t)raw_t + 32768u) >> 16) - 450 + dev->calTemp;

    int16_t h10;
    if(dev->type == SHTXX_TYPE_SHT4X) {
        // H = -6 + 125 * raw/65535
        int32_t h = (int32_t)((1250u * (uint32_t)raw_h + 32768u) >> 16) - 60;
        h10 = (int16_t)h;
    } else {
        // H = 100 * raw/65535
        h10 = (int16_t)((1000u * (uint32_t)raw_h + 32768u) >> 16);
    }

    // clamp humidity
    if(h10 <    0) h10 =    0;
    if(h10 > 1000) h10 = 1000;
    h10 += dev->calHum;

    dev->lastTemp  = t10;
    dev->lastHumid = h10;
    if(dev->channel_temp  >= 0) CHANNEL_Set(dev->channel_temp,  t10, 0);
    if(dev->channel_humid >= 0) CHANNEL_Set(dev->channel_humid, h10, 0);

    int16_t tf = t10 % 10; if(tf < 0) tf = -tf;
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX SHT%cx (SDA=%i): T=%d.%d°C H=%d.%d%%",
                dev->type == SHTXX_TYPE_SHT4X ? '4' : '3', dev->i2c.pin_data,
                t10/10, tf, h10/10, h10%10);
}

// -----------------------------------------------------------------------
// Measure (one-shot)
// -----------------------------------------------------------------------
static void SHTXX_Measure(shtxx_dev_t *dev)
{
    uint8_t d[6];
    // SHT3x: cmd 0x2400 (2 bytes), 16 ms  |  SHT4x: cmd 0xFD (1 byte), 10 ms
    uint8_t cmd = (dev->type == SHTXX_TYPE_SHT3X) ? 0x24 : 0xFD;
    uint8_t len = (dev->type == SHTXX_TYPE_SHT3X) ? 2    : 1;
    uint8_t dly = (dev->type == SHTXX_TYPE_SHT3X) ? 16   : 10;
    if(!SHTXX_CmdRead6(dev, cmd, 0x00, len, dly, d)) return;
    SHTXX_ConvertStore(dev, d);
}

// -----------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------
static void SHTXX_Reset(shtxx_dev_t *dev)
{
    if(dev->type == SHTXX_TYPE_SHT3X)
        I2C_Write(dev, 0x30, 0xA2, 2);
    else
        I2C_Write(dev, 0x94, 0x00, 1);
    rtos_delay_milliseconds(2);
}

// -----------------------------------------------------------------------
// Probe (reads serial, validates CRC)
// -----------------------------------------------------------------------
static bool SHTXX_Probe(shtxx_dev_t *dev)
{
    uint8_t d[6];
    uint8_t cmd = (dev->type == SHTXX_TYPE_SHT3X) ? 0x36 : 0x89;
    uint8_t sub = (dev->type == SHTXX_TYPE_SHT3X) ? 0x82 : 0x00;
    uint8_t len = (dev->type == SHTXX_TYPE_SHT3X) ? 2    : 1;
    if(!SHTXX_CmdRead6(dev, cmd, sub, len, 2, d)) return false;
#ifdef SHTXX_ENABLE_SERIAL_LOG
    dev->serial = ((uint32_t)d[0]<<24)|((uint32_t)d[1]<<16)|((uint32_t)d[3]<<8)|d[4];
#endif
    return true;
}

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------
static void SHTXX_Init_Dev(shtxx_dev_t *dev)
{
#ifdef SHTXX_ENABLE_SERIAL_LOG
    if(!dev->serial) SHTXX_Probe(dev);
#endif
    SHTXX_Reset(dev);
    uint8_t d[6];
    uint8_t cmd = (dev->type == SHTXX_TYPE_SHT3X) ? 0x24 : 0xFD;
    uint8_t len = (dev->type == SHTXX_TYPE_SHT3X) ? 2    : 1;
    uint8_t dly = (dev->type == SHTXX_TYPE_SHT3X) ? 16   : 10;
    dev->isWorking = SHTXX_CmdRead6(dev, cmd, 0x00, len, dly, d);
    if(dev->isWorking) SHTXX_ConvertStore(dev, d);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX SHT%cx %s (SDA=%i)",
                dev->type == SHTXX_TYPE_SHT4X ? '4' : '3',
                dev->isWorking ? "ok" : "FAILED", dev->i2c.pin_data);
}

// -----------------------------------------------------------------------
// Auto-detect: try SHT4x@0x44, SHT3x@0x44, SHT3x@0x45
// -----------------------------------------------------------------------
static bool SHTXX_AutoDetect(shtxx_dev_t *dev)
{
    static const struct { uint8_t type; uint8_t addr; } order[] = {
        { SHTXX_TYPE_SHT4X, SHTXX_ADDR_DEFAULT },
        { SHTXX_TYPE_SHT3X, SHTXX_ADDR_DEFAULT },
        { SHTXX_TYPE_SHT3X, SHTXX_ADDR_ALT     },
    };
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: auto-detect SDA=%i SCL=%i...",
                dev->i2c.pin_data, dev->i2c.pin_clk);
    for(uint8_t i = 0; i < 3; i++) {
        dev->type    = order[i].type;
        dev->i2cAddr = order[i].addr;
        if(SHTXX_Probe(dev)) {
            ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: found SHT%cx at 0x%02X",
                        dev->type == SHTXX_TYPE_SHT4X ? '4' : '3', dev->i2cAddr >> 1);
            return true;
        }
        rtos_delay_milliseconds(10);
    }
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: nothing found, defaulting SHT3x @ 0x44");
    dev->type    = SHTXX_TYPE_SHT3X;
    dev->i2cAddr = SHTXX_ADDR_DEFAULT;
    return false;
}

// -----------------------------------------------------------------------
// Helper: resolve optional [sensorN] argument
// -----------------------------------------------------------------------
static shtxx_dev_t *SHTXX_GetSensor(const char *cmd, int argIdx, bool present)
{
    if(!present) return &g_sensors[0];
    int n = Tokenizer_GetArgInteger(argIdx);
    if(n < 1 || n > (int)g_numSensors) {
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, "%s: sensor %i out of range (1..%i)",
                     cmd, n, g_numSensors);
        return NULL;
    }
    return &g_sensors[n - 1];
}

// -----------------------------------------------------------------------
// SHT3x extended features
// -----------------------------------------------------------------------
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES

// Single rodata entry shared by every REQUIRE_SHT3X call site.
// Without this, the linker emits one copy of the string per call site.
static const char g_onlySht3[] = "SHTXX: SHT3x only.";

#define REQUIRE_SHT3X(dev, code) do { \
    if((dev)->type != SHTXX_TYPE_SHT3X) { \
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, g_onlySht3); return (code); } \
} while(0)

static void SHTXX_SHT3x_StartPeriodic(shtxx_dev_t *dev, uint8_t msb, uint8_t lsb)
{
    I2C_Write(dev, msb, lsb, 2);
    dev->periodicActive = true;
}
static void SHTXX_SHT3x_StopPeriodic(shtxx_dev_t *dev)
{
    if(!dev->periodicActive) return;
    I2C_Write(dev, 0x30, 0x93, 2);
    rtos_delay_milliseconds(1);
    dev->periodicActive = false;
}
static void SHTXX_SHT3x_FetchPeriodic(shtxx_dev_t *dev)
{
    uint8_t d[6];
    if(!SHTXX_CmdRead6(dev, 0xE0, 0x00, 2, 0, d)) return;
    SHTXX_ConvertStore(dev, d);
}

static void SHTXX_SHT3x_ReadAlertReg(shtxx_dev_t *dev, uint8_t sub,
                                      int16_t *out_h10, int16_t *out_t10)
{
    uint8_t d[2];
    I2C_Write(dev, 0xE1, sub, 2);
    I2C_Read(dev, d, 2);
    uint16_t w = ((uint16_t)d[0] << 8) | d[1];
    *out_h10 = (int16_t)((1000u * (uint32_t)(w & 0xFE00u) + 32767u) / 65535u);
    *out_t10 = (int16_t)((1750u * (uint32_t)((uint16_t)(w << 7)) + 32767u) / 65535u) - 450;
}
static void SHTXX_SHT3x_WriteAlertReg(shtxx_dev_t *dev, uint8_t sub,
                                       int16_t h10, int16_t t10)
{
    if(h10 < 0 || h10 > 1000 || t10 < -450 || t10 > 1300) {
        ADDLOG_INFO(LOG_FEATURE_CMD, "SHTXX: Alert value out of range."); return;
    }
    uint16_t rawH = (uint16_t)(((uint32_t)h10 * 65535u + 500u) / 1000u);
    uint16_t rawT = (uint16_t)(((uint32_t)(t10 + 450) * 65535u + 875u) / 1750u);
    uint16_t w    = (rawH & 0xFE00u) | ((rawT >> 7) & 0x01FFu);
    uint8_t  buf[2] = { (uint8_t)(w >> 8), (uint8_t)(w & 0xFF) };
    uint8_t  crc  = SHTXX_CRC8(buf, 2);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr);
    Soft_I2C_WriteByte(&dev->i2c, 0x61);
    Soft_I2C_WriteByte(&dev->i2c, sub);
    Soft_I2C_WriteByte(&dev->i2c, buf[0]);
    Soft_I2C_WriteByte(&dev->i2c, buf[1]);
    Soft_I2C_WriteByte(&dev->i2c, crc);
    Soft_I2C_Stop(&dev->i2c);
}

#endif // SHTXX_ENABLE_SHT3_EXTENDED_FEATURES

// -----------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------
commandResult_t SHTXX_CMD_Calibrate(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 2, Tokenizer_GetArgsCount() >= 3);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    dev->calTemp = (int16_t)(Tokenizer_GetArgFloat(0) * 10.0f);
    dev->calHum  = (int8_t) (Tokenizer_GetArgFloat(1) * 10.0f);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Cycle(const void *context, const char *cmd,
                                 const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 1, Tokenizer_GetArgsCount() >= 2);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    int s = Tokenizer_GetArgInteger(0);
    if(s < 1) { ADDLOG_INFO(LOG_FEATURE_CMD, "SHTXX: min 1s."); return CMD_RES_BAD_ARGUMENT; }
    dev->secondsBetween = (uint8_t)s;
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Force(const void *context, const char *cmd,
                                 const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev || !dev->isWorking) return CMD_RES_BAD_ARGUMENT;
    dev->secondsUntilNext = dev->secondsBetween;
    SHTXX_Measure(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_Reinit(const void *context, const char *cmd,
                                  const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
#ifdef SHTXX_ENABLE_SERIAL_LOG
    dev->serial = 0;
#endif
    SHTXX_Reset(dev);
    SHTXX_Init_Dev(dev);
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_AddSensor(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED; (void)cmd;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    SHTXX_Init();
    return CMD_RES_OK;
}

commandResult_t SHTXX_CMD_ListSensors(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
    CMD_UNUSED; (void)cmd; (void)args;
    for(uint8_t i = 0; i < g_numSensors; i++) {
        shtxx_dev_t *s = &g_sensors[i];
        int16_t tf = s->lastTemp % 10; if(tf < 0) tf = -tf;
#ifdef SHTXX_ENABLE_SERIAL_LOG
        ADDLOG_INFO(LOG_FEATURE_SENSOR,
            "  [%u] SHT%cx sn=%08X SDA=%i SCL=%i addr=0x%02X T=%d.%d°C H=%d.%d%% ch=%i/%i",
            i+1, s->type == SHTXX_TYPE_SHT4X ? '4' : '3', s->serial,
            s->i2c.pin_data, s->i2c.pin_clk, s->i2cAddr>>1,
            s->lastTemp/10, tf, s->lastHumid/10, s->lastHumid%10,
            s->channel_temp, s->channel_humid);
#else
        ADDLOG_INFO(LOG_FEATURE_SENSOR,
            "  [%u] SHT%cx SDA=%i SCL=%i addr=0x%02X T=%d.%d°C H=%d.%d%% ch=%i/%i",
            i+1, s->type == SHTXX_TYPE_SHT4X ? '4' : '3',
            s->i2c.pin_data, s->i2c.pin_clk, s->i2cAddr>>1,
            s->lastTemp/10, tf, s->lastHumid/10, s->lastHumid%10,
            s->channel_temp, s->channel_humid);
#endif
    }
    return CMD_RES_OK;
}

// -----------------------------------------------------------------------
// SHT3x extended commands
// -----------------------------------------------------------------------
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES

// Generic dispatcher for SHT3x-only no-argument commands.
// The action function is passed as context, eliminating four near-identical
// command wrappers (StopPer, GetStatus, ClearStatus, ReadAlert).
typedef void (*shtxx_devcmd_t)(shtxx_dev_t *);
static commandResult_t SHTXX_CMD_SHT3xAction(const void *context, const char *cmd,
                                               const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    ((shtxx_devcmd_t)context)(dev);
    return CMD_RES_OK;
}

// Helper wrappers with the signature expected by SHTXX_CMD_SHT3xAction.
// These are thin forwarding functions so we can pass a plain function pointer
// without casting away the const context argument at every registration site.
static void SHTXX_Act_StopPer    (shtxx_dev_t *dev) { SHTXX_SHT3x_StopPeriodic(dev); }
static void SHTXX_Act_GetStatus  (shtxx_dev_t *dev)
{
    uint8_t buf[3];
    I2C_Write(dev, 0xF3, 0x2D, 2);
    I2C_Read(dev, buf, 3);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX SHT3x status: %02X%02X (SDA=%i)",
                buf[0], buf[1], dev->i2c.pin_data);
}
static void SHTXX_Act_ClearStatus(shtxx_dev_t *dev) { I2C_Write(dev, 0x30, 0x41, 2); }
static void SHTXX_Act_ReadAlert  (shtxx_dev_t *dev)
{
    int16_t t[4], h[4];
    SHTXX_SHT3x_ReadAlertReg(dev, 0x1F, &h[3], &t[3]);
    SHTXX_SHT3x_ReadAlertReg(dev, 0x14, &h[2], &t[2]);
    SHTXX_SHT3x_ReadAlertReg(dev, 0x09, &h[1], &t[1]);
    SHTXX_SHT3x_ReadAlertReg(dev, 0x02, &h[0], &t[0]);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "Alert T: %d.%d/%d.%d/%d.%d/%d.%d",
                t[0]/10, t[0]%10<0?-t[0]%10:t[0]%10,
                t[1]/10, t[1]%10<0?-t[1]%10:t[1]%10,
                t[2]/10, t[2]%10<0?-t[2]%10:t[2]%10,
                t[3]/10, t[3]%10<0?-t[3]%10:t[3]%10);
}

commandResult_t SHTXX_CMD_LaunchPer(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int argc = Tokenizer_GetArgsCount();
    uint8_t msb = 0x23, lsb = 0x22;
    shtxx_dev_t *dev;
    if(argc >= 2) { msb=(uint8_t)Tokenizer_GetArgInteger(0); lsb=(uint8_t)Tokenizer_GetArgInteger(1); dev=SHTXX_GetSensor(cmd,2,argc>=3); }
    else          { dev = SHTXX_GetSensor(cmd, 0, argc >= 1); }
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    SHTXX_SHT3x_StopPeriodic(dev); rtos_delay_milliseconds(25);
    SHTXX_SHT3x_StartPeriodic(dev, msb, lsb);
    return CMD_RES_OK;
}
commandResult_t SHTXX_CMD_FetchPer(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    if(!dev->periodicActive) { ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: periodic not running."); return CMD_RES_ERROR; }
    SHTXX_SHT3x_FetchPeriodic(dev);
    return CMD_RES_OK;
}
// SHTXX_CMD_StopPer / GetStatus / ClearStatus / ReadAlert are registered via
// SHTXX_CMD_SHT3xAction — see CMD_RegisterCommand calls in SHTXX_Init().

commandResult_t SHTXX_CMD_Heater(const void *context, const char *cmd,
                                  const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    int on = Tokenizer_GetArgInteger(0);
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 1, Tokenizer_GetArgsCount() >= 2);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    I2C_Write(dev, 0x30, on ? 0x6D : 0x66, 2);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX SHT3x heater %s (SDA=%i)", on?"on":"off", dev->i2c.pin_data);
    return CMD_RES_OK;
}
commandResult_t SHTXX_CMD_SetAlert(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    shtxx_dev_t *dev = SHTXX_GetSensor(cmd, 4, Tokenizer_GetArgsCount() >= 5);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    int16_t tHS = (int16_t)(Tokenizer_GetArgFloat(0) * 10.0f);
    int16_t tLS = (int16_t)(Tokenizer_GetArgFloat(1) * 10.0f);
    int16_t hHS = (int16_t)(Tokenizer_GetArgFloat(2) * 10.0f);
    int16_t hLS = (int16_t)(Tokenizer_GetArgFloat(3) * 10.0f);
    SHTXX_SHT3x_WriteAlertReg(dev, 0x1D, hHS,     tHS);
    SHTXX_SHT3x_WriteAlertReg(dev, 0x16, hHS - 5, tHS - 5);
    SHTXX_SHT3x_WriteAlertReg(dev, 0x0B, hLS + 5, tLS + 5);
    SHTXX_SHT3x_WriteAlertReg(dev, 0x00, hLS,     tLS);
    return CMD_RES_OK;
}

#endif // SHTXX_ENABLE_SHT3_EXTENDED_FEATURES
// -------------------------------------------------------
// We don't have Tokenizer extension, so we need to emulate it
// helper: search a string and return index
// -------------------------------------------------------
const int checkTokens(const char *str)
{
	for (int i=0; i<Tokenizer_GetArgsCount(); i++){
		if(Tokenizer_GetArg(i) && !stricmp(Tokenizer_GetArg(i),str))
			return i;
	}
	return -1;
}

int Tokenizer_GetPinEqual(const char *str, int def){
	int x = checkTokens(str);
	if (x == -1) return def;
	return Tokenizer_GetPin(x +1 ,def); // no need to check index, tokenizer will do
}

int Tokenizer_GetArgEqualInteger(const char *str, int def){
	int x = checkTokens(str);
	if (x == -1) return def;
	return Tokenizer_GetArgIntegerDefault(x+1,def);
}



// -----------------------------------------------------------------------
// Driver entry points
// -----------------------------------------------------------------------
void SHTXX_Init(void)
{
    if(g_numSensors >= SHTXX_MAX_SENSORS) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHTXX: sensor array full (%i).", SHTXX_MAX_SENSORS);
        return;
    }
    shtxx_dev_t *dev = &g_sensors[g_numSensors];

    dev->i2c.pin_clk  = 9;
    dev->i2c.pin_data = 17;
    dev->channel_temp  = -1;
    dev->channel_humid = -1;
    if(g_numSensors == 0) {
        dev->i2c.pin_clk   = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, dev->i2c.pin_clk);
        dev->i2c.pin_data  = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, dev->i2c.pin_data);
        dev->channel_temp  = g_cfg.pins.channels [dev->i2c.pin_data];
        dev->channel_humid = g_cfg.pins.channels2[dev->i2c.pin_data];
    }
    dev->i2c.pin_clk   = Tokenizer_GetPinEqual("-SCL",   dev->i2c.pin_clk);
    dev->i2c.pin_data  = Tokenizer_GetPinEqual("-SDA",   dev->i2c.pin_data);
    dev->channel_temp  = Tokenizer_GetArgEqualInteger("-chan_t", dev->channel_temp);
    dev->channel_humid = Tokenizer_GetArgEqualInteger("-chan_h", dev->channel_humid);
    dev->secondsBetween   = 10;
    dev->secondsUntilNext = 1;
    dev->calTemp = 0;
    dev->calHum  = 0;
#ifdef SHTXX_ENABLE_SERIAL_LOG
    dev->serial  = 0;
#endif
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES
    dev->periodicActive = false;
#endif

    // type=0/omit → auto,  type=3 → SHT3x,  type=4 → SHT4x
    int reqType = Tokenizer_GetArgEqualInteger("-type", SHTXX_TYPE_AUTO);
    uint8_t addrArg = (uint8_t)Tokenizer_GetArgEqualInteger("-address", 0);

    if(reqType == SHTXX_TYPE_SHT3X || reqType == SHTXX_TYPE_SHT4X) {
        dev->type    = (uint8_t)reqType;
        dev->i2cAddr = addrArg ? (addrArg << 1) : SHTXX_ADDR_DEFAULT;
    } else {
        if(addrArg) dev->i2cAddr = addrArg << 1;
        SHTXX_AutoDetect(dev);
    }

    Soft_I2C_PreInit(&dev->i2c);
    rtos_delay_milliseconds(50);
//    setPinUsedString(dev->i2c.pin_clk,  "SHTXX SCL");
//    setPinUsedString(dev->i2c.pin_data, "SHTXX SDA");
    SHTXX_Init_Dev(dev);

    if(g_numSensors == 0) {
        CMD_RegisterCommand("SHTXX_Calibrate",   SHTXX_CMD_Calibrate,   NULL);
        CMD_RegisterCommand("SHTXX_Cycle",       SHTXX_CMD_Cycle,       NULL);
        CMD_RegisterCommand("SHTXX_Measure",     SHTXX_CMD_Force,       NULL);
        CMD_RegisterCommand("SHTXX_Reinit",      SHTXX_CMD_Reinit,      NULL);
        CMD_RegisterCommand("SHTXX_AddSensor",   SHTXX_CMD_AddSensor,   NULL);
        CMD_RegisterCommand("SHTXX_ListSensors", SHTXX_CMD_ListSensors, NULL);
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES
        CMD_RegisterCommand("SHTXX_LaunchPer",   SHTXX_CMD_LaunchPer,              NULL);
        CMD_RegisterCommand("SHTXX_FetchPer",    SHTXX_CMD_FetchPer,               NULL);
        CMD_RegisterCommand("SHTXX_StopPer",     SHTXX_CMD_SHT3xAction, (void*)SHTXX_Act_StopPer);
        CMD_RegisterCommand("SHTXX_Heater",      SHTXX_CMD_Heater,                 NULL);
        CMD_RegisterCommand("SHTXX_GetStatus",   SHTXX_CMD_SHT3xAction, (void*)SHTXX_Act_GetStatus);
        CMD_RegisterCommand("SHTXX_ClearStatus", SHTXX_CMD_SHT3xAction, (void*)SHTXX_Act_ClearStatus);
        CMD_RegisterCommand("SHTXX_ReadAlert",   SHTXX_CMD_SHT3xAction, (void*)SHTXX_Act_ReadAlert);
        CMD_RegisterCommand("SHTXX_SetAlert",    SHTXX_CMD_SetAlert,               NULL);
#endif
    }
    g_numSensors++;
}

void SHTXX_StopDriver(void)
{
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES
    for(uint8_t i = 0; i < g_numSensors; i++) {
        if(g_sensors[i].type == SHTXX_TYPE_SHT3X && g_sensors[i].periodicActive)
            SHTXX_SHT3x_StopPeriodic(&g_sensors[i]);
    }
#endif
    memset(g_sensors, 0, sizeof(g_sensors));
    g_numSensors = 0;
}

void SHTXX_OnEverySecond(void)
{
    for(uint8_t i = 0; i < g_numSensors; i++) {
        shtxx_dev_t *dev = &g_sensors[i];
        if(dev->secondsUntilNext == 0) {
            if(dev->isWorking) {
#ifdef SHTXX_ENABLE_SHT3_EXTENDED_FEATURES
                if(dev->periodicActive)
                	SHTXX_SHT3x_FetchPeriodic(dev);
                else
#endif
                SHTXX_Measure(dev);
            } else {
                if(SHTXX_AutoDetect(dev))
                    SHTXX_Init_Dev(dev);
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
        hprintf255(request, "<h2>SHT%cx[%u] T=%d.%d°C H=%d.%d%%</h2>",
                   dev->type == SHTXX_TYPE_SHT4X ? '4' : '3', i+1,
                   dev->lastTemp/10, tf, dev->lastHumid/10, dev->lastHumid%10);
        if(!dev->isWorking)
            hprintf255(request, "<p style='color:red'>WARNING: SHT%cx[%u] init failed</p>",
                       dev->type == SHTXX_TYPE_SHT4X ? '4' : '3', i+1);
    }
}

#endif // ENABLE_DRIVER_SHTXX

// drv_xhtxx.c  –  SHT3x/SHT4x (Sensirion), AHT2x (Aosong), CHT83xx (Sensylink)
//
// Bugs fixed vs original driver:
//  [1] AHT2x probe: status mask 0x68→0x08 (bit 3 only, per AHT20 datasheet §5.4)
//  [2] AHT2x probe: read status first; only send 0xBE if calibration bit clear
//  [3] AHT2x measure: read 7 bytes, verify CRC (was 6 bytes, no CRC)
//  [4] AHT2x zero guard: raw_h=0 is valid 0%RH; warn-only, don't discard
//  [5] CHT831x temperature UB: clean int16_t>>3 sign extension
//  [6] CHT831x one-shot: 2-byte write, not 3
//  [7] SHT3x periodic fetch: missing func-name arg (compile error)
//  [8] Cycle/Force/Reinit: accept optional [sensorN] like all other commands
//  [9] SHT4x humidity: clamp before int16_t cast to prevent underflow
// [10] CHT83xx probe: reject 0xFFFF (floating bus) as well as 0x0000
// [11] CHT831x temperature formula: factor-10 error; (s13*50+8)/16 → (s13*5+8)/16

#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "drv_xhtxx.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_XHTXX

#define CMD_UNUSED  (void)context; (void)cmdFlags

static xhtxx_dev_t g_sensors[XHTXX_MAX_SENSORS];
static uint8_t     g_numSensors = 0;

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
static bool XHTXX_SHT3x_Probe  (xhtxx_dev_t *dev);
static void XHTXX_SHT3x_Init   (xhtxx_dev_t *dev);
static void XHTXX_SHT3x_Measure(xhtxx_dev_t *dev);
static void XHTXX_SHT3x_Reset  (xhtxx_dev_t *dev);
static bool XHTXX_SHT4x_Probe  (xhtxx_dev_t *dev);
static void XHTXX_SHT4x_Init   (xhtxx_dev_t *dev);
static void XHTXX_SHT4x_Measure(xhtxx_dev_t *dev);
static void XHTXX_SHT4x_Reset  (xhtxx_dev_t *dev);
static bool XHTXX_AHT2x_Probe  (xhtxx_dev_t *dev);
static void XHTXX_AHT2x_Init   (xhtxx_dev_t *dev);
static void XHTXX_AHT2x_Measure(xhtxx_dev_t *dev);
static void XHTXX_AHT2x_Reset  (xhtxx_dev_t *dev);
static bool XHTXX_CHT83xx_Probe  (xhtxx_dev_t *dev);
static void XHTXX_CHT83xx_Init   (xhtxx_dev_t *dev);
static void XHTXX_CHT83xx_Measure(xhtxx_dev_t *dev);
static void XHTXX_CHT83xx_Reset  (xhtxx_dev_t *dev);

static const xhtxx_family_t g_families[XHTXX_FAMILY_COUNT] = {
    [XHTXX_FAMILY_AUTO]    = { NULL,                NULL,               NULL,                 NULL,                 "auto",    0                  },
    [XHTXX_FAMILY_SHT3X]   = { XHTXX_SHT3x_Probe,  XHTXX_SHT3x_Init,  XHTXX_SHT3x_Measure,  XHTXX_SHT3x_Reset,  "SHT3x",   XHTXX_ADDR_SHT     },
    [XHTXX_FAMILY_SHT4X]   = { XHTXX_SHT4x_Probe,  XHTXX_SHT4x_Init,  XHTXX_SHT4x_Measure,  XHTXX_SHT4x_Reset,  "SHT4x",   XHTXX_ADDR_SHT     },
    [XHTXX_FAMILY_AHT2X]   = { XHTXX_AHT2x_Probe,  XHTXX_AHT2x_Init,  XHTXX_AHT2x_Measure,  XHTXX_AHT2x_Reset,  "AHT2x",   XHTXX_ADDR_AHT2X   },
    [XHTXX_FAMILY_CHT83XX] = { XHTXX_CHT83xx_Probe, XHTXX_CHT83xx_Init, XHTXX_CHT83xx_Measure, XHTXX_CHT83xx_Reset, "CHT83xx", XHTXX_ADDR_CHT83XX },
};

static const struct { uint8_t family; uint8_t addr; } g_probeOrder[] = {
    { XHTXX_FAMILY_SHT4X,   XHTXX_ADDR_SHT     },
    { XHTXX_FAMILY_SHT3X,   XHTXX_ADDR_SHT     },
    { XHTXX_FAMILY_SHT3X,   XHTXX_ADDR_SHT_ALT },
    { XHTXX_FAMILY_AHT2X,   XHTXX_ADDR_AHT2X   },
    { XHTXX_FAMILY_CHT83XX, XHTXX_ADDR_CHT83XX },
};
#define XHTXX_PROBE_STEPS (sizeof(g_probeOrder)/sizeof(g_probeOrder[0]))

// -----------------------------------------------------------------------
// I²C primitives — same structure as original, no extra call layers
// -----------------------------------------------------------------------
static void I2C_Write(xhtxx_dev_t *dev, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t n)
{
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr);
    Soft_I2C_WriteByte(&dev->i2c, b0);
    if(n >= 2) Soft_I2C_WriteByte(&dev->i2c, b1);
    if(n >= 3) Soft_I2C_WriteByte(&dev->i2c, b2);
    Soft_I2C_Stop(&dev->i2c);
}
static void I2C_Read(xhtxx_dev_t *dev, uint8_t *buf, uint8_t n)
{
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr | 1);
    Soft_I2C_ReadBytes(&dev->i2c, buf, n);
    Soft_I2C_Stop(&dev->i2c);
}
static void I2C_ReadReg(xhtxx_dev_t *dev, uint8_t reg, uint8_t *buf, uint8_t n)
{
    I2C_Write(dev, reg, 0, 0, 1);
    I2C_Read(dev, buf, n);
}

// -----------------------------------------------------------------------
// CRC-8 (poly=0x31, init=0xFF) — returns computed value, works for any n.
// Verify:  XHTXX_CRC8(data, n) == expected
// Compute: uint8_t crc = XHTXX_CRC8(data, n)
// -----------------------------------------------------------------------
static uint8_t XHTXX_CRC8(const uint8_t *data, uint8_t n)
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
// Shared helpers
// -----------------------------------------------------------------------
static void XHTXX_StoreAndLog(xhtxx_dev_t *dev, int16_t t10, int16_t h10)
{
    if(h10 <    0) h10 =    0;
    if(h10 > 1000) h10 = 1000;
    dev->lastTemp  = t10;
    dev->lastHumid = h10;
    if(dev->channel_temp  >= 0) CHANNEL_Set(dev->channel_temp,  t10, 0);
    if(dev->channel_humid >= 0) CHANNEL_Set(dev->channel_humid, h10, 0);
    int16_t tf = t10 % 10; if(tf < 0) tf = -tf;
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX %s (SDA=%i): T=%d.%d°C H=%d.%d%%",
                g_families[dev->familyIdx].name, dev->i2c.pin_data,
                t10/10, tf, h10/10, h10%10);
}

static xhtxx_dev_t *XHTXX_GetSensor(const char *cmd, int argIdx, bool present)
{
    if(!present) return &g_sensors[0];
    int n = Tokenizer_GetArgInteger(argIdx);
    if(n < 1 || n > (int)g_numSensors) {
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, "%s: sensor %i out of range (1..%i)", cmd, n, g_numSensors);
        return NULL;
    }
    return &g_sensors[n - 1];
}

// -----------------------------------------------------------------------
// SHT shared: send command, wait, read 6 bytes, verify both CRCs.
// -----------------------------------------------------------------------
static bool XHTXX_SHT_CmdRead6(xhtxx_dev_t *dev, uint8_t b0, uint8_t b1,
                                 uint8_t cmdlen, uint8_t dly_ms, uint8_t *out,
                                 const char *func)
{
    I2C_Write(dev, b0, b1, 0, cmdlen);
    if(dly_ms) rtos_delay_milliseconds(dly_ms);
    I2C_Read(dev, out, 6);
    if(XHTXX_CRC8(&out[0], 2) != out[2] || XHTXX_CRC8(&out[3], 2) != out[5]) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: CRC (SDA=%i)", func, dev->i2c.pin_data);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------
// SHT3x  — T = -45+175×raw/65535, H = 100×raw/65535
// -----------------------------------------------------------------------
static void XHTXX_SHT3x_ConvertStore(xhtxx_dev_t *dev, const uint8_t *d)
{
    uint16_t raw_t = ((uint16_t)d[0] << 8) | d[1];
    uint16_t raw_h = ((uint16_t)d[3] << 8) | d[4];
    int16_t t10 = (int16_t)((1750u * raw_t + 32767u) / 65535u) - 450 + dev->calTemp;
    int16_t h10 = (int16_t)((1000u * (uint32_t)raw_h + 32767u) / 65535u) + dev->calHum;
    XHTXX_StoreAndLog(dev, t10, h10);
}
static bool XHTXX_SHT3x_Probe(xhtxx_dev_t *dev)
{
    uint8_t d[6];
    if(!XHTXX_SHT_CmdRead6(dev, 0x36, 0x82, 2, 2, d, "SHT3x_Probe")) return false;
#ifdef XHTXX_ENABLE_SERIAL_LOG
    dev->serial = ((uint32_t)d[0]<<24)|((uint32_t)d[1]<<16)|((uint32_t)d[3]<<8)|d[4];
#endif
    return true;
}
static void XHTXX_SHT3x_Init(xhtxx_dev_t *dev)
{
#ifdef XHTXX_ENABLE_SERIAL_LOG
    if(!dev->serial) XHTXX_SHT3x_Probe(dev);
#endif
    I2C_Write(dev, 0x30, 0xA2, 0, 2);
    rtos_delay_milliseconds(2);
    uint8_t d[6];
    dev->isWorking = XHTXX_SHT_CmdRead6(dev, 0x24, 0x00, 2, 16, d, "SHT3x_Init");
    if(dev->isWorking) XHTXX_SHT3x_ConvertStore(dev, d);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX SHT3x %s (SDA=%i addr=0x%02X)",
                dev->isWorking ? "ok" : "FAILED", dev->i2c.pin_data, dev->i2cAddr>>1);
}
static void XHTXX_SHT3x_Measure(xhtxx_dev_t *dev)
{
    uint8_t d[6];
    if(!XHTXX_SHT_CmdRead6(dev, 0x24, 0x00, 2, 16, d, "SHT3x")) return;
    XHTXX_SHT3x_ConvertStore(dev, d);
}
static void XHTXX_SHT3x_Reset(xhtxx_dev_t *dev)
{
    I2C_Write(dev, 0x30, 0xA2, 0, 2);
    rtos_delay_milliseconds(2);
}

// -----------------------------------------------------------------------
// SHT4x  — T = -45+175×raw/65535, H = -6+125×raw/65535
// -----------------------------------------------------------------------
static void XHTXX_SHT4x_ConvertStore(xhtxx_dev_t *dev, const uint8_t *d)
{
    uint16_t raw_t = ((uint16_t)d[0] << 8) | d[1];
    uint16_t raw_h = ((uint16_t)d[3] << 8) | d[4];
    int16_t  t10   = (int16_t)((1750u * raw_t + 32767u) / 65535u) - 450 + dev->calTemp;
    int32_t  h_raw = (int32_t)((1250u * (uint32_t)raw_h + 32767u) / 65535u) - 60 + dev->calHum;
    int16_t  h10   = (h_raw < 0) ? 0 : (h_raw > 1000) ? 1000 : (int16_t)h_raw;
    XHTXX_StoreAndLog(dev, t10, h10);
}
static bool XHTXX_SHT4x_Probe(xhtxx_dev_t *dev)
{
    uint8_t d[6];
    if(!XHTXX_SHT_CmdRead6(dev, 0x89, 0x00, 1, 2, d, "SHT4x_Probe")) return false;
#ifdef XHTXX_ENABLE_SERIAL_LOG
    dev->serial = ((uint32_t)d[0]<<24)|((uint32_t)d[1]<<16)|((uint32_t)d[3]<<8)|d[4];
#endif
    return true;
}
static void XHTXX_SHT4x_Init(xhtxx_dev_t *dev)
{
#ifdef XHTXX_ENABLE_SERIAL_LOG
    if(!dev->serial) XHTXX_SHT4x_Probe(dev);
#endif
    I2C_Write(dev, 0x94, 0, 0, 1);
    rtos_delay_milliseconds(1);
    uint8_t d[6];
    dev->isWorking = XHTXX_SHT_CmdRead6(dev, 0xFD, 0x00, 1, 10, d, "SHT4x_Init");
    if(dev->isWorking) XHTXX_SHT4x_ConvertStore(dev, d);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX SHT4x %s (SDA=%i)",
                dev->isWorking ? "ok" : "FAILED", dev->i2c.pin_data);
}
static void XHTXX_SHT4x_Measure(xhtxx_dev_t *dev)
{
    uint8_t d[6];
    if(!XHTXX_SHT_CmdRead6(dev, 0xFD, 0x00, 1, 10, d, "SHT4x")) return;
    XHTXX_SHT4x_ConvertStore(dev, d);
}
static void XHTXX_SHT4x_Reset(xhtxx_dev_t *dev)
{
    I2C_Write(dev, 0x94, 0, 0, 1);
    rtos_delay_milliseconds(1);
}

// -----------------------------------------------------------------------
// AHT2x  (AHT20/21/25)
// Datasheet startup: wait 40ms, read status 0x71, init 0xBE only if cal=0.
// Measurement: 0xAC 0x33 0x00 → poll busy → read 7 bytes → check CRC.
// -----------------------------------------------------------------------
static bool XHTXX_AHT2x_Probe(xhtxx_dev_t *dev)
{
    rtos_delay_milliseconds(40);
    uint8_t s;
    I2C_ReadReg(dev, 0x71, &s, 1);
    if(s == 0xFF) return false;              // bus floating — no device
    if(!(s & 0x08)) {                        // cal bit clear → send init
        I2C_Write(dev, 0xBE, 0x08, 0x00, 3);
        rtos_delay_milliseconds(10);
        I2C_ReadReg(dev, 0x71, &s, 1);
    }
    return (s != 0xFF) && (s & 0x08);       // [1] bit 3 only, not 0x68
}
static void XHTXX_AHT2x_Init(xhtxx_dev_t *dev)
{
    dev->isWorking = XHTXX_AHT2x_Probe(dev);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX AHT2x %s (SDA=%i)",
                dev->isWorking ? "ok" : "FAILED", dev->i2c.pin_data);
}
static void XHTXX_AHT2x_Measure(xhtxx_dev_t *dev)
{
    I2C_Write(dev, 0xAC, 0x33, 0x00, 3);
    rtos_delay_milliseconds(80);

    uint8_t data[7] = { 0 };
    for(uint8_t i = 0; i < 10; i++) {
        I2C_Read(dev, data, 7);
        if(!(data[0] & 0x80)) break;
        ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "XHTXX AHT2x busy (%ims)", (i+1)*20);
        rtos_delay_milliseconds(20);
    }
    if(data[0] & 0x80) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX AHT2x timed out (SDA=%i)", dev->i2c.pin_data);
        return;
    }
    if(XHTXX_CRC8(data, 6) != data[6]) {   // [3] verify 7th-byte CRC
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX AHT2x CRC fail (SDA=%i)", dev->i2c.pin_data);
        return;
    }
    uint32_t raw_h = ((uint32_t)data[1]<<12) | ((uint32_t)data[2]<<4) | (data[3]>>4);
    uint32_t raw_t = ((uint32_t)(data[3]&0x0F)<<16) | ((uint32_t)data[4]<<8) | data[5];
    int16_t h10 = (int16_t)((raw_h * 1000u + 524288u) / 1048576u) + dev->calHum;
    int16_t t10 = (int16_t)((raw_t * 2000u + 524288u) / 1048576u) - 500 + dev->calTemp;
    XHTXX_StoreAndLog(dev, t10, h10);
}
static void XHTXX_AHT2x_Reset(xhtxx_dev_t *dev)
{
    I2C_Write(dev, 0xBA, 0, 0, 1);
    rtos_delay_milliseconds(20);
}

// -----------------------------------------------------------------------
// CHT83xx  (CHT8305 / CHT8310 / CHT8315)
// -----------------------------------------------------------------------
#define CHT_REG_TEMP    0x00
#define CHT_REG_HUM     0x01
#define CHT_REG_STATUS  0x02
#define CHT_REG_CFG     0x03
#define CHT_REG_ONESHOT 0x0F
#define IS_CHT831X(dev) ((dev)->subtype == 0x8215u || (dev)->subtype == 0x8315u)

static bool XHTXX_CHT83xx_Probe(xhtxx_dev_t *dev)
{
    uint8_t buf[2];
    I2C_ReadReg(dev, 0xFE, buf, 2);
    uint16_t mfr = ((uint16_t)buf[0] << 8) | buf[1];
    if(mfr == 0x0000u || mfr == 0xFFFFu) return false;   // [10] also reject float-high
    I2C_ReadReg(dev, 0xFF, buf, 2);
    uint16_t dev_id = ((uint16_t)buf[0] << 8) | buf[1];
    if(dev_id == 0xFFFFu) return false;
    dev->subtype = dev_id;
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX CHT83xx mfr=%04X dev=%04X", mfr, dev_id);
    return true;
}
static void XHTXX_CHT83xx_Init(xhtxx_dev_t *dev)
{
    if(IS_CHT831X(dev)) {
        uint8_t status;
        I2C_ReadReg(dev, CHT_REG_STATUS, &status, 1);
        if(status)
            ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX CHT831x wake status: 0x%02X", status);
        I2C_Write(dev, CHT_REG_CFG, 0x48, 0x80, 3);
    }
    const char *v = IS_CHT831X(dev) ? (dev->subtype==0x8215u ? "CHT8310":"CHT8315") : "CHT8305";
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX %s init (SDA=%i)", v, dev->i2c.pin_data);
    dev->isWorking = true;
}
static void XHTXX_CHT83xx_Measure(xhtxx_dev_t *dev)
{
    if(IS_CHT831X(dev))
        I2C_Write(dev, CHT_REG_ONESHOT, 0x00, 0, 2);   // [6] 2-byte write
    rtos_delay_milliseconds(20);

    uint8_t buf[4];
    I2C_ReadReg(dev, CHT_REG_TEMP, buf, 4);

    int16_t t10, h10;
    if(IS_CHT831X(dev)) {
        // Re-read humidity separately to avoid parity issues in burst read
        I2C_ReadReg(dev, CHT_REG_HUM, buf+2, 2);
        // [5] clean int16_t sign extension; [11] correct formula: ×5/16 not ×50/16
        int16_t s13 = (int16_t)(((uint16_t)buf[0]<<8)|buf[1]) >> 3;
        t10 = (int16_t)((s13 * 5 + 8) / 16) + dev->calTemp;
        uint16_t rh = ((uint16_t)buf[2]<<8) | buf[3];
        h10 = (int16_t)(((rh & 0x7FFFu) * 1000u + 16384u) / 32768u) + dev->calHum;
    } else {
        uint16_t raw_t = ((uint16_t)buf[0]<<8) | buf[1];
        uint16_t raw_h = ((uint16_t)buf[2]<<8) | buf[3];
        // [10] guard against bus-idle reads
        if(raw_t == 0xFFFFu && raw_h == 0xFFFFu) return;
        t10 = (int16_t)((raw_t * 1650u + 32767u) / 65535u) - 400 + dev->calTemp;
        h10 = (int16_t)((raw_h * 1000u + 32767u) / 65535u)       + dev->calHum;
    }
    XHTXX_StoreAndLog(dev, t10, h10);
}
static void XHTXX_CHT83xx_Reset(xhtxx_dev_t *dev)
{
    if(IS_CHT831X(dev))
        I2C_Write(dev, CHT_REG_CFG, 0x08, 0x80, 3);
}

// -----------------------------------------------------------------------
// SHT3x extended features
// -----------------------------------------------------------------------
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES

static const char g_onlySht3[] = "XHTXX: SHT3x only.";
#define REQUIRE_SHT3X(dev, code) do { \
    if((dev)->familyIdx != XHTXX_FAMILY_SHT3X) { \
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, g_onlySht3); return (code); } \
} while(0)

static void XHTXX_SHT3x_StartPeriodic(xhtxx_dev_t *dev, uint8_t msb, uint8_t lsb)
{
    I2C_Write(dev, msb, lsb, 0, 2);
    dev->periodicActive = true;
}
static void XHTXX_SHT3x_StopPeriodic(xhtxx_dev_t *dev)
{
    if(!dev->periodicActive) return;
    I2C_Write(dev, 0x30, 0x93, 0, 2);
    rtos_delay_milliseconds(1);
    dev->periodicActive = false;
}
static void XHTXX_SHT3x_FetchPeriodic(xhtxx_dev_t *dev)
{
    uint8_t d[6];
    if(!XHTXX_SHT_CmdRead6(dev, 0xE0, 0x00, 2, 0, d, "SHT3x_Per")) return; // [7]
    XHTXX_SHT3x_ConvertStore(dev, d);
}
static void XHTXX_SHT3x_ReadAlertReg(xhtxx_dev_t *dev, uint8_t sub,
                                       float *out_hum, float *out_temp)
{
    uint8_t d[2];
    I2C_Write(dev, 0xE1, sub, 0, 2);
    I2C_Read(dev, d, 2);
    uint16_t w = ((uint16_t)d[0] << 8) | d[1];
    *out_hum  = 100.0f * (w & 0xFE00u) / 65535.0f;
    *out_temp = 175.0f * ((uint16_t)(w << 7) / 65535.0f) - 45.0f;
}
static void XHTXX_SHT3x_WriteAlertReg(xhtxx_dev_t *dev, uint8_t sub,
                                        float hum, float temp)
{
    if(hum < 0.0f || hum > 100.0f || temp < -45.0f || temp > 130.0f)
        { ADDLOG_INFO(LOG_FEATURE_CMD, "XHTXX: Alert value out of range."); return; }
    uint16_t rawH = (uint16_t)(hum  / 100.0f * 65535.0f);
    uint16_t rawT = (uint16_t)((temp + 45.0f) / 175.0f * 65535.0f);
    uint16_t w    = (rawH & 0xFE00u) | ((rawT >> 7) & 0x01FFu);
    uint8_t  d[2] = { (uint8_t)(w >> 8), (uint8_t)(w & 0xFF) };
    uint8_t  crc  = XHTXX_CRC8(d, 2);
    Soft_I2C_Start(&dev->i2c, dev->i2cAddr);
    Soft_I2C_WriteByte(&dev->i2c, 0x61);
    Soft_I2C_WriteByte(&dev->i2c, sub);
    Soft_I2C_WriteByte(&dev->i2c, d[0]);
    Soft_I2C_WriteByte(&dev->i2c, d[1]);
    Soft_I2C_WriteByte(&dev->i2c, crc);
    Soft_I2C_Stop(&dev->i2c);
}
#endif // XHTXX_ENABLE_SHT3_EXTENDED_FEATURES

// -----------------------------------------------------------------------
// Auto-detect + init
// -----------------------------------------------------------------------
static bool XHTXX_AutoDetect(xhtxx_dev_t *dev)
{
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX: auto-detect SDA=%i SCL=%i...",
                dev->i2c.pin_data, dev->i2c.pin_clk);
    for(uint8_t i = 0; i < XHTXX_PROBE_STEPS; i++) {
        dev->familyIdx = g_probeOrder[i].family;
        dev->i2cAddr   = g_probeOrder[i].addr;
        if(g_families[dev->familyIdx].probe_fn(dev)) {
            ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX: found %s at 0x%02X",
                        g_families[dev->familyIdx].name, dev->i2cAddr >> 1);
            return true;
        }
        rtos_delay_milliseconds(10);
    }
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX: nothing found, defaulting SHT3x @ 0x44");
    dev->familyIdx = XHTXX_FAMILY_SHT3X;
    dev->i2cAddr   = XHTXX_ADDR_SHT;
    return false;
}

// -----------------------------------------------------------------------
// Commands — all use XHTXX_GetSensor with optional [sensorN]  [8]
// -----------------------------------------------------------------------
//cmddetail:{"name":"XHTXX_Calibrate","args":"[DeltaTemp] [DeltaHum] [sensorN]",
//cmddetail:"descr":"Offset calibration in °C and %%RH (0.1 resolution). sensorN is 1-based.",
//cmddetail:"fn":"XHTXX_CMD_Calibrate","file":"driver/drv_xhtxx.c","requires":"",
//cmddetail:"examples":"XHTXX_Calibrate -1.5 3 <br /> XHTXX_Calibrate -1.5 3 2"}
commandResult_t XHTXX_CMD_Calibrate(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    int argc = Tokenizer_GetArgsCount();
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 2, argc >= 3);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    dev->calTemp = (int16_t)(Tokenizer_GetArgFloat(0) * 10.0f);
    dev->calHum  = (int8_t) (Tokenizer_GetArgFloat(1) * 10.0f);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX calibrate: calT=%d calH=%d (x0.1)",
                dev->calTemp, dev->calHum);
    return CMD_RES_OK;
}

//cmddetail:{"name":"XHTXX_Cycle","args":"[Seconds] [sensorN]",
//cmddetail:"descr":"Measurement interval in seconds (min 1). sensorN is 1-based.",
//cmddetail:"fn":"XHTXX_CMD_Cycle","file":"driver/drv_xhtxx.c","requires":"",
//cmddetail:"examples":"XHTXX_Cycle 30 <br /> XHTXX_Cycle 30 2"}
commandResult_t XHTXX_CMD_Cycle(const void *context, const char *cmd,
                                 const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 1, Tokenizer_GetArgsCount() >= 2);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    int s = Tokenizer_GetArgInteger(0);
    if(s < 1) { ADDLOG_INFO(LOG_FEATURE_CMD, "XHTXX: min 1s."); return CMD_RES_BAD_ARGUMENT; }
    dev->secondsBetween = (uint8_t)s;
    ADDLOG_INFO(LOG_FEATURE_CMD, "XHTXX: measure every %i s", s);
    return CMD_RES_OK;
}

//cmddetail:{"name":"XHTXX_Measure","args":"[sensorN]",
//cmddetail:"descr":"Immediate one-shot measurement. sensorN is 1-based.",
//cmddetail:"fn":"XHTXX_CMD_Force","file":"driver/drv_xhtxx.c","requires":"",
//cmddetail:"examples":"XHTXX_Measure <br /> XHTXX_Measure 2"}
commandResult_t XHTXX_CMD_Force(const void *context, const char *cmd,
                                 const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev || !dev->isWorking) return CMD_RES_BAD_ARGUMENT;
    dev->secondsUntilNext = dev->secondsBetween;
    g_families[dev->familyIdx].measure_fn(dev);
    return CMD_RES_OK;
}

//cmddetail:{"name":"XHTXX_Reinit","args":"[sensorN]",
//cmddetail:"descr":"Soft-reset and re-initialise sensor. sensorN is 1-based.",
//cmddetail:"fn":"XHTXX_CMD_Reinit","file":"driver/drv_xhtxx.c","requires":"",
//cmddetail:"examples":"XHTXX_Reinit <br /> XHTXX_Reinit 2"}
commandResult_t XHTXX_CMD_Reinit(const void *context, const char *cmd,
                                  const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
#ifdef XHTXX_ENABLE_SERIAL_LOG
    dev->serial = 0;
#endif
    g_families[dev->familyIdx].reset_fn(dev);
    g_families[dev->familyIdx].init_fn(dev);
    return CMD_RES_OK;
}

//cmddetail:{"name":"XHTXX_AddSensor","args":"[SDA=pin] [SCL=pin] [family=…] [chan_t=ch] [chan_h=ch]",
//cmddetail:"descr":"Register an additional sensor on different pins or family.",
//cmddetail:"fn":"XHTXX_CMD_AddSensor","file":"driver/drv_xhtxx.c","requires":"",
//cmddetail:"examples":"XHTXX_AddSensor SDA=4 SCL=5 family=aht2"}
commandResult_t XHTXX_CMD_AddSensor(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED; (void)cmd;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    XHTXX_Init();
    return CMD_RES_OK;
}

//cmddetail:{"name":"XHTXX_ListSensors","args":"",
//cmddetail:"descr":"List all registered sensors and their last readings.",
//cmddetail:"fn":"XHTXX_CMD_ListSensors","file":"driver/drv_xhtxx.c","requires":"",
//cmddetail:"examples":"XHTXX_ListSensors"}
commandResult_t XHTXX_CMD_ListSensors(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
    CMD_UNUSED; (void)cmd; (void)args;
    if(!g_numSensors) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX: no sensors registered.");
        return CMD_RES_OK;
    }
    for(uint8_t i = 0; i < g_numSensors; i++) {
        xhtxx_dev_t *s = &g_sensors[i];
        int16_t tf = s->lastTemp % 10; if(tf < 0) tf = -tf;
#ifdef XHTXX_ENABLE_SERIAL_LOG
        ADDLOG_INFO(LOG_FEATURE_SENSOR,
            "  [%u] %s sn=%08X SDA=%i SCL=%i addr=0x%02X T=%d.%d°C H=%d.%d%% ch=%i/%i",
            i+1, g_families[s->familyIdx].name, s->serial,
            s->i2c.pin_data, s->i2c.pin_clk, s->i2cAddr>>1,
            s->lastTemp/10, tf, s->lastHumid/10, s->lastHumid%10,
            s->channel_temp, s->channel_humid);
#else
        ADDLOG_INFO(LOG_FEATURE_SENSOR,
            "  [%u] %s SDA=%i SCL=%i addr=0x%02X T=%d.%d°C H=%d.%d%% ch=%i/%i",
            i+1, g_families[s->familyIdx].name,
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
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES
commandResult_t XHTXX_CMD_LaunchPer(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int argc = Tokenizer_GetArgsCount();
    uint8_t msb = 0x23, lsb = 0x22;
    xhtxx_dev_t *dev;
    if(argc >= 2) { msb=(uint8_t)Tokenizer_GetArgInteger(0); lsb=(uint8_t)Tokenizer_GetArgInteger(1); dev=XHTXX_GetSensor(cmd,2,argc>=3); }
    else          { dev = XHTXX_GetSensor(cmd, 0, argc >= 1); }
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    XHTXX_SHT3x_StopPeriodic(dev);  rtos_delay_milliseconds(25);
    XHTXX_SHT3x_StartPeriodic(dev, msb, lsb);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_FetchPer(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    if(!dev->periodicActive) { ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX: periodic not running."); return CMD_RES_ERROR; }
    XHTXX_SHT3x_FetchPeriodic(dev);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_StopPer(const void *context, const char *cmd,
                                   const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    XHTXX_SHT3x_StopPeriodic(dev);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_Heater(const void *context, const char *cmd,
                                  const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    int on = Tokenizer_GetArgInteger(0);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 1, Tokenizer_GetArgsCount() >= 2);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    I2C_Write(dev, 0x30, on ? 0x6D : 0x66, 0, 2);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX SHT3x heater %s (SDA=%i)", on?"on":"off", dev->i2c.pin_data);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_GetStatus(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    uint8_t buf[3];
    I2C_Write(dev, 0xF3, 0x2D, 0, 2);
    I2C_Read(dev, buf, 3);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX SHT3x status: %02X%02X (SDA=%i)", buf[0], buf[1], dev->i2c.pin_data);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_ClearStatus(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    I2C_Write(dev, 0x30, 0x41, 0, 2);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_ReadAlert(const void *context, const char *cmd,
                                     const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 0, Tokenizer_GetArgsCount() >= 1);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    float tLS, tLC, tHC, tHS, hLS, hLC, hHC, hHS;
    XHTXX_SHT3x_ReadAlertReg(dev, 0x1F, &hHS, &tHS);
    XHTXX_SHT3x_ReadAlertReg(dev, 0x14, &hHC, &tHC);
    XHTXX_SHT3x_ReadAlertReg(dev, 0x09, &hLC, &tLC);
    XHTXX_SHT3x_ReadAlertReg(dev, 0x02, &hLS, &tLS);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "Alert T (LS/LC/HC/HS): %.1f/%.1f/%.1f/%.1f", tLS, tLC, tHC, tHS);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "Alert H (LS/LC/HC/HS): %.1f/%.1f/%.1f/%.1f", hLS, hLC, hHC, hHS);
    return CMD_RES_OK;
}
commandResult_t XHTXX_CMD_SetAlert(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
    CMD_UNUSED;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    xhtxx_dev_t *dev = XHTXX_GetSensor(cmd, 4, Tokenizer_GetArgsCount() >= 5);
    if(!dev) return CMD_RES_BAD_ARGUMENT;
    REQUIRE_SHT3X(dev, CMD_RES_ERROR);
    float tHS=Tokenizer_GetArgFloat(0), tLS=Tokenizer_GetArgFloat(1);
    float hHS=Tokenizer_GetArgFloat(2), hLS=Tokenizer_GetArgFloat(3);
    XHTXX_SHT3x_WriteAlertReg(dev, 0x1D, hHS,        tHS);
    XHTXX_SHT3x_WriteAlertReg(dev, 0x16, hHS - 0.5f, tHS - 0.5f);
    XHTXX_SHT3x_WriteAlertReg(dev, 0x0B, hLS + 0.5f, tLS + 0.5f);
    XHTXX_SHT3x_WriteAlertReg(dev, 0x00, hLS,        tLS);
    return CMD_RES_OK;
}
#endif // XHTXX_ENABLE_SHT3_EXTENDED_FEATURES

// -----------------------------------------------------------------------
// Driver entry points
// -----------------------------------------------------------------------


void XHTXX_Init(void)
{
    if(g_numSensors >= XHTXX_MAX_SENSORS) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "XHTXX: sensor array full (%i).", XHTXX_MAX_SENSORS);
        return;
    }
    xhtxx_dev_t *dev = &g_sensors[g_numSensors];

    dev->i2c.pin_clk  = 9;
    dev->i2c.pin_data = 17;
    dev->channel_temp  = -1;
    dev->channel_humid = -1;
    if(g_numSensors == 0) {
        dev->i2c.pin_clk  = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, dev->i2c.pin_clk);
        dev->i2c.pin_data = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, dev->i2c.pin_data);
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
#ifdef XHTXX_ENABLE_SERIAL_LOG
    dev->serial  = 0;
#endif
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES
    dev->periodicActive = false;
#endif

    const char *fam = Tokenizer_GetArgEqualDefault("-family", "default");
    uint8_t reqFam = XHTXX_FAMILY_AUTO;
    if     (fam[0]=='s'||fam[0]=='S') reqFam = (fam[3]=='4') ? XHTXX_FAMILY_SHT4X : XHTXX_FAMILY_SHT3X;
    else if(fam[0]=='a'||fam[0]=='A') reqFam = XHTXX_FAMILY_AHT2X;
    else if(fam[0]=='c'||fam[0]=='C') reqFam = XHTXX_FAMILY_CHT83XX;

    uint8_t addrArg = (uint8_t)Tokenizer_GetArgEqualInteger("-address", 0);
    if(reqFam == XHTXX_FAMILY_AUTO) {
        if(addrArg) dev->i2cAddr = addrArg << 1;
        XHTXX_AutoDetect(dev);
    } else {
        dev->familyIdx = reqFam;
        dev->i2cAddr   = addrArg ? (addrArg << 1) : g_families[reqFam].defaultAddr;
    }

    Soft_I2C_PreInit(&dev->i2c);
    rtos_delay_milliseconds(50);
    setPinUsedString(dev->i2c.pin_clk,  "XHTXX SCL");
    setPinUsedString(dev->i2c.pin_data, "XHTXX SDA");
    g_families[dev->familyIdx].init_fn(dev);

    if(g_numSensors == 0) {
        CMD_RegisterCommand("XHTXX_Calibrate",   XHTXX_CMD_Calibrate,   NULL);
        CMD_RegisterCommand("XHTXX_Cycle",       XHTXX_CMD_Cycle,       NULL);
        CMD_RegisterCommand("XHTXX_Measure",     XHTXX_CMD_Force,       NULL);
        CMD_RegisterCommand("XHTXX_Reinit",      XHTXX_CMD_Reinit,      NULL);
        CMD_RegisterCommand("XHTXX_AddSensor",   XHTXX_CMD_AddSensor,   NULL);
        CMD_RegisterCommand("XHTXX_ListSensors", XHTXX_CMD_ListSensors, NULL);
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES
        CMD_RegisterCommand("XHTXX_LaunchPer",   XHTXX_CMD_LaunchPer,   NULL);
        CMD_RegisterCommand("XHTXX_FetchPer",    XHTXX_CMD_FetchPer,    NULL);
        CMD_RegisterCommand("XHTXX_StopPer",     XHTXX_CMD_StopPer,     NULL);
        CMD_RegisterCommand("XHTXX_Heater",      XHTXX_CMD_Heater,      NULL);
        CMD_RegisterCommand("XHTXX_GetStatus",   XHTXX_CMD_GetStatus,   NULL);
        CMD_RegisterCommand("XHTXX_ClearStatus", XHTXX_CMD_ClearStatus, NULL);
        CMD_RegisterCommand("XHTXX_ReadAlert",   XHTXX_CMD_ReadAlert,   NULL);
        CMD_RegisterCommand("XHTXX_SetAlert",    XHTXX_CMD_SetAlert,    NULL);
#endif
    }
    g_numSensors++;
}

void XHTXX_StopDriver(void)
{
    for(uint8_t i = 0; i < g_numSensors; i++) {
        xhtxx_dev_t *dev = &g_sensors[i];
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES
        if(dev->familyIdx == XHTXX_FAMILY_SHT3X && dev->periodicActive)
            XHTXX_SHT3x_StopPeriodic(dev);
#endif
        g_families[dev->familyIdx].reset_fn(dev);
    }
}

void XHTXX_OnEverySecond(void)
{
    for(uint8_t i = 0; i < g_numSensors; i++) {
        xhtxx_dev_t *dev = &g_sensors[i];
        if(dev->secondsUntilNext == 0) {
            if(dev->isWorking) {
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES
                if(dev->periodicActive) XHTXX_SHT3x_FetchPeriodic(dev); else
#endif
                g_families[dev->familyIdx].measure_fn(dev);
            } else {
                 if (XHTXX_AutoDetect(dev))
                 	g_families[dev->familyIdx].init_fn(dev);
            }
            dev->secondsUntilNext = dev->secondsBetween;
        } else {
            dev->secondsUntilNext--;
        }
    }
}

void XHTXX_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
    if(bPreState) return;
    for(uint8_t i = 0; i < g_numSensors; i++) {
        xhtxx_dev_t *dev = &g_sensors[i];
        int16_t tf = dev->lastTemp % 10; if(tf < 0) tf = -tf;
        hprintf255(request, "<h2>%s[%u] T=%d.%d°C H=%d.%d%%</h2>",
                   g_families[dev->familyIdx].name, i+1,
                   dev->lastTemp/10, tf, dev->lastHumid/10, dev->lastHumid%10);
        if(!dev->isWorking)
            hprintf255(request, "<p style='color:red'>WARNING: %s[%u] init failed</p>",
                       g_families[dev->familyIdx].name, i+1);
    }
}

#endif // ENABLE_DRIVER_XHTXX

#pragma once
#include <stdint.h>
#include <stdbool.h>

// -------------------------------------------------------
// Feature gates.
//
// ENABLE_SERIAL_READ
//   Reads serial number at init; shows it in ListSensors;
//   enables type=0 auto-detect.  ~150B extra flash, 4B RAM
//   per sensor.  Safe to disable when type= is always
//   specified and serial identification is not needed.
//
// ENABLE_SHT3_EXTENDED_FEATURES
//   SHT3x-only commands: heater, status, alert limits,
//   periodic capture.  Requires float for alert bit-math.
//   ~1.5KB code + float softlib (~2KB) if not already in fw.
//   ENABLE_SERIAL_READ must also be defined when this is on.
// -------------------------------------------------------
#define ENABLE_SERIAL_READ
//#define ENABLE_SHT3_EXTENDED_FEATURES

#define SHTXX_I2C_ADDR   (0x44 << 1)
#define SHTXX_TYPE_SHT3X  0
#define SHTXX_TYPE_SHT4X  1
#define SHTXX_MAX_SENSORS 4

// -------------------------------------------------------
// Config table – one entry per sensor family, in flash.
//
// hum_scale / hum_offset kept as uint8_t/int8_t (same as
// original) so the struct stays compact and the multiply
// stays cheap.  The ×10 scaling is applied inline:
//   RH10 = (10 * hum_scale * raw + 32767) / 65535 + 10 * hum_offset
//   SHT3x: scale=100, offset=0   → 0..1000
//   SHT4x: scale=125, offset=-6  → -60..1190 → clamped to 0..1000
//   max intermediate: 10*125*65535 = 81,918,750 → fits uint32_t ✓
// -------------------------------------------------------
typedef struct
{
    uint8_t  rst_cmd[2];
    uint8_t  rst_len;
    uint8_t  rst_delay_ms;
    uint8_t  meas_cmd[2];
    uint8_t  meas_len;
    uint8_t  meas_delay_ms;
    uint8_t  hum_scale;     // 100 or 125
    int8_t   hum_offset;    // 0 or -6
} shtxx_cfg_t;

// -------------------------------------------------------
// Per-instance state.
//
// lastTemp:  0.1 °C  (e.g. 225 = 22.5 °C)
// lastHumid: 0.1 %RH (e.g. 456 = 45.6 %RH)
// calTemp:   0.1 °C offset
// calHum:    0.1 %RH offset  (int8_t: range ±12.7 %RH –
//            sufficient for any realistic humidity offset)
// -------------------------------------------------------
typedef struct
{
    softI2C_t   i2c;
    int16_t     calTemp;        // 0.1 °C
    int8_t      calHum;         // 0.1 %RH  (±12.7 %RH range)
    int8_t      channel_temp;   // -1 = not used
    int8_t      channel_humid;  // -1 = not used
    uint8_t     i2cAddr;
    uint8_t     typeIdx;
    uint8_t     secondsBetween;
    uint8_t     secondsUntilNext;
    bool        isWorking;
    int16_t     lastTemp;       // 0.1 °C
    int16_t     lastHumid;      // 0.1 %RH
#ifdef ENABLE_SERIAL_READ
    uint32_t    serial;
#endif
#ifdef ENABLE_SHT3_EXTENDED_FEATURES
    bool        periodicActive;
#endif
} shtxx_dev_t;

void SHTXX_Init();
void SHTXX_StopDriver();
void SHTXX_OnEverySecond();
void SHTXX_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

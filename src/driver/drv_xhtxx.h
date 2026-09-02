// drv_xhtxx.h  –  SHT3x/SHT4x (Sensirion), AHT2x (Aosong), CHT83xx (Sensylink)
//
// Public types and API only.  All implementation lives in drv_xhtxx.c.
//
// Supported families
//   SHT3x  – SHT30 / SHT31 / SHT35          (addr 0x44 or 0x45)
//   SHT4x  – SHT40 / SHT41 / SHT43 / SHT45  (addr 0x44)
//   AHT2x  – AHT20 / AHT21 / AHT25          (addr 0x38)
//   CHT83xx– CHT8305 / CHT8310 / CHT8315     (addr 0x40)
//
// startDriver syntax:
//   startDriver XHTXX [-SDA <pin>] [-SCL <pin>]
//                     [-family sht3|sht4|aht2|cht]    		omit → auto-detect
//                     [-address <7-bit hex>]          		override I²C addr
//                     [-chan_t <ch>] [chan_h <ch>]
//
// Additional sensors:
//   XHTXX_AddSensor -SDA <pin> -SCL <pin> [-family …] [-address …] …
//
// Feature gates (define before including or in build flags):
//   XHTXX_ENABLE_SERIAL_LOG          – store + print SHT serial numbers
//   XHTXX_ENABLE_SHT3_EXTENDED_FEATURES – SHT3x heater/alerts/periodic mode

#ifndef DRV_XHTXX_H
#define DRV_XHTXX_H

#define XHTXX_ENABLE_SERIAL_LOG			1
#define XHTXX_ENABLE_SHT3_EXTENDED_FEATURES	1

#include <stdint.h>
#include <stdbool.h>

// -----------------------------------------------------------------------
// Limits
// -----------------------------------------------------------------------
#ifndef XHTXX_MAX_SENSORS
#  define XHTXX_MAX_SENSORS 4
#endif

// -----------------------------------------------------------------------
// Family indices
// -----------------------------------------------------------------------
#define XHTXX_FAMILY_AUTO    0   // auto-detect (sentinel, never stored)
#define XHTXX_FAMILY_SHT3X   1
#define XHTXX_FAMILY_SHT4X   2
#define XHTXX_FAMILY_AHT2X   3
#define XHTXX_FAMILY_CHT83XX 4
#define XHTXX_FAMILY_COUNT   5

// -----------------------------------------------------------------------
// I²C addresses (pre-shifted to 8-bit form, LSB = R/W)
// -----------------------------------------------------------------------
#define XHTXX_ADDR_SHT      (0x44 << 1)
#define XHTXX_ADDR_SHT_ALT  (0x45 << 1)
#define XHTXX_ADDR_AHT2X    (0x38 << 1)
#define XHTXX_ADDR_CHT83XX  (0x40 << 1)

// -----------------------------------------------------------------------
// Per-sensor state (forward-declared for use in family dispatch table)
// -----------------------------------------------------------------------
typedef struct xhtxx_dev_s xhtxx_dev_t;

// -----------------------------------------------------------------------
// Family dispatch table entry (stored in flash)
// -----------------------------------------------------------------------
typedef struct {
    bool       (*probe_fn)  (xhtxx_dev_t *dev); // non-destructive probe
    void       (*init_fn)   (xhtxx_dev_t *dev); // full init, sets isWorking
    void       (*measure_fn)(xhtxx_dev_t *dev); // one-shot measurement
    void       (*reset_fn)  (xhtxx_dev_t *dev); // soft reset
    const char  *name;                           // "SHT3x", "AHT2x", …
    uint8_t     defaultAddr;                     // pre-shifted
} xhtxx_family_t;

// -----------------------------------------------------------------------
// Per-sensor state
//
// lastTemp  : °C  × 10  (e.g. 225 = 22.5 °C)
// lastHumid : %RH × 10  (e.g. 456 = 45.6 %RH)
// calTemp   : °C  × 10  calibration offset
// calHum    : %RH × 10  calibration offset
// -----------------------------------------------------------------------
struct xhtxx_dev_s {
    softI2C_t  i2c;
    uint8_t    i2cAddr;         // pre-shifted 8-bit address
    uint8_t    familyIdx;       // XHTXX_FAMILY_* (never AUTO after init)
    uint16_t   subtype;         // CHT variant id; 0 for others
    int16_t    calTemp;         // °C  × 10
    int8_t     calHum;          // %RH × 10
    int8_t     channel_temp;    // -1 = unused
    int8_t     channel_humid;   // -1 = unused
    uint8_t    secondsBetween;
    uint8_t    secondsUntilNext;
    bool       isWorking;
    int16_t    lastTemp;        // °C  × 10
    int16_t    lastHumid;       // %RH × 10
#ifdef XHTXX_ENABLE_SERIAL_LOG
    uint32_t   serial;
#endif
#ifdef XHTXX_ENABLE_SHT3_EXTENDED_FEATURES
    bool       periodicActive;
#endif
};

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------
void XHTXX_Init(void);
void XHTXX_StopDriver(void);
void XHTXX_OnEverySecond(void);
void XHTXX_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#endif // DRV_XHTXX_H

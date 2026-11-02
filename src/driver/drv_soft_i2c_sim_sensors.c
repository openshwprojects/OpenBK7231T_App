// drv_soft_i2c_sim_sensors.c
// -------------------------------------------------------
// Sensor plugins the soft-I2C "simulator".
//
// All protocol and encoding details are derived from the
// official manufacturer datasheets:
//
//   SHT3x  – Sensirion Datasheet SHT3x-DIS v7 (Dec 2022)
//   SHT4x  – Sensirion Datasheet SHT4x v6.5 (Apr 2024)
//   AHT2x  – Aosong AHT20/AHT21 Datasheet A0/A1 (2020)
//   BMP280 – Bosch BST-BMP280-DS001 v1.14 (2015)
//   BME280 – Bosch BST-BME280-DS002 (2018)
//   CHT8305/8310/8315 – Sensylink CHT83xx Datasheet
//
//   VEML7700 - Vishay VEML7700 Datasheet Rev. 1.8, 28-Nov-2024 (Doc 84286)
//
// Each plugin:
//   init()             – seed default value ranges
//   encode_response()  – produce the exact byte sequence
//                        the real chip would return
//
// -------------------------------------------------------
#ifdef WINDOWS

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../logging/logging.h"

// e.g. for wal_stricmp (instead of missing strcasecmp)
#include "../new_common.h"

// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"


#include "drv_soft_i2c_sim.h"

// ===================================================================
// Shared helpers
// ===================================================================

// CRC-8/NRSC-5  poly=0x31 init=0xFF  (Sensirion convention, all sensors)
/*
static uint8_t crc8_sensirion(uint8_t a, uint8_t b) {
    uint8_t crc = 0xFF ^ a;
    for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    crc ^= b;
    for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    return crc;
}
*/
static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    
    return crc;
}
// Pack a 3-byte Sensirion word [MSB, LSB, CRC] into buf
static void pack_word(uint8_t *buf, uint16_t raw) {
    buf[0] = (uint8_t)(raw >> 8);
    buf[1] = (uint8_t)(raw & 0xFF);
//    buf[2] = crc8_sensirion(buf[0], buf[1]);
    buf[2] = crc8(&buf[0],2);
}

// Pack two Sensirion words (T then H) = 6 bytes
static void pack_th(uint8_t *resp, uint16_t raw_t, uint16_t raw_h) {
    pack_word(resp,     raw_t);
    pack_word(resp + 3, raw_h);
}

// ===================================================================
// SHT3x plugin
// ===================================================================
// Reference: Sensirion Datasheet SHT3x-DIS v7 (Dec 2022)
//
// I2C address: 0x44 (ADDR=GND) or 0x45 (ADDR=VDD)
//
// Commands (16-bit, MSB first):
//   0x2400  single-shot high-rep, no clock stretch
//   0x240B  single-shot medium-rep, no clock stretch
//   0x2416  single-shot low-rep, no clock stretch
//   0x2C06  single-shot high-rep, clock stretch
//   0x2C0D  single-shot medium-rep, clock stretch
//   0x2C10  single-shot low-rep, clock stretch
//   0x20xx..0x27xx  periodic measurement start
//   0xE000  fetch data (periodic mode)
//   0x30A2  soft reset
//   0x3066  heater on
//   0x3098  heater off
//   0xF32D  read status register
//   0x3780  read serial number - clock stretch
//   0x3682  read serial number - no clock stretch 
//
// Response (6 bytes): [T_MSB T_LSB T_CRC H_MSB H_LSB H_CRC]
//
// Conversion (datasheet eq. 1 & 2):
//   T(°C) = -45 + 175 * raw_T / 65535
//   RH(%) = 100 * raw_H / 65535
//   Inversion:
//     raw_T = (T + 45) * 65535 / 175
//     raw_H = RH * 65535 / 100
// ===================================================================

static void sht3x_init(sim_ctx_t *ctx) {
    SoftI2C_Sim_SetValue(ctx, SIM_Q_TEMPERATURE, 220, 200, 250, 3);
    SoftI2C_Sim_SetValue(ctx, SIM_Q_HUMIDITY,    500, 400, 600, 5);
}

static void sht3x_build_meas(sim_ctx_t *ctx) {
    int32_t t10 = SoftI2C_Sim_NextValue(ctx, SIM_Q_TEMPERATURE);
    int32_t h10 = SoftI2C_Sim_NextValue(ctx, SIM_Q_HUMIDITY);
    uint16_t raw_t = (uint16_t)(((int32_t)(t10 + 450) * 65535) / 1750);
    uint16_t raw_h = (uint16_t)(((int32_t) h10        * 65535) / 1000);
    pack_th(ctx->resp, raw_t, raw_h);
    ctx->resp_len = 6;
    printf("[SIM][SHT3x] T=%d.%d C  H=%d.%d%%  raw_T=0x%04X raw_H=0x%04X\n",
           t10/10, abs(t10%10), h10/10, h10%10, raw_t, raw_h);
}

static bool sht3x_encode(sim_ctx_t *ctx) {
    if (ctx->cmd_len < 1) return false;
    uint8_t c0 = ctx->cmd[0], c1 = (ctx->cmd_len >= 2) ? ctx->cmd[1] : 0;

    // Single-shot (all repeatability / clock-stretch variants)
    if (c0 == 0x24 || c0 == 0x2C) { sht3x_build_meas(ctx); return true; }
    // Periodic fetch
    if (c0 == 0xE0 && c1 == 0x00)  { sht3x_build_meas(ctx); return true; }
    // Serial number → 6 bytes (two CRC-valid fake words)
    if ((c0 == 0x36 && c1 == 0x82) || c0 == 0x37 && c1 == 0x80) {
        pack_word(ctx->resp, 0xDEAD); pack_word(ctx->resp+3, 0xBEEF);
        ctx->resp_len = 6; return true;
    }
    // Status register → 2 bytes + CRC (0x0000 = no alerts)
    if (c0 == 0xF3) {
        pack_word(ctx->resp, 0x0000); ctx->resp_len = 3; return true;
    }
    // All other commands (reset, heater, break, periodic start) → ACK only
    ctx->resp_len = 0;
    return true;
}

static const sim_sensor_ops_t g_sht3x_ops = {
    .name = "SHT3x", .init = sht3x_init, .encode_response = sht3x_encode,
};

// ===================================================================
// SHT4x plugin
// ===================================================================
// Reference: Sensirion Datasheet SHT4x v6.5 (Apr 2024)
//
// I2C address: 0x44 (SHT40) or 0x45 (SHT41/42)
//
// Commands (8-bit, unlike SHT3x's 16-bit):
//   0xFD  measure T+RH, high precision
//   0xF6  measure T+RH, medium precision
//   0xE0  measure T+RH, low precision
//   0x39/0x32/0x2F/0x24/0x1E/0x15  heater variants + measure
//   0x89  read serial number
//   0x94  soft reset
//
// Response (6 bytes): [T_MSB T_LSB T_CRC H_MSB H_LSB H_CRC]
//
// Conversion (datasheet eq. 1 & 2):
//   T(°C) = -45 + 175 * raw_T / 65535   ← identical to SHT3x
//   RH(%) =  -6 + 125 * raw_H / 65535   ← different offset/scale from SHT3x
//   Inversion:
//     raw_T = (T + 45) * 65535 / 175
//     raw_H = (RH + 6) * 65535 / 125
// ===================================================================

static void sht4x_init(sim_ctx_t *ctx) {
    SoftI2C_Sim_SetValue(ctx, SIM_Q_TEMPERATURE, 220, 200, 250, 3);
    SoftI2C_Sim_SetValue(ctx, SIM_Q_HUMIDITY,    500, 400, 600, 5);
}

static bool sht4x_encode(sim_ctx_t *ctx) {
    if (ctx->cmd_len < 1) return false;
    uint8_t c0 = ctx->cmd[0];

    // All measurement commands (all precision levels + heater variants)
    if (c0==0xFD||c0==0xF6||c0==0xE0||
        c0==0x39||c0==0x32||c0==0x2F||c0==0x24||c0==0x1E||c0==0x15) {
        int32_t t10 = SoftI2C_Sim_NextValue(ctx, SIM_Q_TEMPERATURE);
        int32_t h10 = SoftI2C_Sim_NextValue(ctx, SIM_Q_HUMIDITY);
        uint16_t raw_t = (uint16_t)(((int32_t)(t10 + 450) * 65535) / 1750);
        uint16_t raw_h = (uint16_t)(((int32_t)(h10 +  60) * 65535) / 1250);
        pack_th(ctx->resp, raw_t, raw_h);
        ctx->resp_len = 6;
        printf("[SIM][SHT4x] T=%d.%d C  H=%d.%d%%  raw_T=0x%04X raw_H=0x%04X\n",
               t10/10, abs(t10%10), h10/10, h10%10, raw_t, raw_h);
        return true;
    }
    // Serial number → 6 bytes
    if (c0 == 0x89) {
        pack_word(ctx->resp, 0xDEAD); pack_word(ctx->resp+3, 0xBEEF);
        ctx->resp_len = 6; return true;
    }
    // Soft reset and any unknown → ACK only
    ctx->resp_len = 0;
    return true;
}

static const sim_sensor_ops_t g_sht4x_ops = {
    .name = "SHT4x", .init = sht4x_init, .encode_response = sht4x_encode,
};

// ===================================================================
// AHT2x plugin  (AHT20 / AHT21)
// ===================================================================
// Reference: Aosong AHT20 Datasheet A0 (2020), AHT21 Datasheet A1 (2020)
//
// I2C address: 0x38 (fixed)
//
// Commands:
//   0x71              status poll (standalone read, no preceding write needed)
//   0xBE 0x08 0x00    initialise / calibrate
//   0xAC 0x33 0x00    trigger measurement
//   0xBA              soft reset
//
// Status byte:
//   bit7 = busy (0=ready, 1=converting)
//   bit3 = calibrated (1=ok)
//
// Measurement response (6 bytes, no CRC in standard mode):
//   [0] = status byte
//   [1] = H[19:12]
//   [2] = H[11:4]
//   [3] = H[3:0] | T[19:16]
//   [4] = T[15:8]
//   [5] = T[7:0]
//
// Conversion (datasheet section 5.4):
//   RH(%) = raw_H / 2^20 * 100     raw_H = RH / 100 * 2^20
//   T(°C) = raw_T / 2^20 * 200 - 50   raw_T = (T+50) / 200 * 2^20
// ===================================================================

typedef struct { bool calibrated; } aht2x_state_t;

static void aht2x_init(sim_ctx_t *ctx) {
    SoftI2C_Sim_SetValue(ctx, SIM_Q_TEMPERATURE, 220, 150, 350, 3);
    SoftI2C_Sim_SetValue(ctx, SIM_Q_HUMIDITY,    500, 200, 900, 5);
    aht2x_state_t *s = (aht2x_state_t *)malloc(sizeof(aht2x_state_t));
    s->calibrated = false;
    ctx->user = s;
}

static bool aht2x_encode(sim_ctx_t *ctx) {
    aht2x_state_t *s = (aht2x_state_t *)ctx->user;
    if (!s) return false;

    // Bare read (cmd_len==0): status poll via 0x71 or post-init poll
    if (ctx->cmd_len == 0) {
        ctx->resp[0] = s->calibrated ? 0x08 : 0x00;
        ctx->resp_len = 1;
        return true;
    }

    switch (ctx->cmd[0]) {
    case 0x71: // Status
        if (s->calibrated) ctx->resp[0] = 0x18;
        else ctx->resp[0] = 0x00; ctx->resp_len = 1;
        return true;

    case 0xBA: // soft reset → uncalibrated
        s->calibrated = false;
        ctx->resp[0] = 0x00; ctx->resp_len = 1;
        return true;

    case 0xBE: // initialise → calibrated; pre-load status for immediate poll
        s->calibrated = true;
        ctx->resp[0] = 0x08; ctx->resp_len = 1;
        return true;

    case 0xAC: { // trigger measurement → 6-byte result
        int32_t  t10   = SoftI2C_Sim_NextValue(ctx, SIM_Q_TEMPERATURE);
        int32_t  h10   = SoftI2C_Sim_NextValue(ctx, SIM_Q_HUMIDITY);
        uint32_t raw_h = (uint32_t)((int64_t) h10         * (1<<20) / 1000);
        uint32_t raw_t = (uint32_t)((int64_t)(t10 + 500)  * (1<<20) / 2000);
        if (raw_h > 0xFFFFF) raw_h = 0xFFFFF;
        if (raw_t > 0xFFFFF) raw_t = 0xFFFFF;
        ctx->resp[0] = s->calibrated ? 0x08 : 0x00;
        ctx->resp[1] = (uint8_t)((raw_h >> 12) & 0xFF);
        ctx->resp[2] = (uint8_t)((raw_h >>  4) & 0xFF);
        ctx->resp[3] = (uint8_t)(((raw_h & 0x0F) << 4) | ((raw_t >> 16) & 0x0F));
        ctx->resp[4] = (uint8_t)((raw_t >>  8) & 0xFF);
        ctx->resp[5] = (uint8_t)( raw_t        & 0xFF);
        ctx->resp_len = 6;

        // Calculate CRC over the 6 measurement bytes
        uint8_t crc = crc8(&ctx->resp[0], 6);
        ctx->resp[6] = crc;
        ctx->resp_len = 7;  // 6 data bytes + 1 CRC byte
        
        printf("[SIM][AHT2x] T=%d.%d C  H=%d.%d%%  raw_T=0x%05X raw_H=0x%05X  CRC=0x%02X\n",
               t10/10, abs(t10%10), h10/10, h10%10, raw_t, raw_h, crc);
/*
        printf("[SIM][AHT2x] T=%d.%d C  H=%d.%d%%  raw_T=0x%05X raw_H=0x%05X\n",
               t10/10, abs(t10%10), h10/10, h10%10, raw_t, raw_h);
*/
        return true;
    }
    default:
        ctx->resp_len = 0;
        return true;
    }
}

static const sim_sensor_ops_t g_aht2x_ops = {
    .name = "AHT2x", .init = aht2x_init, .encode_response = aht2x_encode,
};

// ===================================================================
// BMP280 / BME280 plugin
// ===================================================================
// Reference: Bosch BST-BMP280-DS001 v1.14 (2015)
//            Bosch BST-BME280-DS002 (2018)
//
// I2C address: 0x76 (SDO=GND) or 0x77 (SDO=VDD)
//
// Key registers:
//   0xD0        chip_id   (BMP280=0x58, BME280=0x60)
//   0xE0        reset     (write 0xB6)
//   0xF2        ctrl_hum  (BME280 only, must be written before ctrl_meas)
//   0xF3        status    (bit3=measuring, bit0=im_update)
//   0xF4        ctrl_meas (osrs_t[7:5], osrs_p[4:2], mode[1:0])
//   0xF5        config
//   0x88..0x9F  calibration T1-T3, P1-P9 (24 bytes LE, burst-readable)
//   0xA1        dig_H1 (BME280, 1 byte)
//   0xE1..0xE7  dig_H2..H6 (BME280, 7 bytes LE, burst-readable)
//
// Data registers (0xF7 auto-increments):
//   0xF7 0xF8 0xF9  press_msb/lsb/xlsb → adc_P = [0]<<12|[1]<<4|[2]>>4
//   0xFA 0xFB 0xFC  temp_msb/lsb/xlsb  → adc_T = [3]<<12|[4]<<4|[5]>>4
//   0xFD 0xFE       hum_msb/lsb        → adc_H = [6]<<8|[7]  (BME280 only)
//
// We use fixed calibration constants from a real BMP280 eval board.
// ===================================================================

#define BMP_DIG_T1  27504u
#define BMP_DIG_T2  26435
#define BMP_DIG_T3  -1000
#define BMP_DIG_P1  36477u
#define BMP_DIG_P2  -10685
#define BMP_DIG_P3   3024
#define BMP_DIG_P4   2855
#define BMP_DIG_P5    140
#define BMP_DIG_P6     -7
#define BMP_DIG_P7  15500
#define BMP_DIG_P8 -14600
#define BMP_DIG_P9   6000

typedef struct { uint8_t reg; bool is_bme280; } bmp280_state_t;

// Invert BMP280 temperature compensation (linear approx. of the Bosch formula)
static uint32_t bmp280_temp_to_adc(int32_t t10) {
    int32_t t_fine = (t10 * 10 * 320) / 5;
    int32_t adc_T  = ((t_fine * 2048 / BMP_DIG_T2) + (int32_t)BMP_DIG_T1 * 2) * 8;
    if (adc_T < 0)       adc_T = 0;
    if (adc_T > 0xFFFFF) adc_T = 0xFFFFF;
    return (uint32_t)adc_T;
}

// Empirical linear pressure inversion (calibrated at 1013 hPa / 25°C)
static uint32_t bmp280_press_to_adc(int32_t p10) {
    int32_t adc_P = 415000 + (p10 - 10132) * 38;
    if (adc_P < 0)       adc_P = 0;
    if (adc_P > 0xFFFFF) adc_P = 0xFFFFF;
    return (uint32_t)adc_P;
}

// Produce 24-byte calibration block (0x88..0x9F), little-endian
static void bmp280_pack_calib(uint8_t *buf) {
    static const int16_t c[] = {
        (int16_t)BMP_DIG_T1, BMP_DIG_T2, BMP_DIG_T3,
        (int16_t)BMP_DIG_P1, BMP_DIG_P2, BMP_DIG_P3, BMP_DIG_P4,
        BMP_DIG_P5, BMP_DIG_P6, BMP_DIG_P7, BMP_DIG_P8, BMP_DIG_P9,
    };
    for (int i = 0; i < 12; i++) {
        buf[i*2]   = (uint8_t)((uint16_t)c[i] & 0xFF);
        buf[i*2+1] = (uint8_t)((uint16_t)c[i] >> 8);
    }
}

static void bmp280_init(sim_ctx_t *ctx) {
    SoftI2C_Sim_SetValue(ctx, SIM_Q_TEMPERATURE,   250, 180, 450,  3);
    SoftI2C_Sim_SetValue(ctx, SIM_Q_PRESSURE,    10132, 9800, 10400, 5);
    SoftI2C_Sim_SetValue(ctx, SIM_Q_HUMIDITY,      500, 200,  900,  4);
    bmp280_state_t *s = (bmp280_state_t *)malloc(sizeof(bmp280_state_t));
    s->reg = 0; s->is_bme280 = false;
    ctx->user = s;
}

static bool bmp280_encode(sim_ctx_t *ctx) {
    bmp280_state_t *s = (bmp280_state_t *)ctx->user;
    if (!s || ctx->cmd_len < 1) return false;
    uint8_t reg = ctx->cmd[0];

    // Register write (reg + data bytes) → ACK only
    if (ctx->cmd_len >= 2) { s->reg = reg; ctx->resp_len = 0; return true; }
    s->reg = reg;

    // 0xD0 – chip_id
    if (reg == 0xD0) {
        ctx->resp[0] = s->is_bme280 ? 0x60 : 0x58;
        ctx->resp_len = 1; return true;
    }
    // 0x88..0x9F – calibration (burst or per-register; return all remaining bytes)
    if (reg >= 0x88 && reg <= 0x9F) {
        uint8_t calib[24]; bmp280_pack_calib(calib);
        uint8_t off = reg - 0x88;
        memcpy(ctx->resp, calib + off, 24 - off);
        ctx->resp_len = (uint8_t)(24 - off);
        return true;
    }
    // BME280 humidity calibration
    if (s->is_bme280) {
        // Humidity calibration chosen so the Bosch formula decodes trivially:
        //   adc_H = RH * 65536 / 100  (our encoding, see 0xF7/0xFD handlers)
        // With H1=0, H2=100, H3=H4=H5=H6=0 the compensation reduces to:
        //   RH% = adc_H * 100 / 65536
        if (reg == 0xA1) { ctx->resp[0] = 0x00; ctx->resp_len = 1; return true; } // dig_H1=0
        if (reg >= 0xE1 && reg <= 0xE7) {
            // E1,E2=dig_H2=100(0x0064 LE), E3=H3=0, E4=H4=0, E5=H5=0, E6=H5=0, E7=H6=0
            static const uint8_t hc[] = {0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            uint8_t off = reg - 0xE1;
            memcpy(ctx->resp, hc + off, 7 - off);
            ctx->resp_len = (uint8_t)(7 - off); return true;
        }
        if (reg == 0xF2) { ctx->resp[0] = 0x01; ctx->resp_len = 1; return true; }
    }
    if (reg == 0xF3) { ctx->resp[0] = 0x00; ctx->resp_len = 1; return true; } // status: ready
    if (reg == 0xF4) { ctx->resp[0] = 0x27; ctx->resp_len = 1; return true; } // ctrl_meas
    if (reg == 0xF5) { ctx->resp[0] = 0x00; ctx->resp_len = 1; return true; } // config

    // 0xF7 – data burst (press + temp [+ hum for BME280])
    // Datasheet: 0xF7 auto-increments through 0xFC (0xFE for BME280).
    // Return all bytes so both burst-read and split-read drivers work.
    if (reg == 0xF7) {
        int32_t  t10   = SoftI2C_Sim_NextValue(ctx, SIM_Q_TEMPERATURE);
        int32_t  p10   = SoftI2C_Sim_NextValue(ctx, SIM_Q_PRESSURE);
        uint32_t adc_P = bmp280_press_to_adc(p10);
        uint32_t adc_T = bmp280_temp_to_adc(t10);
        ctx->resp[0] = (uint8_t)((adc_P>>12)&0xFF);
        ctx->resp[1] = (uint8_t)((adc_P>> 4)&0xFF);
        ctx->resp[2] = (uint8_t)((adc_P<< 4)&0xF0);
        ctx->resp[3] = (uint8_t)((adc_T>>12)&0xFF);
        ctx->resp[4] = (uint8_t)((adc_T>> 4)&0xFF);
        ctx->resp[5] = (uint8_t)((adc_T<< 4)&0xF0);
        ctx->resp_len = 6;
        if (s->is_bme280) {
            int32_t  h10   = SoftI2C_Sim_NextValue(ctx, SIM_Q_HUMIDITY);
            uint32_t adc_H = (uint32_t)((int64_t)h10 * 65536 / 1000);
            if (adc_H > 0xFFFF) adc_H = 0xFFFF;
            ctx->resp[6] = (uint8_t)(adc_H>>8);
            ctx->resp[7] = (uint8_t)(adc_H&0xFF);
            ctx->resp_len = 8;
            printf("[SIM][BME280] T=%d.%d C  P=%d.%d hPa  H=%d.%d%%  adc_T=0x%05X adc_P=0x%05X adc_H=0x%04X\n",
                   t10/10, abs(t10%10), p10/10, abs(p10%10),
                   h10/10, abs(h10%10), adc_T, adc_P, adc_H);
        } else {
            printf("[SIM][BMP280] T=%d.%d C  P=%d.%d hPa  adc_T=0x%05X adc_P=0x%05X\n",
                   t10/10, abs(t10%10), p10/10, abs(p10%10), adc_T, adc_P);
        }
        return true;
    }
    // 0xFD – humidity registers (BME280 only, 2 bytes)
    // Some drivers issue a separate Write(0xFD)+Read(2) transaction for humidity
    // instead of relying on the 0xF7 burst. Peek: 0xF7 already advanced the cycle.
    if (reg == 0xFD && s->is_bme280) {
        int32_t  h10   = SoftI2C_Sim_PeekValue(ctx, SIM_Q_HUMIDITY);
        uint32_t adc_H = (uint32_t)((int64_t)h10 * 65536 / 1000);
        if (adc_H > 0xFFFF) adc_H = 0xFFFF;
        ctx->resp[0] = (uint8_t)(adc_H >> 8);
        ctx->resp[1] = (uint8_t)(adc_H & 0xFF);
        ctx->resp_len = 2;
        return true;
    }
    // 0xFA – temperature sub-address (drivers that split the burst at 0xFA)
    // Peek: 0xF7 already advanced the measurement cycle.
    if (reg == 0xFA) {
        int32_t  t10   = SoftI2C_Sim_PeekValue(ctx, SIM_Q_TEMPERATURE);
        uint32_t adc_T = bmp280_temp_to_adc(t10);
        ctx->resp[0] = (uint8_t)((adc_T>>12)&0xFF);
        ctx->resp[1] = (uint8_t)((adc_T>> 4)&0xFF);
        ctx->resp[2] = (uint8_t)((adc_T<< 4)&0xF0);
        ctx->resp_len = 3;
        if (s->is_bme280) {
            int32_t  h10   = SoftI2C_Sim_PeekValue(ctx, SIM_Q_HUMIDITY);
            uint32_t adc_H = (uint32_t)((int64_t)h10 * 65536 / 1000);
            if (adc_H > 0xFFFF) adc_H = 0xFFFF;
            ctx->resp[3] = (uint8_t)(adc_H>>8);
            ctx->resp[4] = (uint8_t)(adc_H&0xFF);
            ctx->resp_len = 5;
        }
        return true;
    }

    // Unknown register → 0x00
    ctx->resp[0] = 0x00; ctx->resp_len = 1;
    return true;
}

static const sim_sensor_ops_t g_bmp280_ops = {
    .name = "BMP280", .init = bmp280_init, .encode_response = bmp280_encode,
};

// ===================================================================
// CHT83xx plugin  (CHT8305, CHT8310, CHT8315)
// ===================================================================
// Reference: Sensylink CHT8305 / CHT8310 / CHT8315 Datasheet
//
// I2C address: 0x40 (default, ADDR pin selectable)
//
// Register map (all 2 bytes wide):
//   0x00  T[1:0] + H[1:0]  read-only, 4 bytes [T_MSB T_LSB H_MSB H_LSB]
//   0x01  H[1:0] only      read-only, 2 bytes (CHT831X)
//   0x02  Status register
//   0x03  Configuration
//   0x04  Conversion rate
//   0x05  Temperature high alert
//   0x06  Temperature low alert
//   0x07  Humidity high alert
//   0x08  Humidity low alert
//   0x0F  One-shot trigger (CHT831X, write-only)
//   0xFE  Manufacturer ID (2 bytes)
//   0xFF  Sensor/chip ID  (0x0000=CHT8305, 0x8215=CHT8310, 0x8315=CHT8315)
//
// CHT8305 encoding:
//   T(°C) = raw_T * 165 / 65535 - 40    → raw_T = (T+40) * 65535 / 165
//   RH(%) = raw_H * 100 / 65535          → raw_H = RH * 65535 / 100
//
// CHT831X (8310/8315) encoding:
//   T(°C) = int16(raw_T) / 32 * 0.03125 → raw_T = int16(T/0.03125) << 3
//   RH(%) = (raw_H & 0x7FFF)/32768*100  → raw_H = RH*32768/100, parity=bit15
// ===================================================================

typedef struct {
    uint8_t  reg;
    uint16_t sensor_id;  // 0x0000=CHT8305, 0x8215=CHT8310, 0x8315=CHT8315
} cht83xx_state_t;

static void cht83xx_init(sim_ctx_t *ctx) {
    SoftI2C_Sim_SetValue(ctx, SIM_Q_TEMPERATURE, 220, 150, 400, 3);
    SoftI2C_Sim_SetValue(ctx, SIM_Q_HUMIDITY,    500, 200, 900, 5);
    cht83xx_state_t *s = (cht83xx_state_t *)malloc(sizeof(cht83xx_state_t));
    s->reg       = 0x00;
    s->sensor_id = 0x0000;  // CHT8305 default; set 0x8215/0x8315 for CHT831X
    ctx->user    = s;
}

static uint16_t cht831x_hum_raw(int32_t h10) {
    uint16_t h15 = (uint16_t)((int64_t)h10 * 32768 / 1000);
    uint8_t par = 0;
    for (int b = 0; b < 15; b++) par ^= (h15 >> b) & 1;
    return (uint16_t)((h15 & 0x7FFF) | (par ? 0x8000u : 0));
}

static bool cht83xx_encode(sim_ctx_t *ctx) {
    cht83xx_state_t *s = (cht83xx_state_t *)ctx->user;
    if (!s) return false;

    // Update register pointer if a write was received
    if (ctx->cmd_len > 0) {
        s->reg = ctx->cmd[0];
        // Multi-byte write (config, alert limit, etc.) → ACK only
        if (ctx->cmd_len > 1) { ctx->resp_len = 0; return true; }
    }

    bool is_831x = (s->sensor_id == 0x8215 || s->sensor_id == 0x8315);
    int32_t t10  = SoftI2C_Sim_NextValue(ctx, SIM_Q_TEMPERATURE);
    int32_t h10  = SoftI2C_Sim_NextValue(ctx, SIM_Q_HUMIDITY);

    switch (s->reg) {
    case 0x00: { // T + H (4 bytes)
        uint16_t raw_t = is_831x
            ? (uint16_t)((int16_t)(t10 * 10000 / 3125) << 3)
            : (uint16_t)(((int32_t)(t10 + 400) * 65535) / 1650);
        uint16_t raw_h = is_831x
            ? cht831x_hum_raw(h10)
            : (uint16_t)(((int32_t)h10 * 65535) / 1000);
        ctx->resp[0]=(uint8_t)(raw_t>>8); ctx->resp[1]=(uint8_t)(raw_t&0xFF);
        ctx->resp[2]=(uint8_t)(raw_h>>8); ctx->resp[3]=(uint8_t)(raw_h&0xFF);
        ctx->resp_len = 4;
        printf("[SIM][CHT83xx] T=%d.%d C  H=%d.%d%%  raw_T=0x%04X raw_H=0x%04X\n",
               t10/10, abs(t10%10), h10/10, h10%10, raw_t, raw_h);
        return true;
    }
    case 0x01: { // H only (CHT831X; peek so 0x00+0x01 agree in same cycle)
        int32_t  hp    = SoftI2C_Sim_PeekValue(ctx, SIM_Q_HUMIDITY);
        uint16_t raw_h = is_831x ? cht831x_hum_raw(hp)
                                 : (uint16_t)(((int32_t)hp * 65535) / 1000);
        ctx->resp[0]=(uint8_t)(raw_h>>8); ctx->resp[1]=(uint8_t)(raw_h&0xFF);
        ctx->resp_len = 2; return true;
    }
    case 0x0F: ctx->resp_len = 0; return true;                // one-shot trigger (write-only)
    case 0xFE: ctx->resp[0]=0x54; ctx->resp[1]=0x53; ctx->resp_len=2; return true; // mfr ID "TS"
    case 0xFF: ctx->resp[0]=(uint8_t)(s->sensor_id>>8);       // chip/sensor ID
               ctx->resp[1]=(uint8_t)(s->sensor_id&0xFF);
               ctx->resp_len=2; return true;
    default:   ctx->resp[0]=0x00; ctx->resp[1]=0x00;           // status, config, limits → 0
               ctx->resp_len=2; return true;
    }
}

static const sim_sensor_ops_t g_cht83xx_ops = {
    .name = "CHT83xx", .init = cht83xx_init, .encode_response = cht83xx_encode,
};

// ===================================================================
// VEML7700 plugin
// ===================================================================
// Reference: Vishay VEML7700 Datasheet Rev. 1.8, 28-Nov-2024 (Doc 84286)
//
// I2C address: 7-bit = 0x10, 8-bit wire = 0x20 (write) / 0x21 (read)
//
// Protocol (datasheet Fig. 3 / Fig. 4):
//   Write: S | 0x20 | A | cmd | A | LSB | A | MSB | A | P
//   Read:  S | 0x20 | A | cmd | A | S | 0x21 | A | LSB | A | MSB | N | P
//   All 16-bit registers are transmitted LSB first.
//
// Register map (datasheet table p.7):
//   0x00  ALS_CONF  R/W  gain[12:11], IT[9:6], INT_EN[1], SD[0]
//   0x01  ALS_WH    R/W  high threshold
//   0x02  ALS_WL    R/W  low threshold
//   0x03  PSM       R/W  power saving mode
//   0x04  ALS       R    16-bit ALS output
//   0x05  WHITE     R    16-bit WHITE output
//   0x06  ALS_INT   R    interrupt status
//   0x07  ID        R    [15:8]=0xC4 (addr 0x20), [7:0]=0x81
//
// ALS_CONF default (POR) = 0x0001 → ALS_SD=1 (shutdown)
// Driver writes 0x0000 to power on with gain x1, IT 100ms.
//
// Lux conversion at default settings (gain x1, IT 100ms):
//   lux = raw_ALS * 0.0042 * 2 * 8 = raw_ALS * 0.0672
//   Inverse: raw_ALS = lux / 0.0672 = lux * 10000 / 672 = lux * 125 / 84
//
// SIM_Q_LIGHT is in x10 units: 3000 = 300.0 lux
// raw_ALS = lux10 * 125 / 84  (integer arithmetic, < 0.1% error)
// raw_WHITE ≈ raw_ALS * 11 / 10  (WHITE is ~10% higher than ALS)
// ===================================================================

typedef struct {
    uint8_t  reg;          // last written command/register address
    uint16_t conf;         // shadow of ALS_CONF (to echo back writes)
    uint16_t wh;           // high threshold shadow
    uint16_t wl;           // low threshold shadow
} veml7700_state_t;

static void veml7700_init(sim_ctx_t *ctx) {
    // Indoor office default: 300 lux, drifts between 50 and 5000 lux
    SoftI2C_Sim_SetValue(ctx, SIM_Q_LIGHT, 3000, 500, 50000, 200);
    veml7700_state_t *s = (veml7700_state_t *)malloc(sizeof(veml7700_state_t));
    s->reg  = 0x04;   // after power-on read, most drivers go straight for ALS
    s->conf = 0x0001; // POR default: shutdown
    s->wh   = 0xFFFF;
    s->wl   = 0x0000;
    ctx->user = s;
}

static bool veml7700_encode(sim_ctx_t *ctx) {
    veml7700_state_t *s = (veml7700_state_t *)ctx->user;
    if (!s) return false;

    // A write transaction sets the register pointer and optionally
    // writes data bytes (16-bit, LSB first).
    // cmd[0] = command code, cmd[1]=LSB, cmd[2]=MSB (if present)
    if (ctx->cmd_len >= 1) {
        s->reg = ctx->cmd[0];
        if (ctx->cmd_len >= 3) {
            // Data write: shadow the register
            uint16_t val = (uint16_t)((ctx->cmd[2] << 8) | ctx->cmd[1]);
            switch (s->reg) {
                case 0x00: s->conf = val; break;
                case 0x01: s->wh   = val; break;
                case 0x02: s->wl   = val; break;
                default: break;
            }
            ctx->resp_len = 0; // write-only transaction, no read data queued
            return true;
        }
    }

    // Now serve the read (the master will issue a repeated-start after setting reg)
    uint16_t val = 0;
    switch (s->reg) {
    case 0x00:  // ALS_CONF – echo back the shadow
        val = s->conf;
        break;
    case 0x01:  // ALS_WH
        val = s->wh;
        break;
    case 0x02:  // ALS_WL
        val = s->wl;
        break;
    case 0x04: { // ALS – advance light value, convert to raw counts
        int32_t lux10   = SoftI2C_Sim_NextValue(ctx, SIM_Q_LIGHT);
        uint32_t raw_als = (uint32_t)((int64_t)lux10 * 125 / 84);
        if (raw_als > 0xFFFF) raw_als = 0xFFFF;
        val = (uint16_t)raw_als;
        printf("[SIM][VEML7700] Lux=%d.%d  raw_ALS=0x%04X",
               (int)(lux10/10), (int)abs(lux10%10), val);
        break;
    }
    case 0x05: { // WHITE – peek same cycle, add ~10%
        int32_t lux10    = SoftI2C_Sim_PeekValue(ctx, SIM_Q_LIGHT);
        uint32_t raw_als = (uint32_t)((int64_t)lux10 * 125 / 84);
        uint32_t raw_w   = raw_als * 11 / 10;   // WHITE ~10% above ALS
        if (raw_w > 0xFFFF) raw_w = 0xFFFF;
        val = (uint16_t)raw_w;
        break;
    }
    case 0x06:  // ALS_INT – no interrupt pending
        val = 0x0000;
        break;
    case 0x07:  // ID – low byte 0x81 (fixed), high byte 0xC4 (addr 0x20)
        val = (uint16_t)((0xC4u << 8) | 0x81u);
        break;
    default:
        val = 0x0000;
        break;
    }

    // Return LSB first (datasheet Fig. 4)
    ctx->resp[0] = (uint8_t)(val & 0xFF);
    ctx->resp[1] = (uint8_t)(val >> 8);
    ctx->resp_len = 2;
    return true;
}

static const sim_sensor_ops_t g_veml7700_ops = {
    .name = "VEML7700", .init = veml7700_init, .encode_response = veml7700_encode,
};




commandResult_t CMD_SoftI2C_simAddSensor(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	
	
	uint8_t pin_data=9, pin_clk=17;
	pin_clk   = (uint8_t)Tokenizer_GetPinEqual("SCL", pin_clk);
	pin_data  = (uint8_t)Tokenizer_GetPinEqual("SDA", pin_data);
	const char *type = Tokenizer_GetArgEqualDefault("type","NO");
	uint8_t def_addr,addr;
	sim_sensor_ops_t *sens_ops;
	if (!strcmp(type,"NO")){
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, "No sensor type given!");
		return CMD_RES_BAD_ARGUMENT;
	}else {
        	if (wal_stricmp(type, "SHT3x") == 0) {
//			printf("Detected: SHT3x\n");
			sens_ops = &g_sht3x_ops;
			def_addr = 0x44 << 1;
		} else if (wal_stricmp(type, "SHT4x") == 0) {
//			printf("Detected: SHT4x\n");
			sens_ops = &g_sht4x_ops;
			def_addr = 0x44 << 1;
		} else if (wal_stricmp(type, "AHT2x") == 0) {
//			printf("Detected: AHT2x\n");
			sens_ops = &g_aht2x_ops;
			def_addr = 0x38 << 1;
		} else if (wal_stricmp(type, "CHT83xx") == 0 || wal_stricmp(type, "CHT8305") == 0 || wal_stricmp(type, "CHT8310") == 0 || wal_stricmp(type, "CHT8315") == 0) {
//			printf("Detected: CHT83xx\n");
			sens_ops = &g_cht83xx_ops;
			def_addr = 0x40 << 1;
		} else if (wal_stricmp(type, "BMP280") == 0 || wal_stricmp(type, "BME280") == 0) {
//			printf("Detected: BMP280\n");
			sens_ops = &g_bmp280_ops;
			def_addr = 0x76 << 1;		// to be dicussed, what is "default"
		} else if (wal_stricmp(type, "VEML7700") == 0 ) {
//			printf("Detected: VEML7700\n");
			sens_ops = &g_veml7700_ops;
			def_addr = 0x10 << 1;
		} else {
			ADDLOG_ERROR(LOG_FEATURE_SENSOR, "Unknown sensor type %s.",type);
			return CMD_RES_BAD_ARGUMENT;
		}
        }
        uint8_t A = (int8_t)(Tokenizer_GetArgEqualInteger("address", 0));
        if (A != 0){
        	addr = A << 1;
        } else {
        	ADDLOG_INFO(LOG_FEATURE_SENSOR, "No device address given, using default 0x%02X!",def_addr >> 1 );
        	addr = def_addr; 
        }
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "Adding %s sensor at address 0x%02X SDA=%i SCL=%i",type, addr >> 1, pin_data, pin_clk );
	int slot = SoftI2C_Sim_Register(pin_data, pin_clk, addr, sens_ops);
	if (slot >= 0){ 
		if (wal_stricmp(type, "BME280") == 0) {
        		((bmp280_state_t *)SoftI2C_Sim_GetCtx(slot)->user)->is_bme280 = true;
        	} else if (wal_stricmp(type, "CHT8305") == 0) {
		//Sensor/chip ID CHT83xx: 0x0000=CHT8305, 0x8215=CHT8310, 0x8315=CHT8315
			((cht83xx_state_t *)SoftI2C_Sim_GetCtx(slot)->user)->sensor_id = 0x0000;
        	} else if (wal_stricmp(type, "CHT8310") == 0) {
		//Sensor/chip ID CHT83xx: 0x0000=CHT8305, 0x8215=CHT8310, 0x8315=CHT8315
			((cht83xx_state_t *)SoftI2C_Sim_GetCtx(slot)->user)->sensor_id = 0x8215;
        	} else if (wal_stricmp(type, "CHT8315") == 0) {
		//Sensor/chip ID CHT83xx: 0x0000=CHT8305, 0x8215=CHT8310, 0x8315=CHT8315
			((cht83xx_state_t *)SoftI2C_Sim_GetCtx(slot)->user)->sensor_id = 0x8315;
        	}
        }
	
	return CMD_RES_OK;
}




// ===================================================================
// Convenience registration functions
// ===================================================================

int SoftI2C_Sim_AddSHT3x(uint8_t pin_data, uint8_t pin_clk, uint8_t addr) {
    return SoftI2C_Sim_Register(pin_data, pin_clk, addr, &g_sht3x_ops);
}
int SoftI2C_Sim_AddSHT4x(uint8_t pin_data, uint8_t pin_clk, uint8_t addr) {
    return SoftI2C_Sim_Register(pin_data, pin_clk, addr, &g_sht4x_ops);
}
int SoftI2C_Sim_AddAHT2x(uint8_t pin_data, uint8_t pin_clk) {
    return SoftI2C_Sim_Register(pin_data, pin_clk, 0x70, &g_aht2x_ops); // 0x38<<1
}
int SoftI2C_Sim_AddBMP280(uint8_t pin_data, uint8_t pin_clk, uint8_t addr) {
    return SoftI2C_Sim_Register(pin_data, pin_clk, addr, &g_bmp280_ops);
}
int SoftI2C_Sim_AddBME280(uint8_t pin_data, uint8_t pin_clk, uint8_t addr) {
    int slot = SoftI2C_Sim_Register(pin_data, pin_clk, addr, &g_bmp280_ops);
    if (slot >= 0) {
        bmp280_state_t *s = (bmp280_state_t *)SoftI2C_Sim_GetCtx(slot)->user;
        s->is_bme280 = true;
    }
    return slot;
}
int SoftI2C_Sim_AddCHT83xx(uint8_t pin_data, uint8_t pin_clk, uint8_t addr) {
    return SoftI2C_Sim_Register(pin_data, pin_clk, addr, &g_cht83xx_ops);
}

#endif // WIN32

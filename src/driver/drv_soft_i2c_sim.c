// drv_soft_i2c_sim.c
// -------------------------------------------------------
// Generic soft-I2C bus simulator – core layer.
//
// This file is entirely wrapped in #ifdef WIN32.
// It replaces the real hardware Soft_I2C_* functions on
// Windows and routes every transaction to a matching
// virtual device whose protocol is handled by plugin
// callbacks (sim_sensor_ops_t).
//
// This file contains NO sensor-specific code.
// See drv_soft_i2c_sim_sensors.c for the sensor plugins.
// -------------------------------------------------------
#ifdef WINDOWS

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "drv_soft_i2c_sim.h"

// Include the real softI2C_t definition so the stub
// signatures match what the sensor drivers expect.
#include "drv_local.h"

// -------------------------------------------------------
// Compile-time limits
// -------------------------------------------------------
#define SIM_MAX_DEVICES   16
#define SIM_DEFAULT_STEP  10   // default drift per read in x10 units




// Internally we use the shifted address in g_devices.
// So e.g.  for SHT3x/SHT4x with address 0x44 we store 0x44 << 1 ( = 0x88)
// but in displaying / logging we should use "known" 7 bit address (like 0x44 for SHT3x)
#define dispI2Cadress(S) 	S >> 1




// -------------------------------------------------------
// Internal device record
// -------------------------------------------------------
typedef struct {
    bool                    active;
    const sim_sensor_ops_t *ops;
    sim_ctx_t               ctx;
} sim_device_t;

static sim_device_t g_devices[SIM_MAX_DEVICES];
static bool         g_inited = false;

// Currently active device for the ongoing I2C transaction
static sim_device_t *g_cur = NULL;
static bool          g_cur_is_read = false;   // current Start was a read

// -------------------------------------------------------
// Minimal LCG – no dependency on rand() quality
// -------------------------------------------------------
static uint32_t g_rng;

static int32_t sim_rand(int32_t lo, int32_t hi) {
    g_rng = g_rng * 1664525u + 1013904223u;
    int32_t range = hi - lo + 1;
    if (range <= 0) return lo;
    return lo + (int32_t)(g_rng % (uint32_t)range);
}

// -------------------------------------------------------
// Device lookup by (pin_data, pin_clk, shifted 7-bit addr)
// -------------------------------------------------------
static sim_device_t *sim_find(uint8_t pin_data, uint8_t pin_clk, uint8_t addr) {
    for (int i = 0; i < SIM_MAX_DEVICES; i++) {
        if (!g_devices[i].active) continue;
        sim_ctx_t *c = &g_devices[i].ctx;
        if (c->pin_data == pin_data &&
            c->pin_clk  == pin_clk  &&
            c->i2c_addr == addr)
            return &g_devices[i];
    }
    return NULL;
}

// -------------------------------------------------------
// Value helpers  (public – used by plugins)
// -------------------------------------------------------

void SoftI2C_Sim_SetValue(sim_ctx_t *ctx, sim_quantity_t q,
                           int32_t initial, int32_t min, int32_t max,
                           int32_t step) {
    if (!ctx || q >= SIM_Q_COUNT) return;
    sim_value_t *v = &ctx->values[q];
    v->active  = true;
    v->current = initial;
    v->min     = min;
    v->max     = max;
    v->step    = step > 0 ? step : SIM_DEFAULT_STEP;
}

int32_t SoftI2C_Sim_NextValue(sim_ctx_t *ctx, sim_quantity_t q) {
    if (!ctx || q >= SIM_Q_COUNT) return 0;
    sim_value_t *v = &ctx->values[q];
    if (!v->active) return 0;
    v->current += sim_rand(-v->step, v->step);
    if (v->current < v->min) v->current = v->min;
    if (v->current > v->max) v->current = v->max;
    return v->current;
}

int32_t SoftI2C_Sim_PeekValue(sim_ctx_t *ctx, sim_quantity_t q) {
    if (!ctx || q >= SIM_Q_COUNT) return 0;
    return ctx->values[q].current;
}

void SoftI2C_Sim_ForceValue(sim_ctx_t *ctx, sim_quantity_t q, int32_t value) {
    if (!ctx || q >= SIM_Q_COUNT) return;
    ctx->values[q].current = value;
}

// -------------------------------------------------------
// Core API
// -------------------------------------------------------

void SoftI2C_Sim_Init(void) {
    if (g_inited) return;
    memset(g_devices, 0, sizeof(g_devices));
    g_rng    = (uint32_t)time(NULL);
    g_inited = true;
}

int SoftI2C_Sim_Register(uint8_t pin_data, uint8_t pin_clk,
                          uint8_t i2c_addr,
                          const sim_sensor_ops_t *ops) {
    SoftI2C_Sim_Init();
    if (!ops || !ops->encode_response) {
        printf("[SIM] Register: ops->encode_response is required\n");
        return -1;
    }
    for (int i = 0; i < SIM_MAX_DEVICES; i++) {
        if (!g_devices[i].active) {
            sim_device_t *d = &g_devices[i];
            memset(d, 0, sizeof(sim_device_t));
            d->active         = true;
            d->ops            = ops;
            d->ctx.pin_data   = pin_data;
            d->ctx.pin_clk    = pin_clk;
            d->ctx.i2c_addr   = i2c_addr & 0xFE; // strip R/W bit just in case
            if (ops->init) ops->init(&d->ctx);
            printf("[SIM] Registered '%s' addr=0x%02X pins(dat=%u,clk=%u) slot=%d\n",
                   ops->name ? ops->name : "?", dispI2Cadress(i2c_addr), pin_data, pin_clk, i);		// dispI2Cadress() display 7 bit adress --> >> 1
            return i;
        }
    }
    printf("[SIM] Register: table full (max %d devices)\n", SIM_MAX_DEVICES);
    return -1;
}

sim_ctx_t *SoftI2C_Sim_GetCtx(int slot) {
    if (slot < 0 || slot >= SIM_MAX_DEVICES) return NULL;
    if (!g_devices[slot].active) return NULL;
    return &g_devices[slot].ctx;
}


// -------------------------------------------------------
// Soft_I2C_* stubs – these replace the hardware versions
// on Windows.  Sensor drivers call these unchanged.
// -------------------------------------------------------

bool Soft_I2C_PreInit(softI2C_t *i2c) {
    SoftI2C_Sim_Init();
    
    printf("[SIM] PreInit pins dat=%u clk=%u\n", i2c->pin_data, i2c->pin_clk);
    return true;  // always report bus healthy
}

// Start a transaction.
// addr is the 8-bit wire address (7-bit device addr | R/W bit).
// Returns true (ACK) if a virtual device is found, false (NACK) otherwise.
// Internally we use the shifted address, e.g. 0x44 << 1 ( =0x88)
// but in displaying / logging use "known" 7 bit address (like 0x44 for SHT3x)
bool Soft_I2C_Start(softI2C_t *i2c, uint8_t addr) {
    SoftI2C_Sim_Init();
    bool    is_read  = (addr & 0x01) != 0;
    // Normalise to 8-bit wire address with R/W bit cleared, matching
    // how SoftI2C_Sim_Register stores it.
    uint8_t wire_addr = addr & 0xFE;

    g_cur = sim_find(i2c->pin_data, i2c->pin_clk, wire_addr);
    if (!g_cur) {
        printf("[SIM] no device: address=0x%02X pins(dat=%u,clk=%u)\n",
               dispI2Cadress(wire_addr), i2c->pin_data, i2c->pin_clk);
        return false;
    }
    // Found – only log on write-start (R/W=0) to avoid flooding on every read
    g_cur_is_read = is_read;
    if (!is_read) {
        printf("[SIM] found: address=0x%02X (%s) pins(dat=%u,clk=%u)\n",
               dispI2Cadress(wire_addr), g_cur->ops->name ? g_cur->ops->name : "?",
               i2c->pin_data, i2c->pin_clk);
    }

    if (is_read) {
        // Reading – response was prepared during the preceding write Stop.
        // Just reset the read cursor; do NOT clear resp_buf.
        g_cur->ctx.resp_pos = 0;
    } else {
        // Writing a new command – clear command + response buffers.
        g_cur->ctx.cmd_len  = 0;
        g_cur->ctx.resp_len = 0;
        g_cur->ctx.resp_pos = 0;
    }
    return true;  // ACK
}

/*
// Identical to Soft_I2C_Start but without writing an address byte.
// Used by BMP280 which calls Soft_I2C_Start_Internal() directly.
void Soft_I2C_Start_Internal(softI2C_t *i2c) {
    // For BMP280 the address is written as the first data byte,
    // so we can't resolve the device here.  We simply leave g_cur
    // as-is (the BMP280 shim handles it at the BMP280.h level).
    (void)i2c;
}
*/
// Used by drv_bmp280.c / BMP280.h instead of Soft_I2C_Start.
// On real hardware the caller clocks the address byte manually.
// In simulation the address is already in i2c->address8bit, so
// we resolve the device here exactly like a normal write Start.
void Soft_I2C_Start_Internal(softI2C_t *i2c) {
    SoftI2C_Sim_Init();
    uint8_t wire_addr = i2c->address8bit & 0xFE;
    g_cur_is_read = false;
    g_cur = sim_find(i2c->pin_data, i2c->pin_clk, wire_addr);
    if (!g_cur) {
        printf("[SIM] no device (Internal): wire=0x%02X (address=0x%02X) pins(dat=%u,clk=%u)\n",
               wire_addr, dispI2Cadress(wire_addr), i2c->pin_data, i2c->pin_clk);
        return;
    }
    printf("[SIM] found (Internal): wire=0x%02X (address=0x%02X) (%s) pins(dat=%u,clk=%u)\n",
           wire_addr, dispI2Cadress(wire_addr), g_cur->ops->name ? g_cur->ops->name : "?",
           i2c->pin_data, i2c->pin_clk);
    g_cur->ctx.cmd_len  = 0;
    g_cur->ctx.resp_len = 0;
    g_cur->ctx.resp_pos = 0;
}

bool Soft_I2C_WriteByte(softI2C_t *i2c, uint8_t value) {
    (void)i2c;
    if (!g_cur || g_cur_is_read) return false;
    sim_ctx_t *ctx = &g_cur->ctx;
    if (ctx->cmd_len < (uint8_t)sizeof(ctx->cmd)) {
        ctx->cmd[ctx->cmd_len++] = value;
    }
    return true;  // ACK
}

void Soft_I2C_Stop(softI2C_t *i2c) {
    (void)i2c;
    if (!g_cur) return;

    if (!g_cur_is_read) {
        // Write phase ended – ask the plugin to prepare a response.
        sim_ctx_t          *ctx = &g_cur->ctx;
        const sim_sensor_ops_t *ops = g_cur->ops;
        ctx->resp_len = 0;
        ctx->resp_pos = 0;
        bool ack = ops->encode_response(ctx);
        if (ack) {
            printf("[SIM] '%s' addr=0x%02X cmd=0x%02X (len=%u) -> %u byte response\n",
                   ops->name ? ops->name : "?",
                   dispI2Cadress(ctx->i2c_addr), ctx->cmd_len ? ctx->cmd[0] : 0xFF,
                   ctx->cmd_len, ctx->resp_len);
        } else {
            printf("[SIM] '%s' addr=0x%02X cmd=0x%02X NACK\n",
                   ops->name ? ops->name : "?",
                   dispI2Cadress(ctx->i2c_addr), ctx->cmd_len ? ctx->cmd[0] : 0xFF);
        }
    } else {
        // Read phase ended.
        if (g_cur->ops->on_read_complete)
            g_cur->ops->on_read_complete(&g_cur->ctx);
    }

    g_cur = NULL;
}

uint8_t Soft_I2C_ReadByte(softI2C_t *i2c, bool nack) {
    (void)i2c;
    (void)nack;
    if (!g_cur) return 0xFF;
    sim_ctx_t *ctx = &g_cur->ctx;
    if (ctx->resp_pos < ctx->resp_len)
        return ctx->resp[ctx->resp_pos++];
    return 0xFF;  // bus float – return high
}

void Soft_I2C_ReadBytes(softI2C_t *i2c, uint8_t *buf, int n) {
    for (int i = 0; i < n; i++) {
        bool nack = (i == n - 1);
        buf[i] = Soft_I2C_ReadByte(i2c, nack);
    }
}

// GPIO and timing stubs – not needed on Windows
void Soft_I2C_SetLow (uint8_t pin) { (void)pin; }
void Soft_I2C_SetHigh(uint8_t pin) { (void)pin; }
void usleep(int r)                  { (void)r;   }

#endif // WIN32

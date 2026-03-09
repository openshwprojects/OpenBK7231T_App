// drv_soft_i2c_sim.h
// -------------------------------------------------------
// Windows-only generic soft-I2C bus simulator.
//
// Architecture
// ------------
//   Core:    drv_soft_i2c_sim.c
//            Intercepts all Soft_I2C_* calls and routes them
//            to a matching virtual device.  The core has NO
//            knowledge of any sensor protocol.
//
//   Plugins: drv_soft_i2c_sim_sensors.c  (and any future file)
//            Each sensor family registers itself by filling a
//            sim_sensor_ops_t and calling SoftI2C_Sim_Register().
//            The only required callback is encode_response().
//
// Dynamic values
// --------------
//   All physical quantities are stored as int32_t in x10 units
//   (e.g. 224 = 22.4 C, 553 = 55.3 %RH, 10132 = 1013.2 hPa).
//   After every read the value drifts by a random +/-step clamped
//   to [min, max], producing natural-looking sensor data.
// -------------------------------------------------------
#pragma once
#ifdef WIN32

#include <stdint.h>
#include <stdbool.h>
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"



// -------------------------------------------------------
// Physical quantity IDs  (extend freely)
// -------------------------------------------------------
typedef enum {
    SIM_Q_TEMPERATURE = 0,   // x10  ->  224 = 22.4 C
    SIM_Q_HUMIDITY    = 1,   // x10  ->  553 = 55.3 %RH
    SIM_Q_PRESSURE    = 2,   // x10  -> 10132 = 1013.2 hPa
    SIM_Q_CO2         = 3,   // x1   ->  412 ppm
    SIM_Q_ALTITUDE    = 4,   // x10  -> 1234 = 123.4 m
    SIM_Q_COUNT               // sentinel – keep last
} sim_quantity_t;

// -------------------------------------------------------
// One dynamic physical quantity
// -------------------------------------------------------
typedef struct {
    bool     active;
    int32_t  current;    // current value (x10 units)
    int32_t  min;        // lower bound
    int32_t  max;        // upper bound
    int32_t  step;       // max random drift per read (0 = default 10)
} sim_value_t;

// -------------------------------------------------------
// Transaction context – one per registered virtual device.
// Passed to every callback so the plugin is self-contained.
// -------------------------------------------------------
typedef struct {
    uint8_t   pin_data;
    uint8_t   pin_clk;
    uint8_t   i2c_addr;       // 7-bit (R/W bit already stripped)

    // Command bytes collected during the current write phase
    uint8_t   cmd[8];
    uint8_t   cmd_len;

    // Response buffer – plugin fills this in encode_response()
    uint8_t   resp[32];
    uint8_t   resp_len;
    uint8_t   resp_pos;       // read cursor (managed by core)

    // Dynamic physical quantities
    sim_value_t values[SIM_Q_COUNT];

    // Arbitrary per-sensor state (plugin allocates and owns this)
    void     *user;
} sim_ctx_t;

// -------------------------------------------------------
// Sensor plugin interface
// The only mandatory callback is encode_response().
// -------------------------------------------------------
typedef struct {
    // Human-readable name printed in debug output, e.g. "AHT2x"
    const char *name;

    // Called once after SoftI2C_Sim_Register() so the plugin can
    // seed ctx->values[] with default ranges and allocate ctx->user.
    // May be NULL.
    void (*init)(sim_ctx_t *ctx);

    // Called every time the I2C master completes a write phase
    // (i.e. after the Stop that follows a write-addressed Start).
    // ctx->cmd[0..cmd_len-1] holds the bytes the master sent.
    // The plugin must fill ctx->resp[] and set ctx->resp_len.
    // Return false to signal NACK (no response).
    bool (*encode_response)(sim_ctx_t *ctx);

    // Called after a read cycle completes (optional).
    // Useful when a sensor needs a side-effect after being polled
    // (e.g. clearing a "data ready" flag).
    // May be NULL.
    void (*on_read_complete)(sim_ctx_t *ctx);
} sim_sensor_ops_t;

// -------------------------------------------------------
// Core API
// -------------------------------------------------------

// Initialise the simulator.  Idempotent – safe to call multiple times.
void SoftI2C_Sim_Init(void);

// Register a virtual device on a specific (pin_data, pin_clk, addr) tuple.
// ops must remain valid for the lifetime of the simulation.
// Returns a slot handle >= 0, or -1 on failure (table full).
int SoftI2C_Sim_Register(uint8_t pin_data, uint8_t pin_clk,
                          uint8_t i2c_addr,
                          const sim_sensor_ops_t *ops);

// Get a pointer to a registered context (for test-script tweaks).
// Returns NULL if the slot is invalid.
sim_ctx_t *SoftI2C_Sim_GetCtx(int slot);

// -------------------------------------------------------
// Value helpers – call from init() or encode_response()
// -------------------------------------------------------

// Define (or redefine) a dynamic quantity.  All values in x10 units.
// step = 0 uses the compiled-in default of 10 (= 1.0 in the physical unit).
void SoftI2C_Sim_SetValue(sim_ctx_t *ctx, sim_quantity_t q,
                           int32_t initial, int32_t min, int32_t max,
                           int32_t step);

// Advance the value by one random drift step and return the new value.
int32_t SoftI2C_Sim_NextValue(sim_ctx_t *ctx, sim_quantity_t q);

// Return the current value without advancing it.
int32_t SoftI2C_Sim_PeekValue(sim_ctx_t *ctx, sim_quantity_t q);

// Force-set the current value without changing the range (test injection).
void SoftI2C_Sim_ForceValue(sim_ctx_t *ctx, sim_quantity_t q, int32_t value);

// -------------------------------------------------------
// Pre-built sensor plugins  (implemented in drv_soft_i2c_sim_sensors.c)
//
// All "addr" parameters are 7-bit addresses (without the R/W bit).
// The drivers use 8-bit addresses (addr << 1), so for example:
//   SHT3x default 0x44 (8-bit: 0x88)  -> pass 0x44 here
//   AHT2x fixed   0x38 (8-bit: 0x70)  -> pass 0x38 here
//   BMP280        0x76 (8-bit: 0xEC)  -> pass 0x76 here
//   CHT83xx       0x40 (8-bit: 0x80)  -> pass 0x40 here
// -------------------------------------------------------
int SoftI2C_Sim_AddSHT3x (uint8_t pin_data, uint8_t pin_clk, uint8_t addr);
int SoftI2C_Sim_AddSHT4x (uint8_t pin_data, uint8_t pin_clk, uint8_t addr);
int SoftI2C_Sim_AddAHT2x (uint8_t pin_data, uint8_t pin_clk);
int SoftI2C_Sim_AddBMP280(uint8_t pin_data, uint8_t pin_clk, uint8_t addr);
int SoftI2C_Sim_AddCHT83xx(uint8_t pin_data, uint8_t pin_clk, uint8_t addr);

// defined in drv_soft_i2c_sim_sensors.c, initialized in drv_main.c
commandResult_t CMD_SoftI2C_simAddSensor(const void* context, const char* cmd, const char* args, int cmdFlags);
// -------------------------------------------------------
// Minimal usage example (WIN32 startup code):
//
//   SoftI2C_Sim_Init();
//
//   // SHT3x on SDA=17, SCL=9, address 0x44
//   int slot = SoftI2C_Sim_AddSHT3x(17, 9, 0x44);
//   // Narrow temperature range to a warm summer room:
//   SoftI2C_Sim_SetValue(SoftI2C_Sim_GetCtx(slot),
//                        SIM_Q_TEMPERATURE, 280, 250, 320, 3);
//
//   // AHT2x on same bus (different pin pair)
//   SoftI2C_Sim_AddAHT2x(4, 5);
//
//   // BMP280 with pressure simulation
//   SoftI2C_Sim_AddBMP280(17, 9, 0x76);
//
//   // Now just run the real driver init – no changes there:
//   SHTXX_StartDriver(...);
//   AHT2X_Init();
//   BMP280_Init();
// -------------------------------------------------------

#endif // WIN32

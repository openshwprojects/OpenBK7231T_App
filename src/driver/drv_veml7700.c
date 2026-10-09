// drv_veml7700.c
// VEML7700 High Accuracy Ambient Light Sensor driver
// Reference: Vishay VEML7700 Datasheet Rev. 1.8, 28-Nov-2024 (Doc 84286)
//
// Protocol (datasheet Fig. 9):
//   All registers are 16-bit, addressed by a 1-byte command code (0x00-0x07).
//   Write: S | addr_W | A | cmd | A | LSB | A | MSB | A | P
//   Read:  S | addr_W | A | cmd | A | S | addr_R | A | LSB | A | MSB | N | P
//   All 16-bit values are transmitted LSB first on the wire.
//
// Lux formula (datasheet base resolution 0.0042 lx/ct at gain x2, IT 800ms):
//   lux = raw_ALS * 0.0042 * gain_factor * it_factor
//   Gain factors: x1→×2,  x2→×1,  x1/8→×16,  x1/4→×8
//   IT factors:   800ms→×1, 400ms→×2, 200ms→×4, 100ms→×8, 50ms→×16, 25ms→×32
//   Default (gain x1, IT 100ms): lux = raw_ALS * 0.0672
//
// startDriver VEML7700 [SCL=pin] [SDA=pin] [chan_lux=ch] [chan_white=ch]
// chan_lux stores lux*10 (0.1 lux resolution)
// chan_white stores raw WHITE count


// My personal extensions, to tokenizer and PIN cfg gui not (yet?) merged
// can be enabled here


#define TOKENIZER_EXT		0
#define PINUSED_EXT		0


#include "../obk_config.h"

#if ENABLE_DRIVER_VEML7700

#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "drv_veml7700.h"

// Single sensor instance (VEML7700 has a fixed I2C address, so one per bus)
static veml7700_dev_t g_dev;

// -------------------------------------------------------
// Low-level I2C helpers
// -------------------------------------------------------

// Write a 16-bit register (LSB first on wire, datasheet Fig. 3)
static void VEML7700_WriteReg(veml7700_dev_t *dev, uint8_t cmd, uint16_t val)
{
    Soft_I2C_Start(&dev->i2c, VEML7700_I2C_ADDR);
    Soft_I2C_WriteByte(&dev->i2c, cmd);
    Soft_I2C_WriteByte(&dev->i2c, (uint8_t)(val & 0xFF));        // LSB first
    Soft_I2C_WriteByte(&dev->i2c, (uint8_t)((val >> 8) & 0xFF)); // then MSB
    Soft_I2C_Stop(&dev->i2c);
}

// Read a 16-bit register (LSB first on wire, datasheet Fig. 4)
static uint16_t VEML7700_ReadReg(veml7700_dev_t *dev, uint8_t cmd)
{
    uint8_t lo, hi;
    Soft_I2C_Start(&dev->i2c, VEML7700_I2C_ADDR);
    Soft_I2C_WriteByte(&dev->i2c, cmd);
    Soft_I2C_Stop(&dev->i2c);

    Soft_I2C_Start(&dev->i2c, VEML7700_I2C_ADDR | 1);
    lo = Soft_I2C_ReadByte(&dev->i2c, true);   // ACK – more bytes follow
    hi = Soft_I2C_ReadByte(&dev->i2c, false);  // NACK – last byte
    Soft_I2C_Stop(&dev->i2c);

    return ((uint16_t)hi << 8) | lo;
}

// -------------------------------------------------------
// Lux calculation
// Base resolution 0.0042 lx/ct at gain x2, IT 800ms.
// Other settings: multiply by gain_factor * it_factor.
// gain_mul10000[]: (0.0042 * gain_factor) * 10000, indexed by gain_sel [12:11]
//   gain_sel 00 (x1)   → gain_factor 2  → 0.0042*2  = 0.0084 → *10000 = 84
//   gain_sel 01 (x2)   → gain_factor 1  → 0.0042*1  = 0.0042 → *10000 = 42
//   gain_sel 10 (x1/8) → gain_factor 16 → 0.0042*16 = 0.0672 → *10000 = 672
//   gain_sel 11 (x1/4) → gain_factor 8  → 0.0042*8  = 0.0336 → *10000 = 336
// it_fac: relative to 800ms=×1; each halving of IT doubles the factor.
// -------------------------------------------------------
static float VEML7700_CalcLux(veml7700_dev_t *dev, uint16_t raw)
{
    static const uint16_t gain_mul10000[] = { 84, 42, 672, 336 };
    uint8_t gain_sel = (uint8_t)((dev->conf >> 11) & 0x03u);
    uint8_t it_sel   = (uint8_t)((dev->conf >>  6) & 0x0Fu);

    uint8_t it_fac;
    switch (it_sel) {
        case 0x00: it_fac =  8; break;  // 100 ms
        case 0x01: it_fac =  4; break;  // 200 ms
        case 0x02: it_fac =  2; break;  // 400 ms
        case 0x03: it_fac =  1; break;  // 800 ms
        case 0x08: it_fac = 16; break;  //  50 ms
        case 0x0C: it_fac = 32; break;  //  25 ms
        default:   it_fac =  8; break;  // unknown → treat as 100 ms
    }

    return (float)raw * (float)((uint32_t)gain_mul10000[gain_sel] * it_fac) / 10000.0f;
}

// -------------------------------------------------------
// Initialization – sets dev->isWorking (mirrors SHTXX_Initialization)
// -------------------------------------------------------
static void VEML7700_Initialization(veml7700_dev_t *dev)
{
    // Power on with gain x1, IT 100ms.
    // POR value is 0x0001 (ALS_SD=1, shutdown), so we must write 0x0000.
    dev->conf = VEML7700_GAIN_x1 | VEML7700_IT_100MS;  // = 0x0000
    VEML7700_WriteReg(dev, VEML7700_REG_ALS_CONF, dev->conf);
    rtos_delay_milliseconds(5);

    // Verify device ID – low byte must be 0x81 (datasheet Table 8)
    uint16_t id = VEML7700_ReadReg(dev, VEML7700_REG_ID);
    dev->isWorking = ((id & 0xFF) == VEML7700_DEVICE_ID_LO);

    ADDLOG_INFO(LOG_FEATURE_SENSOR,
                "VEML7700 (SDA=%i SCL=%i): Init %s (ID=0x%04X).",
                dev->i2c.pin_data, dev->i2c.pin_clk,
                dev->isWorking ? "ok" : "failed", id);
}

// -------------------------------------------------------
// Measure + store – mirrors SHTXX_ConvertAndStore
// -------------------------------------------------------
static void VEML7700_Measure(veml7700_dev_t *dev)
{
    dev->rawALS   = VEML7700_ReadReg(dev, VEML7700_REG_ALS);
    dev->rawWhite = VEML7700_ReadReg(dev, VEML7700_REG_WHITE);
    dev->lux      = VEML7700_CalcLux(dev, dev->rawALS);

    // Store lux*10 so 0.1 lux resolution is preserved in the channel integer
    if(dev->channel_lux   >= 0) CHANNEL_Set(dev->channel_lux,   (int)(dev->lux * 10.0f), 0);
    if(dev->channel_white >= 0) CHANNEL_Set(dev->channel_white,  dev->rawWhite,           0);

    ADDLOG_INFO(LOG_FEATURE_SENSOR,
                "VEML7700 (SDA=%i SCL=%i): Lux=%.2f ALS=%u WHITE=%u",
                dev->i2c.pin_data, dev->i2c.pin_clk,
                dev->lux, dev->rawALS, dev->rawWhite);
}

// -------------------------------------------------------
// Command handlers – all take (context, cmd, args, flags)
// context is always the veml7700_dev_t pointer (same as SHT pattern)
// -------------------------------------------------------

// VEML7700_ALS [gain] [it]
// Gain: 0=x1 1=x2 2=x1/8 3=x1/4
// IT:   0=100ms 1=200ms 2=400ms 3=800ms 8=50ms 12=25ms
//	VEML7700: ALS gain and integration time. Gain: 0=x1 1=x2 2=x1/8 3=x1/4. IT: 0=100ms 1=200ms 2=400ms 3=800ms 8=50ms 12=25ms
commandResult_t VEML7700_CMD_ALS(const void *context, const char *cmd,
                                  const char *args, int flags)
{
    veml7700_dev_t *dev = (veml7700_dev_t *)context;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

    uint8_t gain = (uint8_t)(Tokenizer_GetArgInteger(0) & 0x03);
    uint8_t it   = (uint8_t)(Tokenizer_GetArgInteger(1) & 0x0F);

    // Preserve INT_EN[1], SD[0] and ALS_PERS[5:4]; replace gain[12:11] and IT[9:6]
    dev->conf = (uint16_t)((dev->conf & 0xE83Fu) | ((uint16_t)gain << 11) | ((uint16_t)it << 6));
    VEML7700_WriteReg(dev, VEML7700_REG_ALS_CONF, dev->conf);
    return CMD_RES_OK;
}

// VEML7700_INT [enable] [als_low] [als_high] [persist]
// enable=0/1, als_low/als_high=raw thresholds (0-65535), persist=0..3 (1/2/4/8 samples)
//	VEML7700 Interrupt config. enable=0/1. als_low/als_high raw thresholds (0-65535). persist=0..3 (1/2/4/8 samples)
commandResult_t VEML7700_CMD_INT(const void *context, const char *cmd,
                                  const char *args, int flags)
{
    veml7700_dev_t *dev = (veml7700_dev_t *)context;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

    int en      = Tokenizer_GetArgInteger(0);
    int lo      = Tokenizer_GetArgInteger(1);
    int hi      = Tokenizer_GetArgInteger(2);
    int persist = Tokenizer_GetArgInteger(3) & 0x03;

    VEML7700_WriteReg(dev, VEML7700_REG_ALS_WL, (uint16_t)lo);
    VEML7700_WriteReg(dev, VEML7700_REG_ALS_WH, (uint16_t)hi);

    // ALS_PERS lives in ALS_CONF[5:4]; INT_EN in ALS_CONF[1]
    dev->conf &= ~(VEML7700_CONF_INT_EN | (uint16_t)(0x03u << 4));
    dev->conf |= (uint16_t)((uint16_t)(persist & 0x03) << 4);
    if(en) dev->conf |= VEML7700_CONF_INT_EN;
    VEML7700_WriteReg(dev, VEML7700_REG_ALS_CONF, dev->conf);
    return CMD_RES_OK;
}

// VEML7700_Cycle [seconds]
//	VEML7700 Measurement interval in seconds (min 1, max 255)
commandResult_t VEML7700_CMD_Cycle(const void *context, const char *cmd,
                                    const char *args, int flags)
{
    veml7700_dev_t *dev = (veml7700_dev_t *)context;
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    int s = Tokenizer_GetArgInteger(0);
    if(s < 1) { ADDLOG_INFO(LOG_FEATURE_CMD, "VEML7700: Min 1s."); return CMD_RES_BAD_ARGUMENT; }
    dev->secondsBetween = (uint8_t)s;
    return CMD_RES_OK;
}

// VEML7700_Measure – immediate on-demand read
//	VEML7700: Trigger an immediate ALS + WHITE measurement
commandResult_t VEML7700_CMD_Measure(const void *context, const char *cmd,
                                      const char *args, int flags)
{
    veml7700_dev_t *dev = (veml7700_dev_t *)context;
    dev->secondsUntilNext = dev->secondsBetween;
    VEML7700_Measure(dev);
    return CMD_RES_OK;
}

// VEML7700_Reinit – soft reinitialise
//	VEML7700: Re-run sensor initialisation (power-on and ID check)
commandResult_t VEML7700_CMD_Reinit(const void *context, const char *cmd,
                                     const char *args, int flags)
{
    veml7700_dev_t *dev = (veml7700_dev_t *)context;
    dev->secondsUntilNext = dev->secondsBetween;
    VEML7700_Initialization(dev);
    return CMD_RES_OK;
}

// -------------------------------------------------------
// Public entry points
// -------------------------------------------------------

// startDriver VEML7700 [SCL=pin] [SDA=pin] [chan_lux=ch] [chan_white=ch]
// Named-arg style is identical to SHTXX_Init().
void VEML7700_Init(void)
{
    veml7700_dev_t *dev = &g_dev;

    // Default pins 
    dev->i2c.pin_clk    = 9;
    dev->i2c.pin_data   = 14;
    dev->channel_lux    = -1;
    dev->channel_white  = -1;

/*
    // Role-based pin lookup (first sensor only, same guard as SHT)
    // - not implemented - no IORoles (yet)
    dev->i2c.pin_clk    = PIN_FindPinIndexForRole(IOR_VEML7700_CLK, dev->i2c.pin_clk);
    dev->i2c.pin_data   = PIN_FindPinIndexForRole(IOR_VEML7700_DAT, dev->i2c.pin_data);
    dev->channel_lux    = g_cfg.pins.channels [dev->i2c.pin_data];
    dev->channel_white  = g_cfg.pins.channels2[dev->i2c.pin_data];
*/

    dev->i2c.pin_clk  = Tokenizer_GetPin(1, dev->i2c.pin_clk);
    dev->i2c.pin_data = Tokenizer_GetPin(2, dev->i2c.pin_data);

    dev->channel_lux   = Tokenizer_GetArgIntegerDefault(3, dev->channel_lux);
    dev->channel_white = Tokenizer_GetArgIntegerDefault(4, dev->channel_white);

#if TOKENIZER_EXT
    // My personal extensions, not (yet?) merged 

    // Named-arg overrides (startDriver line arguments)
    // test, if extended arguments are present
    dev->i2c.pin_clk    = Tokenizer_GetPinEqual("SCL",         dev->i2c.pin_clk);
    dev->i2c.pin_data   = Tokenizer_GetPinEqual("SDA",         dev->i2c.pin_data);
    dev->channel_lux    = Tokenizer_GetArgEqualInteger("chan_lux",   dev->channel_lux);
    dev->channel_white  = Tokenizer_GetArgEqualInteger("chan_white", dev->channel_white);
#endif

#if PINUSED_EXT
    // My personal extensions, not (yet?) merged 
    setPinUsedString(dev->i2c.pin_clk, "VEML7700 SCL");
    setPinUsedString(dev->i2c.pin_data, "VEML7700 SDA");
#endif    


    dev->secondsBetween   = 10;
    dev->secondsUntilNext = 1;

    Soft_I2C_PreInit(&dev->i2c);
    rtos_delay_milliseconds(5);   // allow supply to settle after bus init

    VEML7700_Initialization(dev);

    //cmddetail:{"name":"VEML7700_ALS","args":"[gain] [it]",
    //cmddetail:"descr":"ALS gain and integration time. Gain: 0=x1 1=x2 2=x1/8 3=x1/4. IT: 0=100ms 1=200ms 2=400ms 3=800ms 8=50ms 12=25ms.",
    //cmddetail:"fn":"VEML7700_CMD_ALS","file":"driver/drv_veml7700.c","requires":"",
    //cmddetail:"examples":"VEML7700_ALS 1 3"}
    CMD_RegisterCommand("VEML7700_ALS",     VEML7700_CMD_ALS,     dev);

    //cmddetail:{"name":"VEML7700_INT","args":"[enable] [als_low] [als_high] [persist]",
    //cmddetail:"descr":"Interrupt config. enable=0/1. als_low/als_high raw thresholds. persist=0..3.",
    //cmddetail:"fn":"VEML7700_CMD_INT","file":"driver/drv_veml7700.c","requires":"",
    //cmddetail:"examples":"VEML7700_INT 1 500 8000 1"}
    CMD_RegisterCommand("VEML7700_INT",     VEML7700_CMD_INT,     dev);

    //cmddetail:{"name":"VEML7700_Cycle","args":"[IntervalSeconds]",
    //cmddetail:"descr":"Measurement interval in seconds (min 1).",
    //cmddetail:"fn":"VEML7700_CMD_Cycle","file":"driver/drv_veml7700.c","requires":"",
    //cmddetail:"examples":"VEML7700_Cycle 10"}
    CMD_RegisterCommand("VEML7700_Cycle",   VEML7700_CMD_Cycle,   dev);

    //cmddetail:{"name":"VEML7700_Measure","args":"",
    //cmddetail:"descr":"Trigger an immediate ALS + WHITE measurement.",
    //cmddetail:"fn":"VEML7700_CMD_Measure","file":"driver/drv_veml7700.c","requires":"",
    //cmddetail:"examples":"VEML7700_Measure"}
    CMD_RegisterCommand("VEML7700_Measure", VEML7700_CMD_Measure, dev);

    //cmddetail:{"name":"VEML7700_Reinit","args":"",
    //cmddetail:"descr":"Re-run sensor initialisation (power-on and ID check).",
    //cmddetail:"fn":"VEML7700_CMD_Reinit","file":"driver/drv_veml7700.c","requires":"",
    //cmddetail:"examples":"VEML7700_Reinit"}
    CMD_RegisterCommand("VEML7700_Reinit",  VEML7700_CMD_Reinit,  dev);
}

// Put sensor into shutdown on driver stop (mirrors SHTXX_StopDriver reset)
void VEML7700_StopDriver(void)
{
    veml7700_dev_t *dev = &g_dev;
    VEML7700_WriteReg(dev, VEML7700_REG_ALS_CONF, VEML7700_CONF_SD);
}

void VEML7700_OnEverySecond(void)
{
    veml7700_dev_t *dev = &g_dev;
    if(dev->secondsUntilNext == 0)
    {
        if(dev->isWorking)
            VEML7700_Measure(dev);
        dev->secondsUntilNext = dev->secondsBetween;
    }
    else
    {
        dev->secondsUntilNext--;
    }
}

void VEML7700_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
    if(bPreState) return;
    veml7700_dev_t *dev = &g_dev;
    if(!dev->isWorking)
        hprintf255(request, "<b>VEML7700 not detected</b>");
    else
        hprintf255(request, "<h2>VEML7700 Lux=%.2f WHITE=%u</h2>", dev->lux, dev->rawWhite);
}

#endif // ENABLE_DRIVER_VEML7700

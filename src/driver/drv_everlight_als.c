// Everlight EAALSDIC2020A2 / ALS-DPDIC17-78C/L749/TR8 ambient light sensor (soft I2C, addr 0x38)
//
// Register-based device: every access is [addr+W][reg] then data. The sensor
// is disabled after power-on and must be enabled through SYSM_CTRL.EN_ALS.
//
// Register addresses below marked (DS) come from the Everlight EAALSDIC2020A2
// datasheet. Items marked (MEASURED) were determined on a FH_PIR_400A device
// (2026-11-16) by comparing readings under a fixed light source:
//   - ALS data register is 0x1E/0x1F (0x1C/0x1D responds ~100x weaker)
//   - ALS_GAIN codes: 0x00/0x01 = x1, 0x02 = x4, 0x04 = x16, 0x08 = x64, 0x10 = x256
//   - 1.975 lux per count at x1 gain / 16-bit resolution, from comparison against a
//     phone light sensor at ~29, ~130 and ~380 lux (agreement within about 11%).
//     This includes the housing window of that device; other housings may need
//     a different scalePercent.

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_public.h"

#if ENABLE_DRIVER_EVERLIGHT_ALS

#define EVL_REG_SYSM_CTRL	0x00	// (DS) bit0 = EN_ALS, bit7 = SWRST
#define EVL_REG_INT_FLAG	0x02	// (DS)
#define EVL_REG_WAIT_TIME	0x03	// (DS)
#define EVL_REG_ALS_GAIN	0x04	// (DS) PGA_ALS, codes (MEASURED) see EVL_GainMultiplier
#define EVL_REG_ALS_TIME	0x05	// (DS) ALSCONV
#define EVL_REG_DATA_DEFAULT	0x1E	// (MEASURED) ALS data low byte, high byte at +1

#define EVL_EN_ALS		0x01
// readings dropped after the hold channel returns to zero
#define EVL_SKIP_AFTER_HOLD	2
#define EVL_DEFAULT_ADDR	0x38
#define EVL_DEFAULT_ALSCONV	0x3F	// full 16-bit range, ~171 ms conversion
#define EVL_DEFAULT_GAIN	0x00	// x1
// (MEASURED) lux per count at x1 gain, ALSCONV=0x3F, on a FH_PIR_400A
#define EVL_BASE_LUX_PER_COUNT	1.975f

static softI2C_t g_i2c;
static int g_targetChannel = 2;
static uint8_t g_i2cAddr = EVL_DEFAULT_ADDR;
static uint8_t g_gain = EVL_DEFAULT_GAIN;
static uint8_t g_alsConv = EVL_DEFAULT_ALSCONV;
static uint8_t g_dataReg = EVL_REG_DATA_DEFAULT;
static int g_scalePercent = 100;
static bool g_ready = false;
static uint8_t g_skipSamples = 0;
static bool g_saturated = false;
// while this channel is non-zero, readings are ignored (e.g. indicator LED shining into the sensor)
static int g_holdChannel = -1;
static bool g_held = false;
// 0 = channel holds whole lux (Illuminance), 1 = channel holds lux * 10 (Illuminance_div10)
static int g_decimals = 0;
// the channel is only updated when lux moved by at least max(g_thrAbsLux, g_thrPercent % of the last sent value)
static float g_thrPercent = 0.0f;
static float g_thrAbsLux = 0.0f;
static float g_sentLux = 0.0f;
static bool g_haveSent = false;
static uint16_t g_raw = 0;
static float g_lux = 0.0f;

static bool EVL_WriteReg(uint8_t reg, uint8_t val) {
	bool ok = Soft_I2C_Start(&g_i2c, (g_i2cAddr << 1) | 0);
	ok = ok && Soft_I2C_WriteByte(&g_i2c, reg);
	ok = ok && Soft_I2C_WriteByte(&g_i2c, val);
	Soft_I2C_Stop(&g_i2c);
	return ok;
}

// Same stop/start sequence as drv_ltr_als.c (Soft_I2C_Start cannot issue a repeated start)
static bool EVL_ReadReg(uint8_t reg, uint8_t *out) {
	bool ok = Soft_I2C_Start(&g_i2c, (g_i2cAddr << 1) | 0);
	ok = ok && Soft_I2C_WriteByte(&g_i2c, reg);
	Soft_I2C_Stop(&g_i2c);
	if (!ok) {
		return false;
	}
	if (!Soft_I2C_Start(&g_i2c, (g_i2cAddr << 1) | 1)) {
		Soft_I2C_Stop(&g_i2c);
		return false;
	}
	*out = Soft_I2C_ReadByte(&g_i2c, true);
	Soft_I2C_Stop(&g_i2c);
	return true;
}

// (MEASURED) gain multiplier for a given ALS_GAIN register value, 0 = unknown code
static int EVL_GainMultiplier(uint8_t gainReg) {
	switch (gainReg) {
	case 0x00:
	case 0x01: return 1;
	case 0x02: return 4;
	case 0x04: return 16;
	case 0x08: return 64;
	case 0x10: return 256;
	default: return 0;
	}
}

static bool EVL_Setup() {
	uint8_t ctrl = 0, gain = 0, conv = 0;

	g_ready = false;
	// disable while configuring
	if (!EVL_WriteReg(EVL_REG_SYSM_CTRL, 0x00)) {
		ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: no ACK at 0x%02X (SCL=%d, SDA=%d)",
			g_i2cAddr, g_i2c.pin_clk, g_i2c.pin_data);
		return false;
	}
	EVL_WriteReg(EVL_REG_ALS_GAIN, g_gain);
	EVL_WriteReg(EVL_REG_ALS_TIME, g_alsConv);
	EVL_WriteReg(EVL_REG_SYSM_CTRL, EVL_EN_ALS);

	// read back, so a wrong chip / wrong register model is visible in the log
	if (!EVL_ReadReg(EVL_REG_SYSM_CTRL, &ctrl) || !EVL_ReadReg(EVL_REG_ALS_GAIN, &gain)
		|| !EVL_ReadReg(EVL_REG_ALS_TIME, &conv)) {
		ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: readback failed");
		return false;
	}
	if (!(ctrl & EVL_EN_ALS) || gain != g_gain || conv != g_alsConv) {
		ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: readback mismatch CTRL=0x%02X GAIN=0x%02X TIME=0x%02X",
			ctrl, gain, conv);
		return false;
	}
	// the data register still holds the result of the previous configuration
	g_skipSamples = 1;
	g_saturated = false;
	g_ready = true;
	return true;
}

// EverlightALS_Hold [channel]
static commandResult_t EVL_Cmd_Hold(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_holdChannel = Tokenizer_GetArgInteger(0);
	g_held = false;
	return CMD_RES_OK;
}

static void EVL_ApplyChannelType() {
	if (g_targetChannel >= 0) {
		CHANNEL_SetType(g_targetChannel, g_decimals ? ChType_Illuminance_div10 : ChType_Illuminance);
	}
	// force the next reading out in the new unit
	g_haveSent = false;
}

// EverlightALS_Decimals [0 or 1]
static commandResult_t EVL_Cmd_Decimals(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_decimals = Tokenizer_GetArgInteger(0) ? 1 : 0;
	EVL_ApplyChannelType();
	return CMD_RES_OK;
}

// EverlightALS_Threshold [percent] [absoluteLux]
static commandResult_t EVL_Cmd_Threshold(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_thrPercent = Tokenizer_GetArgFloat(0);
	g_thrAbsLux = Tokenizer_GetArgFloatDefault(1, 0.0f);
	if (g_thrPercent < 0.0f) {
		g_thrPercent = 0.0f;
	}
	if (g_thrAbsLux < 0.0f) {
		g_thrAbsLux = 0.0f;
	}
	return CMD_RES_OK;
}

// EverlightALS_Dump - prints registers 0x00..0x1F to the log
static commandResult_t EVL_Cmd_Dump(const void *context, const char *cmd, const char *args, int flags) {
	int row, i;
	for (row = 0; row < 0x20; row += 8) {
		uint8_t v[8];
		for (i = 0; i < 8; i++) {
			v[i] = 0;
			if (!EVL_ReadReg((uint8_t)(row + i), &v[i])) {
				ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: read of reg 0x%02X failed", row + i);
				return CMD_RES_ERROR;
			}
		}
		ADDLOG_INFO(LOG_FEATURE_DRV, "EverlightALS 0x%02X: %02X %02X %02X %02X %02X %02X %02X %02X",
			row, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
	}
	return CMD_RES_OK;
}

// Usage:
//   startDriver EverlightALS [targetChannel] [sclPin] [sdaPin] [i2cAddr] [scalePercent] [gain] [resolution] [dataReg] [holdChannel]
//   - targetChannel: channel receiving lux as integer (default 2), or lux x 10
//                    after EverlightALS_Decimals 1
//   - sclPin/sdaPin: default = pins with SoftSCL/SoftSDA roles, else P6/P7
//   - i2cAddr:       7-bit chip address (default 0x38)
//   - scalePercent:  calibration applied to the computed lux (default 100)
//   - gain:          value for ALS_GAIN register 0x04 (default 0x00):
//                    0x00 = x1, 0x02 = x4, 0x04 = x16, 0x08 = x64, 0x10 = x256.
//                    Lux is corrected for the gain; higher gain = finer steps, lower maximum.
//   - resolution:    raw value for ALS_TIME register 0x05 / ALSCONV (default 0x3F = 16 bit,
//                    max count = 1024 * (value + 1) - 1, lower = faster and coarser)
//   - dataReg:       address of ALS data low byte (default 0x1E, high byte at +1)
//   - holdChannel:   while this channel is non-zero the lux value is frozen (default -1 = off).
//                    Use it when the device's own LED lights the sensor. Can also be set
//                    later with: EverlightALS_Hold [channel]
// Related commands (run after startDriver, e.g. in autoexec.bat):
//   EverlightALS_Decimals 1        - publish tenths of a lux (channel type Illuminance_div10)
//   EverlightALS_Threshold 10 0.2  - only update when lux moved by 10% or 0.2 lux, whichever is larger
//   EverlightALS_Hold 1            - freeze lux while channel 1 is non-zero
//   Example: startDriver EverlightALS 2
//   Example: startDriver EverlightALS 2 6 7 0x38 100 0x04
void EverlightALS_Init() {
	g_targetChannel = Tokenizer_GetArgIntegerDefault(1, 2);
	g_i2c.pin_clk = Tokenizer_GetArgIntegerDefault(2, PIN_FindPinIndexForRole(IOR_SOFT_SCL, 6));
	g_i2c.pin_data = Tokenizer_GetArgIntegerDefault(3, PIN_FindPinIndexForRole(IOR_SOFT_SDA, 7));
	g_i2cAddr = (uint8_t)Tokenizer_GetArgIntegerDefault(4, EVL_DEFAULT_ADDR);
	g_scalePercent = Tokenizer_GetArgIntegerDefault(5, 100);
	g_gain = (uint8_t)Tokenizer_GetArgIntegerDefault(6, EVL_DEFAULT_GAIN);
	g_alsConv = (uint8_t)Tokenizer_GetArgIntegerDefault(7, EVL_DEFAULT_ALSCONV);
	g_dataReg = (uint8_t)Tokenizer_GetArgIntegerDefault(8, EVL_REG_DATA_DEFAULT);
	g_holdChannel = Tokenizer_GetArgIntegerDefault(9, -1);
	g_held = false;
	g_raw = 0;
	g_lux = 0.0f;

	if (!Soft_I2C_PreInit(&g_i2c)) {
		ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: bus line stuck low (SCL=%d, SDA=%d)",
			g_i2c.pin_clk, g_i2c.pin_data);
	}

	//cmddetail:{"name":"EverlightALS_Hold","args":"[channel]",
	//cmddetail:"descr":"While the given channel is non-zero, the lux value is frozen. Use -1 to disable.",
	//cmddetail:"fn":"EVL_Cmd_Hold","file":"driver/drv_everlight_als.c","requires":"",
	//cmddetail:"examples":"EverlightALS_Hold 1"}
	CMD_RegisterCommand("EverlightALS_Hold", EVL_Cmd_Hold, NULL);
	//cmddetail:{"name":"EverlightALS_Decimals","args":"[0 or 1]",
	//cmddetail:"descr":"0: target channel holds whole lux (type Illuminance). 1: it holds lux x 10 (type Illuminance_div10), giving 0.1 lux resolution.",
	//cmddetail:"fn":"EVL_Cmd_Decimals","file":"driver/drv_everlight_als.c","requires":"",
	//cmddetail:"examples":"EverlightALS_Decimals 1"}
	CMD_RegisterCommand("EverlightALS_Decimals", EVL_Cmd_Decimals, NULL);
	//cmddetail:{"name":"EverlightALS_Threshold","args":"[percent] [absoluteLux]",
	//cmddetail:"descr":"The target channel is only updated when lux differs from the last sent value by at least the given percent or the absolute amount, whichever is larger. 0 0 disables.",
	//cmddetail:"fn":"EVL_Cmd_Threshold","file":"driver/drv_everlight_als.c","requires":"",
	//cmddetail:"examples":"EverlightALS_Threshold 10 0.2"}
	CMD_RegisterCommand("EverlightALS_Threshold", EVL_Cmd_Threshold, NULL);
	//cmddetail:{"name":"EverlightALS_Dump","args":"",
	//cmddetail:"descr":"Prints sensor registers 0x00-0x1F to the log.",
	//cmddetail:"fn":"EVL_Cmd_Dump","file":"driver/drv_everlight_als.c","requires":"",
	//cmddetail:"examples":"EverlightALS_Dump"}
	CMD_RegisterCommand("EverlightALS_Dump", EVL_Cmd_Dump, NULL);

	EVL_ApplyChannelType();
	if (EVL_GainMultiplier(g_gain) == 0) {
		ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: unknown gain code 0x%02X, lux is computed as for x1", g_gain);
	}
	if (EVL_Setup()) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "EverlightALS ready at 0x%02X (SCL=%d, SDA=%d, Ch=%d, Scale=%d%%, Gain=0x%02X, Res=0x%02X, DataReg=0x%02X)",
			g_i2cAddr, g_i2c.pin_clk, g_i2c.pin_data, g_targetChannel, g_scalePercent, g_gain, g_alsConv, g_dataReg);
	}
}

void EverlightALS_OnEverySecond() {
	uint8_t lo = 0, hi = 0;
	uint32_t maxCount;
	int mult;

	if (!g_ready) {
		// sensor missing or lost power - retry quietly once a second
		EVL_Setup();
		return;
	}
	// low byte first: the high byte is latched when the low byte is read
	if (!EVL_ReadReg(g_dataReg, &lo) || !EVL_ReadReg((uint8_t)(g_dataReg + 1), &hi)) {
		ADDLOG_ERROR(LOG_FEATURE_DRV, "EverlightALS: data read failed");
		g_ready = false;
		return;
	}
	if (g_holdChannel >= 0 && CHANNEL_Get(g_holdChannel) != 0) {
		// keep the last lux and channel value; also drop the first two samples
		// after release. (MEASURED) on a FH_PIR_400A one skipped sample still let
		// a ~0.4 lux trace of the indicator LED through.
		g_held = true;
		g_skipSamples = EVL_SKIP_AFTER_HOLD;
		return;
	}
	g_held = false;
	if (g_skipSamples > 0) {
		g_skipSamples--;
		return;
	}
	g_raw = ((uint16_t)hi << 8) | lo;

	// (DS) max count = min(1024 * (ALSCONV + 1) - 1, 65535)
	maxCount = 1024u * ((uint32_t)g_alsConv + 1u) - 1u;
	if (maxCount > 65535u) {
		maxCount = 65535u;
	}
	g_saturated = (g_raw >= maxCount);
	if (g_saturated) {
		// reading is clipped - keep the last valid lux and channel value
		ADDLOG_DEBUG(LOG_FEATURE_DRV, "EverlightALS: saturated (raw=%u), lower the gain", g_raw);
		return;
	}
	mult = EVL_GainMultiplier(g_gain);
	if (mult == 0) {
		mult = 1;
	}
	// the base factor is for x1 gain at 16-bit resolution; higher gain and longer
	// integration both give proportionally more counts for the same light
	g_lux = (float)g_raw * EVL_BASE_LUX_PER_COUNT / (float)mult
		* (64.0f / ((float)g_alsConv + 1.0f))
		* ((float)g_scalePercent / 100.0f);

	ADDLOG_DEBUG(LOG_FEATURE_DRV, "EverlightALS raw=%u lux=%.2f", g_raw, g_lux);

	if (g_targetChannel < 0) {
		return;
	}
	if (g_haveSent) {
		float diff = g_lux - g_sentLux;
		float thr = g_sentLux * g_thrPercent / 100.0f;
		if (diff < 0.0f) {
			diff = -diff;
		}
		if (thr < g_thrAbsLux) {
			thr = g_thrAbsLux;
		}
		if (diff < thr) {
			return;
		}
	}
	g_sentLux = g_lux;
	g_haveSent = true;
	CHANNEL_Set(g_targetChannel, (int)((g_decimals ? g_lux * 10.0f : g_lux) + 0.5f), 0);
}

void EverlightALS_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState) {
		return;
	}
	if (!g_ready) {
		hprintf255(request, "<b>EverlightALS sensor not detected</b>");
	} else {
		hprintf255(request, "<h2>EverlightALS Lux=%.1f (raw %u%s)</h2>", g_lux, g_raw, g_held ? ", held" : (g_saturated ? ", saturated" : ""));
	}
}

void EverlightALS_Stop() {
	if (g_ready) {
		EVL_WriteReg(EVL_REG_SYSM_CTRL, 0x00);
	}
	g_ready = false;
}

#endif

#include "../obk_config.h"

#if ENABLE_DRIVER_LTR_ALS

#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_ltr_als.h"

static softI2C_t g_softI2C;
static bool is_init = false;

static int ch_lux = -1;

static uint16_t als_ch0 = 0;
static uint16_t als_ch1 = 0;
static float als_lux = 0.0f;

static byte sec_left = 0;
static byte sec_cycle = 4;

static void LTR_WriteReg(byte reg, byte val)
{
	Soft_I2C_Start(&g_softI2C, LTR_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, reg);
	Soft_I2C_WriteByte(&g_softI2C, val);
	Soft_I2C_Stop(&g_softI2C);
}

static byte LTR_Read8(byte reg)
{
	byte v;
	Soft_I2C_Start(&g_softI2C, LTR_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, reg);
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, LTR_I2C_ADDR | 1);
	v = Soft_I2C_ReadByte(&g_softI2C, true);
	Soft_I2C_Stop(&g_softI2C);
	return v;
}

static void LTR_Reset()
{
	LTR_WriteReg(LTR_REG_ALS_CONTR, 0x02);
	rtos_delay_milliseconds(10);
}

static void LTR_Setup()
{
	LTR_Reset();
	byte manufac = LTR_Read8(LTR_REG_MANUFAC_ID);
	if(manufac != 0x05)
	{
		// retry once
		manufac = LTR_Read8(LTR_REG_MANUFAC_ID);
		if(manufac != 0x05)
		{
			ADDLOG_ERROR(LOG_FEATURE_SENSOR, "LTR unknown manufacturer %i", manufac);
			is_init = false;
			return;
		}
	}
	LTR_WriteReg(LTR_REG_ALS_CONTR, 0x01);
	rtos_delay_milliseconds(20);
	// dummy read
	LTR_Read8(LTR_REG_STATUS);
	is_init = true;
}

static void LTR_ReadData()
{
	if(!(LTR_Read8(LTR_REG_STATUS) & 0x04)) return;

	byte c1l = LTR_Read8(LTR_REG_CH1_0);
	byte c1h = LTR_Read8(LTR_REG_CH1_1);
	byte c0l = LTR_Read8(LTR_REG_CH0_0);
	byte c0h = LTR_Read8(LTR_REG_CH0_1);

	als_ch1 = ((uint16_t)c1h << 8) | c1l;
	als_ch0 = ((uint16_t)c0h << 8) | c0l;
}

static void LTR_CalcLux()
{
	if(als_ch0 == 0 && als_ch1 == 0)
	{
		als_lux = 0;
		return;
	}

	float r = (float)als_ch1 / (float)(als_ch0 + als_ch1);

	if(r < 0.45f)      als_lux = 1.7743f * als_ch0 + 1.1059f * als_ch1;
	else if(r < 0.64f) als_lux = 4.2785f * als_ch0 - 1.9548f * als_ch1;
	else if(r < 0.85f) als_lux = 0.5926f * als_ch0 + 0.1185f * als_ch1;
	else               als_lux = 0.0f;
}

static void LTR_Measure()
{
	if(!is_init) return;

	LTR_ReadData();
	LTR_CalcLux();

	if(ch_lux >= 0) CHANNEL_Set(ch_lux, (int)(als_lux * 10), 0);
}

commandResult_t LTR_ALS(const void* ctx, const char* cmd, const char* args, int flags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);

	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	int gain = Tokenizer_GetArgInteger(0);
	int integ = Tokenizer_GetArgInteger(1);
	int rep = Tokenizer_GetArgInteger(2);

	LTR_WriteReg(LTR_REG_ALS_CONTR, 0x01 | (gain << 2));
	LTR_WriteReg(LTR_REG_MEAS_RATE, (rep & 0x07) | ((integ & 0x07) << 3));
	return CMD_RES_OK;
}

commandResult_t LTR_INT(const void* ctx, const char* cmd, const char* args, int flags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);

	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	int en = Tokenizer_GetArgInteger(0);
	int low = Tokenizer_GetArgInteger(1);
	int high = Tokenizer_GetArgInteger(2);
	int persist = Tokenizer_GetArgInteger(3);

	LTR_WriteReg(LTR_REG_ALS_THRES_LOW_0, low & 0xFF);
	LTR_WriteReg(LTR_REG_ALS_THRES_LOW_1, low >> 8);
	LTR_WriteReg(LTR_REG_ALS_THRES_UP_0, high & 0xFF);
	LTR_WriteReg(LTR_REG_ALS_THRES_UP_1, high >> 8);
	LTR_WriteReg(LTR_REG_INTERRUPT_PERSIST, persist & 0x0F);
	LTR_WriteReg(LTR_REG_INTERRUPT, en ? 0x02 : 0x00);

	return CMD_RES_OK;
}

commandResult_t LTR_Cycle(const void* ctx, const char* cmd, const char* args, int flags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);

	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	int s = Tokenizer_GetArgInteger(0);
	if(s < 1) return CMD_RES_BAD_ARGUMENT;

	sec_cycle = s;
	sec_left = s;
	return CMD_RES_OK;
}

void LTR_Init()
{
	g_softI2C.pin_clk = Tokenizer_GetPin(1, 9);
	g_softI2C.pin_data = Tokenizer_GetPin(2, 14);

	ch_lux = Tokenizer_GetArgIntegerDefault(3, -1);

	Soft_I2C_PreInit(&g_softI2C);
	rtos_delay_milliseconds(100);

	LTR_Setup();

	//cmddetail:{"name":"LTR_ALS","args":"[gain] [integration] [repeat]",
	//cmddetail:"descr":"ALS configuration. Gain mappings: 0-x1 1-x2 2-x4 3-x8 6-x48 7-x96. Integration mappings (ms): 0-100 1-50 2-200 3-400 4-150 5-250 6-300 7-350. Repeat mappings (ms): 0-50 1-100 2-200 3-500 4-1000 5-2000.",
	//cmddetail:"fn":"LTR_ALS","file":"driver/drv_ltr_als.c","requires":"",
	//cmddetail:"examples":"LTR_ALS 3 0 3 <br /> gain 8x, integration 100ms, repeat 500ms"}
	CMD_RegisterCommand("LTR_ALS", LTR_ALS, NULL);
	//cmddetail:{"name":"LTR_INT","args":"[enable] [als_low] [als_high] [persistance]",
	//cmddetail:"descr":"Interrupt configuration. Enable = 0-1, persistance (oversampling) = 0-15. als_low/als_high - raw thresholds",
	//cmddetail:"fn":"LTR_INT","file":"driver/drv_ltr_als.c","requires":"",
	//cmddetail:"examples":"LTR_INT 1 200 2000 4 <br /> enable interrupt, when measurement is lower than 200 or higher than 2000 4 times"}
	CMD_RegisterCommand("LTR_INT", LTR_INT, NULL);
	//cmddetail:{"name":"LTR_Cycle","args":"[IntervalSeconds]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 1. Max is 255.",
	//cmddetail:"fn":"LTR_Cycle","file":"driver/drv_ltr_als.c","requires":"",
	//cmddetail:"examples":"LTR_Cycle 60 <br /> measurement is taken every 60 seconds"}
	CMD_RegisterCommand("LTR_Cycle", LTR_Cycle, NULL);
}

void LTR_OnEverySecond()
{
	if(sec_left-- <= 0)
	{
		LTR_Measure();
		sec_left = sec_cycle;
	}
}

void LTR_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if(bPreState)
		return;

	if(!is_init) hprintf255(request, "<b>LTR sensor not detected</b>");
	else         hprintf255(request, "<h2>LTR Lux=%.1f</h2>", als_lux);
}

#endif

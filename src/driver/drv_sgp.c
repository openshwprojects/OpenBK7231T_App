#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

#include "drv_sgp.h"


#define SGP_I2C_ADDRESS (0x58 << 1)

static byte channel_co2 = 0, channel_tvoc = 0, g_sgpcycle = 1, g_sgpcycleref = 10, g_sgpstate = 0;
static float g_co2 = 0.0, g_tvoc = 0.0;
static softI2C_t g_sgpI2C;


void SGP_Readmeasure() {
#if WINDOWS
	// TODO: values for simulator so I can test SGP
	// on my Windows machine
	g_co2 = 120;
	g_tvoc = 130;
#else
	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	// launch measurement on sensor. 
	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x20);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x08);
	Soft_I2C_Stop(&g_sgpI2C);

	rtos_delay_milliseconds(12);

	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS | 1);
	Soft_I2C_ReadBytes(&g_sgpI2C, buff, 6);
	Soft_I2C_Stop(&g_sgpI2C);

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	addLogAdv(LOG_DEBUG, LOG_FEATURE_SENSOR, "SGP_Measure: Bits %02X %02X %02X %02X", buff[0], buff[1], buff[3], buff[4]);

	g_co2 = ((th * 256 + tl));
	g_tvoc = ((hh * 256 + hl));
#endif

	channel_co2 = g_cfg.pins.channels[g_sgpI2C.pin_data];
	channel_tvoc = g_cfg.pins.channels2[g_sgpI2C.pin_data];
	if (g_co2 == 400.00 && g_tvoc == 0.00)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP_Measure: Baseline init in progress");
		g_sgpstate = 0;
	}
	else {
		g_sgpstate = 1;
		CHANNEL_Set(channel_co2, (int)(g_co2), 0);
		CHANNEL_Set(channel_tvoc, (int)(g_tvoc), 0);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP_Measure: CO2 :%.1f ppm tvoc:%.0f ppb", g_co2, g_tvoc);
}

// StopDriver SGP
void SGP_StopDriver() {
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP : Stopping Driver and reset sensor");
}


void SGP_GetBaseline()
{
	uint8_t buff[6];
	// launch measurement on sensor. 
	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x20);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x15);
	Soft_I2C_Stop(&g_sgpI2C);

	rtos_delay_milliseconds(10);

	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS | 1);
	Soft_I2C_ReadBytes(&g_sgpI2C, buff, 6);
	Soft_I2C_Stop(&g_sgpI2C);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP : baseline : %02X %02X %02X %02X", buff[0], buff[1], buff[3], buff[4]);
}
commandResult_t SGP_GetBaselinecmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	SGP_GetBaseline();
	return CMD_RES_OK;
}

void SGP_GetVersion()
{
	uint8_t buff[3];
	uint16_t feature_set_version;
	uint8_t product_type;

	// launch measurement on sensor. 
	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x20);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x2f);
	Soft_I2C_Stop(&g_sgpI2C);

	rtos_delay_milliseconds(10);

	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS | 1);
	Soft_I2C_ReadBytes(&g_sgpI2C, buff, 3);
	Soft_I2C_Stop(&g_sgpI2C);
	feature_set_version = ((buff[1] * 256) + buff[0]);
	product_type = (uint8_t)((feature_set_version & 0xF000) >> 12);
	feature_set_version = feature_set_version & 0x00FF;

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP : Version : %i %i ", feature_set_version, product_type);
}
commandResult_t SGP_GetVersioncmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	SGP_GetVersion();
	return CMD_RES_OK;
}

void SGP_SoftReset()
{
	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x00);			//Clear status
	Soft_I2C_WriteByte(&g_sgpI2C, 0x06);          //clear status
	Soft_I2C_Stop(&g_sgpI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP : SOFT RESET");
}
commandResult_t SGP_SoftResetcmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	SGP_SoftReset();
	return CMD_RES_OK;
}

void SGP_INIT_BASELINE()
{
	Soft_I2C_Start(&g_sgpI2C, SGP_I2C_ADDRESS);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x20);
	Soft_I2C_WriteByte(&g_sgpI2C, 0x03);
	Soft_I2C_Stop(&g_sgpI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SGP : Launch INIT Baseline");
}


// Read Baseline
static void SGP_Read_IAQ(float* co2, float* tvoc)
{
}

static uint8_t SGP_CalcCrc(uint8_t* data)
{
	uint8_t bit;        // bit mask
	uint8_t crc = 0xFF; // calculated checksum

	// calculates 8-Bit checksum with given polynomial
	crc ^= (data[0]);
	for (bit = 8; bit > 0; --bit)
	{
		if ((crc & 0x80))
		{
			crc = (crc << 1) ^ 0x131;
		}
		else
		{
			crc = (crc << 1);
		}
	}

	crc ^= (data[1]);
	for (bit = 8; bit > 0; --bit)
	{
		if ((crc & 0x80))
		{
			crc = (crc << 1) ^ 0x131;
		}
		else
		{
			crc = (crc << 1);
		}
	}

	return crc;
}
commandResult_t SGP_cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "SHT Cycle : Need integer args for seconds cycle");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_sgpcycleref = Tokenizer_GetArgFloat(0);

	ADDLOG_INFO(LOG_FEATURE_CMD, "SGP Cycle : Measurement will run every %i seconds", g_sgpcycleref);

	return CMD_RES_OK;
}
// startDriver SGP
void SGP_Init() {


	g_sgpI2C.pin_clk = 9;
	g_sgpI2C.pin_data = 14;

	g_sgpI2C.pin_clk = PIN_FindPinIndexForRole(IOR_SGP_CLK, g_sgpI2C.pin_clk);
	g_sgpI2C.pin_data = PIN_FindPinIndexForRole(IOR_SGP_DAT, g_sgpI2C.pin_data);

	Soft_I2C_PreInit(&g_sgpI2C);

	//init the baseline
	SGP_INIT_BASELINE();

	rtos_delay_milliseconds(10);

	//cmddetail:{"name":"SGP_cycle","args":"[int]",
	//cmddetail:"descr":"change cycle of measurement by default every 10 seconds 0 to deactivate",
	//cmddetail:"fn":"SGP_cycle","file":"drv/drv_sgp.c","requires":"",
	//cmddetail:"examples":"SGP_Cycle 60"}
	CMD_RegisterCommand("SGP_cycle", SGP_cycle, NULL);
	//cmddetail:{"name":"SGP_GetVersion","args":"",
	//cmddetail:"descr":"SGP : get version",
	//cmddetail:"fn":"SGP_GetVersion","file":"drv/drv_sgp.c","requires":"",
	//cmddetail:"examples":"SGP_GetVersion"}
	CMD_RegisterCommand("SGP_GetVersion", SGP_GetVersioncmd, NULL);
	//cmddetail:{"name":"SGP_GetBaseline","args":"",
	//cmddetail:"descr":"SGP Get baseline",
	//cmddetail:"fn":"SGP_GetBaseline","file":"drv/drv_sgp.c","requires":"",
	//cmddetail:"examples":"SGP_GetBaseline"}
	CMD_RegisterCommand("SGP_GetBaseline", SGP_GetBaselinecmd, NULL);
	//cmddetail:{"name":"SGP_SoftReset","args":"",
	//cmddetail:"descr":"SGP i2C soft reset",
	//cmddetail:"fn":"SGP_SoftReset","file":"drv/drv_sgp.c","requires":"",
	//cmddetail:"examples":"SGP_SoftReset"}
	CMD_RegisterCommand("SGP_SoftReset", SGP_SoftResetcmd, NULL);
}
void SGP_OnEverySecond()
{

	if (g_sgpcycle == 1 || g_sgpstate == 0) {
		SGP_Readmeasure();
		g_sgpcycle = g_sgpcycleref;
	}
	if (g_sgpcycle > 0) {
		--g_sgpcycle;
	}
	ADDLOG_DEBUG(LOG_FEATURE_DRV, "DRV_SGP : Measurement will run in %i cycle", g_sgpcycle);
}

void SGP_AppendInformationToHTTPIndexPage(http_request_t* request)
{

	hprintf255(request, "<h2>SGP CO2=%.1f ppm, TVoc=%.0f ppb</h2>", g_co2, g_tvoc);
	if (channel_co2 == channel_tvoc) {
		hprintf255(request, "WARNING: You don't have configured target channels for co2 and tvoc results, set the first and second channel index in Pins!");
	}
	if (g_sgpstate == 0) {
		hprintf255(request, "WARNING: Baseline calculation in progress");
	}
}

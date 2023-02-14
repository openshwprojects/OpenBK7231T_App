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

#include "drv_sm2135.h"

// Some platforms have less pins than BK7231T.
// For example, BL602 doesn't have pin number 26.
// The pin code would crash BL602 while trying to access pin 26.
// This is why the default settings here a per-platform.
#if PLATFORM_BEKEN
int g_i2c_pin_clk = 26;
int g_i2c_pin_data = 24;
#else
int g_i2c_pin_clk = 0;
int g_i2c_pin_data = 1;
#endif

static int g_current_setting_cw = SM2135_20MA;
static int g_current_setting_rgb = SM2135_20MA;


void usleep(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#ifdef WIN32
	// not possible on Windows port
#else
	for (volatile int i = 0; i < r; i++)
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
#endif
}

void Soft_I2C_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

void Soft_I2C_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

bool Soft_I2C_PreInit(void) {
	HAL_PIN_SetOutputValue(g_i2c_pin_data, 0);
	HAL_PIN_SetOutputValue(g_i2c_pin_clk, 0);
	Soft_I2C_SetHigh(g_i2c_pin_data);
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_i2c_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_i2c_pin_clk) == 0)));
}

bool Soft_I2C_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0X80; curr != 0; curr >>= 1) {
		if (curr & value) {
			Soft_I2C_SetHigh(g_i2c_pin_data);
		} else {
			Soft_I2C_SetLow(g_i2c_pin_data);
		}
		Soft_I2C_SetHigh(g_i2c_pin_clk);
		usleep(SM2135_DELAY);
		Soft_I2C_SetLow(g_i2c_pin_clk);
	}
	// get Ack or Nak
	Soft_I2C_SetHigh(g_i2c_pin_data);
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	usleep(SM2135_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_i2c_pin_data);
	Soft_I2C_SetLow(g_i2c_pin_clk);
	usleep(SM2135_DELAY / 2);
	Soft_I2C_SetLow(g_i2c_pin_data);
	return (0 == ack);
}

bool Soft_I2C_Start(uint8_t addr) {
	Soft_I2C_SetLow(g_i2c_pin_data);
	usleep(SM2135_DELAY);
	Soft_I2C_SetLow(g_i2c_pin_clk);
	return Soft_I2C_WriteByte(addr);
}

void Soft_I2C_Stop(void) {
	Soft_I2C_SetLow(g_i2c_pin_data);
	usleep(SM2135_DELAY);
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	usleep(SM2135_DELAY);
	Soft_I2C_SetHigh(g_i2c_pin_data);
	usleep(SM2135_DELAY);
}


void Soft_I2C_ReadBytes(uint8_t* buf, int numOfBytes)
{

	for (int i = 0; i < numOfBytes - 1; i++)
	{

		buf[i] = Soft_I2C_ReadByte(false);

	}

	buf[numOfBytes - 1] = Soft_I2C_ReadByte(true); //Give NACK on last byte read
}
uint8_t Soft_I2C_ReadByte(bool nack)
{
	uint8_t val = 0;

	Soft_I2C_SetHigh(g_i2c_pin_data);
	for (int i = 0; i < 8; i++)
	{
		usleep(SM2135_DELAY);
		Soft_I2C_SetHigh(g_i2c_pin_clk);
		val <<= 1;
		if (HAL_PIN_ReadDigitalInput(g_i2c_pin_data))
		{
			val |= 1;
		}
		Soft_I2C_SetLow(g_i2c_pin_clk);
	}
	if (nack)
	{
		Soft_I2C_SetHigh(g_i2c_pin_data);
	}
	else
	{
		Soft_I2C_SetLow(g_i2c_pin_data);
	}
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	usleep(SM2135_DELAY);
	Soft_I2C_SetLow(g_i2c_pin_clk);
	usleep(SM2135_DELAY);
	Soft_I2C_SetLow(g_i2c_pin_data);

	return val;
}
void SM2135_Write(float *rgbcw) {
	int i;
	int bRGB;

	if(CFG_HasFlag(OBK_FLAG_SM2135_SEPARATE_MODES)) {
		bRGB = 0;
		for(i = 0; i < 3; i++){
			if(rgbcw[g_cfg.ledRemap.ar[i]]!=0) {
				bRGB = 1;
				break;
			}
		}
		if(bRGB) {
			Soft_I2C_Start(SM2135_ADDR_MC);
			Soft_I2C_WriteByte(g_current_setting_rgb);
			Soft_I2C_WriteByte(SM2135_RGB);
			Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.r]);
			Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.g]);
			Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.b]); 
			Soft_I2C_Stop();
		} else {
			Soft_I2C_Start(SM2135_ADDR_MC);
			Soft_I2C_WriteByte(g_current_setting_cw);
			Soft_I2C_WriteByte(SM2135_CW);
			Soft_I2C_Stop();
			usleep(SM2135_DELAY);

			Soft_I2C_Start(SM2135_ADDR_C);
			Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.c]);
			Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.w]); 
			Soft_I2C_Stop();

		}
	} else {
		Soft_I2C_Start(SM2135_ADDR_MC);
		Soft_I2C_WriteByte(g_current_setting_rgb);
		Soft_I2C_WriteByte(SM2135_RGB);
		Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.r]);
		Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.g]);
		Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.b]); 
		Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.c]); 
		Soft_I2C_WriteByte(rgbcw[g_cfg.ledRemap.w]); 
		Soft_I2C_Stop();
	}
}

commandResult_t CMD_LEDDriver_WriteRGBCW(const void *context, const char *cmd, const char *args, int flags){
	const char *c = args;
	float col[5] = { 0, 0, 0, 0, 0 };
	int ci;
	int val;

	ci = 0;

	// some people prefix colors with #
	if(c[0] == '#')
		c++;
	while (*c){
		char tmp[3];
		int r;
		tmp[0] = *(c++);
		if (!*c)
			break;
		tmp[1] = *(c++);
		tmp[2] = '\0';
		r = sscanf(tmp, "%x", &val);
		if (!r) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "No sscanf hex result from %s", tmp);
			break;
		}

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "Found chan %d -> val255 %d (from %s)", ci, val, tmp);

		col[ci] = val;

		// move to next channel.
		ci ++;
		if(ci>=5)
			break;
	}

	LED_I2CDriver_WriteRGBCW(col);

	return CMD_RES_OK;
}
// SM2135_Map is used to map the RGBCW indices to SM2135 indices
// This is how you uset RGB CW order:
// SM2135_Map 0 1 2 3 4
// This is the order used on my polish Spectrum WOJ14415 bulb:
// SM2135_Map 2 1 0 4 3 

commandResult_t CMD_LEDDriver_Map(const void *context, const char *cmd, const char *args, int flags){
	int r, g, b, c, w;
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Current map is %i %i %i %i %i",
			(int)g_cfg.ledRemap.r,(int)g_cfg.ledRemap.g,(int)g_cfg.ledRemap.b,(int)g_cfg.ledRemap.c,(int)g_cfg.ledRemap.w);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	r = Tokenizer_GetArgIntegerRange(0, 0, 4);
	g = Tokenizer_GetArgIntegerRange(1, 0, 4);
	b = Tokenizer_GetArgIntegerRange(2, 0, 4);
	c = Tokenizer_GetArgIntegerRange(3, 0, 4);
	w = Tokenizer_GetArgIntegerRange(4, 0, 4);

	CFG_SetLEDRemap(r, g, b, c, w);

	ADDLOG_INFO(LOG_FEATURE_CMD, "New map is %i %i %i %i %i",
		(int)g_cfg.ledRemap.r,(int)g_cfg.ledRemap.g,(int)g_cfg.ledRemap.b,(int)g_cfg.ledRemap.c,(int)g_cfg.ledRemap.w);

	return CMD_RES_OK;
}

static void SM2135_SetCurrent(int curValRGB, int curValCW) {
	g_current_setting_rgb = curValRGB;
	g_current_setting_cw = curValCW;
}

static commandResult_t SM2135_Current(const void *context, const char *cmd, const char *args, int flags){
	int valRGB;
	int valCW;
	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	valRGB = Tokenizer_GetArgInteger(0);
	valCW = Tokenizer_GetArgInteger(1);

	SM2135_SetCurrent(valRGB,valCW);
	return CMD_RES_OK;
}

// startDriver SM2135
// CMD_LEDDriver_WriteRGBCW FF00000000
void SM2135_Init() {

	// default setting (applied only if none was applied earlier)
	CFG_SetDefaultLEDRemap(2, 1, 0, 4, 3);

	g_i2c_pin_clk = PIN_FindPinIndexForRole(IOR_SM2135_CLK,g_i2c_pin_clk);
	g_i2c_pin_data = PIN_FindPinIndexForRole(IOR_SM2135_DAT,g_i2c_pin_data);

	Soft_I2C_PreInit();

	//cmddetail:{"name":"SM2135_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of SM2135 driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"SM2135_RGBCW","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2135_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
	//cmddetail:{"name":"SM2135_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of SM2135 channels. This is because SM2135 channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: SM2135_Map 0 1 2 3 4",
	//cmddetail:"fn":"SM2135_Map","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2135_Map", CMD_LEDDriver_Map, NULL);
	//cmddetail:{"name":"SM2135_Current","args":"[Value]",
	//cmddetail:"descr":"Sets the maximum current for LED driver.",
	//cmddetail:"fn":"SM2135_Current","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2135_Current", SM2135_Current, NULL);
}



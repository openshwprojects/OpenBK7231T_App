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

#include "drv_sm2135.h"

static int g_pin_clk = 26;
static int g_pin_data = 24;
static int g_current_setting = SM2135_20MA;

static void SM2135_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

static void SM2135_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

static bool SM2135_PreInit(void) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	SM2135_SetHigh(g_pin_data);
	SM2135_SetHigh(g_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_pin_clk) == 0)));
}

static bool SM2135_WriteByte(uint8_t value) {
	uint8_t curr;

	for (curr = 0X80; curr != 0; curr >>= 1) {
		if (curr & value) {
			SM2135_SetHigh(g_pin_data);
		} else {
			SM2135_SetLow(g_pin_data);
		}
		SM2135_SetHigh(g_pin_clk);
		rtos_delay_milliseconds(SM2135_DELAY);
		SM2135_SetLow(g_pin_clk);
	}
	// get Ack or Nak
	SM2135_SetHigh(g_pin_data);
	SM2135_SetHigh(g_pin_clk);
	rtos_delay_milliseconds(SM2135_DELAY / 2);
	uint8_t ack = HAL_PIN_ReadDigitalInput(g_pin_data);
	SM2135_SetLow(g_pin_clk);
	rtos_delay_milliseconds(SM2135_DELAY / 2);
	SM2135_SetLow(g_pin_data);
	return (0 == ack);
}

static bool SM2135_Start(uint8_t addr) {
	SM2135_SetLow(g_pin_data);
	rtos_delay_milliseconds(SM2135_DELAY);
	SM2135_SetLow(g_pin_clk);
	return SM2135_WriteByte(addr);
}

static void SM2135_Stop(void) {
	SM2135_SetLow(g_pin_data);
	rtos_delay_milliseconds(SM2135_DELAY);
	SM2135_SetHigh(g_pin_clk);
	rtos_delay_milliseconds(SM2135_DELAY);
	SM2135_SetHigh(g_pin_data);
	rtos_delay_milliseconds(SM2135_DELAY);
}

void SM2135_Write(byte *rgbcw) {
    SM2135_Start(SM2135_ADDR_MC);
    SM2135_WriteByte(g_current_setting);
    SM2135_WriteByte(SM2135_RGB);
    SM2135_WriteByte(rgbcw[2]);
    SM2135_WriteByte(rgbcw[1]);
    SM2135_WriteByte(rgbcw[0]); 
    SM2135_WriteByte(rgbcw[4]); 
    SM2135_WriteByte(rgbcw[3]); 
    SM2135_Stop();
}

static int SM2135_RGBCW(const void *context, const char *cmd, const char *args){
	const char *c = args;
	byte col[5] = { 0, 0, 0, 0, 0 };
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
			ADDLOG_ERROR(LOG_FEATURE_CMD, "SM2135_RGBCW no sscanf hex result from %s", tmp);
			break;
		}

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2135_RGBCW found chan %d -> val255 %d (from %s)", ci, val, tmp);

		col[ci] = val;

		// move to next channel.
		ci ++;
		if(ci>=5)
			break;
	}

	SM2135_Write(col);

	return 0;
}
// startDriver SM2135
// SM2135_RGBCW FF00000000
void SM2135_Init() {

    SM2135_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_SM2135_CLK,g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_SM2135_DAT,g_pin_data);

    CMD_RegisterCommand("SM2135_RGBCW", "", SM2135_RGBCW, "qq", NULL);
}

void SM2135_RunFrame() {

}


void SM2135_OnChannelChanged(int ch, int value) {
#if 0
	byte col[5];
	int channel;
	int c;

	for(channel = 0; channel < CHANNEL_MAX; channel++){
		if(IOR_PWM == CHANNEL_GetRoleForOutputChannel(channel)){
			col[c] = CHANNEL_Get(channel);
			c++;
		}
	}
	for( ; c < 5; c++){
		col[c] = 0;
	}

	SM2135_Write(col);
#endif
}



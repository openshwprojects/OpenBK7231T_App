#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"

// UCS1912 is 12 channels each.
// We send 96 bits. 96 / 8 = 12. Send byte per each channel. 

// values in ns
#define UCS1912_T0H	250
#define UCS1912_T0L	1000

#define UCS1912_T1H	1000
#define UCS1912_T1L	250

// Those times differ from WS2812B, from what I can see...

// Sending one bit to UCS1912 takes 1250ns.
// So sending 96 bits would take 96 * 1250 = 120000ns = 0.12ms 

#define UCS1912_RESET 24000

static int g_pin_di = 0;


#define UCS1912_SLEEP_250	__asm__("nop\nnop");
#define UCS1912_SLEEP_1000	__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");

#define UCS1912_SEND_T0 HAL_PIN_SetOutputValue(g_pin_di,true); UCS1912_SLEEP_250; HAL_PIN_SetOutputValue(g_pin_di,false); UCS1912_SLEEP_1000;
#define UCS1912_SEND_T1 HAL_PIN_SetOutputValue(g_pin_di,true); UCS1912_SLEEP_1000; HAL_PIN_SetOutputValue(g_pin_di,false); UCS1912_SLEEP_250;

#ifndef CHECK_BIT
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#endif

#define UCS1912_SEND_BIT(var,pos) if(((var) & (1<<(pos)))) { UCS1912_SEND_T1; } else { UCS1912_SEND_T0; };

static void UCS1912_Send(byte *data, int dataSize){
	int i;
	byte b;

	for(i = 0; i < dataSize; i++) {
		b = data[i];
		UCS1912_SEND_BIT(b,0);
		UCS1912_SEND_BIT(b,1);
		UCS1912_SEND_BIT(b,2);
		UCS1912_SEND_BIT(b,3);
		UCS1912_SEND_BIT(b,4);
		UCS1912_SEND_BIT(b,5);
		UCS1912_SEND_BIT(b,6);
		UCS1912_SEND_BIT(b,7);
	}
}
static commandResult_t UCS1912_Test(const void *context, const char *cmd, const char *args, int flags){
	byte test[12];
	int i;

	for(i = 0; i < 12; i++){
		test[i] = rand();
	}
	UCS1912_Send(test,12);

	return CMD_RES_OK;
}

// startDriver UCS1912
void UCS1912_Init() {


	g_pin_di = PIN_FindPinIndexForRole(IOR_UCS1912_DIN,g_pin_di);

	HAL_PIN_Setup_Output(g_pin_di);

	//cmddetail:{"name":"UCS1912_Test","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"UCS1912_Test","file":"driver/drv_ucs1912.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("UCS1912_Test", UCS1912_Test, NULL);
}



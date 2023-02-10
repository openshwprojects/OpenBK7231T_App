#if PLATFORM_BEKEN
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"

// SM16703P is 3 channels each.
// We send 24 bits. 24 / 8 = 3. Send byte per each channel. 

static int g_pin_di = 0;

/*1 equals about 5 us*/
// From platforms/bk7231t/bk7231t_os/beken378/driver/codec/driver_codec_es8374.c 
static void es8374_codec_i2c_delay1equalsabout5us(int us)
{
	volatile int i, j;
	for (i = 0; i < us; i++)
	{
		j = 50;
		while (j--);
	}
}
/*1 equals about 0.3 us*/
static void es8374_codec_i2c_delay(int us)
{
	volatile int i, j;
	for (i = 0; i < us; i++)
	{
		j = 3;
		while (j--);
	}
}
#define SM16703P_SLEEP_300	__asm("nop\nnop\nnop");
#define SM16703P_SLEEP_900	__asm("nop\nnop\nnop\nnop\nnop\nnop\nnop");

void gpio_output(UINT32 id, UINT32 val);


//#define SM16703P_SET_HIGH gpio_output(g_pin_di,true);
//#define SM16703P_SET_LOW gpio_output(g_pin_di,false);

// NOTE: #define REG_WRITE(addr, _data) 	(*((volatile UINT32 *)(addr)) = (_data))
#define SM16703P_SET_HIGH REG_WRITE(gpio_cfg_addr, reg_val_HIGH);
#define SM16703P_SET_LOW REG_WRITE(gpio_cfg_addr, reg_val_LOW);

// gpio_output is faster 
#define SM16703P_SEND_T0 SM16703P_SET_HIGH; SM16703P_SLEEP_300; SM16703P_SET_LOW; SM16703P_SLEEP_900;
#define SM16703P_SEND_T1 SM16703P_SET_HIGH; SM16703P_SLEEP_900; SM16703P_SET_LOW; SM16703P_SLEEP_300;

#ifndef CHECK_BIT
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#endif

#define SM16703P_SEND_BIT(var,pos) if(((var) & (1<<(pos)))) { SM16703P_SEND_T1; } else { SM16703P_SEND_T0; };


#include "include.h"
#include "arm_arch.h"
#include "gpio_pub.h"
#include "gpio.h"
#include "drv_model_pub.h"
#include "sys_ctrl_pub.h"
#include "uart_pub.h"
#include "intc_pub.h"
#include "icu_pub.h"

static void SM16703P_Send(byte *data, int dataSize){
	//int i;
	//byte b;
	UINT32 reg_val = 0;
	volatile UINT32 *gpio_cfg_addr;
	UINT32 id;
	volatile UINT32 reg_val_HIGH = 0x02;
	volatile UINT32 reg_val_LOW = 0x00;


	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	// let things settle down a bit
	es8374_codec_i2c_delay1equalsabout5us(100);

	id = g_pin_di;

#if (CFG_SOC_NAME != SOC_BK7231)
	if (id >= GPIO32)
		id += 16;
#endif // (CFG_SOC_NAME != SOC_BK7231)
	gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);

	SM16703P_SET_LOW;
	SM16703P_SET_LOW;
	SM16703P_SET_LOW;
	SM16703P_SET_LOW;

	SM16703P_SEND_T0;
	SM16703P_SEND_T0;
	SM16703P_SEND_T0;
	SM16703P_SEND_T0;
	SM16703P_SEND_T0;
	SM16703P_SEND_T0;
	SM16703P_SEND_T0; 
	SM16703P_SEND_T0;

/*	SM16703P_SLEEP_900;
	SM16703P_SLEEP_900;
	SM16703P_SLEEP_900;
	SM16703P_SLEEP_900;

	for(i = 0; i < dataSize; i++) {
		b = data[i];
		SM16703P_SEND_BIT(b,0);
		SM16703P_SEND_BIT(b,1);
		SM16703P_SEND_BIT(b,2);
		SM16703P_SEND_BIT(b,3);
		SM16703P_SEND_BIT(b,4);
		SM16703P_SEND_BIT(b,5);
		SM16703P_SEND_BIT(b,6);
		SM16703P_SEND_BIT(b,7);
	}*/
	GLOBAL_INT_RESTORE();
	// For P0, it says 
	// For P11, it says 0
	// For P12, it says 0
	addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Reg val is %i", reg_val);
}
static commandResult_t SM16703P_Test(const void *context, const char *cmd, const char *args, int flags){
	byte test[3];
	int i;

	for(i = 0; i < 3; i++){
		test[i] = rand();
	}
	SM16703P_Send(test,3);

	return CMD_RES_OK;
}

// backlog startDriver SM16703P; SM16703P_Test_3xZero
static commandResult_t SM16703P_Test_3xZero(const void *context, const char *cmd, const char *args, int flags) {
	byte test[3];
	int i;

	for (i = 0; i < 3; i++) {
		test[i] = 0;
	}
	SM16703P_Send(test, 3);


	return CMD_RES_OK;
}
// backlog startDriver SM16703P; SM16703P_Test_3xOne
static commandResult_t SM16703P_Test_3xOne(const void *context, const char *cmd, const char *args, int flags) {
	byte test[3];
	int i;

	for (i = 0; i < 3; i++) {
		test[i] = 0xFF;
	}
	SM16703P_Send(test, 3);


	return CMD_RES_OK;
}
static commandResult_t SM16703P_Send_Cmd(const void *context, const char *cmd, const char *args, int flags) {
	byte test[32];
	int i;
	const char *c;
	int numBytes;

	numBytes = 0;
	c = args;

	for (i = 0; i < sizeof(test) && *c; i++) {
		char tmp[3];
		int val;
		int r;
		tmp[0] = *c;
		c++;
		if (!*c)
			break;
		tmp[1] = *c;
		c++;
		tmp[2] = 0;
		r = sscanf(tmp, "%x", &val);
		if (!r) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "SM16703P_Send_Cmd no sscanf hex result from %s", tmp);
			break;
		}
		test[i] = val;
		numBytes++;
	}
	ADDLOG_ERROR(LOG_FEATURE_CMD, "Will send %i bytes", numBytes);
	SM16703P_Send(test, numBytes);


	return CMD_RES_OK;
}
// startDriver SM16703P
// backlog startDriver SM16703P; SM16703P_Test
void SM16703P_Init() {


	g_pin_di = PIN_FindPinIndexForRole(IOR_SM16703P_DIN,g_pin_di);

	HAL_PIN_Setup_Output(g_pin_di);

	//cmddetail:{"name":"SM16703P_Test","args":"",
	//cmddetail:"descr":"qq",
	//cmddetail:"fn":"SM16703P_Test","file":"driver/drv_ucs1912.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM16703P_Test", SM16703P_Test, NULL);
	//cmddetail:{"name":"SM16703P_Send","args":"",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"SM16703P_Send_Cmd","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Send", SM16703P_Send_Cmd, NULL);
	//cmddetail:{"name":"SM16703P_Test_3xZero","args":"",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"SM16703P_Test_3xZero","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Test_3xZero", SM16703P_Test_3xZero, NULL);
	//cmddetail:{"name":"SM16703P_Test_3xOne","args":"",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"SM16703P_Test_3xOne","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Test_3xOne", SM16703P_Test_3xOne, NULL);
}
#endif


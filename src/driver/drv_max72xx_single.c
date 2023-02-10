#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_max72xx_internal.h"

static max72XX_t *g_max = 0;

unsigned char testhello[4][8] = {
  {
	0b10000010,
	0b01000100,
	0b00101000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00010000,
  },
  {
	0b10000010,
	0b01000100,
	0b00101000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00010000,
  },
  {
	0b01111110,
	0b01000000,
	0b01000000,
	0b01111110,
	0b01000000,
	0b01000000,
	0b01000000,
	0b01111110,
  },
  {
	0b10000001,
	0b10000001,
	0b10000001,
	0b10000001,
	0b11111111,
	0b10000001,
	0b10000001,
	0b10000001,
  },
};

static commandResult_t DRV_MAX72XX_Setup(const void *context, const char *cmd, const char *args, int flags) {
	int din;
	int cs;
	int clk;

	clk = 26;
	cs = 24;
	din = 7;

	g_max = MAX72XX_alloc();

	MAX72XX_setupPins(g_max, cs, clk, din, 4);    
	MAX72XX_init(g_max);
	MAX72XX_displayArray(g_max, &testhello[0][0], 4, 0);

	return CMD_RES_OK;
}
// backlog startDriver MAX72XX; MAX72XX_Setup
void DRV_MAX72XX_Init() {

	//cmddetail:{"name":"MAX72XX_Setup","args":"[Value]",
	//cmddetail:"descr":"Sets the maximum current for LED driver.",
	//cmddetail:"fn":"SM2135_Current","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MAX72XX_Setup", DRV_MAX72XX_Setup, NULL);
}






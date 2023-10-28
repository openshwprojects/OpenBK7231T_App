
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

byte pt_inh = 10;
byte pt_ce = 11;
byte pt_clk = 24;
byte pt_di = 8;

void PT6523_Init()
{
}


void PT6523_RunFrame()
{
}





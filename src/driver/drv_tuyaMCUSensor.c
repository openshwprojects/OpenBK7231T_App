
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_tuyaMCU.h"
#include "drv_uart.h"
#include <time.h>
#include "drv_ntp.h"

static int g_elapsedTime = 0;

static byte g_hello[] =  { 0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00 };
void TuyaMCU_Sensor_RunFrame() {

	g_elapsedTime++;

	if(g_elapsedTime == 5) {
		TuyaMCU_Send_RawBuffer(g_hello,sizeof(g_hello));
	}
}



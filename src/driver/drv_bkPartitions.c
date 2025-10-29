#include "../new_common.h"
#include "../new_pins.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "drv_local.h"
#include "drv_public.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_adc.h"
#include "../mqtt/new_mqtt.h"

static int cur_adr = 0x11000;
static int start_adr = 0x11000;
static int max_adr = 0x200000;
static int read_len = 0x1000;
static byte *g_buf;

void BKPartitions_Init() {
	g_buf = (byte*)malloc(read_len);

}

void BKPartitions_QuickFrame() {
	int r = HAL_FlashRead(g_buf, read_len, start_adr);

}


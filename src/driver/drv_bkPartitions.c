#include "../new_common.h"
#include "../new_pins.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "drv_local.h"
#include "drv_public.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_ota.h"
#include "../hal/hal_adc.h"
#include "../mqtt/new_mqtt.h"

static const char *search_magic = "01PE";
static int cur_adr = 0x0; 
static int start_adr = 0x0; 
static int max_adr = 0x200000; 
static int read_len = 0x1000;
static int record_size = 34;
static int data_size = 32;
static int records_per_chunk = 64;
static int raw_chunk = 34 * 64; // 2176
static int collapsed_chunk = 32 * 64; // 2048
static byte *g_buf;
static int magic_len = 0;

void BKPartitions_Init() {
	magic_len = (int)strlen(search_magic);
	g_buf = (byte*)malloc(raw_chunk);
	cur_adr = start_adr;
}


static int is_position_in_data_zone(int abs_offset) {
	int mod = abs_offset % 34;
	return (mod < 32);
}

void BKPartitions_QuickFrame() {
	if (!g_buf) return;
	if (cur_adr >= max_adr) {
		cur_adr = start_adr;
	}


	int to_read = raw_chunk;
	if (cur_adr + to_read > max_adr)
		to_read = max_adr - cur_adr;
	if (to_read <= 0) return;

	// read raw 2176 bytes
	int r = HAL_FlashRead((char*)g_buf, to_read, cur_adr + 0x200000);
	r = to_read;
	if (r <= 0) {
		cur_adr += data_size * (records_per_chunk - 1);
		return;
	}

	// In-place collapse (skip bytes 32 and 33 of each 34-byte record)
	int w = 0;
	for (int rec = 0; rec < records_per_chunk; rec++) {
		int base = rec * record_size;
		if (base + data_size > r) break;
		// copy first 32 bytes to current write position
		memmove(&g_buf[w], &g_buf[base], data_size);
		w += data_size;
	}

	int scan_len = w - magic_len;
	for (int pos = 0; pos <= scan_len; pos++) {
		if (memcmp(&g_buf[pos], search_magic, magic_len) == 0) {
			unsigned int abs_off = (unsigned int)(cur_adr + pos);
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "AS: found magic at 0x%X\n", abs_off);
		}
	}

	//printf("At %i\n", cur_adr);

	// move by 32*63 = 2016 bytes (overlap one record)
	cur_adr += data_size * (records_per_chunk - 1);
}


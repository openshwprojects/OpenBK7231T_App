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
static byte *g_buf;
static int magic_len = 0;
static int crc_ok = 0;
static int crc_error = 0;

void BKPartitions_Init() {
	magic_len = (int)strlen(search_magic);
	g_buf = (byte*)malloc(raw_chunk);
	cur_adr = start_adr;
}


static int is_position_in_data_zone(int abs_offset) {
	int mod = abs_offset % 34;
	return (mod < 32);
}
uint16_t crc16(const uint8_t *b, int from, int to, uint16_t initial_value)
{
	uint16_t reg = initial_value;
	uint16_t poly = 0x8005;
	for (int p = from; p < to; p++)
	{
		uint8_t octet = b[p];
		for (int i = 0; i < 8; i++)
		{
			uint16_t topbit = reg & 0x8000;
			if (octet & (0x80 >> i))
				topbit ^= 0x8000;
			reg <<= 1;
			if (topbit)
				reg ^= poly;
			reg &= 0xFFFF;
		}
	}
	return reg;
}
int block_crc_check(const uint8_t *data, size_t len, const uint8_t *crc_bytes)
{
	uint16_t stored = (uint16_t)crc_bytes[0] | ((uint16_t)crc_bytes[1] << 8);
	uint16_t calc = crc16(data, 0, len, 0xffff);
	return (stored == calc);
}
int ReadSection(int ofs) {
	int to_read = raw_chunk;
	if (ofs + to_read > max_adr)
		to_read = max_adr - ofs;
	if (to_read <= 0) return 0;

	int r = HAL_FlashRead((char*)g_buf, to_read, ofs);
	r = to_read;
	if (r <= 0) {
		return 0;
	}
	// In-place collapse (skip bytes 32 and 33 of each 34-byte record)
	int realSize = 0;
	for (int rec = 0; rec < records_per_chunk; rec++) {
		int base = rec * record_size;
		if (base + record_size > to_read)
			break;

		// Compute CRC of the 32 bytes of data
		uint16_t crc = crc16(g_buf, base, base + data_size, 0xffff);

		// Read stored CRC (little-endian)
		uint16_t stored = g_buf[base + data_size + 1] |
			(g_buf[base + data_size ] << 8);

		if (crc != stored) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"Invalid CRC at %i",base);
			crc_error++;
			// CRC FAIL
		}
		else {
			crc_ok++;
		}
		// CRC OK → copy 32 bytes to the output position
		memmove(&g_buf[realSize], &g_buf[base], data_size);
		realSize += data_size;
	}
	return realSize;
}
typedef struct {
	char name[25];
	char flash[17];
	uint32_t offset;
	uint32_t length;
	uint32_t extra;
	char layout[8]; // "fal64" or "fal48"
} PartitionRecord;

static int is_printable_str(const char *s) {
	if (!s || !*s) return 0;
	for (int i = 0; s[i]; i++) {
		if (!(s[i] >= 32 && s[i] <= 126)) return 0;
	}
	return 1;
}

bool is_valid_pr(PartitionRecord *out) {

	if (out->length > 0x400000) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
			"Abnormal partition length, probably invalid, skipping");
		return 0;
	}
	if (!is_printable_str(out->name) || !is_printable_str(out->flash))
		return 0;
	return 1;
}
static int parse_fal64(byte *buf, int pos, PartitionRecord *out) {
	uint32_t magic = buf[pos] | (buf[pos + 1] << 8) | (buf[pos + 2] << 16) | (buf[pos + 3] << 24);
	if (magic != 0x45503130) return 0;

	memcpy(out->name, &buf[pos + 4], 24); out->name[24] = 0;
	memcpy(out->flash, &buf[pos + 28], 16); out->flash[16] = 0;

	out->offset = buf[pos + 52] | (buf[pos + 53] << 8) | (buf[pos + 54] << 16) | (buf[pos + 55] << 24);
	out->length = buf[pos + 56] | (buf[pos + 57] << 8) | (buf[pos + 58] << 16) | (buf[pos + 59] << 24);
	out->extra = buf[pos + 60] | (buf[pos + 61] << 8) | (buf[pos + 62] << 16) | (buf[pos + 63] << 24);

	strcpy(out->layout, "fal64");

	return is_valid_pr(out);
}

static int parse_fal48(byte *buf, int pos, PartitionRecord *out) {
	uint32_t magic = buf[pos] | (buf[pos + 1] << 8) | (buf[pos + 2] << 16) | (buf[pos + 3] << 24);
	if (magic != 0x45503130) return 0;

	memcpy(out->name, &buf[pos + 4], 16); out->name[16] = 0;
	memcpy(out->flash, &buf[pos + 20], 16); out->flash[16] = 0;

	out->offset = buf[pos + 36] | (buf[pos + 37] << 8) | (buf[pos + 38] << 16) | (buf[pos + 39] << 24);
	out->length = buf[pos + 40] | (buf[pos + 41] << 8) | (buf[pos + 42] << 16) | (buf[pos + 43] << 24);
	out->extra = buf[pos + 44] | (buf[pos + 45] << 8) | (buf[pos + 46] << 16) | (buf[pos + 47] << 24);

	strcpy(out->layout, "fal48");

	return is_valid_pr(out);
}

void ReadPartition(int ofs) {
	int realSize = ReadSection(ofs);
	int scan_len = realSize - magic_len;

	for (int pos = 0; pos <= scan_len; pos++) {
		if (memcmp(&g_buf[pos], search_magic, magic_len) != 0) continue;

		PartitionRecord rec;
		if (
			parse_fal64(g_buf, pos, &rec) || 
			parse_fal48(g_buf, pos, &rec)) {
			unsigned int abs_off = ofs + pos;
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
				"Partition found at 0x%X: %s flash=%s offset=0x%X size=%u extra=%u layout=%s\n",
				abs_off, rec.name, rec.flash, rec.offset, rec.length, rec.extra, rec.layout);
		}
	}
}


void BKPartitions_QuickFrame() {
	if (!g_buf) return;
	if (cur_adr >= max_adr) {
		cur_adr = start_adr;
	}

	int realSize = ReadSection(cur_adr);
	int scan_len = realSize - magic_len;
	for (int pos = 0; pos <= scan_len; pos++) {
		if (memcmp(&g_buf[pos], search_magic, magic_len) == 0) {
			unsigned int abs_off = (unsigned int)(cur_adr + pos);
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "AS: found magic at 0x%X\n", abs_off);
			// find nearest first record 
			int nearest = (abs_off/ 34) * 34;
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "AS: nearest at 0x%X\n", nearest);
			ReadPartition(nearest);
		}
	}

	//printf("At %i\n", cur_adr);

	// move by one record less
	cur_adr += record_size * (records_per_chunk - 1);
}


#include "../obk_config.h"

#if ENABLE_DRIVER_BKSDCARD

#include "../new_common.h"
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_local.h"

#include "include.h"
#include "sd_card.h"
#include "sdio_host.h"

#define BKSD_BLOCK_LEN     (512)
#define BKSD_DUMP_BYTES    (64)
#define BKSD_MAX_READ      (8)
#define BKSD_MAX_ZERO      (256)
#define BKSD_PART_START    (2048)
#define BKSD_RSVD          (32)
#define BKSD_SPC           (64)
#define BKSD_TMPVAL2       ((256 * BKSD_SPC + 2) / 2)

static int g_sdDetectPin = 23;
static int g_sdDetectActiveLow = 1;
static int g_sdMounted;
static int g_sdPresent = -1;
static int g_sdLastError;
static UINT32 g_sdSectors;
static sd_card_info_t g_sdInfo;
static UINT32 g_sdReads;
static UINT32 g_sdReadFail;
static int g_sdLastBlock = -1;
static UINT32 g_sdDumpOff;
static UINT32 g_sdFatSize;
static UINT32 g_sdClusters;
static UINT32 g_sdPartStart;
static UINT32 g_sdClearFrom;
static UINT32 g_sdClearTo;
static UINT32 g_sdZeroed;
static int g_sdFormatResult = -1;
static int g_sdEraseFailed;
static UINT8 g_sdHead[8];
static UINT8 g_sdTail[2];
static UINT8 g_sdBuf[BKSD_BLOCK_LEN] __attribute__((aligned(4)));

static int BKSDCard_ReadDetect(void)
{
	int lvl;

	if (g_sdDetectPin < 0)
		return 1;
	lvl = HAL_PIN_ReadDigitalInput(g_sdDetectPin);
	return g_sdDetectActiveLow ? !lvl : lvl;
}

static void BKSDCard_SetupDetect(void)
{
	if (g_sdDetectPin < 0)
		return;
	if (g_sdDetectActiveLow)
		HAL_PIN_Setup_Input_Pullup(g_sdDetectPin);
	else
		HAL_PIN_Setup_Input_Pulldown(g_sdDetectPin);
}

static const char *BKSDCard_FsResultName(int r)
{
	switch (r) {
	case 0:  return "FR_OK";
	case 1:  return "FR_DISK_ERR";
	case 2:  return "FR_INT_ERR";
	case 3:  return "FR_NOT_READY";
	case 4:  return "FR_NO_FILE";
	case 5:  return "FR_NO_PATH";
	case 6:  return "FR_INVALID_NAME";
	case 9:  return "FR_INVALID_OBJECT";
	case 11: return "FR_INVALID_DRIVE";
	case 12: return "FR_NOT_ENABLED";
	case 13: return "FR_NO_FILESYSTEM";
	default: return "?";
	}
}

static const char *BKSDCard_TypeName(void)
{
	return g_sdInfo.card_type ? "SDHC/SDXC" : "SDSC";
}

static int BKSDCard_Mount(void)
{
	bk_err_t err;

	if (g_sdMounted)
		return 0;

	err = bk_sdio_host_driver_init();
	if (err != BK_OK) {
		g_sdLastError = err;
		return err;
	}

	err = bk_sd_card_init();
	g_sdLastError = err;
	if (err != BK_OK) {
		bk_sdio_host_driver_deinit();
		return err;
	}

	if (bk_sd_card_get_card_info(&g_sdInfo) != BK_OK)
		memset(&g_sdInfo, 0, sizeof(g_sdInfo));
	g_sdSectors = bk_sd_card_get_card_size();
	g_sdMounted = 1;
	return 0;
}

static void BKSDCard_Unmount(void)
{
	if (!g_sdMounted)
		return;
	BKSDCardFs_Unmount();
	bk_sd_card_deinit();
	bk_sdio_host_driver_deinit();
	g_sdMounted = 0;
	g_sdSectors = 0;
	memset(&g_sdInfo, 0, sizeof(g_sdInfo));
}

void BKSDCard_OnEverySecond(void)
{
	int present = BKSDCard_ReadDetect();

	if (present == g_sdPresent)
		return;
	g_sdPresent = present;

	if (!present && g_sdMounted) {
		BKSDCard_Unmount();
		ADDLOG_INFO(LOG_FEATURE_DRV, "SDCard: card removed, unmounted");
	} else {
		ADDLOG_INFO(LOG_FEATURE_DRV, "SDCard: card %s", present ? "inserted" : "absent");
	}
}

void BKSDCard_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	UINT32 mib;

	if (bPreState)
		return;

	if (g_sdPresent <= 0) {
		hprintf255(request, "<h5>SD card: %s, detect P%i reads %i</h5>",
		           g_sdPresent < 0 ? "not polled yet" : "no card in slot",
		           g_sdDetectPin, g_sdDetectPin < 0 ? -1 : HAL_PIN_ReadDigitalInput(g_sdDetectPin));
		return;
	}
	if (!g_sdMounted) {
		hprintf255(request, "<h5>SD card: present, not mounted, last error %i</h5>", g_sdLastError);
		return;
	}
	mib = g_sdSectors >> 11;
	hprintf255(request, "<h5>SD card: %s %uMiB, %u sectors, class %u</h5>",
	           BKSDCard_TypeName(), mib, g_sdSectors, g_sdInfo.class);
	hprintf255(request, "<h5>SD reads %u, failed %u, state %i</h5>",
	           g_sdReads, g_sdReadFail, (int)bk_sd_card_get_card_state());
	if (BKSDCardFs_IsMounted())
		hprintf255(request, "<h5>SD filesystem: mounted, base LBA %u</h5>", BKSDCardFs_GetBase());
	else if (BKSDCardFs_LastResult() >= 0)
		hprintf255(request, "<h5>SD filesystem: not mounted, %s (%i), base LBA %u</h5>",
		           BKSDCard_FsResultName(BKSDCardFs_LastResult()), BKSDCardFs_LastResult(),
		           BKSDCardFs_GetBase());
	hprintf255(request, "<h5>SD fs sector reads %u, failed %u</h5>",
	           BKSDCardFs_Reads(), BKSDCardFs_ReadFails());
	if (g_sdFormatResult >= 0) {
		hprintf255(request, "<h5>SD format: result %i, part %u, fat %u sectors, %u clusters, zeroed %u</h5>",
		           g_sdFormatResult, g_sdPartStart, g_sdFatSize, g_sdClusters, g_sdZeroed);
		if (g_sdEraseFailed)
			hprintf255(request, "<h5>SD format INCOMPLETE: tables not cleared, zero blocks %u to %u</h5>",
			           g_sdClearFrom, g_sdClearTo);
	}
	if (g_sdLastBlock >= 0) {
		UINT32 o = g_sdDumpOff;
		hprintf255(request, "<h5>SD block %i at %u: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X</h5>",
		           g_sdLastBlock, o,
		           g_sdBuf[o + 0], g_sdBuf[o + 1], g_sdBuf[o + 2], g_sdBuf[o + 3],
		           g_sdBuf[o + 4], g_sdBuf[o + 5], g_sdBuf[o + 6], g_sdBuf[o + 7],
		           g_sdBuf[o + 8], g_sdBuf[o + 9], g_sdBuf[o + 10], g_sdBuf[o + 11],
		           g_sdBuf[o + 12], g_sdBuf[o + 13], g_sdBuf[o + 14], g_sdBuf[o + 15]);
	}
}

static commandResult_t CMD_SDCardMount(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
	int err;

	if (!BKSDCard_ReadDetect()) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardMount: no card detected on P%i", g_sdDetectPin);
		return CMD_RES_ERROR;
	}
	err = BKSDCard_Mount();
	if (err) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardMount: init failed, error %i", err);
		return CMD_RES_ERROR;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardMount: %s, %u sectors, %uMiB, ver %u, rca %u",
	            BKSDCard_TypeName(), g_sdSectors, g_sdSectors >> 11,
	            g_sdInfo.card_version, g_sdInfo.relative_card_addr);
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardUnmount(const void *context, const char *cmd,
                                         const char *args, int cmdFlags)
{
	BKSDCard_Unmount();
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardUnmount: done");
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardInfo(const void *context, const char *cmd,
                                      const char *args, int cmdFlags)
{
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCard: detect P%i activeLow %i, present %i, mounted %i",
	            g_sdDetectPin, g_sdDetectActiveLow, g_sdPresent, g_sdMounted);
	if (!g_sdMounted) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "SDCard: not mounted, last error %i", g_sdLastError);
		return CMD_RES_OK;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCard: %s, ver %u, class %u, rca %u",
	            BKSDCard_TypeName(), g_sdInfo.card_version, g_sdInfo.class,
	            g_sdInfo.relative_card_addr);
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCard: %u sectors, %uMiB, state %i, reads %u, failed %u",
	            g_sdSectors, g_sdSectors >> 11, (int)bk_sd_card_get_card_state(),
	            g_sdReads, g_sdReadFail);
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardDetectPin(const void *context, const char *cmd,
                                           const char *args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardDetectPin: needs a pin, -1 to disable");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_sdDetectPin = Tokenizer_GetArgInteger(0);
	if (Tokenizer_GetArgsCount() > 1)
		g_sdDetectActiveLow = Tokenizer_GetArgInteger(1) ? 1 : 0;
	BKSDCard_SetupDetect();
	g_sdPresent = -1;
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardDetectPin: P%i, activeLow %i",
	            g_sdDetectPin, g_sdDetectActiveLow);
	return CMD_RES_OK;
}

static void BKSDCard_Put16(UINT8 *p, UINT32 v)
{
	p[0] = (UINT8)v;
	p[1] = (UINT8)(v >> 8);
}

static void BKSDCard_Put32(UINT8 *p, UINT32 v)
{
	p[0] = (UINT8)v;
	p[1] = (UINT8)(v >> 8);
	p[2] = (UINT8)(v >> 16);
	p[3] = (UINT8)(v >> 24);
}

static int BKSDCard_WriteOne(UINT32 lba)
{
	return bk_sd_card_write_blocks(g_sdBuf, lba, 1) == 0 ? 0 : -1;
}

static int BKSDCard_FormatFat32(UINT32 total)
{
	UINT32 part = BKSD_PART_START;
	UINT32 psec, fatsz, clusters, fatBase, rootBase, i;

	if (total <= part + 8u)
		return -1;
	psec = total - part;

	fatsz = (psec - BKSD_RSVD + BKSD_TMPVAL2 - 1) / BKSD_TMPVAL2;
	if (fatsz == 0)
		return -2;
	if (psec < BKSD_RSVD + 2 * fatsz)
		return -3;
	clusters = (psec - BKSD_RSVD - 2 * fatsz) / BKSD_SPC;
	if (clusters < 65525)
		return -4;

	fatBase = part + BKSD_RSVD;
	rootBase = fatBase + 2 * fatsz;

	memset(g_sdBuf, 0, BKSD_BLOCK_LEN);
	g_sdBuf[446 + 0] = 0x00;
	g_sdBuf[446 + 1] = 0xFE;
	g_sdBuf[446 + 2] = 0xFF;
	g_sdBuf[446 + 3] = 0xFF;
	g_sdBuf[446 + 4] = 0x0C;
	g_sdBuf[446 + 5] = 0xFE;
	g_sdBuf[446 + 6] = 0xFF;
	g_sdBuf[446 + 7] = 0xFF;
	BKSDCard_Put32(&g_sdBuf[446 + 8], part);
	BKSDCard_Put32(&g_sdBuf[446 + 12], psec);
	g_sdBuf[510] = 0x55;
	g_sdBuf[511] = 0xAA;
	if (BKSDCard_WriteOne(0))
		return -5;

	memset(g_sdBuf, 0, BKSD_BLOCK_LEN);
	g_sdBuf[0] = 0xEB;
	g_sdBuf[1] = 0x58;
	g_sdBuf[2] = 0x90;
	memcpy(&g_sdBuf[3], "MSWIN4.1", 8);
	BKSDCard_Put16(&g_sdBuf[11], BKSD_BLOCK_LEN);
	g_sdBuf[13] = BKSD_SPC;
	BKSDCard_Put16(&g_sdBuf[14], BKSD_RSVD);
	g_sdBuf[16] = 2;
	BKSDCard_Put16(&g_sdBuf[17], 0);
	BKSDCard_Put16(&g_sdBuf[19], 0);
	g_sdBuf[21] = 0xF8;
	BKSDCard_Put16(&g_sdBuf[22], 0);
	BKSDCard_Put16(&g_sdBuf[24], 63);
	BKSDCard_Put16(&g_sdBuf[26], 255);
	BKSDCard_Put32(&g_sdBuf[28], part);
	BKSDCard_Put32(&g_sdBuf[32], psec);
	BKSDCard_Put32(&g_sdBuf[36], fatsz);
	BKSDCard_Put16(&g_sdBuf[40], 0);
	BKSDCard_Put16(&g_sdBuf[42], 0);
	BKSDCard_Put32(&g_sdBuf[44], 2);
	BKSDCard_Put16(&g_sdBuf[48], 1);
	BKSDCard_Put16(&g_sdBuf[50], 6);
	g_sdBuf[64] = 0x80;
	g_sdBuf[66] = 0x29;
	BKSDCard_Put32(&g_sdBuf[67], 0x4F424B32);
	memcpy(&g_sdBuf[71], "NO NAME    ", 11);
	memcpy(&g_sdBuf[82], "FAT32   ", 8);
	g_sdBuf[510] = 0x55;
	g_sdBuf[511] = 0xAA;
	if (BKSDCard_WriteOne(part))
		return -6;
	if (BKSDCard_WriteOne(part + 6))
		return -7;

	memset(g_sdBuf, 0, BKSD_BLOCK_LEN);
	BKSDCard_Put32(&g_sdBuf[0], 0x41615252);
	BKSDCard_Put32(&g_sdBuf[484], 0x61417272);
	BKSDCard_Put32(&g_sdBuf[488], clusters - 1);
	BKSDCard_Put32(&g_sdBuf[492], 3);
	g_sdBuf[510] = 0x55;
	g_sdBuf[511] = 0xAA;
	if (BKSDCard_WriteOne(part + 1))
		return -8;
	if (BKSDCard_WriteOne(part + 7))
		return -9;

	g_sdClearFrom = fatBase;
	g_sdClearTo = rootBase + BKSD_SPC - 1;
	if (bk_sd_card_erase(fatBase, rootBase + BKSD_SPC - 1) != 0)
		g_sdEraseFailed = 1;

	memset(g_sdBuf, 0, BKSD_BLOCK_LEN);
	BKSDCard_Put32(&g_sdBuf[0], 0x0FFFFFF8);
	BKSDCard_Put32(&g_sdBuf[4], 0xFFFFFFFF);
	BKSDCard_Put32(&g_sdBuf[8], 0x0FFFFFFF);
	if (BKSDCard_WriteOne(fatBase))
		return -10;
	if (BKSDCard_WriteOne(fatBase + fatsz))
		return -11;

	memset(g_sdBuf, 0, BKSD_BLOCK_LEN);
	for (i = 0; i < BKSD_SPC; i++) {
		if (BKSDCard_WriteOne(rootBase + i))
			return -12;
	}

	bk_sd_card_rw_sync();
	g_sdFatSize = fatsz;
	g_sdClusters = clusters;
	g_sdPartStart = part;
	return 0;
}

static commandResult_t CMD_SDCardFormat(const void *context, const char *cmd,
                                        const char *args, int cmdFlags)
{
	UINT32 given, total;
	int r;

	if (!g_sdMounted) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardFormat: card not mounted");
		return CMD_RES_ERROR;
	}
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardFormat: pass the exact sector count to confirm");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	given = (UINT32)strtoul(Tokenizer_GetArg(0), NULL, 0);
	total = g_sdSectors;
	if (given != total) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardFormat: refused, card has %u sectors, not %u", total, given);
		return CMD_RES_BAD_ARGUMENT;
	}

	BKSDCardFs_Unmount();
	g_sdEraseFailed = 0;
	g_sdZeroed = 0;
	r = BKSDCard_FormatFat32(total);
	if (r) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardFormat: failed at step %i", r);
		g_sdFormatResult = r;
		return CMD_RES_ERROR;
	}
	g_sdFormatResult = 0;
	if (g_sdEraseFailed) {
		ADDLOG_ERROR(LOG_FEATURE_CMD,
		             "SDCardFormat: INCOMPLETE, the card refused to erase, clear blocks %u to %u with SDCardZero before using it",
		             g_sdClearFrom, g_sdClearTo);
		return CMD_RES_ERROR;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardFormat: done, fat %u sectors, %u clusters", g_sdFatSize, g_sdClusters);
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardZero(const void *context, const char *cmd,
                                      const char *args, int cmdFlags)
{
	UINT32 lba, cnt, i;

	if (!g_sdMounted) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardZero: card not mounted");
		return CMD_RES_ERROR;
	}
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardZero: needs a start block and a count");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	lba = (UINT32)strtoul(Tokenizer_GetArg(0), NULL, 0);
	cnt = (UINT32)strtoul(Tokenizer_GetArg(1), NULL, 0);
	if (cnt > BKSD_MAX_ZERO)
		cnt = BKSD_MAX_ZERO;
	if (g_sdSectors && lba + cnt > g_sdSectors) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardZero: range past the end");
		return CMD_RES_BAD_ARGUMENT;
	}
	memset(g_sdBuf, 0, BKSD_BLOCK_LEN);
	for (i = 0; i < cnt; i++) {
		if (BKSDCard_WriteOne(lba + i)) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardZero: write failed at %u", lba + i);
			return CMD_RES_ERROR;
		}
	}
	bk_sd_card_rw_sync();
	g_sdZeroed += cnt;
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardDump(const void *context, const char *cmd,
                                      const char *args, int cmdFlags)
{
	UINT32 off;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardDump: needs a byte offset");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	off = (UINT32)strtoul(Tokenizer_GetArg(0), NULL, 0);
	if (off > BKSD_BLOCK_LEN - 16) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardDump: offset must be at most %i", BKSD_BLOCK_LEN - 16);
		return CMD_RES_BAD_ARGUMENT;
	}
	g_sdDumpOff = off;
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardFsBase(const void *context, const char *cmd,
                                        const char *args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardFsBase: needs a starting LBA");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BKSDCardFs_Unmount();
	BKSDCardFs_SetBase((unsigned int)strtoul(Tokenizer_GetArg(0), NULL, 0));
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardFsBase: %u", BKSDCardFs_GetBase());
	return CMD_RES_OK;
}

static commandResult_t CMD_SDCardFs(const void *context, const char *cmd,
                                    const char *args, int cmdFlags)
{
	int on = 1, r;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() > 0)
		on = Tokenizer_GetArgInteger(0) ? 1 : 0;

	if (!on) {
		BKSDCardFs_Unmount();
		ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardFs: unmounted");
		return CMD_RES_OK;
	}
	if (!g_sdMounted) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardFs: card not mounted, run SDCardMount first");
		return CMD_RES_ERROR;
	}
	r = BKSDCardFs_Mount();
	ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardFs: f_mount returned %s (%i)",
	            BKSDCard_FsResultName(r), r);
	return r ? CMD_RES_ERROR : CMD_RES_OK;
}

static commandResult_t CMD_SDCardRead(const void *context, const char *cmd,
                                      const char *args, int cmdFlags)
{
	UINT32 block, count, i;
	char line[3 * 16 + 2];
	bk_err_t err;

	if (!g_sdMounted) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardRead: card not mounted");
		return CMD_RES_ERROR;
	}
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardRead: needs a block number");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	block = (UINT32)strtoul(Tokenizer_GetArg(0), NULL, 0);
	count = Tokenizer_GetArgsCount() > 1 ? (UINT32)Tokenizer_GetArgInteger(1) : 1;
	if (g_sdSectors && block >= g_sdSectors) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardRead: block %u past the end, card has %u", block, g_sdSectors);
		return CMD_RES_BAD_ARGUMENT;
	}
	if (count < 1)
		count = 1;
	if (count > BKSD_MAX_READ)
		count = BKSD_MAX_READ;

	for (; count; count--, block++) {
		err = bk_sd_card_read_blocks(g_sdBuf, block, 1);
		if (err != BK_OK) {
			g_sdReadFail++;
			ADDLOG_ERROR(LOG_FEATURE_CMD, "SDCardRead: block %u failed, error %i", block, err);
			return CMD_RES_ERROR;
		}
		g_sdReads++;
		memcpy(g_sdHead, g_sdBuf, sizeof(g_sdHead));
		g_sdTail[0] = g_sdBuf[510];
		g_sdTail[1] = g_sdBuf[511];
		g_sdLastBlock = (int)block;
		ADDLOG_INFO(LOG_FEATURE_CMD, "SDCardRead: block %u, tail %02X%02X",
		            block, g_sdBuf[510], g_sdBuf[511]);
		for (i = 0; i < BKSD_DUMP_BYTES; i += 16) {
			snprintf(line, sizeof(line),
			         "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
			         g_sdBuf[i + 0], g_sdBuf[i + 1], g_sdBuf[i + 2], g_sdBuf[i + 3],
			         g_sdBuf[i + 4], g_sdBuf[i + 5], g_sdBuf[i + 6], g_sdBuf[i + 7],
			         g_sdBuf[i + 8], g_sdBuf[i + 9], g_sdBuf[i + 10], g_sdBuf[i + 11],
			         g_sdBuf[i + 12], g_sdBuf[i + 13], g_sdBuf[i + 14], g_sdBuf[i + 15]);
			ADDLOG_INFO(LOG_FEATURE_CMD, "  %03X: %s", i, line);
		}
	}
	return CMD_RES_OK;
}

void BKSDCard_StopDriver(void)
{
	BKSDCardFs_Unmount();
	BKSDCard_Unmount();
}

void BKSDCard_Init(void)
{
	g_sdMounted = 0;
	g_sdPresent = -1;
	g_sdSectors = 0;
	g_sdLastError = 0;
	g_sdReads = 0;
	g_sdReadFail = 0;
	g_sdLastBlock = -1;
	BKSDCard_SetupDetect();

	//cmddetail:{"name":"SDCardMount","args":"",
	//cmddetail:"descr":"Initialises the card in the slot and reports its type and capacity.",
	//cmddetail:"fn":"CMD_SDCardMount","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardMount"}
	CMD_RegisterCommand("SDCardMount", CMD_SDCardMount, NULL);
	//cmddetail:{"name":"SDCardUnmount","args":"",
	//cmddetail:"descr":"Releases the card and powers the host down.",
	//cmddetail:"fn":"CMD_SDCardUnmount","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardUnmount"}
	CMD_RegisterCommand("SDCardUnmount", CMD_SDCardUnmount, NULL);
	//cmddetail:{"name":"SDCardInfo","args":"",
	//cmddetail:"descr":"Prints the detect pin state, card identity and read counters.",
	//cmddetail:"fn":"CMD_SDCardInfo","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardInfo"}
	CMD_RegisterCommand("SDCardInfo", CMD_SDCardInfo, NULL);
	//cmddetail:{"name":"SDCardDetectPin","args":"[Pin][ActiveLow]",
	//cmddetail:"descr":"Sets the card detect pin, or -1 to assume a card is always present. ActiveLow defaults to one.",
	//cmddetail:"fn":"CMD_SDCardDetectPin","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardDetectPin 23 1"}
	CMD_RegisterCommand("SDCardDetectPin", CMD_SDCardDetectPin, NULL);
	//cmddetail:{"name":"SDCardRead","args":"[Block][Count]",
	//cmddetail:"descr":"Reads up to eight blocks and dumps the first sixty four bytes of each.",
	//cmddetail:"fn":"CMD_SDCardRead","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardRead 0"}
	CMD_RegisterCommand("SDCardRead", CMD_SDCardRead, NULL);
	//cmddetail:{"name":"SDCardFs","args":"[Enable]",
	//cmddetail:"descr":"Mounts the FAT filesystem on the card, or unmounts it when given zero.",
	//cmddetail:"fn":"CMD_SDCardFs","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardFs 1"}
	CMD_RegisterCommand("SDCardFs", CMD_SDCardFs, NULL);
	//cmddetail:{"name":"SDCardDump","args":"[Offset]",
	//cmddetail:"descr":"Selects which sixteen bytes of the last read block are shown on the status page.",
	//cmddetail:"fn":"CMD_SDCardDump","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardDump 446"}
	CMD_RegisterCommand("SDCardDump", CMD_SDCardDump, NULL);
	//cmddetail:{"name":"SDCardFsBase","args":"[StartLBA]",
	//cmddetail:"descr":"Offsets every filesystem sector access, so a partition can be mounted as if it were the whole card.",
	//cmddetail:"fn":"CMD_SDCardFsBase","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardFsBase 2048"}
	CMD_RegisterCommand("SDCardFsBase", CMD_SDCardFsBase, NULL);
	//cmddetail:{"name":"SDCardFormat","args":"[SectorCount]",
	//cmddetail:"descr":"Writes a fresh FAT32 filesystem, destroying everything on the card. The card's exact sector count must be passed to confirm.",
	//cmddetail:"fn":"CMD_SDCardFormat","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardFormat 124735488"}
	CMD_RegisterCommand("SDCardFormat", CMD_SDCardFormat, NULL);
	//cmddetail:{"name":"SDCardZero","args":"[Block][Count]",
	//cmddetail:"descr":"Writes zeroes over a range of blocks, at most two hundred and fifty six per call.",
	//cmddetail:"fn":"CMD_SDCardZero","file":"driver/drv_sdcard.c","requires":"",
	//cmddetail:"examples":"SDCardZero 2080 256"}
	CMD_RegisterCommand("SDCardZero", CMD_SDCardZero, NULL);
}

#endif

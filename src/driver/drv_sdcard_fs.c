#include "../obk_config.h"

#if ENABLE_DRIVER_BKSDCARD

#include "include.h"
#include "sd_card.h"
#include "diskio.h"
#include "ff.h"

static FATFS g_fatfs;
static int g_fsMounted;
static int g_fsResult = -1;
static unsigned int g_fsBase;
static unsigned int g_fsReads;
static unsigned int g_fsReadFail;

DSTATUS disk_initialize(uint8 pdrv)
{
	if (pdrv != 0)
		return STA_NOINIT;
	if (bk_sd_card_get_card_size() == 0)
		return STA_NOINIT;
	return 0;
}

DSTATUS disk_status(uint8 pdrv)
{
	if (pdrv != 0)
		return STA_NOINIT;
	if (bk_sd_card_get_card_size() == 0)
		return STA_NOINIT;
	return 0;
}

DRESULT disk_read(uint8 pdrv, uint8 *buff, uint32 start_sector, uint32 sector_cnt)
{
	if (pdrv != 0)
		return RES_PARERR;
	if (buff == 0 || sector_cnt == 0)
		return RES_PARERR;
	g_fsReads++;
	if (bk_sd_card_read_blocks(buff, start_sector + g_fsBase, sector_cnt) != 0) {
		g_fsReadFail++;
		return RES_ERROR;
	}
	return RES_OK;
}

int BKSDCardFs_Mount(void)
{
	if (g_fsMounted)
		return 0;
	g_fsResult = (int)f_mount(0, &g_fatfs);
	if (g_fsResult != FR_OK)
		return g_fsResult;

	g_fsResult = (int)chk_mounted_con(&g_fatfs, DISK_TYPE_SD);
	if (g_fsResult == FR_OK)
		g_fsMounted = 1;
	return g_fsResult;
}

int BKSDCardFs_Unmount(void)
{
	if (!g_fsMounted)
		return 0;
	f_unmount(&g_fatfs);
	g_fsMounted = 0;
	g_fsResult = -1;
	return 0;
}

void BKSDCardFs_SetBase(unsigned int lba)
{
	g_fsBase = lba;
}

unsigned int BKSDCardFs_GetBase(void)
{
	return g_fsBase;
}

unsigned int BKSDCardFs_Reads(void)
{
	return g_fsReads;
}

unsigned int BKSDCardFs_ReadFails(void)
{
	return g_fsReadFail;
}

int BKSDCardFs_IsMounted(void)
{
	return g_fsMounted;
}

int BKSDCardFs_LastResult(void)
{
	return g_fsResult;
}

#endif

/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define DEV_RAM		1	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		0	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* adaptor layer start                                                      */
/*-----------------------------------------------------------------------*/

#include "wm_sdio_host.h"
#include <string.h>
#include "wm_include.h"

extern int wm_sd_card_set_blocklen(uint32_t blocklen);

#define BLOCK_SIZE  512
#define TRY_COUNT   10

static uint32_t fs_rca;


static int RAM_disk_status(void)
{
	return 0;
}

static int MMC_disk_status(void)
{
	return 0;
}

static int USB_disk_status(void)
{
	return 0;
}

static int RAM_disk_initialize(void)
{
	return 0;
}

static int MMC_disk_initialize(void)
{
	int ret;

	ret= sdh_card_init(&fs_rca);
	if(ret)
		goto end;
	ret = wm_sd_card_set_bus_width(fs_rca, 2);
	if(ret)
		goto end;
	ret = wm_sd_card_set_blocklen(BLOCK_SIZE); //512
	if(ret)
		goto end;
end:
	return ret;
}

static int USB_disk_initialize(void)
{
	return 0;
}

static int RAM_disk_read(	BYTE *buff, LBA_t sector, UINT count)
{
	return 0;
}

static int MMC_disk_read(	BYTE *buff, LBA_t sector, UINT count)
{
	int ret, i;
	int buflen = BLOCK_SIZE*count;
    BYTE *rdbuff = buff;

	if (((u32)buff)&0x3) /*non aligned 4*/
	{
	    rdbuff = tls_mem_alloc(buflen);
		if (rdbuff == NULL)
		{
			return -1;
		}
	}
    
	for( i=0; i<TRY_COUNT; i++ )
	{   
	    if(count == 1)
	    {
            ret = wm_sd_card_block_read(fs_rca, sector, (char *)rdbuff);
        }
        else if(count > 1)
        {
		    ret = wm_sd_card_blocks_read(fs_rca, sector, (char *)rdbuff, buflen);
        }
		if( ret == 0 ) 
			break;
	}

    if(rdbuff != buff)
    {
        if(ret == 0)
        {
            memcpy(buff, rdbuff, buflen);
        }
        tls_mem_free(rdbuff);
    }

	return ret;
}

static int USB_disk_read(	BYTE *buff, LBA_t sector, UINT count)
{
	return 0;
}

static int RAM_disk_write(	BYTE *buff, LBA_t sector, UINT count)
{
	return 0;
}

static int MMC_disk_write(	BYTE *buff, LBA_t sector, UINT count)
{
	int ret, i;
	int buflen = BLOCK_SIZE*count;
    BYTE *wrbuff = buff;
    
	if (((u32)buff)&0x3)
	{
	    wrbuff = tls_mem_alloc(buflen);
		if (wrbuff == NULL) /*non aligned 4*/
		{
			return -1;
		}
        memcpy(wrbuff, buff, buflen);
	}
	
	for( i = 0; i < TRY_COUNT; i++ )
	{
	    if(count == 1)
	    {
            ret = wm_sd_card_block_write(fs_rca, sector, (char *)wrbuff);
        }
        else if(count > 1)
	    {
		    ret = wm_sd_card_blocks_write(fs_rca, sector, (char *)wrbuff, buflen);
	    }
		if( ret == 0 ) 
		{
			break;
		}
	}

    if(wrbuff != buff)
    {
        tls_mem_free(wrbuff);
    }

	return ret;
}

static int USB_disk_write(	BYTE *buff, LBA_t sector, UINT count)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* adaptor layer end                                                      */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;

	switch (pdrv) {
	case DEV_RAM :
		RAM_disk_status();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		MMC_disk_status();

		// translate the reslut code here
		stat &= ~STA_NOINIT;

		return stat;

	case DEV_USB :
		USB_disk_status();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		result = RAM_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		result = MMC_disk_initialize();

		// translate the reslut code here
		(result == 0)?(stat &= ~STA_NOINIT):(stat = STA_NOINIT);

		return stat;

	case DEV_USB :
		result = USB_disk_initialize();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_PARERR;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_read(buff, sector, count);

		// translate the reslut code here
		(result == 0)?(res = RES_OK):(res = RES_ERROR);

		return res;

	case DEV_USB :
		// translate the arguments here

		result = USB_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_PARERR;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_write((BYTE *)buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_write((BYTE *)buff, sector, count);

		// translate the reslut code here
		(result == 0)?(res = RES_OK):(res = RES_ERROR);

		return res;

	case DEV_USB :
		// translate the arguments here

		result = USB_disk_write((BYTE *)buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = 0;

	switch (pdrv) {
	case DEV_RAM :

		// Process of the command for the RAM drive

		return res;

	case DEV_MMC :

		// Process of the command for the MMC/SD card
		switch(cmd)
		{
#if (FF_FS_READONLY == 0)
			case CTRL_SYNC:
				res = RES_OK;
				break;
#endif
			case GET_SECTOR_COUNT:
				*(DWORD*)buff = SDCardInfo.CardCapacity / BLOCK_SIZE;
				res = RES_OK;
				break;
			case GET_SECTOR_SIZE:
				*(WORD*)buff = BLOCK_SIZE;
				res = RES_OK;
				break;
			case GET_BLOCK_SIZE:
				*(DWORD*)buff = SDCardInfo.CardBlockSize;
				res = RES_OK;
				break;
#if (FF_USE_TRIM == 1)
			case CTRL_TRIM:
				break;
#endif
			default:
				break;
		}

		return res;

	case DEV_USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}


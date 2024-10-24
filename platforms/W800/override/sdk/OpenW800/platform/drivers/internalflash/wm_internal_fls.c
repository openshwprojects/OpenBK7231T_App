/**
 * @file    wm_internal_fls.c
 *
 * @brief   flash Driver Module
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wm_dbg.h"
#include "wm_mem.h"
#include "list.h"
#include "wm_regs.h"
#include "wm_internal_flash.h"
#include "wm_flash_map.h"

static struct tls_inside_fls *inside_fls = NULL;

/**System parameter, default for 2M flash*/
unsigned int  TLS_FLASH_MESH_PARAM_ADDR      =        (0x81FA000UL);
unsigned int  TLS_FLASH_PARAM_DEFAULT        =		  (0x81FB000UL);
unsigned int  TLS_FLASH_PARAM1_ADDR          =		  (0x81FC000UL);
unsigned int  TLS_FLASH_PARAM2_ADDR          =		  (0x81FD000UL);
unsigned int  TLS_FLASH_PARAM_RESTORE_ADDR   =	      (0x81FE000UL);
unsigned int  TLS_FLASH_OTA_FLAG_ADDR        =	      (0x81FF000UL);
unsigned int  TLS_FLASH_END_ADDR             =		  (0x81FFFFFUL);


static vu32 read_first_value(void)
{
    return M32(RSA_BASE_ADDRESS);
}

static void writeEnable(void)
{
    M32(HR_FLASH_CMD_ADDR) = 0x6;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
}

unsigned char readRID(void)
{
    M32(HR_FLASH_CMD_ADDR) = 0x2c09F;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    return read_first_value() & 0xFF;
}

static void writeBpBit_for_1wreg(char cmp, char bp4, char bp3, char bp2, char bp1, char bp0)
{
    int status = 0;
    int bpstatus = 0;

    M32(HR_FLASH_CMD_ADDR) = 0x0C005;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status =  read_first_value() & 0xFF;

    M32(HR_FLASH_CMD_ADDR) = 0x0C035;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status  |=  (read_first_value() & 0xFF) << 8;
    bpstatus  = (bp4 << 6) | (bp3 << 5) | (bp2 << 4) | (bp1 << 3) | (bp0 << 2);
	if ((status & 0x407C) != (bpstatus|(cmp<<14)))
	{
	    status      = (status & 0xBF83) | bpstatus | (cmp << 14);
	    /*Write Enable*/
	    M32(HR_FLASH_CMD_ADDR) = 0x6;
	    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

	    M32(RSA_BASE_ADDRESS)  = status;
	    M32(HR_FLASH_CMD_ADDR) = 0x1A001;
	    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
	}
}

static void writeBpBit_for_2wreg(char cmp, char bp4, char bp3, char bp2, char bp1, char bp0)
{
    int status = 0;
    int bpstatus = 0;

    M32(HR_FLASH_CMD_ADDR) = 0x0C005;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status =  read_first_value() & 0xFF;

    M32(HR_FLASH_CMD_ADDR) = 0x0C035;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status  |=  (read_first_value() & 0xFF) << 8;

    /*Write Enable*/
    bpstatus  = (bp4 << 6) | (bp3 << 5) | (bp2 << 4) | (bp1 << 3) | (bp0 << 2);
	if ((bpstatus != (status & 0x7C)))
	{
	    bpstatus      = (status & 0x83) | bpstatus;

	    M32(HR_FLASH_CMD_ADDR) = 0x6;
	    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

	    M32(RSA_BASE_ADDRESS)  = bpstatus;
	    M32(HR_FLASH_CMD_ADDR) = 0xA001;
	    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
	}

	if (((status & 0x4000)>>8) != (cmp << 6))
	{
	    M32(HR_FLASH_CMD_ADDR) = 0x6;
	    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

	    status      = ((status>>8) & 0xBF) | (cmp << 6);
	    M32(RSA_BASE_ADDRESS)   = status;
	    M32(HR_FLASH_CMD_ADDR)  = 0xA031;
	    M32(HR_FLASH_CMD_START) = CMD_START_Msk;	
	}
}


static void writeESMTBpBit(char cmp, char bp4, char bp3, char bp2, char bp1, char bp0)
{
    int status = 0;
    int bpstatus = 0;

    M32(HR_FLASH_CMD_ADDR) = 0x0C005;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status =  read_first_value() & 0xFF;
    bpstatus  = (bp4 << 6) | (bp3 << 5) | (bp2 << 4) | (bp1 << 3) | (bp0 << 2);
    status      = (status & 0x83) | bpstatus;

    /*Write Enable*/
    M32(HR_FLASH_CMD_ADDR) = 0x6;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

    bpstatus  = (bp4 << 6) | (bp3 << 5) | (bp2 << 4) | (bp1 << 3) | (bp0 << 2);
    status      = (status & 0x83) | bpstatus | (cmp << 14);

    M32(RSA_BASE_ADDRESS)  = status;
    M32(HR_FLASH_CMD_ADDR) = 0x0A001;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;


    M32(HR_FLASH_CMD_ADDR) = 0x0C085;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status  =  read_first_value() & 0xFF;

    /*Write Enable*/
    M32(HR_FLASH_CMD_ADDR) = 0x6;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

    status		= (status & 0xBF) | (cmp << 6);
    M32(RSA_BASE_ADDRESS)  = status;
    M32(HR_FLASH_CMD_ADDR) = 0x0A0C1;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
}

static int flashunlock(void)
{
    switch(readRID())
    {
    case SPIFLASH_MID_GD:
	case SPIFLASH_MID_TSINGTENG:
        writeBpBit_for_1wreg(0, 0, 0, 0, 0, 0);
        break;
    case SPIFLASH_MID_PUYA:
	case SPIFLASH_MID_TSINGTENG_1MB_4MB:
		if (inside_fls->density == 0x100000)/*PUYA 1M Flash use 1 register to set lock/unlock*/
		{
			writeBpBit_for_1wreg(0, 0, 0, 0, 0, 0);
		}
		else
		{
			writeBpBit_for_2wreg(0, 0, 0, 0, 0, 0);
		}
		break;
	case SPIFLASH_MID_XTX:
	case SPIFLASH_MID_BOYA:
	case SPIFLASH_MID_FUDANMICRO:
	case SPIFLASH_MID_XMC:
		writeBpBit_for_2wreg(0, 0, 0, 0, 0, 0);
		break;
    case SPIFLASH_MID_ESMT:
        writeESMTBpBit(0, 0, 0, 0, 0, 0);
        break;
    default:
        return -1;
    }
    return 0;
}

static int flashlock(void)
{
    switch(readRID())
    {
    case SPIFLASH_MID_GD:
	case SPIFLASH_MID_TSINGTENG:	
        writeBpBit_for_1wreg(0, 1, 1, 0, 1, 0);
		break;
    case SPIFLASH_MID_PUYA:
	case SPIFLASH_MID_TSINGTENG_1MB_4MB:
		if (inside_fls->density == 0x100000) /*PUYA 1M Flash use 1 register to set lock/unlock*/
		{
			writeBpBit_for_1wreg(0, 1, 1, 0, 1, 0);
		}
		else
		{
			writeBpBit_for_2wreg(0, 1, 1, 0, 1, 0);
		}
        break;
	case SPIFLASH_MID_XTX:
	case SPIFLASH_MID_BOYA:
	case SPIFLASH_MID_FUDANMICRO:
	case SPIFLASH_MID_XMC:
		writeBpBit_for_2wreg(0, 1, 1, 0, 1, 0);
        break;
    case SPIFLASH_MID_ESMT:
        writeESMTBpBit(0, 1, 1, 0, 1, 0);
        break;
    default:
        return -1;/*do not clear QIO Mode*/
    }
    return 0;
}

static void writeLbBit_for_1wreg(unsigned int val)
{
    int status = 0;

    M32(HR_FLASH_CMD_ADDR) = 0x0C005;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status =  read_first_value() & 0xFF;

    M32(HR_FLASH_CMD_ADDR) = 0x0C035;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status  |=  (read_first_value() & 0xFF) << 8;

    /*Write Enable*/
    M32(HR_FLASH_CMD_ADDR) = 0x6;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

    status |= (val);

    M32(RSA_BASE_ADDRESS)  = status;
    M32(HR_FLASH_CMD_ADDR) = 0x1A001;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
}

static void writeLbBit_for_2wreg(unsigned int val)
{
    int status = 0;

    M32(HR_FLASH_CMD_ADDR) = 0x0C005;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status =  read_first_value() & 0xFF;

    M32(HR_FLASH_CMD_ADDR) = 0x0C035;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    status  |=  (read_first_value() & 0xFF) << 8;

    /*Write Enable*/
    M32(HR_FLASH_CMD_ADDR) = 0x6;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

	status |= (val);
    status      = (status>>8);
    M32(RSA_BASE_ADDRESS)   = status;
    M32(HR_FLASH_CMD_ADDR)  = 0xA031;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
}

static int programSR(unsigned int  cmd, unsigned long addr, unsigned char *buf,  unsigned int sz)
{
    unsigned long base_addr = 0;
    unsigned int size = 0;


    if (sz > INSIDE_FLS_PAGE_SIZE)
    {
        sz = INSIDE_FLS_PAGE_SIZE;
    }

    base_addr = RSA_BASE_ADDRESS;
    size = sz;
    while(size)
    {
        M32(base_addr) = *((unsigned long *)buf);
        base_addr += 4;
        buf += 4;
        size -= 4;
    }

    writeEnable();
    M32(HR_FLASH_CMD_ADDR) = cmd | ((sz - 1) << 16);
    M32(HR_FLASH_ADDR) = (addr & 0x1FFFFFF);
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

    return 0;
}


static int programPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{
    programSR(0x80009002, adr, buf, sz);
    return(0);
}

static int eraseSR(unsigned int cmd, unsigned long addr)
{
    /*Write Enable*/
    writeEnable();
    M32(HR_FLASH_CMD_ADDR) = cmd;
    M32(HR_FLASH_ADDR) = (addr & 0x1FFFFFF);
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

    return 0;
}

static int eraseSector (unsigned long adr)
{
    eraseSR(0x80000820, adr);

    return (0);                                  				// Finished without Errors
}
#if 0
/*only for w800 c400 flash*/
static int erasePage (unsigned long addr)
{
	eraseSR(0x80000881, addr);
    return (0);                                  				// Finished without Errors
}
#endif

static unsigned int getFlashDensity(void)
{
    unsigned char density = 0;

    M32(HR_FLASH_CMD_ADDR) = 0x2c09F;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;

    density = ((read_first_value() & 0xFFFFFF) >> 16) & 0xFF;
    //	printf("density %x\n", density);
    if (density && (density <= 0x21))  /*just limit to (1<<33UL) Byte*/
    {
        return (1 << density);
    }

    return 0;
}

int __readByCMD(unsigned char cmd, unsigned long addr, unsigned char *buf, unsigned long sz)
{
    int i = 0;
    int word = sz / 4;
    int byte = sz % 4;
    unsigned long addr_read;
	if (!(M32(HR_FLASH_CR)&0x1))/*non-QIO mode, only single line command can be used*/
	{
		if (cmd > 0x0B)
		{
			cmd = 0x0B;
		}
	}
	
    switch (cmd)
    {
    case 0x03:
        M32(HR_FLASH_CMD_ADDR) = 0x8000C003 | (((sz - 1) & 0x3FF) << 16);
        M32(HR_FLASH_ADDR) = addr & 0x1FFFFFF;
        M32(HR_FLASH_CMD_START) = CMD_START_Msk;
        break;
    case 0x0B:
        if((M32(HR_FLASH_CR) & 0x2) == 0x2)
        {
            M32(HR_FLASH_CMD_ADDR) = 0xB400C00B | (((sz - 1) & 0x3FF) << 16);
        }
        else
        {
            M32(HR_FLASH_CMD_ADDR) = 0xBC00C00B | (((sz - 1) & 0x3FF) << 16);
        }
        M32(HR_FLASH_ADDR) = addr & 0x1FFFFFF;
        M32(HR_FLASH_CMD_START) = CMD_START_Msk;
        break;
    case 0xBB:
        M32(HR_FLASH_CMD_ADDR) = 0xE400C0BB | (((sz - 1) & 0x3FF) << 16);
        M32(HR_FLASH_ADDR) = addr & 0x1FFFFFF;
        M32(HR_FLASH_CMD_START) = CMD_START_Msk;
        break;

    case 0xEB:
        M32(HR_FLASH_CMD_ADDR) = 0xEC00C0EB | (((sz - 1) & 0x3FF) << 16);
        M32(HR_FLASH_ADDR) = addr & 0x1FFFFFF;
        M32(HR_FLASH_CMD_START) = CMD_START_Msk;
        break;


    default:
        return -1;
    }

    //	printf("delay delay delay delay\n");
    //	dumpUint32("readByCMD RSA_BASE_ADDRESS", RSA_BASE_ADDRESS, sz/4);
    addr_read = RSA_BASE_ADDRESS;
    for(i = 0; i < word; i ++)
    {
        M32(buf) = M32(addr_read);
        buf += 4;
        addr_read += 4;
    }

    if(byte > 0)
    {
        M32(buf) = M32(addr_read);
        buf += 3;							//point last byte
        byte = 4 - byte;
        while(byte)
        {
            *buf = 0;
            buf --;
            byte --;
        }
    }
    return 0;
}

/**
 * @brief          This function is used to read data from the flash.
 *
 * @param[in]      cmd                 0xEB in QSPI mode; 0x0b in SPI mode.
 * @param[in]      addr                is byte offset addr for read from the flash.
 * @param[in]      buf                 is user for data buffer of flash read
 * @param[in]      len                 is byte length for read.
 *
 * @retval         TLS_FLS_STATUS_OK	    if read sucsess
 * @retval         TLS_FLS_STATUS_EPERM	    if inside fls does not initialized.
 *
 * @note           None
 */
int readByCMD(unsigned char cmd, unsigned long addr, unsigned char *buf, unsigned long sz)
{
    if (inside_fls == NULL)
    {
        return TLS_FLS_STATUS_EPERM;
    }

	tls_os_sem_acquire(inside_fls->fls_lock, 0);
	__readByCMD(cmd, addr, buf, sz);
	tls_os_sem_release(inside_fls->fls_lock);
    return TLS_FLS_STATUS_OK;
}

int config_flash_decrypt_param(uint32_t code_decrypt, uint32_t dbus_decrypt, uint32_t data_decrypt)
{
	FLASH_ENCRYPT_CTRL_Type encrypt_ctrl;
	encrypt_ctrl.w = M32(HR_FLASH_ENCRYPT_CTRL);
	encrypt_ctrl.b.code_decrypt = code_decrypt;
	encrypt_ctrl.b.dbus_decrypt = dbus_decrypt;
	encrypt_ctrl.b.data_decrypt = data_decrypt;
	M32(HR_FLASH_ENCRYPT_CTRL) = encrypt_ctrl.w;
	return 0;
}

int flashRead(unsigned long addr, unsigned char *buf, unsigned long sz)
{
#define INSIDE_FLS_MAX_RD_SIZE (1024)

    unsigned int flash_addr;
    unsigned int sz_pagenum = 0;
    unsigned int sz_remain = 0;
    int i = 0;
    int page_offset = addr & (INSIDE_FLS_PAGE_SIZE - 1);
	unsigned int max_size = 0;

	if ((page_offset == 0) 
		&& (((unsigned int)buf&0x3) == 0) 
		&& ((sz&0x3) == 0))/*Use 4-bytes aligned and buf must be 4 times, sz must be 4 times*/
	{
		flash_addr = addr;
		if (sz >= 512)
		{
			max_size = INSIDE_FLS_MAX_RD_SIZE;
		}
		else
		{
			max_size = INSIDE_FLS_PAGE_SIZE;
		}

		sz_pagenum = sz / max_size;
		sz_remain = sz % max_size;
		for (i = 0; i < sz_pagenum; i++)
		{
			__readByCMD(0xEB, flash_addr, (unsigned char *)buf, max_size);
			buf 		+= max_size;
			flash_addr	+= max_size;
		}
		
		if (sz_remain)
		{
			__readByCMD(0xEB, flash_addr, (unsigned char *)buf, sz_remain);
		}

	}
	else
	{
	    char *cache = NULL;

	    cache = tls_mem_alloc(INSIDE_FLS_PAGE_SIZE);
	    if (cache == NULL)
	    {
	        TLS_DBGPRT_ERR("allocate sector cache memory fail!\n");
	        return TLS_FLS_STATUS_ENOMEM;
	    }
	    flash_addr = addr & ~(INSIDE_FLS_PAGE_SIZE - 1);
	    __readByCMD(0xEB, flash_addr, (unsigned char *)cache, INSIDE_FLS_PAGE_SIZE);
	    if (sz > INSIDE_FLS_PAGE_SIZE - page_offset)
	    {
	        MEMCPY(buf, cache + page_offset, INSIDE_FLS_PAGE_SIZE - page_offset);
	        buf += INSIDE_FLS_PAGE_SIZE - page_offset;
	        flash_addr 	+= INSIDE_FLS_PAGE_SIZE;

	        sz_pagenum = (sz - (INSIDE_FLS_PAGE_SIZE - page_offset)) / INSIDE_FLS_PAGE_SIZE;
	        sz_remain = (sz - (INSIDE_FLS_PAGE_SIZE - page_offset)) % INSIDE_FLS_PAGE_SIZE;
	        for (i = 0; i < sz_pagenum; i++)
	        {

	            __readByCMD(0xEB, flash_addr, (unsigned char *)cache, INSIDE_FLS_PAGE_SIZE);
	            MEMCPY(buf, cache, INSIDE_FLS_PAGE_SIZE);
	            buf 		+= INSIDE_FLS_PAGE_SIZE;
	            flash_addr 	+= INSIDE_FLS_PAGE_SIZE;
	        }

	        if (sz_remain)
	        {
	            __readByCMD(0xEB, flash_addr, (unsigned char *)cache, sz_remain + (4- sz_remain%4));
	            MEMCPY(buf, cache, sz_remain);
	        }
	    }
	    else
	    {
	        MEMCPY(buf, cache + page_offset, sz);
	    }
	    tls_mem_free(cache);
	}

    return 0;
}

/**
 * @brief           This function is used to unlock flash protect area [0x0~0x2000].
 *
 * @param	   None
 *
 * @return         0-success,non-zero-failure
 *
 * @note           None
 */
int tls_flash_unlock(void)
{
	int ret = 0;
    if (inside_fls == NULL)
    {
        return TLS_FLS_STATUS_EPERM;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);
    ret = flashunlock();
    tls_os_sem_release(inside_fls->fls_lock);

	return ret;
}

/**
 * @brief           This function is used to lock flash protect area [0x0~0x2000].
 *
 * @param	   None
 *
 * @return        0-success,non-zero-failure
 *
 * @note           None
 */
int tls_flash_lock(void)
{
	int ret = 0;
    if (inside_fls == NULL)
    {
        return TLS_FLS_STATUS_EPERM;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);
    ret = flashlock();
    tls_os_sem_release(inside_fls->fls_lock);

	return ret;

}


/**
 * @brief           This function is used to semaphore protect.
 *
 * @param	   None
 *
 * @return         None
 *
 * @note           None
 */
void tls_fls_sem_lock(void)
{
    if (inside_fls == NULL)
    {
        return;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);
}


/**
 * @brief           This function is used to semaphore protect cancel.
 *
 * @param	   None
 *
 * @return         None
 *
 * @note           None
 */
void tls_fls_sem_unlock(void)
{
    if (inside_fls == NULL)
    {
        return;
    }
    tls_os_sem_release(inside_fls->fls_lock);
}

/**
 * @brief          This function is used to read the unique id of the internal flash.
 *
 * @param[out]      uuid                 Specified the address to save the uuid, the length must be greater than or equals to 18 bytes.
 *
 * @retval         TLS_FLS_STATUS_OK	    if read sucsess
 * @retval         TLS_FLS_STATUS_EIO	    if read fail
 *
 * @note           The uuid's length must be greater than or equals to 18 bytes.
 */
int tls_fls_read_unique_id(unsigned  char *uuid)
{
	unsigned int value = 0;
	unsigned int addr_read = 0;
	int i = 0;
	int len;
	unsigned char FLASH_BUF[20];
	unsigned char *addr = &FLASH_BUF[0];
	int dumy_bytes = 0;
	int uni_bytes = 0;
	unsigned char rid;
	int word;
	int byte;
    if (inside_fls == NULL)
    {
    	return TLS_FLS_STATUS_EPERM;
    }

	tls_os_sem_acquire(inside_fls->fls_lock, 0);
	memset(uuid, 0xFF, 18);
	rid = readRID();
	switch(rid)
	{
		case SPIFLASH_MID_GD:
		case SPIFLASH_MID_PUYA:
		case SPIFLASH_MID_TSINGTENG:
		case SPIFLASH_MID_TSINGTENG_1MB_4MB:
			dumy_bytes = 4;
			uni_bytes = 16;
			break;
		case SPIFLASH_MID_WINBOND:
		case SPIFLASH_MID_FUDANMICRO:
		case SPIFLASH_MID_BOYA:
		case SPIFLASH_MID_XMC:
			dumy_bytes = 4;
			uni_bytes = 8;
			break;
		case SPIFLASH_MID_ESMT:
		case SPIFLASH_MID_XTX:
		default:
			tls_os_sem_release(inside_fls->fls_lock);
			return -1;
	}
	uuid[0] = rid;
	uuid[1] = (unsigned char)(uni_bytes & 0xFF);
	len = dumy_bytes + uni_bytes;
	word = len/4;
	byte = len%4;
	
	value = 0xC04B|((len-1) << 16);
	M32(HR_FLASH_CMD_ADDR) = value;
	M32(HR_FLASH_CMD_START) = CMD_START_Msk;		

	addr_read = RSA_BASE_ADDRESS;
	for(i = 0;i < word; i ++)
	{
		M32(addr) = M32(addr_read);	
		addr += 4;
		addr_read += 4;
	}

	if(byte > 0)
	{
		M32(addr) = M32(addr_read);	
		addr += 3;							//point last byte
		while(byte)
		{
			*addr = 0;
			addr --;
			byte --;
		}
	}
	addr = &FLASH_BUF[0];
	memcpy(&uuid[2], addr + dumy_bytes, uni_bytes);
	tls_os_sem_release(inside_fls->fls_lock);

	return 0;
}

int tls_fls_otp_read(u32 addr, u8 *buf, u32 len)
{
    int err;
	
	int i = 0;
	int word = len/4;
	int byte = len%4;
	unsigned long addr_read = 0xBC00C048;
	volatile unsigned long value;
	unsigned long addr_offset = 0;
	unsigned long sz_need = len;

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);

	if(buf)
	{
		addr_offset = addr % 16;
		sz_need = (addr_offset + len + 16) / 16 * 16;
		addr = addr / 16 * 16;
	}
	M32(HR_FLASH_CMD_ADDR) = addr_read|(((sz_need-1)&0x3FF)<<16);
	M32(HR_FLASH_ADDR) = (addr&0x1FFFFFF);
	M32(HR_FLASH_CMD_START) = tls_reg_read32(HR_FLASH_CMD_START) | CMD_START_Msk;

	if(buf)
	{
		addr_read = RSA_BASE_ADDRESS + (addr_offset / 4 * 4);
		i = (4 - addr_offset % 4) % 4;
		if(i > len)
		{
			byte = len;
		}
		else
		{
			byte = i;
		}
		if(byte)
		{
			value = M32(addr_read);
			memcpy(buf, ((char *)&value) + 4 - i, byte);
			addr_read += 4;
			buf += byte;
		}
		word = (len - byte) / 4;
		for(i = 0;i < word; i ++)
		{
			value = M32(addr_read);
			memcpy(buf, (char*)&value, 4);
			buf += 4;
			addr_read += 4;
		}
		byte = (len - byte) % 4;
		if(byte > 0)
		{
			value = M32(addr_read);
			memcpy(buf, (char *)&value, byte);
		}
	}
	
    err = TLS_FLS_STATUS_OK;
    tls_os_sem_release(inside_fls->fls_lock);
    return err;
}

int tls_fls_otp_write(u32 addr, u8 *buf, u32 len)
{
	int ret = 0;
	unsigned int erasecmd = 0x80000844;
	unsigned int writecmd = 0x80009042; 
	uint32_t eraseAddr = 0;
	uint16_t eraseSize = 0;
	uint16_t pageSize = 0;

	int l = 0;
	unsigned char *backbuf = NULL;
	unsigned long size = 0;
	unsigned long p = 0;
	unsigned char *q = NULL;

	if (!buf)
	{
		return TLS_FLS_STATUS_EINVAL;
	}
    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }
	eraseSize = inside_fls->OTPWRParam.eraseSize;
	pageSize = inside_fls->OTPWRParam.pageSize;
	if (eraseSize == 0 || pageSize == 0)
	{
		TLS_DBGPRT_ERR("flash type is not supported!\n");
		return TLS_FLS_STATUS_ENOSUPPORT;
	}
	eraseAddr = addr & ~(eraseSize - 1);
	if(addr < eraseAddr || len > eraseSize - (addr - eraseAddr))
	{
		return TLS_FLS_STATUS_EINVAL;
	}
	TLS_DBGPRT_INFO("addr 0x%x, eraseAddr 0x%x, eraseSize 0x%x, pageSize 0x%x\n", addr, eraseAddr, eraseSize, pageSize);
	backbuf = tls_mem_alloc(eraseSize);
	if (!backbuf)
	{
		ret = TLS_FLS_STATUS_ENOMEM;
		goto out;
	}
	p = eraseAddr;
	q = backbuf;
	size = eraseSize;
	while(size > 0)
	{
		l = size > pageSize ? pageSize : size;
		if(tls_fls_otp_read(p, q, l) != TLS_FLS_STATUS_OK)
		{
			ret = TLS_FLS_STATUS_EPERM;
			goto out;
		}
		q += l;
		p += l;
		size -= l;
	}
	tls_os_sem_acquire(inside_fls->fls_lock, 0);
	eraseSR(erasecmd, eraseAddr);
	memcpy(backbuf + (addr - eraseAddr), buf, len);
	p = eraseAddr;
	q = backbuf;
	size = eraseSize;
	while(size > 0)
	{
		l = size > pageSize ? pageSize : size;
		programSR(writecmd, p, q, l);
		q += l;
		p += l;
		size -= l;
	}
	tls_os_sem_release(inside_fls->fls_lock);
out:
	if(backbuf)
		tls_mem_free(backbuf);
	return ret;
}

int tls_fls_otp_lock(void)
{
    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);
	switch(inside_fls->flashid)
	{
	case SPIFLASH_MID_GD:
	case SPIFLASH_MID_TSINGTENG:
		writeLbBit_for_1wreg((1<<10));
		break;
	case SPIFLASH_MID_TSINGTENG_1MB_4MB:
		writeLbBit_for_1wreg((7<<11));
		break;
	case SPIFLASH_MID_FUDANMICRO:
		writeLbBit_for_2wreg((1<<10));
		break;
	case SPIFLASH_MID_BOYA:
	case SPIFLASH_MID_XMC:
	case SPIFLASH_MID_WINBOND:
    case SPIFLASH_MID_PUYA:
		writeLbBit_for_2wreg((7<<11));
		break;
	case SPIFLASH_MID_XTX:
    case SPIFLASH_MID_ESMT:
    default:
        TLS_DBGPRT_ERR("flash is not supported!\n");
        return TLS_FLS_STATUS_ENOSUPPORT;
	}
    tls_os_sem_release(inside_fls->fls_lock);	
	return 0;
}

/**
 * @brief          This function is used to read data from the flash.
 *
 * @param[in]      addr                 is byte offset addr for read from the flash.
 * @param[in]      buf                   is user for data buffer of flash read
 * @param[in]      len                   is byte length for read.
 *
 * @retval         TLS_FLS_STATUS_OK	    if read sucsess
 * @retval         TLS_FLS_STATUS_EIO	    if read fail
 *
 * @note           None
 */
int tls_fls_read(u32 addr, u8 *buf, u32 len)
{
    int err;

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }

    if (((addr & (INSIDE_FLS_BASE_ADDR - 1)) >=  inside_fls->density) || (len == 0) || (buf == NULL))
    {
        return TLS_FLS_STATUS_EINVAL;
    }

    tls_os_sem_acquire(inside_fls->fls_lock, 0);

    flashRead(addr, buf, len);

    err = TLS_FLS_STATUS_OK;
    tls_os_sem_release(inside_fls->fls_lock);
    return err;
}

/**
 * @brief          This function is used to write data to the flash.
 *
 * @param[in]      addr     is byte offset addr for write to the flash
 * @param[in]      buf       is the data buffer want to write to flash
 * @param[in]      len       is the byte length want to write
 *
 * @retval         TLS_FLS_STATUS_OK	           if write flash success
 * @retval         TLS_FLS_STATUS_EPERM	    if flash struct point is null
 * @retval         TLS_FLS_STATUS_ENODRV	    if flash driver is not installed
 * @retval         TLS_FLS_STATUS_EINVAL	    if argument is invalid
 * @retval         TLS_FLS_STATUS_EIO           if io error
 *
 * @note           None
 */
int tls_fls_write(u32 addr, u8 *buf, u32 len)
{
    u8 *cache;
    unsigned int secpos;
    unsigned int secoff;
    unsigned int secremain;
    unsigned int i;
    unsigned int offaddr;

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }

    if (((addr & (INSIDE_FLS_BASE_ADDR - 1)) >=  inside_fls->density) || (len == 0) || (buf == NULL))
    {
        return TLS_FLS_STATUS_EINVAL;
    }

    tls_os_sem_acquire(inside_fls->fls_lock, 0);

    cache = tls_mem_alloc(INSIDE_FLS_SECTOR_SIZE);
    if (cache == NULL)
    {
        tls_os_sem_release(inside_fls->fls_lock);
        TLS_DBGPRT_ERR("allocate sector cache memory fail!\n");
        return TLS_FLS_STATUS_ENOMEM;
    }

    offaddr = addr & (INSIDE_FLS_BASE_ADDR - 1);			//Offset of 0X08000000
    secpos = offaddr / INSIDE_FLS_SECTOR_SIZE;				//Section addr
    secoff = (offaddr % INSIDE_FLS_SECTOR_SIZE);			//Offset in section
    secremain = INSIDE_FLS_SECTOR_SIZE - secoff;    // 扇区剩余空间大小
    if(len <= secremain)
    {
        secremain = len;								//Not bigger with remain size in section
    }
    while (1)
    {
        flashRead(secpos * INSIDE_FLS_SECTOR_SIZE, cache, INSIDE_FLS_SECTOR_SIZE);

        eraseSector(secpos * INSIDE_FLS_SECTOR_SIZE);
        for (i = 0; i < secremain; i++) // 复制
        {
            cache[i + secoff] = buf[i];
        }
        for (i = 0; i < (INSIDE_FLS_SECTOR_SIZE / INSIDE_FLS_PAGE_SIZE); i++)
        {
            programPage(secpos * INSIDE_FLS_SECTOR_SIZE + i * INSIDE_FLS_PAGE_SIZE, INSIDE_FLS_PAGE_SIZE, &cache[i * INSIDE_FLS_PAGE_SIZE]);	//Write
        }
        if(len == secremain)
        {
            break;              // 写入结束了
        }
        else                    // 写入未结束
        {
            secpos++;           // 扇区地址增1
            secoff = 0;         // 偏移位置为0
            buf += secremain;   // 指针偏移
            len -= secremain;
            if(len > (INSIDE_FLS_SECTOR_SIZE))
            {
                secremain = INSIDE_FLS_SECTOR_SIZE; // 下一个扇区还是写不完
            }
            else
            {
                secremain = len;					//Next section will finish
            }
        }
    }

    tls_mem_free(cache);
    tls_os_sem_release(inside_fls->fls_lock);
    return TLS_FLS_STATUS_OK;
}

/**
 * @brief          This function is used to write data into the flash without erase.
 *
 * @param[in]      addr     Specifies the starting address to write to
 * @param[in]      buf      Pointer to a byte array that is to be written
 * @param[in]      len      Specifies the length of the data to be written
 *
 * @retval         TLS_FLS_STATUS_OK	        if write flash success
 * @retval         TLS_FLS_STATUS_EPERM	        if flash struct point is null
 * @retval         TLS_FLS_STATUS_ENODRV	    if flash driver is not installed
 * @retval         TLS_FLS_STATUS_EINVAL	    if argument is invalid
 *
 * @note           Erase action should be excuted by API tls_fls_erase in user layer.
 */
int tls_fls_write_without_erase(u32 addr, u8 *buf, u32 len)
{
    u8 *cache;
    unsigned int pagepos;
    unsigned int pageoff;
    unsigned int pageremain;
    unsigned int i;
    unsigned int offaddr;

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }

    if (((addr & (INSIDE_FLS_BASE_ADDR - 1)) >=  inside_fls->density) || (len == 0) || (buf == NULL))
    {
        return TLS_FLS_STATUS_EINVAL;
    }

    tls_os_sem_acquire(inside_fls->fls_lock, 0);

    cache = tls_mem_alloc(INSIDE_FLS_PAGE_SIZE);
    if (cache == NULL)
    {
        tls_os_sem_release(inside_fls->fls_lock);
        TLS_DBGPRT_ERR("allocate page cache memory fail!\n");
        return TLS_FLS_STATUS_ENOMEM;
    }

    offaddr = addr & (INSIDE_FLS_BASE_ADDR - 1);	//Offset of 0X08000000
    pagepos = offaddr / INSIDE_FLS_PAGE_SIZE;		//Page addr
    pageoff = (offaddr % INSIDE_FLS_PAGE_SIZE);		//Offset in page
    pageremain = INSIDE_FLS_PAGE_SIZE - pageoff;    // size remained in one page
    if(len <= pageremain)
    {
        pageremain = len;							//Not bigger with remain size in one page
    }

	flashRead(pagepos * INSIDE_FLS_PAGE_SIZE, cache, INSIDE_FLS_PAGE_SIZE);
    while (1)
    {
        for (i = 0; i < pageremain; i++)
        {
            cache[i + pageoff] = buf[i];
        }

        programPage(pagepos * INSIDE_FLS_PAGE_SIZE, INSIDE_FLS_PAGE_SIZE, &cache[0]); //Write
        if(len == pageremain)// page program over
        {
            break;              
        }
        else                    
        {
            pagepos++;           // next page
            pageoff = 0;         // page offset set to zero
            buf += pageremain;   // buffer modified 
            len -= pageremain;   // len decrease
            if(len > (INSIDE_FLS_PAGE_SIZE))
            {
                pageremain = INSIDE_FLS_PAGE_SIZE; // size next to write
            }
            else
            {
                pageremain = len;					//last data to write
               	flashRead(pagepos * INSIDE_FLS_PAGE_SIZE, cache, INSIDE_FLS_PAGE_SIZE);
            }
        }
    }

    tls_mem_free(cache);
    tls_os_sem_release(inside_fls->fls_lock);
    return TLS_FLS_STATUS_OK;
}


/**
 * @brief          	This function is used to erase the appoint sector
 *
 * @param[in]      	sector 	sector num of the flash, 4K byte a sector
 *
 * @retval         	TLS_FLS_STATUS_OK	    	if read sucsess
 * @retval         	other	    				if read fail
 *
 * @note           	None
 */
int tls_fls_erase(u32 sector)
{
    u32 addr;
    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }

    if (sector >= (inside_fls->density / INSIDE_FLS_SECTOR_SIZE + INSIDE_FLS_BASE_ADDR / INSIDE_FLS_SECTOR_SIZE))
    {
        TLS_DBGPRT_ERR("the sector to be erase overflow!\n");
        return TLS_FLS_STATUS_EINVAL;
    }

    tls_os_sem_acquire(inside_fls->fls_lock, 0);

    addr = sector * INSIDE_FLS_SECTOR_SIZE;

    eraseSector(addr);

    tls_os_sem_release(inside_fls->fls_lock);

    return TLS_FLS_STATUS_OK;
}


static u8 *gsflscache = NULL;
//static u32 gsSecOffset = 0;
static u32 gsSector = 0;


/**
 * @brief          	This function is used to flush the appoint sector
 *
 * @param      	None
 *
 * @return         	None
 *
 * @note           	None
 */
static void tls_fls_flush_sector(void)
{
    int i;
    u32 addr;
    if (gsSector < (inside_fls->density / INSIDE_FLS_SECTOR_SIZE + INSIDE_FLS_BASE_ADDR / INSIDE_FLS_SECTOR_SIZE))
    {
        addr = gsSector * INSIDE_FLS_SECTOR_SIZE;

        eraseSector(addr);
        for (i = 0; i < INSIDE_FLS_SECTOR_SIZE / INSIDE_FLS_PAGE_SIZE; i++)
        {
            programPage(gsSector * INSIDE_FLS_SECTOR_SIZE +
                        i * INSIDE_FLS_PAGE_SIZE, INSIDE_FLS_PAGE_SIZE,
                        &gsflscache[i * INSIDE_FLS_PAGE_SIZE]);
        }
    }
    //gsSecOffset = 0;
}


/**
 * @brief          	This function is used to fast write flash initialize
 *
 * @param      	None
 *
 * @retval         	TLS_FLS_STATUS_OK	    	sucsess
 * @retval         	other	    				fail
 *
 * @note           	None
 */
int tls_fls_fast_write_init(void)
{

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }
    if (NULL != gsflscache)
    {
        TLS_DBGPRT_ERR("tls_fls_fast_write_init installed!\n");
        return -1;
    }
    gsflscache = tls_mem_alloc(INSIDE_FLS_SECTOR_SIZE);
    if (NULL == gsflscache)
    {
        TLS_DBGPRT_ERR("tls_fls_fast_write_init malloc err!\n");
        return -1;
    }
    return TLS_FLS_STATUS_OK;
}

/**
 * @brief          	This function is used to destroy fast write flash
 *
 * @param      	None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_fls_fast_write_destroy(void)
{
    if (NULL != gsflscache)
    {
        if (inside_fls == NULL)
        {
            TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
            return;
        }
        else
        {
            tls_os_sem_acquire(inside_fls->fls_lock, 0);
        	tls_fls_flush_sector();
            tls_os_sem_release(inside_fls->fls_lock);
        }

        tls_mem_free(gsflscache);
        gsflscache = NULL;
    }
}

/**
 * @brief          	This function is used to fast write data to the flash.
 *
 * @param[in]      	addr     	is byte offset addr for write to the flash
 * @param[in]      	buf       	is the data buffer want to write to flash
 * @param[in]      	length  	is the byte length want to write
 *
 * @retval         	TLS_FLS_STATUS_OK	success
 * @retval        	other				fail
 *
 * @note           	None
 */
int tls_fls_fast_write(u32 addr, u8 *buf, u32 length)
{

    u32 sector, offset, maxlen, len;

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }
    if(((addr & (INSIDE_FLS_BASE_ADDR - 1)) >=  inside_fls->density) || (length == 0) || (buf == NULL))
    {
        return TLS_FLS_STATUS_EINVAL;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);

    sector = addr / INSIDE_FLS_SECTOR_SIZE;
    offset = addr % INSIDE_FLS_SECTOR_SIZE;
    maxlen = INSIDE_FLS_SECTOR_SIZE;

    if ((sector != gsSector) && (gsSector != 0))
    {
        tls_fls_flush_sector();
    }
    gsSector = sector;
    if (offset > 0)
    {
        maxlen -= offset;
    }
    while (length > 0)
    {
        len = (length > maxlen) ? maxlen : length;
        MEMCPY(gsflscache + offset, buf, len);
        if (offset + len >= INSIDE_FLS_SECTOR_SIZE)
        {
            tls_fls_flush_sector();
            gsSector++;
        }
        offset = 0;
        maxlen = INSIDE_FLS_SECTOR_SIZE;
        sector++;
        buf += len;
        length -= len;
    }

    tls_os_sem_release(inside_fls->fls_lock);

    return TLS_FLS_STATUS_OK;
}


/**
 * @brief          	This function is used to erase flash all chip
 *
 * @param      	None
 *
 * @retval         	TLS_FLS_STATUS_OK	    	sucsess
 * @retval         	other	    				fail
 *
 * @note           	None
 */
int tls_fls_chip_erase(void)
{
    int i, j;
    u8 *cache;

    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }

    tls_os_sem_acquire(inside_fls->fls_lock, 0);

    cache = tls_mem_alloc(INSIDE_FLS_SECTOR_SIZE);
    if (cache == NULL)
    {
        tls_os_sem_release(inside_fls->fls_lock);
        TLS_DBGPRT_ERR("allocate sector cache memory fail!\n");
        return TLS_FLS_STATUS_ENOMEM;
    }


    for( i = 0; i < ( inside_fls->density - (INSIDE_FLS_SECBOOT_ADDR & 0xFFFFF)) / INSIDE_FLS_SECTOR_SIZE; i ++)
    {
        flashRead(INSIDE_FLS_SECBOOT_ADDR + i * INSIDE_FLS_SECTOR_SIZE, cache, INSIDE_FLS_SECTOR_SIZE);
        for (j = 0; j < INSIDE_FLS_SECTOR_SIZE; j++)
        {
            if (cache[j] != 0xFF)
            {
                eraseSector(INSIDE_FLS_SECBOOT_ADDR + i * INSIDE_FLS_SECTOR_SIZE);
                break;
            }
        }
    }

    tls_mem_free(cache);

    tls_os_sem_release(inside_fls->fls_lock);

    return TLS_FLS_STATUS_OK;
}


/**
 * @brief          	This function is used to get flash param
 *
 * @param[in]      	type		the type of the param need to get
 * @param[out]     	param	point to addr of out param
 *
 * @retval         	TLS_FLS_STATUS_OK	    	sucsess
 * @retval         	other	    				fail
 *
 * @note           	None
 */
int tls_fls_get_param(u8 type, void *param)
{
    int err;


    if (inside_fls == NULL)
    {
        TLS_DBGPRT_ERR("flash driver module not beed installed!\n");
        return TLS_FLS_STATUS_EPERM;
    }

    if (param == NULL)
    {
        return TLS_FLS_STATUS_EINVAL;
    }
    tls_os_sem_acquire(inside_fls->fls_lock, 0);
    err = TLS_FLS_STATUS_OK;
    switch (type)
    {
    case TLS_FLS_PARAM_TYPE_ID:
        *((u32 *) param) = 0x2013;
        break;

    case TLS_FLS_PARAM_TYPE_SIZE:
        *((u32 *) param) = inside_fls->density;
        break;

    case TLS_FLS_PARAM_TYPE_PAGE_SIZE:
        *((u32 *) param) = INSIDE_FLS_PAGE_SIZE;
        break;

    case TLS_FLS_PARAM_TYPE_PROG_SIZE:
        *((u32 *) param) = INSIDE_FLS_PAGE_SIZE;
        break;

    case TLS_FLS_PARAM_TYPE_SECTOR_SIZE:
        *((u32 *) param) = INSIDE_FLS_SECTOR_SIZE;
        break;

    default:
        TLS_DBGPRT_WARNING("invalid parameter ID!\n");
        err = TLS_FLS_STATUS_EINVAL;
        break;
    }
    tls_os_sem_release(inside_fls->fls_lock);
    return err;
}

/**
 * @brief          	This function is used to initialize the flash module
 *
 * @param      	None
 *
 * @retval         	TLS_FLS_STATUS_OK	    	sucsess
 * @retval         	other	    				fail
 *
 * @note           	None
 */
int tls_fls_init(void)
{
    struct tls_inside_fls *fls;
    int err;

    if (inside_fls != NULL)
    {
        TLS_DBGPRT_ERR("flash driver module has been installed!\n");
        return TLS_FLS_STATUS_EBUSY;
    }

    fls = (struct tls_inside_fls *) tls_mem_alloc(sizeof(struct tls_inside_fls));
    if (fls == NULL)
    {
        TLS_DBGPRT_ERR("allocate @inside_fls fail!\n");
        return TLS_FLS_STATUS_ENOMEM;
    }

    memset(fls, 0, sizeof(*fls));
    err = tls_os_sem_create(&fls->fls_lock, 1);
    if (err != TLS_OS_SUCCESS)
    {
        tls_mem_free(fls);
        TLS_DBGPRT_ERR("create semaphore @fls_lock fail!\n");
        return TLS_FLS_STATUS_ENOMEM;
    }
	fls->flashid = readRID();
	//printf("flashid %x\n", fls->flashid);
	fls->density = getFlashDensity();
	fls->OTPWRParam.pageSize = 256;
	switch(fls->flashid)
	{
	case SPIFLASH_MID_TSINGTENG_1MB_4MB:
		if (fls->density == 0x100000)
		{
			fls->OTPWRParam.eraseSize = 1024;
		}
		else if (fls->density == 0x400000)
		{
			fls->OTPWRParam.eraseSize = 2048;
		}
		else
		{
			fls->OTPWRParam.eraseSize = 256;
		}
		break;	
	case SPIFLASH_MID_GD:
		fls->OTPWRParam.eraseSize = 1024;
		break;
	case SPIFLASH_MID_FUDANMICRO:
		fls->OTPWRParam.eraseSize = 1024;
		if(fls->density <= (1 << 20))//8Mbit
		{
			fls->OTPWRParam.eraseSize = 256;
		}
		break;
	case SPIFLASH_MID_TSINGTENG:
	case SPIFLASH_MID_BOYA:
	case SPIFLASH_MID_XMC:
	case SPIFLASH_MID_WINBOND:
		fls->OTPWRParam.eraseSize = 256;
		break;
    case SPIFLASH_MID_PUYA:
		fls->OTPWRParam.eraseSize = 512;
		break;
	case SPIFLASH_MID_XTX:
    case SPIFLASH_MID_ESMT:
		fls->OTPWRParam.eraseSize = 0;//not support
		break;
    default:
		tls_mem_free(fls);
        TLS_DBGPRT_ERR("flash is not supported!\n");
        return TLS_FLS_STATUS_ENOSUPPORT;
	}
	
    inside_fls = fls;

    return TLS_FLS_STATUS_OK;
}

int tls_fls_exit(void)
{
    TLS_DBGPRT_FLASH_INFO("Not support flash driver module uninstalled!\n");
    return TLS_FLS_STATUS_EPERM;
}

/**
 * @brief          	This function is used to initialize system parameter postion by flash density
 *
 * @param      	None
 *
 * @retval         	None
 *
 * @note           	must be called before function tls_param_init
 */
void tls_fls_sys_param_postion_init(void)
{
    unsigned int density = 0;
    int err;
    err = tls_fls_get_param(TLS_FLS_PARAM_TYPE_SIZE, (void *)&density);
    if (TLS_FLS_STATUS_OK == err)
    {
        TLS_FLASH_END_ADDR            = (FLASH_BASE_ADDR|density) - 1;
        TLS_FLASH_OTA_FLAG_ADDR       = (FLASH_BASE_ADDR|density) - 0x1000;
        TLS_FLASH_PARAM_RESTORE_ADDR  =	(FLASH_BASE_ADDR|density) - 0x2000;
        TLS_FLASH_PARAM2_ADDR 		  =	(FLASH_BASE_ADDR|density) - 0x3000;
        TLS_FLASH_PARAM1_ADDR 		  =	(FLASH_BASE_ADDR|density) - 0x4000;
        TLS_FLASH_PARAM_DEFAULT	      =	(FLASH_BASE_ADDR|density) - 0x5000;
        TLS_FLASH_MESH_PARAM_ADDR     = (FLASH_BASE_ADDR|density) - 0x6000;
    }
    else
    {
        TLS_DBGPRT_ERR("system parameter postion use default!\n");
    }
}


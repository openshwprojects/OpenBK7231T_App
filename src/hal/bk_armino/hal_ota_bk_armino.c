#if PLATFORM_ARMINO

#include "../hal_ota.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "driver/flash.h"

static unsigned char *sector = (void *)0;
int sectorlen = 0;
unsigned int addr = 0xff000;
#define SECTOR_SIZE 0x1000
static void store_sector(unsigned int addr, unsigned char *data);

extern int HAL_FlashRead(char* buffer, int readlen, int startaddr);
extern int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr);
extern int HAL_FlashEraseSector(int startaddr);

int init_ota(unsigned int startaddr)
{
	if(startaddr > 0xff000)
	{
		if(sector)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "aborting OTA, sector already non-null");
			return 0;
		}
		sector = os_malloc(SECTOR_SIZE);
		sectorlen = 0;
		addr = startaddr;
		addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "init OTA, startaddr 0x%x", startaddr);
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "aborting OTA, startaddr 0x%x < 0xff000", startaddr);
	return 0;
}

void close_ota()
{
	addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "");
	if(sectorlen)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "close OTA, additional 0x%x FF added", SECTOR_SIZE - sectorlen);
		memset(sector + sectorlen, 0xff, SECTOR_SIZE - sectorlen);
		sectorlen = SECTOR_SIZE;
		store_sector(addr, sector);
		addr += 1024;
		sectorlen = 0;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "close OTA, addr 0x%x", addr);

	os_free(sector);
}

void add_otadata(unsigned char* data, int len)
{
	if(!sector) return;
	while(len > 0)
	{
		if(sectorlen < SECTOR_SIZE)
		{
			int lenstore = SECTOR_SIZE - sectorlen;
			if(lenstore > len)
				lenstore = len;
			memcpy(sector + sectorlen, data, lenstore);
			data += lenstore;
			len -= lenstore;
			sectorlen += lenstore;
		}

		if(sectorlen == SECTOR_SIZE)
		{
			store_sector(addr, sector);
			addr += SECTOR_SIZE;
			sectorlen = 0;
		}
		else
		{
			rtos_delay_milliseconds(10);
		}
	}
}

static void store_sector(unsigned int addr, unsigned char* data)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "%x", addr);
	//HAL_FlashEraseSector(addr);
	//HAL_FlashWrite((char *)data , SECTOR_SIZE, addr);
	flash_protect_type_t protect_type = bk_flash_get_protect_type();
	if(FLASH_PROTECT_NONE != protect_type)
	{
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	}
	bk_flash_erase_sector(addr & 0x00FFF000);
	bk_flash_write_bytes(addr, (const uint8_t*)data, SECTOR_SIZE);
	if(FLASH_PROTECT_NONE != protect_type)
	{
		bk_flash_set_protect_type(protect_type);
	}
	OTA_IncrementProgress(SECTOR_SIZE);
}

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;
	int initial_addr = startaddr;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	init_ota(startaddr);

	if (request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	if (writelen < 0 || (startaddr + writelen > maxaddr))
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "ABORTED: %d bytes to write", writelen);
		return http_rest_error(request, -20, "writelen < 0 or end > limit");
	}

	do
	{
		//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);
		add_otadata((unsigned char*)writebuf, writelen);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if (towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if (writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while ((towrite > 0) && (writelen >= 0));
	close_ota();
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	extern uint32_t g_ota_start_addr;
	if(initial_addr == g_ota_start_addr) CFG_IncrementOTACount();
	return 0;
}

#endif

#if PLATFORM_MT7687

#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../../httpclient/http_client.h"
#include "../../driver/drv_public.h"
#include "../../driver/drv_bl_shared.h"
#include "../hal_ota.h"

#include "flash_map.h"
#include "fota.h"
#include "hal.h"

static unsigned char* sector = (void*)0;
int sectorlen = 0;
unsigned int addr = 0xff000;
#define SECTOR_SIZE 0x1000
static void store_sector(unsigned int addr, unsigned char* data);

extern int HAL_FlashRead(char* buffer, int readlen, int startaddr);
extern int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr);
extern int HAL_FlashEraseSector(int startaddr);
uint32_t g_ota_start_addr = FOTA_BASE;
uint32_t g_ota_end_addr = FOTA_BASE + FOTA_LENGTH;

int init_ota(unsigned int startaddr)
{
	if(startaddr > FOTA_BASE - 1)
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
			rtos_delay_milliseconds(1);
		}
	}
}

static void store_sector(unsigned int addr, unsigned char* data)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "%x", addr);
	HAL_FlashEraseSector(addr);
	HAL_FlashWrite((char *)data , SECTOR_SIZE, addr);
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

	if(request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	if(writelen < 0 || towrite <= 0 || startaddr < 0 || startaddr > maxaddr || towrite > maxaddr - startaddr)
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "ABORTED: %d bytes to write", towrite);
		return http_rest_error(request, -20, "invalid length, or end beyond maxaddr");
	}

	init_ota(startaddr);

	do
	{
		//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);
		add_otadata((unsigned char*)writebuf, writelen);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if(writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while((towrite > 0) && (writelen >= 0));
	close_ota();
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	if(initial_addr == g_ota_start_addr)
	{
		fota_trigger_update();
		CFG_IncrementOTACount();
	}
	return 0;
}


httprequest_t httprequest;
extern uint32_t g_ota_start_addr;

int myhttpclientcallback(httprequest_t* request)
{

	//httpclient_t *client = &request->client;
	httpclient_data_t* client_data = &request->client_data;

	// NOTE: Called from the client thread, beware
	//It is not clear if we can just update total_bytes instead of incrementing. Maintaining previous behavior.
	OTA_SetTotalBytes(OTA_GetTotalBytes() + request->client_data.response_buf_filled);

	switch(request->state)
	{
		case 0: // start
			//init_ota(0xff000);

			init_ota(g_ota_start_addr);
			addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "\rmyhttpclientcallback state %d total %d/%d", request->state, OTA_GetTotalBytes(), request->client_data.response_content_len);
			break;
		case 1: // data
			if(request->client_data.response_buf_filled)
			{
				unsigned char* d = (unsigned char*)request->client_data.response_buf;
				int l = request->client_data.response_buf_filled;
				add_otadata(d, l);
			}
			break;
		case 2: // ended, write any remaining bytes to the sector
			close_ota();
			OTA_ResetProgress();
			fota_trigger_update();
			addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "\rmyhttpclientcallback state %d total %d/%d", request->state, OTA_GetTotalBytes(), request->client_data.response_content_len);

			addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "Rebooting in 1 seconds...");

			// record this OTA
			CFG_IncrementOTACount();
			// make sure it's saved before reboot
			CFG_Save_IfThereArePendingChanges();
#if ENABLE_BL_SHARED
			if(DRV_IsMeasuringPower())
			{
				BL09XX_SaveEmeteringStatistics();
			}
#endif
			DRV_SavePowerMeterDriverStatistics();
			rtos_delay_milliseconds(1000);
			HAL_RebootModule();
			break;
	}

	//rtos_delay_milliseconds(500);

	if(request->state == 2)
	{
		//os_free(client_data->response_buf);
		client_data->response_buf = (void*)0;
		client_data->response_buf_len = 0;
	}
	//rtos_delay_milliseconds(100);

	return 0;
}

// NOTE: these MUST persist
// note: url must have a '/' after host, else it can;t parse it..
static char url[256] = "http://raspberrypi:1880/firmware";
static const char* header = "";
static char* content_type = "text/csv";
static char* post_data = "";
#define BUF_SIZE 2048

char* http_buf = (void*)0;

// Set once a fetch's thread is started. Every way the fetch ends restarts the
// device (myhttpclientcallback, state 2), so it is never cleared; request->state
// is no guide, being 0 both before a fetch and while it connects, and a second
// fetch would reset the request under the first one's thread.
static int http_ota_started = 0;

int otarequest(const char* urlin)
{
	httprequest_t* request = &httprequest;
	if(http_ota_started)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "********************http in progress, not starting another");
		return -1;
	}

	strncpy(url, urlin, sizeof(url));

	OTA_SetTotalBytes(0);
	memset(request, 0, sizeof(*request));
	httpclient_t* client = &request->client;
	httpclient_data_t* client_data = &request->client_data;

	if(http_buf == (void*)0)
	{
		http_buf = os_malloc(BUF_SIZE + 1);
		if(http_buf == (void*)0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "startrequest Malloc failed.");
			return -1;
		}
		memset(http_buf, 0, BUF_SIZE);
	}
	client_data->response_buf = http_buf;  //Sets a buffer to store the result.
	client_data->response_buf_len = BUF_SIZE;  //Sets the buffer size.
	HTTPClient_SetCustomHeader(client, header);  //Sets the custom header if needed.
	client_data->post_buf = post_data;  //Sets the user data to be posted.
	client_data->post_buf_len = strlen(post_data);  //Sets the post data length.
	client_data->post_content_type = content_type;  //Sets the content type.
	request->data_callback = &myhttpclientcallback;
	request->port = 80;//HTTP_PORT;
	request->url = url;
	request->method = HTTPCLIENT_GET;
	request->timeout = 10000;
	//+2 Updating ota_status to 0 as before.
	OTA_ResetProgress();
	OTA_IncrementProgress(1);
	http_ota_started = 1;
	if(HTTPClient_Async_SendGeneric(request) != 0)
	{
		http_ota_started = 0;
		OTA_ResetProgress();
		return -1;
	}
	return 0;
}

/*
int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;
	int ret = -1;
	int32_t res;
	startaddr = FOTA_BASE;
	maxaddr = FOTA_BASE + FOTA_LENGTH;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	if (request->contentLength <= 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Content-length is 0");
		goto update_ota_exit;
	}
	else
	{
		towrite = request->contentLength;
	}

	if(towrite > maxaddr)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA image too big");
		goto update_ota_exit;
	}

	uint32_t erase_upto = startaddr;
	do
	{
		uint32_t write_start = startaddr;
		uint32_t write_end = startaddr + writelen;
		uint32_t erase_start = write_start & ~(0x1000 - 1);
		uint32_t erase_end = (write_end + 0x1000 - 1) & ~(0x1000 - 1);
		uint32_t addr = (erase_start > erase_upto) ? erase_start : erase_upto;
		while(addr < erase_end)
		{
			if(HAL_FlashEraseSector(addr) < 0)
			{
				goto update_ota_exit;
			}
			addr += 0x1000;
		}
		if(erase_end > erase_upto)
		{
			erase_upto = erase_end;
		}

		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at 0x%06X", writelen, startaddr);
		HAL_FlashWrite(writebuf, writelen, startaddr);
		rtos_delay_milliseconds(10);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, towrite > request->receivedLenmax ? request->receivedLenmax : towrite, 0);
			if (writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				goto update_ota_exit;
			}
		}
	} while ((towrite > 0) && (writelen >= 0));
	ret = 0;
update_ota_exit:
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
		res = fota_trigger_update();
		if(res != FOTA_TRIGGER_SUCCESS)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "FOTA trigger failed! (res = %d)", res);
			goto update_ota_exit;
		}
	}
	else
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA failed!");
		return http_rest_error(request, ret, "error");
	}

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	CFG_IncrementOTACount();
	return 0;
}
*/

#endif

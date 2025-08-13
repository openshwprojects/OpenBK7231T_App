#if PLATFORM_ECR6600 || PLATFORM_TR6260

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#if PLATFORM_TR6260

#include "otaHal.h"
#include "drv_spiflash.h"

#define OTA_INIT otaHal_init
#define OTA_WRITE otaHal_write
#define OTA_DONE(x) otaHal_done()

int HAL_FlashRead(char*buffer, int readlen, int startaddr) {
	int res;
	res = hal_spiflash_read(startaddr, (uint8_t*)buffer, readlen);
	return res;
}

#elif PLATFORM_ECR6600

#include "flash.h"

extern int ota_init(void);
extern int ota_write(unsigned char* data, unsigned int len);
extern int ota_done(bool reset);

#define OTA_INIT ota_init
#define OTA_WRITE ota_write
#define OTA_DONE(x) ota_done(x)

int HAL_FlashRead(char*buffer, int readlen, int startaddr) {
	int res;
	res = drv_spiflash_read(startaddr, (uint8_t*)buffer, readlen);
	return res;
}

#endif


int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);


	int ret = 0;

	if (request->contentLength > 0)
	{
		towrite = request->contentLength;
	}
	else
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Content-length is 0");
		goto update_ota_exit;
	}

	if (OTA_INIT() != 0)
	{
		ret = -1;
		goto update_ota_exit;
	}

	do
	{
		if (OTA_WRITE((unsigned char*)writebuf, writelen) != 0)
		{
			ret = -1;
			goto update_ota_exit;
		}
		delay_ms(10);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
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
				ret = -1;
			}
		}
	} while ((towrite > 0) && (writelen >= 0));

update_ota_exit:
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
		OTA_DONE(0);
	}
	else
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA failed. Reboot to retry");
		return http_rest_error(request, ret, "error");
	}

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	CFG_IncrementOTACount();
	return 0;
}

#endif



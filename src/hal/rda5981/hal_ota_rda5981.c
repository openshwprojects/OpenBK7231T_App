#if PLATFORM_RDA5981

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#include "rda5981_ota.h"

uint32_t OTA_Offset = 0x18095000;

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	int ret = 0;

	if (request->contentLength <= 0)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Content-length is 0");
		goto update_ota_exit;
	}
	else
	{
		towrite = request->contentLength;
	}
	startaddr = OTA_Offset;
	int ret1 = rda5981_erase_flash(startaddr, towrite);
	if(ret1 != 0)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "rda5981_erase_flash failed. %i", ret);
		goto update_ota_exit;
	}

	do
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		ret1 = HAL_FlashWrite(writebuf, writelen, startaddr - 0x18000000);
		if(ret1 != 0)
		{
			ret = -1;
			ADDLOG_ERROR(LOG_FEATURE_OTA, "flash_write failed. %i", ret1);
			goto update_ota_exit;
		}
		delay_ms(5);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, 1024, 0);
			if (writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				ret = -1;
				goto update_ota_exit;
			}
		}
	} while ((towrite > 0) && (writelen >= 0));

update_ota_exit:
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
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

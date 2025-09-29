#if PLATFORM_RDA5981

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#include "rda5981_ota.h"

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
	startaddr = 0x1807E000;
	// if compressed ota
	//startaddr = 0x1809C000;
	if(rda5981_write_partition_start(startaddr, towrite) != 0)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "rda5981_write_partition_start failed");
		goto update_ota_exit;
	}

	do
	{
		if(rda5981_write_partition(startaddr, (unsigned char*)writebuf, writelen) != 0)
		{
			ret = -1;
			goto update_ota_exit;
		}
		delay_ms(5);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if (towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, 2048, 0);
			if (writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				ret = -1;
			}
		}
	} while ((towrite > 0) && (writelen >= 0));

	int check = rda5981_write_partition_end();
	if(check != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "rda5981_write_partition_end failed");
		ret = -1;
	}
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

#if PLATFORM_TXW81X

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"
#include "lib/ota/fw.h"

typedef struct tcpNetUpgradeInfo
{
	char version[16];
	int fileSize;
}NetUpgrade_Info;

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	int ret = 0;
	int res = 0;

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

	do
	{
		int res = libota_write_fw(request->contentLength, startaddr, writebuf, writelen);
		if(res != -1)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "libota_write_fw,res:%d\n", res);
			ret = -1;
			goto update_ota_exit;
		}
		rtos_delay_milliseconds(10);
		ADDLOG_INFO(LOG_FEATURE_OTA, "Writelen %i at %i, res %i", writelen, total, res);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, 1464, 0);
			if(writelen < 0)
			{
				ADDLOG_ERROR(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				ret = -1;
			}
		}
	} while((towrite > 0) && (writelen >= 0));

update_ota_exit:
	if(ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
	}
	else
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA failed");
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

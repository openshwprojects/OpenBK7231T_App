#if PLATFORM_XRADIO

#include <image/flash.h>
#include <ota/ota.h>
#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../httpserver/new_http.h"
#include "../../logging/logging.h"

uint32_t flash_read(uint32_t flash, uint32_t addr, void* buf, uint32_t size);
#define FLASH_INDEX_XR809 0

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	bool recvfp = true;

	ota_status_t ota_update_rest_init(void* url)
	{
		return OTA_STATUS_OK;
	}
	ota_status_t ota_update_rest_get(uint8_t* buf, uint32_t buf_size, uint32_t* recv_size, uint8_t* eof_flag)
	{
		if (recvfp)
		{
			//free(buf);
			//recvfp = false;
			//buf = writebuf;
			//*recv_size = writelen;
			//return OTA_STATUS_OK;
			int bsize = (writelen > buf_size ? buf_size : writelen);
			memcpy(buf, writebuf + startaddr, bsize);
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", bsize, startaddr);
			startaddr += bsize;
			*recv_size = bsize;
			*eof_flag = 0;
			total += bsize;
			towrite -= bsize;
			writelen -= bsize;
			recvfp = writelen > 0;
			return OTA_STATUS_OK;
		}
		if (towrite > 0)
		{
			*recv_size = writelen = recv(request->fd, buf, (request->receivedLenmax > buf_size ? buf_size : request->receivedLenmax), 0);
			//*recv_size = writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
			if (writelen < 0)
			{
				ADDLOG_INFO(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				*eof_flag = 1;
				*recv_size = 0;
				return OTA_STATUS_OK;
				//return OTA_STATUS_ERROR;
			}
		}
		total += writelen;
		towrite -= writelen;

		if ((towrite > 0) && (writelen >= 0))
		{
			*eof_flag = 0;
			rtos_delay_milliseconds(10);
			return OTA_STATUS_OK;
		}
		*eof_flag = 1;
		return OTA_STATUS_OK;
	}

	int ret = 0;
	uint32_t* verify_value;
	ota_verify_t verify_type;
	ota_verify_data_t verify_data;

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

	ota_init();

	if (ota_update_image(NULL, ota_update_rest_init, ota_update_rest_get) != OTA_STATUS_OK)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "ota_update_image failed");
		goto update_ota_exit;
	}

	if (ota_get_verify_data(&verify_data) != OTA_STATUS_OK)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "ota_get_verify_data not ok, OTA_VERIFY_NONE");
		verify_type = OTA_VERIFY_NONE;
		verify_value = NULL;
	}
	else
	{
		verify_type = verify_data.ov_type;
		ADDLOG_INFO(LOG_FEATURE_OTA, "ota_get_verify_data ok");
		verify_value = (uint32_t*)(verify_data.ov_data);
	}

	if (ota_verify_image(verify_type, verify_value) != OTA_STATUS_OK)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA verify image failed");
		goto update_ota_exit;
	}

update_ota_exit:
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
	}
	else
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA failed.");
		return http_rest_error(request, ret, "error");
	}
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	CFG_IncrementOTACount();
	return 0;
}

int HAL_FlashRead(char*buffer, int readlen, int startaddr) {
	int res;
	//uint32_t flash_read(uint32_t flash, uint32_t addr,void *buf, uint32_t size)
	res = flash_read(0, startaddr, buffer, readlen);
	return res;
}
#endif
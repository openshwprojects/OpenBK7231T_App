#if PLATFORM_TR6260

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#include "easyflash.h"

#define OTA_HEADER_MAGIC 0x32365254U
typedef struct
{
	uint32_t magic;
	uint32_t ota_length;
	uint32_t ota_crc;
	uint32_t image_length;
	uint32_t image_crc;
	uint32_t header_crc;
} ota_header_t;
uint32_t crc32(uint32_t crc, const void* buf, size_t size);

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
	unsigned int ota_len, full_maxlen;
	partion_info_get(PARTION_NAME_DATA_OTA, &startaddr, &ota_len);
	partion_info_get(PARTION_NAME_CPU1, NULL, &full_maxlen);
	if(towrite > ota_len)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA image is too big!");
		goto update_ota_exit;
	}
	int need = sizeof(ota_header_t);
	int got = writelen;
	while(writelen < sizeof(ota_header_t))
	{
		int chunk = recv(request->fd, request->bodystart + writelen, need - got, 0);
		got += chunk;
		writelen += chunk;
	}
	ota_header_t hdr;
	memcpy(&hdr, request->bodystart, sizeof(ota_header_t));
	if(hdr.magic != OTA_HEADER_MAGIC || crc32(0, &hdr, sizeof(ota_header_t) - 4) != hdr.header_crc)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA header parse error!");
		goto update_ota_exit;
	}
	if(hdr.image_length > full_maxlen)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "OTA uncompressed image is too big!");
		goto update_ota_exit;
	}

	OTA_ResetProgress();
	OTA_IncrementProgress(startaddr + 1);

	hal_spiflash_erase(startaddr, (towrite + 0xfff) & ~0xfff);

	do
	{
		OTA_SetTotalBytes(writelen);
		OTA_IncrementProgress(writelen);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at 0x%X", writelen, startaddr);
		hal_spiflash_write(startaddr, (unsigned char*)writebuf, writelen);
		delay_ms(5);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax < towrite ? request->receivedLenmax : towrite, 0);
			if(writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				ret = -1;
				goto update_ota_exit;
			}
		}
	} while((towrite > 0) && (writelen >= 0));

update_ota_exit:
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
		uint8_t ota_status = 1;
		ef_set_env_blob("XZOtaStatus", &ota_status, 1);
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



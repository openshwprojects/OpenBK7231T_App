#if PLATFORM_GD32VW553

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#include "config_gdm32.h"
#include "rom_export.h"
#include "raw_flash_api.h"
#include "mbedtls/md.h"

#include "mbedtls/md5.h"

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;
	int ret = -1;
	int32_t res;
	mbedtls_md_context_t ctx;
	uint8_t running_idx = IMAGE_0;
	unsigned char appended_checksum[16] = { 0 };
	unsigned char image_checksum[16] = { 0 };
	uint32_t image_start;
	uint32_t image_size;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	if (request->contentLength <= 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Content-length is 0");
		goto update_ota_exit;
	}
	else
	{
		towrite = image_size = request->contentLength - 16; // appended md5
	}

	res = rom_sys_status_get(SYS_RUNNING_IMG, LEN_SYS_RUNNING_IMG, &running_idx);
	if(res < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Get sys running idx failed! (res = %d)", res);
		goto update_ota_exit;
	}

	if(running_idx == IMAGE_0)
	{
		image_start = startaddr = RE_IMG_1_OFFSET;
		maxaddr = RE_IMG_1_END - RE_IMG_1_OFFSET;
	}
	else
	{
		image_start = startaddr = RE_IMG_0_OFFSET;
		maxaddr = RE_IMG_1_OFFSET - RE_IMG_0_OFFSET;
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
			if(raw_flash_erase(addr, 0x1000) < 0)
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


	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 0);
	mbedtls_md_starts(&ctx);

	uint32_t addr = image_start;
	uint32_t remaining = image_size;
	while(remaining > 0)
	{
		uint32_t chunk = remaining > request->receivedLenmax ? request->receivedLenmax : remaining;
		HAL_FlashRead(request->received, chunk, addr);
		mbedtls_md_update(&ctx, (const unsigned char*)request->received, chunk);
		addr += chunk;
		remaining -= chunk;
	}

	mbedtls_md_finish(&ctx, image_checksum);
	writelen = 0;
	while(writelen < 16)
	{
		uint32_t r = recv(request->fd, appended_checksum + writelen, 16 - writelen, 0);
		if(r <= 0)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Checksum recv failed", r);
			goto update_ota_exit;
		}
		writelen += r;
	}
	if(memcmp(image_checksum, appended_checksum, 16) != 0)
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "MD5 check failed");
		goto update_ota_exit;
	}
	ret = 0;
update_ota_exit:
	mbedtls_md_free(&ctx);
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
		res = rom_sys_set_img_flag(running_idx, (IMG_FLAG_IA_MASK | IMG_FLAG_NEWER_MASK), (IMG_FLAG_IA_OK | IMG_FLAG_OLDER));
		res |= rom_sys_set_img_flag(!running_idx, (IMG_FLAG_IA_MASK | IMG_FLAG_VERIFY_MASK | IMG_FLAG_NEWER_MASK), 0);
		res |= rom_sys_set_img_flag(!running_idx, (IMG_FLAG_IA_MASK | IMG_FLAG_VERIFY_MASK | IMG_FLAG_NEWER_MASK), IMG_FLAG_NEWER | IMG_FLAG_VERIFY_NONE | IMG_FLAG_IA_NONE);
		if(res != 0)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Set sys image status failed! (res = %d)", res);
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

#endif

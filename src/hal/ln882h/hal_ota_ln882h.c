#if PLATFORM_LN882H || PLATFORM_LN8825

#include "hal/hal_flash.h"
#include "flash_partition_table.h"
#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#include "ota_port.h"
#include "ota_image.h"
#include "ota_types.h"
#include "hal/hal_flash.h"
#include "netif/ethernetif.h"
#include "flash_partition_table.h"


#define KV_OTA_UPG_STATE           ("kv_ota_upg_state")
#define HTTP_OTA_DEMO_STACK_SIZE   (1024 * 16)

#define SECTOR_SIZE_4KB            (1024 * 4)

static char g_http_uri_buff[512] = "http://192.168.122.48:9090/ota-images/otaimage-v1.3.bin";

// a block to save http data.
static char *temp4K_buf = NULL;
static int   temp4k_offset = 0;

// where to save OTA data in flash.
static int32_t flash_ota_start_addr = OTA_SPACE_OFFSET;
static int32_t flash_ota_offset = 0;
static uint8_t is_persistent_started = LN_FALSE;
static uint8_t is_ready_to_verify = LN_FALSE;
static uint8_t is_precheck_ok = LN_FALSE;
static uint8_t httpc_ota_started = 0;

/**
 * @brief Pre-check the image file to be downloaded.
 *
 * @attention None
 *
 * @param[in]  app_offset  The offset of the APP partition in Flash.
 * @param[in]  ota_hdr     pointer to ota partition info struct.
 *
 * @return  whether the check is successful.
 * @retval  #LN_TRUE     successful.
 * @retval  #LN_FALSE    failed.
 */
static int ota_download_precheck(uint32_t app_offset, image_hdr_t * ota_hdr)
{

	image_hdr_t *app_hdr = NULL;
	if (NULL == (app_hdr = OS_Malloc(sizeof(image_hdr_t)))) {
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "[%s:%d] malloc failed.\r\n", __func__, __LINE__);
		return LN_FALSE;
	}

	if (OTA_ERR_NONE != image_header_fast_read(app_offset, app_hdr)) {
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "failed to read app header.\r\n");
		goto ret_err;
	}

	if ((ota_hdr->image_type == IMAGE_TYPE_ORIGINAL) || \
		(ota_hdr->image_type == IMAGE_TYPE_ORIGINAL_XZ))
	{
		// check version
		if (((ota_hdr->ver.ver_major << 8) + ota_hdr->ver.ver_minor) == \
			((app_hdr->ver.ver_major << 8) + app_hdr->ver.ver_minor)) {
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "[%s:%d] same version, do not upgrade!\r\n",
				__func__, __LINE__);
		}

		// check file size
		if (((ota_hdr->img_size_orig + sizeof(image_hdr_t)) > APP_SPACE_SIZE) || \
			((ota_hdr->img_size_orig_xz + sizeof(image_hdr_t)) > OTA_SPACE_SIZE)) {
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "[%s:%d] size check failed.\r\n", __func__, __LINE__);
			goto ret_err;
		}
	}
	else {
		//image type not support!
		goto ret_err;
	}

	OS_Free(app_hdr);
	return LN_TRUE;

ret_err:
	OS_Free(app_hdr);
	return LN_FALSE;
}

static int ota_persistent_start(void)
{
	if (NULL == temp4K_buf) {
		temp4K_buf = OS_Malloc(SECTOR_SIZE_4KB);
		if (NULL == temp4K_buf) {
			LOG(LOG_LVL_INFO, "failed to alloc 4KB!!!\r\n");
			return LN_FALSE;
		}
		memset(temp4K_buf, 0, SECTOR_SIZE_4KB);
	}

	temp4k_offset = 0;
	flash_ota_start_addr = OTA_SPACE_OFFSET;
	flash_ota_offset = 0;
	is_persistent_started = LN_TRUE;
	return LN_TRUE;
}

/**
 * @brief Save block to flash.
 *
 * @param buf
 * @param buf_len
 * @return return LN_TRUE on success, LN_FALSE on failure.
 */
static int ota_persistent_write(const char *buf, const int32_t buf_len)
{
	int part_len = SECTOR_SIZE_4KB; // we might have a buffer so large, that we need to write multiple 4K segments ...
	int buf_offset = 0; // ... and we need to keep track, what is already written

	if (!is_persistent_started) {
		return LN_TRUE;
	}

	if (temp4k_offset + buf_len < SECTOR_SIZE_4KB) {
		// just copy all buf data to temp4K_buf
		memcpy(temp4K_buf + temp4k_offset, buf, buf_len);
		temp4k_offset += buf_len;
		part_len = 0;
	}
	while (part_len >= SECTOR_SIZE_4KB) {           // so we didn't copy all data to buffer (part_len would be 0 then)
		// just copy part of buf to temp4K_buf
		part_len = temp4k_offset + buf_len - buf_offset - SECTOR_SIZE_4KB;		// beware, this can be > SECTOR_SIZE_4KB !!!
		memcpy(temp4K_buf + temp4k_offset, buf + buf_offset, buf_len - buf_offset - part_len);
		temp4k_offset += buf_len - buf_offset - part_len;
		buf_offset = buf_len - part_len;

		if (temp4k_offset >= SECTOR_SIZE_4KB) {
			// write to flash
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "write at flash: 0x%08x (temp4k_offset=%i)\r\n", flash_ota_start_addr + flash_ota_offset, temp4k_offset);

			if (flash_ota_offset == 0) {
				if (LN_TRUE != ota_download_precheck(APP_SPACE_OFFSET, (image_hdr_t *)temp4K_buf))
				{
					ADDLOG_DEBUG(LOG_FEATURE_OTA, "ota download precheck failed!\r\n");
					is_precheck_ok = LN_FALSE;
					return LN_FALSE;
				}
				is_precheck_ok = LN_TRUE;
			}

			hal_flash_erase(flash_ota_start_addr + flash_ota_offset, SECTOR_SIZE_4KB);
			hal_flash_program(flash_ota_start_addr + flash_ota_offset, SECTOR_SIZE_4KB, (uint8_t *)temp4K_buf);

			flash_ota_offset += SECTOR_SIZE_4KB;
			memset(temp4K_buf, 0, SECTOR_SIZE_4KB);
			temp4k_offset = 0;
		}
	}
	if (part_len > 0) {
		memcpy(temp4K_buf + temp4k_offset, buf + (buf_len - part_len), part_len);
		temp4k_offset += part_len;
	}

	return LN_TRUE;
}

/**
 * @brief save last block and clear flags.
 * @return return LN_TRUE on success, LN_FALSE on failure.
 */
static int ota_persistent_finish(void)
{
	if (!is_persistent_started) {
		return LN_FALSE;
	}

	// write to flash
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "write at flash: 0x%08x\r\n", flash_ota_start_addr + flash_ota_offset);
	hal_flash_erase(flash_ota_start_addr + flash_ota_offset, SECTOR_SIZE_4KB);
	hal_flash_program(flash_ota_start_addr + flash_ota_offset, SECTOR_SIZE_4KB, (uint8_t *)temp4K_buf);

	OS_Free(temp4K_buf);
	temp4K_buf = NULL;
	temp4k_offset = 0;

	flash_ota_offset = 0;
	is_persistent_started = LN_FALSE;
	return LN_TRUE;
}

static int update_ota_state(void)
{
	upg_state_t state = UPG_STATE_DOWNLOAD_OK;
	ln_nvds_set_ota_upg_state(state);
	return LN_TRUE;
}
/**
 * @brief check ota image header, body.
 * @return return LN_TRUE on success, LN_FALSE on failure.
 */
static int ota_verify_download(void)
{
	image_hdr_t ota_header;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "Succeed to verify OTA image content.\r\n");
	if (OTA_ERR_NONE != image_header_fast_read(OTA_SPACE_OFFSET, &ota_header)) {
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "failed to read ota header.\r\n");
		return LN_FALSE;
	}

	if (OTA_ERR_NONE != image_header_verify(&ota_header)) {
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "failed to verify ota header.\r\n");
		return LN_FALSE;
	}

	if (OTA_ERR_NONE != image_body_verify(OTA_SPACE_OFFSET, &ota_header)) {
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "failed to verify ota body.\r\n");
		return LN_FALSE;
	}

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "Succeed to verify OTA image content.\r\n");
	return LN_TRUE;
}

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "Ota start!\r\n");
	if (LN_TRUE != ota_persistent_start())
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Ota start error, exit...\r\n");
		return 0;
	}

	if (request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	do
	{
		//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);

		if (LN_TRUE != ota_persistent_write(writebuf, writelen))
		{
			//	ADDLOG_DEBUG(LOG_FEATURE_OTA, "ota write err.\r\n");
			return -1;
		}

		rtos_delay_milliseconds(10);
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
			}
		}
	} while ((towrite > 0) && (writelen >= 0));

	ota_persistent_finish();
	is_ready_to_verify = LN_TRUE;
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "cb info: recv %d finished, no more data to deal with.\r\n", towrite);


	ADDLOG_DEBUG(LOG_FEATURE_OTA, "http client job done, exit...\r\n");
	if (LN_TRUE == is_precheck_ok)
	{
		if ((LN_TRUE == is_ready_to_verify) && (LN_TRUE == ota_verify_download()))
		{
			update_ota_state();
			//ln_chip_reboot();
		}
		else
		{
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "Veri bad\r\n");
		}
	}
	else
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Precheck bad\r\n");
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
	res = hal_flash_read(startaddr, readlen, (uint8_t *)buffer);
	return res;
}

#endif // PLATFORM_LN882H


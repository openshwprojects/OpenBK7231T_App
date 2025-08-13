#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../httpserver/new_http.h"
#include "../../logging/logging.h"

#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#if PLATFORM_ESPIDF
#include "esp_flash.h"
#include "esp_pm.h"
#else
#include "esp_image_format.h"
#include "spi_flash.h"
#define esp_flash_read(a,b,c,d) spi_flash_read(c,b,d)
#define OTA_WITH_SEQUENTIAL_WRITES OTA_SIZE_UNKNOWN
#define esp_ota_abort esp_ota_end
#endif


int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "Ota start!\r\n");
	esp_err_t err;
	esp_ota_handle_t update_handle = 0;
	const esp_partition_t* update_partition = NULL;
	const esp_partition_t* running = esp_ota_get_running_partition();
	update_partition = esp_ota_get_next_update_partition(NULL);
	if (request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	esp_wifi_set_ps(WIFI_PS_NONE);
	bool image_header_was_checked = false;
	do
	{
		if (image_header_was_checked == false)
		{
			esp_app_desc_t new_app_info;
			if (towrite > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
			{
				memcpy(&new_app_info, &writebuf[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "New firmware version: %s", new_app_info.version);

				esp_app_desc_t running_app_info;
				if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
				{
					ADDLOG_DEBUG(LOG_FEATURE_OTA, "Running firmware version: %s", running_app_info.version);
				}

				image_header_was_checked = true;

				err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
				if (err != ESP_OK)
				{
					ADDLOG_ERROR(LOG_FEATURE_OTA, "esp_ota_begin failed (%s)", esp_err_to_name(err));
					esp_ota_abort(update_handle);
					return -1;
				}
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "esp_ota_begin succeeded");
			}
			else
			{
				ADDLOG_ERROR(LOG_FEATURE_OTA, "received package is not fit len");
				esp_ota_abort(update_handle);
				return -1;
			}
		}
		err = esp_ota_write(update_handle, (const void*)writebuf, writelen);
		if (err != ESP_OK)
		{
			esp_ota_abort(update_handle);
			return -1;
		}

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

	ADDLOG_INFO(LOG_FEATURE_OTA, "OTA in progress: 100%%, total Write binary data length: %d", total);

	err = esp_ota_end(update_handle);
	if (err != ESP_OK)
	{
		if (err == ESP_ERR_OTA_VALIDATE_FAILED)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Image validation failed, image is corrupted");
		}
		else
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "esp_ota_end failed (%s)!", esp_err_to_name(err));
		}
		return -1;
	}
	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		return -1;
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
	res = esp_flash_read(NULL, (void*)buffer, startaddr, readlen);
	return res;
}

#endif


#include "../../new_common.h"
#include "../hal_ota.h"
#include "../../httpserver/new_http.h"
#include "../../logging/logging.h"
#include "hal_88pax01.h"
#include "layout.h"
#include "image.h"
#include "crc.h"
#include "spi_flash.h"
#include "slots.h"

int HAL_FlashRead(char *buffer, int readlen, int startaddr);

int http_rest_post_flash(http_request_t *request, int startaddr, int maxaddr)
{
	(void)startaddr;
	(void)maxaddr;

	int total = 0;
	int towrite = request->bodylen;
	char *writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_INFO(LOG_FEATURE_OTA,
	            "[88PAX01] Starting Web OTA (content-length=%d)",
	            request->contentLength);

	if (request->contentLength <= 0 ||
	    (uint32_t)request->contentLength > OTA_BUF_MAX_SIZE) {
		ADDLOG_ERROR(LOG_FEATURE_OTA,
		             "[88PAX01] Invalid OTA size %d (max %u)",
		             request->contentLength, (unsigned)OTA_BUF_MAX_SIZE);
		http_setup(request, httpMimeTypeJson);
		hprintf255(request, "{\"error\":-1,\"msg\":\"Invalid OTA size\"}");
		poststr(request, NULL);
		return 0;
	}

	uint8_t *ota_buf = (uint8_t *)OTA_BUF_MEM_BASE;

	if (request->contentLength >= 0)
		towrite = request->contentLength;

	do {
		if (writelen > 0) {
			if ((uint32_t)(total + writelen) <= OTA_BUF_MAX_SIZE)
				memcpy(ota_buf + total, writebuf, (size_t)writelen);
			total += writelen;
			towrite -= writelen;
		}
		if (towrite > 0) {
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if (writelen <= 0)
				break;
		}
	} while (towrite > 0 && writelen >= 0);

	if (total <= 0 || (request->contentLength > 0 && total != request->contentLength)) {
		ADDLOG_ERROR(LOG_FEATURE_OTA,
		             "[88PAX01] Incomplete OTA upload (%d / %d bytes)",
		             total, request->contentLength);
		http_setup(request, httpMimeTypeJson);
		hprintf255(request, "{\"error\":-2,\"size\":%d}", total);
		poststr(request, NULL);
		return 0;
	}

	slot_info_t infos[FLASH_SLOT_COUNT];
	for (int i = 0; i < FLASH_SLOT_COUNT; i++)
		slot_inspect(i, &infos[i], 0);
	int best = slot_select_best(infos);
	int target_slot = (best == 0) ? 1 : 0;

	uint32_t max_seq = 1;
	for (int i = 0; i < FLASH_SLOT_COUNT; i++) {
		if (infos[i].status == IMG_OK && infos[i].hdr->sequence >= max_seq)
			max_seq = infos[i].hdr->sequence;
	}

	image_header_t *hdr = (image_header_t *)ota_buf;
	uint32_t flash_bytes = 0;

	if ((uint32_t)total > sizeof(image_header_t) && hdr->magic == IMAGE_MAGIC) {
		uint32_t psize = hdr->payload_size;
		if (psize == 0 || sizeof(image_header_t) + psize > (uint32_t)total ||
		    psize > APP_CODE_MAX) {
			http_setup(request, httpMimeTypeJson);
			hprintf255(request, "{\"error\":-3,\"msg\":\"Bad HPDJ payload_size\"}");
			poststr(request, NULL);
			return 0;
		}
		uint32_t calc = crc32_compute(ota_buf + sizeof(image_header_t), psize);
		if (calc != hdr->payload_crc32) {
			http_setup(request, httpMimeTypeJson);
			hprintf255(request, "{\"error\":-4,\"msg\":\"HPDJ payload CRC mismatch\"}");
			poststr(request, NULL);
			return 0;
		}
		if (hdr->sequence <= max_seq) {
			hdr->sequence = max_seq + 1;
			hdr->header_crc32 = crc32_compute(hdr, IMAGE_HEADER_SIZE - 4);
		}
		flash_bytes = (uint32_t)sizeof(image_header_t) + psize;
	} else {
		if ((uint32_t)total + IMAGE_HEADER_SIZE > OTA_BUF_MAX_SIZE) {
			http_setup(request, httpMimeTypeJson);
			hprintf255(request, "{\"error\":-5,\"msg\":\"Raw binary exceeds slot capacity\"}");
			poststr(request, NULL);
			return 0;
		}
		uint32_t psize = (uint32_t)total;
		memmove(ota_buf + sizeof(image_header_t), ota_buf, psize);
		memset(hdr, 0, sizeof(*hdr));
		hdr->magic          = IMAGE_MAGIC;
		hdr->header_version = IMAGE_HEADER_VERSION;
		hdr->image_type     = IMAGE_TYPE_APP;
		hdr->version        = 0x01000000u;
		hdr->sequence       = max_seq + 1;
		hdr->payload_size   = psize;
		hdr->load_address   = APP_LOAD_ADDR;
		hdr->entry_address  = APP_LOAD_ADDR;
		hdr->payload_crc32  = crc32_compute(ota_buf + sizeof(image_header_t), psize);
		strncpy(hdr->name, "Open88PAX01", IMAGE_NAME_LEN);
		hdr->header_crc32   = crc32_compute(hdr, IMAGE_HEADER_SIZE - 4);
		flash_bytes         = (uint32_t)sizeof(image_header_t) + psize;
	}

	ADDLOG_INFO(LOG_FEATURE_OTA,
	            "[88PAX01] Writing %u bytes to Slot %c (0x%06X), seq=%u...",
	            (unsigned)flash_bytes, slot_letter(target_slot),
	            (unsigned)slot_offset(target_slot), (unsigned)hdr->sequence);

	if (slot_write(target_slot, ota_buf, flash_bytes, NULL) != FLASH_OK) {
		http_setup(request, httpMimeTypeJson);
		hprintf255(request, "{\"error\":-6,\"msg\":\"SPI flash write failed\"}");
		poststr(request, NULL);
		return 0;
	}

	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d,\"slot\":\"%c\",\"seq\":%u}",
	           total, slot_letter(target_slot), (unsigned)hdr->sequence);
	poststr(request, NULL);
	return 0;
}

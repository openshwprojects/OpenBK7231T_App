#ifdef PLATFORM_RTL8196C

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"
#include "hal_rtl8196c.h"
#include "flash_layout.h"
#include "ota.h"

extern int http_rest_error(http_request_t *request, int code, char *msg);

int http_rest_post_flash(http_request_t *request, int startaddr, int maxaddr)
{
    (void)startaddr;
    (void)maxaddr;

    int total = 0;
    int towrite = request->bodylen;
    char *writebuf = request->bodystart;
    int writelen = request->bodylen;

    if (request->contentLength <= 0 || (uint32_t)request->contentLength > WEB_OTA_MAX_SIZE) {
        ADDLOG_ERROR(LOG_FEATURE_OTA, "Invalid OTA content-length %d", request->contentLength);
        return http_rest_error(request, -1, "invalid content-length");
    }
    towrite = request->contentLength;
    uint8_t *stage = (uint8_t *)(uintptr_t)WEB_OTA_STAGE_ADDR;

    while (towrite > 0 && writelen >= 0) {
        if (writelen > 0) {
            if ((uint32_t)(total + writelen) > WEB_OTA_MAX_SIZE)
                return http_rest_error(request, -1, "image too large");
            memcpy(stage + total, writebuf, (size_t)writelen);
            total += writelen;
            towrite -= writelen;
        }
        if (towrite > 0) {
            writebuf = request->received;
            writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
            if (writelen <= 0)
                break;
        }
    }

    if (total != request->contentLength || total < 64) {
        ADDLOG_ERROR(LOG_FEATURE_OTA, "Incomplete OTA upload (%d / %d)", total, request->contentLength);
        return http_rest_error(request, -1, "incomplete upload");
    }

    /* Support both RAPP-wrapped images and raw .bin images */
    const uint8_t *payload = stage;
    uint32_t payload_len = (uint32_t)total;
    const struct app_header *ah = (const struct app_header *)stage;
    if (ah->magic == APP_MAGIC) {
        if (!app_image_valid_mem(stage, (uint32_t)total, WEB_OTA_MAX_SIZE)) {
            ADDLOG_ERROR(LOG_FEATURE_OTA, "RAPP CRC mismatch");
            return http_rest_error(request, -1, "CRC mismatch");
        }
        /* If the SPI flash partition table is installed, also program the inactive A/B slot */
        struct part_table pt;
        if (part_table_read(&pt, FLASH_PARTTABLE_OFF) == 0)
            ota_flash_inactive_slot(stage, (uint32_t)total, 0, 0, 0);
        payload = stage + sizeof(*ah);
        payload_len = ah->length;
        memmove(stage, payload, payload_len);
    }

    rtl8196c_vflash_init_if_needed();
    vflash_hdr->pending_ota_len = payload_len;

    ADDLOG_INFO(LOG_FEATURE_OTA, "RTL8196C Web OTA staged %u bytes at 0x%08x",
                (unsigned)payload_len, WEB_OTA_STAGE_ADDR);
    http_setup(request, httpMimeTypeJson);
    hprintf255(request, "{\"size\":%d}", total);
    poststr(request, NULL);
    CFG_IncrementOTACount();
    RESET_ScheduleModuleReset(2);
    return 0;
}

#endif /* PLATFORM_RTL8196C */

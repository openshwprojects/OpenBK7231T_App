#if PLATFORM_RDA5981

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"

#include "rda5981_ota.h"

uint32_t OTA_Offset = 0x18095000;

static int flash_write(unsigned int addr, char* buf, unsigned int len)
{
	if(len == 0) return 0;
	int ret;
	int left;
	unsigned int addr_t, len_t;
	char* temp_buf = NULL;
	addr_t = addr & 0xffffff00;
	len_t = addr - addr_t + len;
	if(len_t % 256)
		len_t += 256 - len_t % 256;
	//printf("addr %x addr_t %x\r\n", addr, addr_t);
	//printf("len %d len_t %d\r\n", len, len_t);
	temp_buf = (char*)malloc(256);
	if(temp_buf == NULL)
		return -1;

	ret = rda5981_read_flash(addr_t, temp_buf, 256);
	if(ret)
	{
		free(temp_buf);
		return -1;
	}
	left = 256 - (addr - addr_t);
	if(len < left)
		memcpy(temp_buf + addr - addr_t, buf, len);//256-(addr-addr_t));
	else
		memcpy(temp_buf + addr - addr_t, buf, left);
	ret = rda5981_write_flash(addr_t, temp_buf, 256);
	if(ret)
	{
		free(temp_buf);
		return -1;
	}

	len_t -= 256;
	buf += 256 - (addr - addr_t);
	len -= 256 - (addr - addr_t);
	addr_t += 256;

	while(len_t != 0)
	{
		//printf("len_t %d buf %x len %d addr_t %x\r\n", len_t, buf, len, addr_t);
		if(len >= 256)
		{
			memcpy(temp_buf, buf, 256);
		}
		else
		{
			ret = rda5981_read_flash(addr_t, temp_buf, 256);
			if(ret)
			{
				free(temp_buf);
				return -1;
			}
			memcpy(temp_buf, buf, len);
		}
		ret = rda5981_write_flash(addr_t, temp_buf, 256);
		if(ret)
		{
			free(temp_buf);
			return -1;
		}
		len_t -= 256;
		buf += 256;
		len -= 256;
		addr_t += 256;
	}
	free(temp_buf);
	return 0;
}

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
		ret1 = flash_write(startaddr, writebuf, writelen);
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

#if defined(PLATFORM_W800) || defined(PLATFORM_W600) 

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"
#include "lwip/sockets.h"

#include "wm_internal_flash.h"
#include "wm_socket_fwup.h"
#include "wm_fwup.h"

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

#ifdef PLATFORM_W600
	int nRetCode = 0;
	char error_message[256];

	if (writelen < 0)
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "ABORTED: %d bytes to write", writelen);
		return http_rest_error(request, -20, "writelen < 0");
	}

	struct pbuf* p;

	//Data is uploaded in 1024 sized chunks, creating a bigger buffer just in case this assumption changes.
	//The code below is based on sdk\OpenW600\src\app\ota\wm_http_fwup.c
	char* Buffer = (char*)os_malloc(2048 + 3);
	memset(Buffer, 0, 2048 + 3);

	if (request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	int recvLen = 0;
	int totalLen = 0;
	//printf("\ntowrite %d writelen=%d\n", towrite, writelen);

	do
	{
		if (writelen > 0)
		{
			//bk_printf("Copying %d from writebuf to Buffer towrite=%d\n", writelen, towrite);
			memcpy(Buffer + 3, writebuf, writelen);

			if (recvLen == 0)
			{
				T_BOOTER* booter = (T_BOOTER*)(Buffer + 3);
				bk_printf("magic_no=%u, img_type=%u, zip_type=%u\n", booter->magic_no, booter->img_type, booter->zip_type);

				if (TRUE == tls_fwup_img_header_check(booter))
				{
					totalLen = booter->upd_img_len + sizeof(T_BOOTER);
					OTA_ResetProgress();
					OTA_SetTotalBytes(totalLen);
				}
				else
				{
					sprintf(error_message, "Image header check failed");
					nRetCode = -19;
					break;
				}

				nRetCode = socket_fwup_accept(0, ERR_OK);
				if (nRetCode != ERR_OK)
				{
					sprintf(error_message, "Firmware update startup failed");
					break;
				}
			}

			p = pbuf_alloc(PBUF_TRANSPORT, writelen + 3, PBUF_REF);
			if (!p)
			{
				sprintf(error_message, "Unable to allocate memory for buffer");
				nRetCode = -18;
				break;
			}

			if (recvLen == 0)
			{
				*Buffer = SOCKET_FWUP_START;
			}
			else if (recvLen == (totalLen - writelen))
			{
				*Buffer = SOCKET_FWUP_END;
			}
			else
			{
				*Buffer = SOCKET_FWUP_DATA;
			}

			*(Buffer + 1) = (writelen >> 8) & 0xFF;
			*(Buffer + 2) = writelen & 0xFF;
			p->payload = Buffer;
			p->len = p->tot_len = writelen + 3;

			nRetCode = socket_fwup_recv(0, p, ERR_OK);
			if (nRetCode != ERR_OK)
			{
				sprintf(error_message, "Firmware data processing failed");
				break;
			}
			else
			{
				OTA_IncrementProgress(writelen);
				recvLen += writelen;
				printf("Downloaded %d / %d\n", recvLen, totalLen);
			}

			towrite -= writelen;
		}

		if (towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if (writelen < 0)
			{
				sprintf(error_message, "recv returned %d - end of data - remaining %d", writelen, towrite);
				nRetCode = -17;
			}
		}
	} while ((nRetCode == 0) && (towrite > 0) && (writelen >= 0));

	tls_mem_free(Buffer);

	if (nRetCode != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, error_message);
		socket_fwup_err(0, nRetCode);
		return http_rest_error(request, nRetCode, error_message);
	}


#elif PLATFORM_W800
	int nRetCode = 0;
	char error_message[256];

	if (writelen < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "ABORTED: %d bytes to write", writelen);
		return http_rest_error(request, -20, "writelen < 0");
	}

	struct pbuf* p;

	//The code below is based on W600 code and adopted to the differences in sdk\OpenW800\src\app\ota\wm_http_fwup.c
	// fiexd crashing caused by not checking "writelen" before doing memcpy
	// e.g. if more than 2 packets arrived before next loop, writelen could be > 2048 !!
#define FWUP_MSG_SIZE			 3
#define MAX_BUFF_SIZE			 2048
	char* Buffer = (char*)os_malloc(MAX_BUFF_SIZE + FWUP_MSG_SIZE);

	if (!Buffer)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "ABORTED: failed to allocate buffer");
		return http_rest_error(request, -20, "");
	}

	if (request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	int recvLen = 0;
	int totalLen = 0;
	printf("\ntowrite %d writelen=%d\n", towrite, writelen);

	do
	{
		while (writelen > 0)
		{
			int actwrite = writelen < MAX_BUFF_SIZE ? writelen : MAX_BUFF_SIZE;	// mustn't write more than Buffers size! Will crash else!
			//bk_printf("Copying %d from writebuf to Buffer (writelen=%d) towrite=%d -- free_heap:%d\n", actwrite, writelen, towrite, xPortGetFreeHeapSize());
			memset(Buffer, 0, MAX_BUFF_SIZE + FWUP_MSG_SIZE);
			memcpy(Buffer + FWUP_MSG_SIZE, writebuf, actwrite);
			if (recvLen == 0)
			{
				IMAGE_HEADER_PARAM_ST *booter = (IMAGE_HEADER_PARAM_ST*)(Buffer + FWUP_MSG_SIZE);
				bk_printf("magic_no=%u, img_type=%u, zip_type=%u, signature=%u\n",
					booter->magic_no, booter->img_attr.b.img_type, booter->img_attr.b.zip_type, booter->img_attr.b.signature);

				if (TRUE == tls_fwup_img_header_check(booter))
				{
					totalLen = booter->img_len + sizeof(IMAGE_HEADER_PARAM_ST);
					if (booter->img_attr.b.signature)
					{
						totalLen += 128;
					}
				}
				else
				{
					sprintf(error_message, "Image header check failed");
					nRetCode = -19;
					break;
				}

				nRetCode = socket_fwup_accept(0, ERR_OK);
				if (nRetCode != ERR_OK)
				{
					sprintf(error_message, "Firmware update startup failed");
					break;
				}
			}

			p = pbuf_alloc(PBUF_TRANSPORT, actwrite + FWUP_MSG_SIZE, PBUF_REF);
			if (!p)
			{
				sprintf(error_message, "Unable to allocate memory for buffer");
				nRetCode = -18;
				break;
			}

			if (recvLen == 0)
			{
				*Buffer = SOCKET_FWUP_START;
			}
			else if (recvLen == (totalLen - actwrite))
			{
				*Buffer = SOCKET_FWUP_END;
			}
			else
			{
				*Buffer = SOCKET_FWUP_DATA;
			}

			*(Buffer + 1) = (actwrite >> 8) & 0xFF;
			*(Buffer + 2) = actwrite & 0xFF;
			p->payload = Buffer;
			p->len = p->tot_len = actwrite + FWUP_MSG_SIZE;

			nRetCode = socket_fwup_recv(0, p, ERR_OK);
			if (nRetCode != ERR_OK)
			{
				sprintf(error_message, "Firmware data processing failed");
				break;
			}
			else
			{
				recvLen += actwrite;
			}

			towrite -= actwrite;
			writelen -= actwrite;	// calculate, how much is left to write
			writebuf += actwrite;	// in case, we only wrote part of buffer, advance in buffer
		}

		if (towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if (writelen < 0)
			{
				sprintf(error_message, "recv returned %d - end of data - remaining %d", writelen, towrite);
				nRetCode = -17;
			}
		}
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Downloaded %d / %d", recvLen, totalLen);
		rtos_delay_milliseconds(10);	// give some time for flashing - will else increase used memory fast 
	} while ((nRetCode == 0) && (towrite > 0) && (writelen >= 0));
	bk_printf("Download completed (%d / %d)\n", recvLen, totalLen);
	if (Buffer) os_free(Buffer);
	if (p) pbuf_free(p);


	if (nRetCode != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, error_message);
		socket_fwup_err(0, nRetCode);
		return http_rest_error(request, nRetCode, error_message);
	}



#endif
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	CFG_IncrementOTACount();
	return 0;
}

int HAL_FlashRead(char*buffer, int readlen, int startaddr) {
	int res;
	res = tls_fls_read(startaddr, (uint8_t*)buffer, readlen);
	return res;
}

#endif

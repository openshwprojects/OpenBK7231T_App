#ifdef PLATFORM_REALTEK

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_ota.h"


#include "flash_api.h"
#include "device_lock.h"
#include "sys_api.h"

extern flash_t flash;

#if PLATFORM_RTL87X0C

#include "ota_8710c.h"

extern uint32_t sys_update_ota_get_curr_fw_idx(void);
extern uint32_t sys_update_ota_prepare_addr(void);
extern void sys_disable_fast_boot(void);
extern void get_fw_info(uint32_t* targetFWaddr, uint32_t* currentFWaddr, uint32_t* fw1_sn, uint32_t* fw2_sn);
static flash_t flash_ota;

#elif PLATFORM_RTL8710B

#include "rtl8710b_ota.h"

extern uint32_t current_fw_idx;

#elif PLATFORM_RTL8710A

extern uint32_t current_fw_idx;

#undef DEFAULT_FLASH_LEN
#define DEFAULT_FLASH_LEN 0x400000

#elif PLATFORM_RTL8720D

#include "rtl8721d_boot.h"
#include "rtl8721d_ota.h"
#include "diag.h"
#include "wdt_api.h"

extern uint32_t current_fw_idx;
extern uint8_t flash_size_8720;

#undef DEFAULT_FLASH_LEN
#define DEFAULT_FLASH_LEN (flash_size_8720 << 20)

#elif PLATFORM_REALTEK_NEW

#include "ameba_ota.h"
extern uint32_t current_fw_idx;
extern uint32_t IMG_ADDR[OTA_IMGID_MAX][2];
extern uint8_t flash_size_8720;
#undef DEFAULT_FLASH_LEN
#define DEFAULT_FLASH_LEN (flash_size_8720 << 20)

#endif

int HAL_FlashRead(char*buffer, int readlen, int startaddr) {
	int res;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, startaddr, readlen, (uint8_t*)buffer);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return res;
}

#if PLATFORM_RTL87X0C
int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);


	uint32_t NewFWLen = 0, NewFWAddr = 0;
	uint32_t address = 0;
	uint32_t curr_fw_idx = 0;
	uint32_t flash_checksum = 0;
	uint32_t targetFWaddr;
	uint32_t currentFWaddr;
	uint32_t fw1_sn;
	uint32_t fw2_sn;
	_file_checksum file_checksum;
	file_checksum.u = 0;
	unsigned char sig_backup[32];
	int ret = 1;

	if (request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}
	NewFWAddr = sys_update_ota_prepare_addr();
	if (NewFWAddr == -1)
	{
		ret = -1;
		goto update_ota_exit;
	}
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);
	ADDLOG_INFO(LOG_FEATURE_OTA, "Current FW addr: 0x%08X, target FW addr: 0x%08X, fw1 sn: %u, fw2 sn: %u", currentFWaddr, targetFWaddr, fw1_sn, fw2_sn);
	curr_fw_idx = sys_update_ota_get_curr_fw_idx();
	ADDLOG_INFO(LOG_FEATURE_OTA, "Current firmware index is %d", curr_fw_idx);
	int reserase = update_ota_erase_upg_region(towrite, 0, NewFWAddr);
	NewFWLen = towrite;
	if (reserase == -1)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Erase failed");
		ret = -1;
		goto update_ota_exit;
	}
	if (NewFWAddr != ~0x0)
	{
		address = NewFWAddr;
		ADDLOG_INFO(LOG_FEATURE_OTA, "Start to read data %i bytes", NewFWLen);
	}
	do
	{
		// back up signature and only write it to flash till the end of OTA
		if (startaddr < 32)
		{
			memcpy(sig_backup + startaddr, writebuf, (startaddr + writelen > 32 ? (32 - startaddr) : writelen));
			memset(writebuf, 0xFF, (startaddr + writelen > 32 ? (32 - startaddr) : writelen));
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "sig_backup for% d bytes from index% d", (startaddr + writelen > 32 ? (32 - startaddr) : writelen), startaddr);
		}

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (flash_burst_write(&flash_ota, address + startaddr, writelen, (uint8_t*)writebuf) < 0)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Write stream failed");
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			ret = -1;
			goto update_ota_exit;
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		rtos_delay_milliseconds(10);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;

		// checksum attached at file end
		if (startaddr + writelen > NewFWLen - 4)
		{
			file_checksum.c[0] = writebuf[writelen - 4];
			file_checksum.c[1] = writebuf[writelen - 3];
			file_checksum.c[2] = writebuf[writelen - 2];
			file_checksum.c[3] = writebuf[writelen - 1];
		}
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
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written, verifying checksum", total);
	uint8_t* buf = (uint8_t*)os_malloc(2048);
	memset(buf, 0, 2048);
	// read flash data back and calculate checksum
	for (int i = 0; i < NewFWLen; i += 2048)
	{
		int k;
		int rlen = (startaddr - 4 - i) > 2048 ? 2048 : (startaddr - 4 - i);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash_ota, NewFWAddr + i, rlen, buf);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		for (k = 0; k < rlen; k++)
		{
			if (i + k < 32)
			{
				flash_checksum += sig_backup[i + k];
			}
			else
			{
				flash_checksum += buf[k];
			}
		}
	}

	ADDLOG_INFO(LOG_FEATURE_OTA, "flash checksum 0x%8x attached checksum 0x%8x", flash_checksum, file_checksum.u);

	if (file_checksum.u != flash_checksum)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "The checksum is wrong!");
		ret = -1;
		goto update_ota_exit;
	}
	ret = update_ota_signature(sig_backup, NewFWAddr);
	if (ret == -1)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Update signature fail");
		goto update_ota_exit;
	}
update_ota_exit:
	if (ret != -1)
	{
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_write_word(&flash, targetFWaddr, 4294967295);
		flash_write_word(&flash, currentFWaddr, 0);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		sys_disable_fast_boot();
		if (buf) free(buf);
	}
	else
	{
		if (buf) free(buf);
		return http_rest_error(request, ret, "error");
	}


	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	CFG_IncrementOTACount();
	return 0;
}

#elif PLATFORM_RTL8710B
int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	int NewImg2BlkSize = 0;
	uint32_t NewFWLen = 0, NewFWAddr = 0;
	uint32_t address = 0;
	uint32_t flash_checksum = 0;
	uint32_t ota2_addr = OTA2_ADDR;
	union { uint32_t u; unsigned char c[4]; } file_checksum;
	unsigned char sig_backup[32];
	int ret = 1;
	char* hbuf = NULL;
	bool foundhdr = false;

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
	int ota1_len = 0, ota2_len = 0;
	char* msg = "Incorrect amount of bytes received: %i, required: %i";

	writebuf = request->received;
	writelen = recv(request->fd, &ota1_len, sizeof(ota1_len), 0);
	if (writelen != sizeof(ota1_len))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, msg, writelen, sizeof(ota1_len));
		ret = -1;
		goto update_ota_exit;
	}

	writebuf = request->received;
	writelen = recv(request->fd, &ota2_len, sizeof(ota2_len), 0);
	if (writelen != sizeof(ota2_len))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, msg, writelen, sizeof(ota2_len));
		ret = -1;
		goto update_ota_exit;
	}
	ADDLOG_INFO(LOG_FEATURE_OTA, "OTA1 len: %u, OTA2 len: %u", ota1_len, ota2_len);

	if (ota1_len <= 0 || ota2_len <= 0)
	{
		ret = -1;
		goto update_ota_exit;
	}
	towrite -= 8;

	if (current_fw_idx == OTA_INDEX_1)
	{
		towrite = ota2_len;
		// skip ota1
		int toskip = ota1_len;
		do
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax < toskip ? request->receivedLenmax : toskip, 0);
			ADDLOG_EXTRADEBUG(LOG_FEATURE_OTA, "Skipping %i at %i", writelen, total);
			total += writelen;
			toskip -= writelen;
		} while ((toskip > 0) && (writelen >= 0));
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Skipped %i bytes, towrite: %i", total, towrite);
	}
	else
	{
		towrite = ota1_len;
	}

	writelen = 0;
	total = 0;
	NewFWAddr = update_ota_prepare_addr();
	ADDLOG_INFO(LOG_FEATURE_OTA, "OTA address: %#010x, len: %u", NewFWAddr - SPI_FLASH_BASE, towrite);
	if (NewFWAddr == -1 || NewFWAddr == 0xFFFFFFFF)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Wrong OTA address:", NewFWAddr);
		goto update_ota_exit;
	}
	NewFWLen = towrite;
	if (NewFWAddr == OTA1_ADDR)
	{
		if (NewFWLen > (OTA2_ADDR - OTA1_ADDR))
		{
			// firmware size too large
			ret = -1;
			ADDLOG_ERROR(LOG_FEATURE_OTA, "image size should not cross OTA2");
			goto update_ota_exit;
		}
	}
	else if (NewFWAddr == OTA2_ADDR)
	{
		if (NewFWLen > (0x195000 + SPI_FLASH_BASE - OTA2_ADDR))
		{
			ret = -1;
			ADDLOG_ERROR(LOG_FEATURE_OTA, "image size crosses OTA2 boundary");
			goto update_ota_exit;
		}
	}
	else if (NewFWAddr == 0xFFFFFFFF)
	{
		ret = -1;
		ADDLOG_ERROR(LOG_FEATURE_OTA, "update address is invalid");
		goto update_ota_exit;
	}
	address = NewFWAddr - SPI_FLASH_BASE;
	NewImg2BlkSize = ((NewFWLen - 1) / 4096) + 1;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	for (int i = 0; i < NewImg2BlkSize; i++)
	{
		flash_erase_sector(&flash, address + i * 4096);
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	OTF_Mask(1, (address), NewImg2BlkSize, 1);

	do
	{
		if (startaddr < 32)
		{
			memcpy(sig_backup + startaddr, writebuf, (startaddr + writelen > 32 ? (32 - startaddr) : writelen));
			memset(writebuf, 0xFF, (startaddr + writelen > 32 ? (32 - startaddr) : writelen));
			ADDLOG_DEBUG(LOG_FEATURE_OTA, "sig_backup for% d bytes from index% d", (startaddr + writelen > 32 ? (32 - startaddr) : writelen), startaddr);
		}
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (flash_burst_write(&flash, address + startaddr, writelen, (uint8_t*)writebuf) < 0)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Write stream failed");
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			ret = -1;
			goto update_ota_exit;
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		rtos_delay_milliseconds(10);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;

		if (startaddr + writelen > NewFWLen - 4)
		{
			file_checksum.c[0] = writebuf[writelen - 4];
			file_checksum.c[1] = writebuf[writelen - 3];
			file_checksum.c[2] = writebuf[writelen - 2];
			file_checksum.c[3] = writebuf[writelen - 1];
		}

		if (towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax < towrite ? request->receivedLenmax : towrite, 0);
			if (writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while ((towrite > 0) && (writelen >= 0));

	uint8_t* buf = (uint8_t*)os_malloc(2048);
	memset(buf, 0, 2048);
	for (int i = 0; i < NewFWLen; i += 2048)
	{
		int k;
		int rlen = (startaddr - 4 - i) > 2048 ? 2048 : (startaddr - 4 - i);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, address + i, rlen, buf);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		for (k = 0; k < rlen; k++)
		{
			if (i + k < 32)
			{
				flash_checksum += sig_backup[i + k];
			}
			else
			{
				flash_checksum += buf[k];
			}
		}
	}
	ADDLOG_INFO(LOG_FEATURE_OTA, "Update file size = %d flash checksum 0x%8x attached checksum 0x%8x", NewFWLen, flash_checksum, file_checksum.u);
	OTF_Mask(1, (address), NewImg2BlkSize, 0);
	if (file_checksum.u == flash_checksum)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "Update OTA success!");

		ret = 0;
	}
	else
	{
		/*if checksum error, clear the signature zone which has been
		written in flash in case of boot from the wrong firmware*/
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, address);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		ret = -1;
	}
	// make it to be similar to rtl8720c - write signature only after success - to prevent boot failure if something goes wrong
	if (flash_burst_write(&flash, address + 16, 16, sig_backup + 16) < 0)
	{
		ret = -1;
	}
	else
	{
		if (flash_burst_write(&flash, address, 16, sig_backup) < 0)
		{
			ret = -1;
		}
	}
	if (current_fw_idx != OTA_INDEX_1)
	{
		// receive file fully
		int toskip = ota2_len;
		do
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax < toskip ? request->receivedLenmax : toskip, 0);
			total += writelen;
			toskip -= writelen;
		} while ((toskip > 0) && (writelen >= 0));
	}
update_ota_exit:
	if (ret != -1)
	{
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (current_fw_idx == OTA_INDEX_1)
		{
			OTA_Change(OTA_INDEX_2);
			//ota_write_ota2_addr(OTA2_ADDR);
		}
		else
		{
			OTA_Change(OTA_INDEX_1);
			//ota_write_ota2_addr(0xffffffff);
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		if (buf) free(buf);
	}
	else
	{
		if (buf) free(buf);
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

#elif PLATFORM_RTL8710A

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	uint32_t NewFWLen = 0, NewFWAddr = 0;
	uint32_t address = 0;
	uint32_t flash_checksum = 0;
	union { uint32_t u; unsigned char c[4]; } file_checksum;
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

	NewFWAddr = update_ota_prepare_addr();
	if (NewFWAddr == -1)
	{
		goto update_ota_exit;
	}

	int reserase = update_ota_erase_upg_region(towrite, 0, NewFWAddr);
	NewFWLen = towrite;
	if (reserase == -1)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Erase failed");
		ret = -1;
		goto update_ota_exit;
	}

	address = NewFWAddr;
	ADDLOG_INFO(LOG_FEATURE_OTA, "Start to read data %i bytes, flash address: 0x%8x", NewFWLen, address);

	do
	{
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		if (flash_stream_write(&flash, address + startaddr, writelen, (uint8_t*)writebuf) < 0)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Write stream failed");
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			ret = -1;
			goto update_ota_exit;
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		rtos_delay_milliseconds(10);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;

		// checksum attached at file end
		if (startaddr + writelen > NewFWLen - 4)
		{
			file_checksum.c[0] = writebuf[writelen - 4];
			file_checksum.c[1] = writebuf[writelen - 3];
			file_checksum.c[2] = writebuf[writelen - 2];
			file_checksum.c[3] = writebuf[writelen - 1];
		}
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
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written, verifying checksum %u", total, flash_checksum);
	uint8_t* buf = (uint8_t*)os_malloc(512);
	memset(buf, 0, 512);
	// read flash data back and calculate checksum
	for (int i = 0; i < NewFWLen; i += 512)
	{
		int k;
		int rlen = (NewFWLen - 4 - i) > 512 ? 512 : (NewFWLen - 4 - i);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, NewFWAddr + i, rlen, buf);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		for (k = 0; k < rlen; k++)
		{
			flash_checksum += buf[k];
		}
	}

	ADDLOG_INFO(LOG_FEATURE_OTA, "flash checksum 0x%8x attached checksum 0x%8x", flash_checksum, file_checksum.u);
	delay_ms(50);

	ret = update_ota_checksum(&file_checksum, flash_checksum, NewFWAddr);
	if (ret == -1)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "The checksum is wrong!");
		goto update_ota_exit;
	}

update_ota_exit:
	if (ret != -1)
	{
		//device_mutex_lock(RT_DEV_LOCK_FLASH);
		//device_mutex_unlock(RT_DEV_LOCK_FLASH);
		if (buf) free(buf);
	}
	else
	{
		if (buf) free(buf);
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

#elif PLATFORM_RTL8720D
int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	int ret = -1;
	uint32_t ota_target_index = OTA_INDEX_2;
	update_file_hdr* pOtaFileHdr;
	update_file_img_hdr* pOtaFileImgHdr;
	update_ota_target_hdr OtaTargetHdr;
	uint32_t ImageCnt, TempLen;
	update_dw_info DownloadInfo[MAX_IMG_NUM];

	int size = 0;
	int read_bytes;
	int read_bytes_buf;
	uint8_t* buf;
	uint32_t OtaFg = 0;
	uint32_t IncFg = 0;
	int RemainBytes = 0;
	uint32_t SigCnt = 0;
	uint32_t TempCnt = 0;
	uint8_t* signature;
	uint32_t sector_cnt = 0;

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

	if (flash_size_8720 == 2)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Only 2MB flash detected - OTA is not supported");
		ret = -1;
		goto update_ota_exit;
	}

	memset((uint8_t*)&OtaTargetHdr, 0, sizeof(update_ota_target_hdr));

	DBG_INFO_MSG_OFF(MODULE_FLASH);

	ADDLOG_INFO(LOG_FEATURE_OTA, "Current firmware index is %d", current_fw_idx + 1);
	if (current_fw_idx == OTA_INDEX_1)
	{
		ota_target_index = OTA_INDEX_2;
	}
	else
	{
		ota_target_index = OTA_INDEX_1;
	}

	// get ota header
	writebuf = request->received;
	writelen = recv(request->fd, writebuf, 16, 0);
	if (writelen != 16)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "failed to recv file header");
		ret = -1;
		goto update_ota_exit;
	}

	pOtaFileHdr = (update_file_hdr*)writebuf;
	pOtaFileImgHdr = (update_file_img_hdr*)(writebuf + 8);

	OtaTargetHdr.FileHdr.FwVer = pOtaFileHdr->FwVer;
	OtaTargetHdr.FileHdr.HdrNum = pOtaFileHdr->HdrNum;

	writelen = recv(request->fd, writebuf + 16, (pOtaFileHdr->HdrNum * pOtaFileImgHdr->ImgHdrLen) - 8, 0);
	writelen = (pOtaFileHdr->HdrNum * pOtaFileImgHdr->ImgHdrLen) + 8;

	// verify ota header
	if (!get_ota_tartget_header((uint8_t*)writebuf, writelen, &OtaTargetHdr, ota_target_index))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Get OTA header failed");
		goto update_ota_exit;
	}

	//ADDLOG_INFO(LOG_FEATURE_OTA, "Erasing...");
	for (int i = 0; i < OtaTargetHdr.ValidImgCnt; i++)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "Target addr:0x%08x, img len: %i", OtaTargetHdr.FileImgHdr[i].FlashAddr, OtaTargetHdr.FileImgHdr[i].ImgLen);
		if (OtaTargetHdr.FileImgHdr[i].ImgLen >= 0x1A8000)
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Img%i too big, skipping", i);
			OtaTargetHdr.FileImgHdr[i].ImgLen = 0;
			continue;
		}
		//watchdog_stop();
		//erase_ota_target_flash(OtaTargetHdr.FileImgHdr[i].FlashAddr, OtaTargetHdr.FileImgHdr[i].ImgLen);
		//watchdog_start();
	}

	memset((uint8_t*)DownloadInfo, 0, MAX_IMG_NUM * sizeof(update_dw_info));

	ImageCnt = OtaTargetHdr.ValidImgCnt;
	for (uint32_t i = 0; i < ImageCnt; i++)
	{
		if (OtaTargetHdr.FileImgHdr[i].ImgLen == 0)
		{
			DownloadInfo[i].ImageLen = 0;
			continue;
		}
		/* get OTA image and Write New Image to flash, skip the signature,
			not write signature first for power down protection*/
		DownloadInfo[i].ImgId = OTA_IMAG;
		DownloadInfo[i].FlashAddr = OtaTargetHdr.FileImgHdr[i].FlashAddr - SPI_FLASH_BASE + 8;
		DownloadInfo[i].ImageLen = OtaTargetHdr.FileImgHdr[i].ImgLen - 8; /*skip the signature*/
		DownloadInfo[i].ImgOffset = OtaTargetHdr.FileImgHdr[i].Offset;
	}

	/*initialize the reveiving counter*/
	TempLen = (OtaTargetHdr.FileHdr.HdrNum * OtaTargetHdr.FileImgHdr[0].ImgHdrLen) + sizeof(update_file_hdr);

	for (uint32_t i = 0; i < ImageCnt; i++)
	{
		if (DownloadInfo[i].ImageLen == 0) continue;
		/*the next image length*/
		RemainBytes = DownloadInfo[i].ImageLen;
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Remain: %i", RemainBytes);
		signature = &(OtaTargetHdr.Sign[i][0]);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		FLASH_EraseXIP(EraseSector, DownloadInfo[i].FlashAddr - SPI_FLASH_BASE);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		/*download the new firmware from server*/
		while (RemainBytes > 0)
		{
			buf = (uint8_t*)request->received;
			if (IncFg == 1)
			{
				IncFg = 0;
				read_bytes = read_bytes_buf;
			}
			else
			{
				memset(buf, 0, request->receivedLenmax);
				read_bytes = recv(request->fd, buf, request->receivedLenmax, 0);
				if (read_bytes == 0)
				{
					break; // Read end
				}
				if (read_bytes < 0)
				{
					//OtaImgSize = -1;
					//printf("\n\r[%s] Read socket failed", __FUNCTION__);
					//ret = -1;
					//goto update_ota_exit;
					break;
				}
				read_bytes_buf = read_bytes;
				TempLen += read_bytes;
			}

			if (TempLen > DownloadInfo[i].ImgOffset)
			{
				if (!OtaFg)
				{
					/*reach the desired image, the first packet process*/
					OtaFg = 1;
					TempCnt = TempLen - DownloadInfo[i].ImgOffset;
					if (TempCnt < 8)
					{
						SigCnt = TempCnt;
					}
					else
					{
						SigCnt = 8;
					}

					memcpy(signature, buf + read_bytes - TempCnt, SigCnt);

					if ((SigCnt < 8) || (TempCnt - 8 == 0))
					{
						continue;
					}

					buf = buf + (read_bytes - TempCnt + 8);
					read_bytes = TempCnt - 8;
				}
				else
				{
					/*normal packet process*/
					if (SigCnt < 8)
					{
						if (read_bytes < (int)(8 - SigCnt))
						{
							memcpy(signature + SigCnt, buf, read_bytes);
							SigCnt += read_bytes;
							continue;
						}
						else
						{
							memcpy(signature + SigCnt, buf, (8 - SigCnt));
							buf = buf + (8 - SigCnt);
							read_bytes -= (8 - SigCnt);
							SigCnt = 8;
							if (!read_bytes)
							{
								continue;
							}
						}
					}
				}

				RemainBytes -= read_bytes;
				if (RemainBytes < 0)
				{
					read_bytes = read_bytes - (-RemainBytes);
				}

				device_mutex_lock(RT_DEV_LOCK_FLASH);
				if (DownloadInfo[i].FlashAddr + size >= DownloadInfo[i].FlashAddr + sector_cnt * 4096)
				{
					sector_cnt++;
					FLASH_EraseXIP(EraseSector, DownloadInfo[i].FlashAddr - SPI_FLASH_BASE + sector_cnt * 4096);
				}
				if (ota_writestream_user(DownloadInfo[i].FlashAddr + size, read_bytes, buf) < 0)
				{
					ADDLOG_ERROR(LOG_FEATURE_OTA, "Writing failed");
					device_mutex_unlock(RT_DEV_LOCK_FLASH);
					ret = -1;
					goto update_ota_exit;
				}
				device_mutex_unlock(RT_DEV_LOCK_FLASH);
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "Written %i bytes at 0x%08x", read_bytes, DownloadInfo[i].FlashAddr + size);
				rtos_delay_milliseconds(5);
				size += read_bytes;
			}
		}

		if ((uint32_t)size != (OtaTargetHdr.FileImgHdr[i].ImgLen - 8))
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Received size != ota size");
			ret = -1;
			goto update_ota_exit;
		}

		/*update flag status*/
		size = 0;
		OtaFg = 0;
		IncFg = 1;
	}

	if (verify_ota_checksum(&OtaTargetHdr))
	{
		if (!change_ota_signature(&OtaTargetHdr, ota_target_index))
		{
			ADDLOG_ERROR(LOG_FEATURE_OTA, "Change signature failed");
			ret = -1;
			goto update_ota_exit;
		}
		ret = 0;
	}

update_ota_exit:
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
		total = RemainBytes;
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
#elif PLATFORM_REALTEK_NEW
int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{
	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);


	// Bad implementation. While it somewhat works, it is not recommended to use. HTTP OTA is preferable.
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

	flash_get_layout_info(IMG_BOOT, &IMG_ADDR[OTA_IMGID_BOOT][OTA_INDEX_1], NULL);
	flash_get_layout_info(IMG_BOOT_OTA2, &IMG_ADDR[OTA_IMGID_BOOT][OTA_INDEX_2], NULL);
	flash_get_layout_info(IMG_APP_OTA1, &IMG_ADDR[OTA_IMGID_APP][OTA_INDEX_1], NULL);
	flash_get_layout_info(IMG_APP_OTA2, &IMG_ADDR[OTA_IMGID_APP][OTA_INDEX_2], NULL);

	ota_context ctx;
	memset(&ctx, 0, sizeof(ota_context));
	ctx.otactrl = malloc(sizeof(update_ota_ctrl_info));
	memset(ctx.otactrl, 0, sizeof(update_ota_ctrl_info));

	ctx.otaTargetHdr = malloc(sizeof(update_ota_target_hdr));
	memset(ctx.otaTargetHdr, 0, sizeof(update_ota_target_hdr));
	update_file_hdr* pOtaFileHdr = (update_file_hdr*)(writebuf);
	ctx.otaTargetHdr->FileHdr.FwVer = pOtaFileHdr->FwVer;
	ctx.otaTargetHdr->FileHdr.HdrNum = pOtaFileHdr->HdrNum;

	uint32_t RevHdrLen = pOtaFileHdr->HdrNum * SUB_HEADER_LEN + HEADER_LEN;
	if (writelen < RevHdrLen)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "failed to recv file header");
		ret = -1;
		goto update_ota_exit;
	}

	if (!get_ota_tartget_header(&ctx, writebuf, RevHdrLen))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Get OTA header failed");
		ret = -1;
		goto update_ota_exit;
	}
	if (!ota_checkimage_layout(ctx.otaTargetHdr))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "ota_checkimage_layout failed");
		ret = -1;
		goto update_ota_exit;
	}
	ctx.otactrl->IsGetOTAHdr = 1;
	writebuf += RevHdrLen;
	// magic values to make it somewhat work.
	writelen -= RevHdrLen;
	towrite -= RevHdrLen;
	ctx.otactrl->NextImgLen = towrite;
	do
	{
		int size = download_packet_process(&ctx, writebuf, writelen);
		download_percentage(&ctx, size, ctx.otactrl->ImageLen);
		rtos_delay_milliseconds(10);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		total += writelen;
		towrite -= writelen;

		if (towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, 2048 < towrite ? 2048 : towrite, 0);
			if (writelen < 0)
			{
				ADDLOG_ERROR(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
				ret = -1;
				goto update_ota_exit;
			}
		}
	} while ((towrite > 0) && (writelen >= 0));

	//erase manifest
	flash_erase_sector(&flash, ctx.otactrl->FlashAddr);

	if (!verify_ota_checksum(ctx.otaTargetHdr, ctx.otactrl->targetIdx, ctx.otactrl->index))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "The checksum is wrong!");
		ret = -1;
		goto update_ota_exit;
	}

	if (!ota_update_manifest(ctx.otaTargetHdr, ctx.otactrl->targetIdx, ctx.otactrl->index))
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "Change signature failed!");
		ret = -1;
		goto update_ota_exit;
	}

update_ota_exit:
	ota_update_deinit(&ctx);
	if (ret != -1)
	{
		ADDLOG_INFO(LOG_FEATURE_OTA, "OTA is successful");
		sys_clear_ota_signature();
		//sys_recover_ota_signature();
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
#endif


#endif


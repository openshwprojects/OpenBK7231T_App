#include "../obk_config.h"

#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
#include "../new_pins.h"
#include "../jsmn/jsmn_h.h"
#include "../ota/ota.h"
#include "../hal/hal_wifi.h"
#include "../hal/hal_flashVars.h"
#include "../littlefs/our_lfs.h"
#include "lwip/sockets.h"

#if PLATFORM_XR809

#include <image/flash.h>

uint32_t flash_read(uint32_t flash, uint32_t addr, void* buf, uint32_t size);
#define FLASH_INDEX_XR809 0

#elif PLATFORM_BL602
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/dhcp.h>
#include <lwip/tcpip.h>
#include <lwip/ip_addr.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include <cli.h>
#include <hal_boot2.h>
#include <hal_sys.h>
#include <utils_sha256.h>
#include <bl_sys_ota.h>
#include <bl_mtd.h>
#include <bl_flash.h>
#elif PLATFORM_W600

#include "wm_socket_fwup.h"
#include "wm_fwup.h"

#elif PLATFORM_W800

#elif PLATFORM_LN882H

#elif PLATFORM_ESPIDF

#else

extern UINT32 flash_read(char* user_buf, UINT32 count, UINT32 address);

#endif

#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"

#ifndef OBK_DISABLE_ALL_DRIVERS
#include "../driver/drv_local.h"
#endif

#define MAX_JSON_VALUE_LENGTH   128


static int http_rest_error(http_request_t* request, int code, char* msg);

static int http_rest_get(http_request_t* request);
static int http_rest_post(http_request_t* request);
static int http_rest_app(http_request_t* request);

static int http_rest_post_pins(http_request_t* request);
static int http_rest_get_pins(http_request_t* request);

static int http_rest_get_channelTypes(http_request_t* request);
static int http_rest_post_channelTypes(http_request_t* request);

static int http_rest_get_seriallog(http_request_t* request);

static int http_rest_post_logconfig(http_request_t* request);
static int http_rest_get_logconfig(http_request_t* request);

#if ENABLE_LITTLEFS
static int http_rest_get_lfs_delete(http_request_t* request);
static int http_rest_get_lfs_file(http_request_t* request);
static int http_rest_post_lfs_file(http_request_t* request);
#endif

static int http_rest_post_reboot(http_request_t* request);
static int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr);
static int http_rest_get_flash(http_request_t* request, int startaddr, int len);
static int http_rest_get_flash_advanced(http_request_t* request);
static int http_rest_post_flash_advanced(http_request_t* request);

static int http_rest_get_info(http_request_t* request);

static int http_rest_get_dumpconfig(http_request_t* request);
static int http_rest_get_testconfig(http_request_t* request);

static int http_rest_post_channels(http_request_t* request);
static int http_rest_get_channels(http_request_t* request);

static int http_rest_get_flash_vars_test(http_request_t* request);

static int http_rest_post_cmd(http_request_t* request);


void init_rest() {
	HTTP_RegisterCallback("/api/", HTTP_GET, http_rest_get, 1);
	HTTP_RegisterCallback("/api/", HTTP_POST, http_rest_post, 1);
	HTTP_RegisterCallback("/app", HTTP_GET, http_rest_app, 1);
}

/* Extracts string token value into outBuffer (128 char). Returns true if the operation was successful. */
bool tryGetTokenString(const char* json, jsmntok_t* tok, char* outBuffer) {
	int length;
	if (tok == NULL || tok->type != JSMN_STRING) {
		return false;
	}

	length = tok->end - tok->start;

	//Don't have enough buffer
	if (length > MAX_JSON_VALUE_LENGTH) {
		return false;
	}

	memset(outBuffer, '\0', MAX_JSON_VALUE_LENGTH); //Wipe previous value
	strncpy(outBuffer, json + tok->start, length);
	return true;
}

static int http_rest_get(http_request_t* request) {
	ADDLOG_DEBUG(LOG_FEATURE_API, "GET of %s", request->url);

	if (!strcmp(request->url, "api/channels")) {
		return http_rest_get_channels(request);
	}

	if (!strcmp(request->url, "api/pins")) {
		return http_rest_get_pins(request);
	}
	if (!strcmp(request->url, "api/channelTypes")) {
		return http_rest_get_channelTypes(request);
	}
	if (!strcmp(request->url, "api/logconfig")) {
		return http_rest_get_logconfig(request);
	}

	if (!strncmp(request->url, "api/seriallog", 13)) {
		return http_rest_get_seriallog(request);
	}

#if ENABLE_LITTLEFS
	if (!strcmp(request->url, "api/fsblock")) {
		uint32_t newsize = CFG_GetLFS_Size();
		uint32_t newstart = (LFS_BLOCKS_END - newsize);

		newsize = (newsize / LFS_BLOCK_SIZE) * LFS_BLOCK_SIZE;

		// double check again that we're within bounds - don't want
		// boot overwrite or anything nasty....
		if (newstart < LFS_BLOCKS_START_MIN) {
			return http_rest_error(request, -20, "LFS Size mismatch");
		}
		if ((newstart + newsize > LFS_BLOCKS_END) ||
			(newstart + newsize < LFS_BLOCKS_START_MIN)) {
			return http_rest_error(request, -20, "LFS Size mismatch");
		}

		return http_rest_get_flash(request, newstart, newsize);
	}
#endif

#if ENABLE_LITTLEFS
	if (!strncmp(request->url, "api/lfs/", 8)) {
		return http_rest_get_lfs_file(request);
	}
	if (!strncmp(request->url, "api/del/", 8)) {
		return http_rest_get_lfs_delete(request);
	}
#endif

	if (!strcmp(request->url, "api/info")) {
		return http_rest_get_info(request);
	}

	if (!strncmp(request->url, "api/flash/", 10)) {
		return http_rest_get_flash_advanced(request);
	}

	if (!strcmp(request->url, "api/dumpconfig")) {
		return http_rest_get_dumpconfig(request);
	}

	if (!strcmp(request->url, "api/testconfig")) {
		return http_rest_get_testconfig(request);
	}

	if (!strncmp(request->url, "api/testflashvars", 17)) {
		return http_rest_get_flash_vars_test(request);
	}

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "GET REST API");
	poststr(request, "GET of ");
	poststr(request, request->url);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

static int http_rest_post(http_request_t* request) {
	char tmp[20];
	ADDLOG_DEBUG(LOG_FEATURE_API, "POST to %s", request->url);

	if (!strcmp(request->url, "api/channels")) {
		return http_rest_post_channels(request);
	}
	
	if (!strcmp(request->url, "api/pins")) {
		return http_rest_post_pins(request);
	}
	if (!strcmp(request->url, "api/channelTypes")) {
		return http_rest_post_channelTypes(request);
	}
	if (!strcmp(request->url, "api/logconfig")) {
		return http_rest_post_logconfig(request);
	}

	if (!strcmp(request->url, "api/reboot")) {
		return http_rest_post_reboot(request);
	}
	if (!strcmp(request->url, "api/ota")) {
#if PLATFORM_BK7231T
		return http_rest_post_flash(request, START_ADR_OF_BK_PARTITION_OTA, LFS_BLOCKS_END);
#elif PLATFORM_BK7231N
		return http_rest_post_flash(request, START_ADR_OF_BK_PARTITION_OTA, LFS_BLOCKS_END);
#elif PLATFORM_W600
		return http_rest_post_flash(request, -1, -1);
#elif PLATFORM_BL602
		return http_rest_post_flash(request, -1, -1);
#elif PLATFORM_LN882H
		return http_rest_post_flash(request, -1, -1);
#elif PLATFORM_ESPIDF
		return http_rest_post_flash(request, -1, -1);
#else
		// TODO
		ADDLOG_DEBUG(LOG_FEATURE_API, "No OTA");
#endif
	}
	if (!strncmp(request->url, "api/flash/", 10)) {
		return http_rest_post_flash_advanced(request);
	}

	if (!strcmp(request->url, "api/cmnd")) {
		return http_rest_post_cmd(request);
	}


#if ENABLE_LITTLEFS
	if (!strcmp(request->url, "api/fsblock")) {
		if (lfs_present()) {
			release_lfs();
		}
		uint32_t newsize = CFG_GetLFS_Size();
		uint32_t newstart = (LFS_BLOCKS_END - newsize);

		newsize = (newsize / LFS_BLOCK_SIZE) * LFS_BLOCK_SIZE;

		// double check again that we're within bounds - don't want
		// boot overwrite or anything nasty....
		if (newstart < LFS_BLOCKS_START_MIN) {
			return http_rest_error(request, -20, "LFS Size mismatch");
		}
		if ((newstart + newsize > LFS_BLOCKS_END) ||
			(newstart + newsize < LFS_BLOCKS_START_MIN)) {
			return http_rest_error(request, -20, "LFS Size mismatch");
		}

		// we are writing the lfs block
		int res = http_rest_post_flash(request, newstart, LFS_BLOCKS_END);
		// initialise the filesystem, it should be there now.
		// don't create if it does not mount
		init_lfs(0);
		return res;
	}
	if (!strncmp(request->url, "api/lfs/", 8)) {
		return http_rest_post_lfs_file(request);
	}
#endif

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "POST REST API");
	poststr(request, "POST to ");
	poststr(request, request->url);
	poststr(request, "<br/>Content Length:");
	sprintf(tmp, "%d", request->contentLength);
	poststr(request, tmp);
	poststr(request, "<br/>Content:[");
	poststr(request, request->bodystart);
	poststr(request, "]<br/>");
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

static int http_rest_app(http_request_t* request) {
	const char* webhost = CFG_GetWebappRoot();
	const char* ourip = HAL_GetMyIPString(); //CFG_GetOurIP();
	http_setup(request, httpMimeTypeHTML);
	if (webhost && ourip) {
		poststr(request, htmlDoctype);

		poststr(request, "<head><title>");
		poststr(request, CFG_GetDeviceName());
		poststr(request, "</title>");

		poststr(request, htmlShortcutIcon);
		poststr(request, htmlHeadMeta);
		hprintf255(request, "<script>var root='%s',device='http://%s';</script>", webhost, ourip);
		hprintf255(request, "<script src='%s/startup.js'></script>", webhost);
		poststr(request, "</head><body></body></html>");
	}
	else {
		http_html_start(request, "Not available");
		poststr(request, htmlFooterReturnToMainPage);
		poststr(request, "no APP available<br/>");
		http_html_end(request);
	}
	poststr(request, NULL);
	return 0;
}

#if ENABLE_LITTLEFS

int EndsWith(const char* str, const char* suffix)
{
	if (!str || !suffix)
		return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix > lenstr)
		return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static int http_rest_get_lfs_file(http_request_t* request) {
	char* fpath;
	char* buff;
	int len;
	int lfsres;
	int total = 0;
	lfs_file_t* file;
	char *args;

	// don't start LFS just because we're trying to read a file -
	// it won't exist anyway
	if (!lfs_present()) {
		request->responseCode = HTTP_RESPONSE_NOT_FOUND;
		http_setup(request, httpMimeTypeText);
		poststr(request, NULL);
		return 0;
	}

	fpath = os_malloc(strlen(request->url) - strlen("api/lfs/") + 1);

	buff = os_malloc(1024);
	file = os_malloc(sizeof(lfs_file_t));
	memset(file, 0, sizeof(lfs_file_t));

	strcpy(fpath, request->url + strlen("api/lfs/"));

	// strip HTTP args with ?
	args = strchr(fpath, '?');
	if (args) {
		*args = 0;
	}

	ADDLOG_DEBUG(LOG_FEATURE_API, "LFS read of %s", fpath);
	lfsres = lfs_file_open(&lfs, file, fpath, LFS_O_RDONLY);

	if (lfsres == -21) {
		lfs_dir_t* dir;
		ADDLOG_DEBUG(LOG_FEATURE_API, "%s is a folder", fpath);
		dir = os_malloc(sizeof(lfs_dir_t));
		os_memset(dir, 0, sizeof(*dir));
		// if the thing is a folder.
		lfsres = lfs_dir_open(&lfs, dir, fpath);

		if (lfsres >= 0) {
			// this is needed during iteration...?
			struct lfs_info info;
			int count = 0;
			http_setup(request, httpMimeTypeJson);
			ADDLOG_DEBUG(LOG_FEATURE_API, "opened folder %s lfs result %d", fpath, lfsres);
			hprintf255(request, "{\"dir\":\"%s\",\"content\":[", fpath);
			do {
				// Read an entry in the directory
				//
				// Fills out the info structure, based on the specified file or directory.
				// Returns a positive value on success, 0 at the end of directory,
				// or a negative error code on failure.
				lfsres = lfs_dir_read(&lfs, dir, &info);
				if (lfsres > 0) {
					if (count) poststr(request, ",");
					hprintf255(request, "{\"name\":\"%s\",\"type\":%d,\"size\":%d}",
						info.name, info.type, info.size);
				}
				else {
					if (lfsres < 0) {
						if (count) poststr(request, ",");
						hprintf255(request, "{\"error\":%d}", lfsres);
					}
				}
				count++;
			} while (lfsres > 0);

			hprintf255(request, "]}");

			lfs_dir_close(&lfs, dir);
			if (dir) os_free(dir);
			dir = NULL;
		}
		else {
			if (dir) os_free(dir);
			dir = NULL;
			request->responseCode = HTTP_RESPONSE_NOT_FOUND;
			http_setup(request, httpMimeTypeJson);
			ADDLOG_DEBUG(LOG_FEATURE_API, "failed to open %s lfs result %d", fpath, lfsres);
			hprintf255(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, lfsres);
		}
	}
	else {
		ADDLOG_DEBUG(LOG_FEATURE_API, "LFS open [%s] gives %d", fpath, lfsres);
		if (lfsres >= 0) {
			const char* mimetype = httpMimeTypeBinary;
			do {
				if (EndsWith(fpath, ".ico")) {
					mimetype = "image/x-icon";
					break;
				}
				if (EndsWith(fpath, ".js")) {
					mimetype = "text/javascript";
					break;
				}
				if (EndsWith(fpath, ".json")) {
					mimetype = httpMimeTypeJson;
					break;
				}
				if (EndsWith(fpath, ".html")) {
					mimetype = "text/html";
					break;
				}
				if (EndsWith(fpath, ".vue")) {
					mimetype = "application/javascript";
					break;
				}
				break;
			} while (0);

			http_setup(request, mimetype);
			do {
				len = lfs_file_read(&lfs, file, buff, 1024);
				total += len;
				if (len) {
					//ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes read", len);
					postany(request, buff, len);
				}
			} while (len > 0);
			lfs_file_close(&lfs, file);
			ADDLOG_DEBUG(LOG_FEATURE_API, "%d total bytes read", total);
		}
		else {
			request->responseCode = HTTP_RESPONSE_NOT_FOUND;
			http_setup(request, httpMimeTypeJson);
			ADDLOG_DEBUG(LOG_FEATURE_API, "failed to open %s lfs result %d", fpath, lfsres);
			hprintf255(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, lfsres);
		}
	}
	poststr(request, NULL);
	if (fpath) os_free(fpath);
	if (file) os_free(file);
	if (buff) os_free(buff);
	return 0;
}

static int http_rest_get_lfs_delete(http_request_t* request) {
	char* fpath;
	int lfsres;

	// don't start LFS just because we're trying to read a file -
	// it won't exist anyway
	if (!lfs_present()) {
		request->responseCode = HTTP_RESPONSE_NOT_FOUND;
		http_setup(request, httpMimeTypeText);
		poststr(request, "Not found");
		poststr(request, NULL);
		return 0;
	}

	fpath = os_malloc(strlen(request->url) - strlen("api/del/") + 1);

	strcpy(fpath, request->url + strlen("api/del/"));

	ADDLOG_DEBUG(LOG_FEATURE_API, "LFS delete of %s", fpath);
	lfsres = lfs_remove(&lfs, fpath);

	if (lfsres == LFS_ERR_OK) {
		ADDLOG_DEBUG(LOG_FEATURE_API, "LFS delete of %s OK", fpath);

		poststr(request, "OK");
	}
	else {
		ADDLOG_DEBUG(LOG_FEATURE_API, "LFS delete of %s error %i", fpath, lfsres);
		poststr(request, "Error");
	}
	poststr(request, NULL);
	if (fpath) os_free(fpath);
	return 0;
}

static int http_rest_post_lfs_file(http_request_t* request) {
	int len;
	int lfsres;
	int total = 0;
	int loops = 0;

	// allocated variables
	lfs_file_t* file;
	char* fpath;
	char* folder;

	// create if it does not exist
	init_lfs(1);

	if (!lfs_present()) {
		request->responseCode = 400;
		http_setup(request, httpMimeTypeText);
		poststr(request, "LittleFS is not abailable");
		poststr(request, NULL);
		return 0;
	}

	fpath = os_malloc(strlen(request->url) - strlen("api/lfs/") + 1);
	file = os_malloc(sizeof(lfs_file_t));
	memset(file, 0, sizeof(lfs_file_t));

	strcpy(fpath, request->url + strlen("api/lfs/"));
	ADDLOG_DEBUG(LOG_FEATURE_API, "LFS write of %s len %d", fpath, request->contentLength);

	folder = strchr(fpath, '/');
	if (folder) {
		int folderlen = folder - fpath;
		folder = os_malloc(folderlen + 1);
		strncpy(folder, fpath, folderlen);
		folder[folderlen] = 0;
		ADDLOG_DEBUG(LOG_FEATURE_API, "file is in folder %s try to create", folder);
		lfsres = lfs_mkdir(&lfs, folder);
		if (lfsres < 0) {
			ADDLOG_DEBUG(LOG_FEATURE_API, "mkdir error %d", lfsres);
		}
	}

	//ADDLOG_DEBUG(LOG_FEATURE_API, "LFS write of %s len %d", fpath, request->contentLength);

	lfsres = lfs_file_open(&lfs, file, fpath, LFS_O_RDWR | LFS_O_CREAT);
	if (lfsres >= 0) {
		//ADDLOG_DEBUG(LOG_FEATURE_API, "opened %s");
		int towrite = request->bodylen;
		char* writebuf = request->bodystart;
		int writelen = request->bodylen;
		if (request->contentLength >= 0) {
			towrite = request->contentLength;
		}
		//ADDLOG_DEBUG(LOG_FEATURE_API, "bodylen %d, contentlen %d", request->bodylen, request->contentLength);

		if (writelen < 0) {
			ADDLOG_DEBUG(LOG_FEATURE_API, "ABORTED: %d bytes to write", writelen);
			lfs_file_close(&lfs, file);
			request->responseCode = HTTP_RESPONSE_SERVER_ERROR;
			http_setup(request, httpMimeTypeJson);
			hprintf255(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, -20);
			goto exit;
		}

		do {
			loops++;
			if (loops > 10) {
				loops = 0;
				rtos_delay_milliseconds(10);
			}
			//ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes to write", writelen);
			len = lfs_file_write(&lfs, file, writebuf, writelen);
			if (len < 0) {
				ADDLOG_ERROR(LOG_FEATURE_API, "Failed to write to %s with error %i", fpath,len);
				break;
			}
			total += len;
			if (len > 0) {
				//ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes written", len);
			}
			towrite -= len;
			if (towrite > 0) {
				writebuf = request->received;
				writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
				if (writelen < 0) {
					ADDLOG_DEBUG(LOG_FEATURE_API, "recv returned %d - end of data - remaining %d", writelen, towrite);
				}
			}
		} while ((towrite > 0) && (writelen >= 0));

		// no more data
		lfs_file_truncate(&lfs, file, total);

		//ADDLOG_DEBUG(LOG_FEATURE_API, "closing %s", fpath);
		lfs_file_close(&lfs, file);
		ADDLOG_DEBUG(LOG_FEATURE_API, "%d total bytes written", total);
		http_setup(request, httpMimeTypeJson);
		hprintf255(request, "{\"fname\":\"%s\",\"size\":%d}", fpath, total);
	}
	else {
		request->responseCode = HTTP_RESPONSE_SERVER_ERROR;
		http_setup(request, httpMimeTypeJson);
		ADDLOG_DEBUG(LOG_FEATURE_API, "failed to open %s err %d", fpath, lfsres);
		hprintf255(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, lfsres);
	}
exit:
	poststr(request, NULL);
	if (folder) os_free(folder);
	if (file) os_free(file);
	if (fpath) os_free(fpath);
	return 0;
}

// static int http_favicon(http_request_t* request) {
// 	request->url = "api/lfs/favicon.ico";
// 	return http_rest_get_lfs_file(request);
// }

#else
// static int http_favicon(http_request_t* request) {
// 	request->responseCode = HTTP_RESPONSE_NOT_FOUND;
// 	http_setup(request, httpMimeTypeHTML);
// 	poststr(request, NULL);
// 	return 0;
// }
#endif



static int http_rest_get_seriallog(http_request_t* request) {
	if (request->url[strlen(request->url) - 1] == '1') {
		direct_serial_log = 1;
	}
	else {
		direct_serial_log = 0;
	}
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "Direct serial logging set to %d", direct_serial_log);
	poststr(request, NULL);
	return 0;
}



static int http_rest_get_pins(http_request_t* request) {
	int i;
	int maxNonZero;
	http_setup(request, httpMimeTypeJson);
	poststr(request, "{\"rolenames\":[");
	for (i = 0; i < IOR_Total_Options; i++) {
		if (i) {
			hprintf255(request, ",");
		}
		hprintf255(request, "\"%s\"", htmlPinRoleNames[i]);
	}
	poststr(request, "],\"roles\":[");

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (i) {
			hprintf255(request, ",");
		}
		hprintf255(request, "%d", g_cfg.pins.roles[i]);
	}
	// TODO: maybe we should cull futher channels that are not used?
	// I support many channels because I plan to use 16x relays module with I2C MCP23017 driver

	// find max non-zero ch
	//maxNonZero = -1;
	//for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
	//	if (g_cfg.pins.channels[i] != 0) {
	//		maxNonZero = i;
	//	}
	//}

	poststr(request, "],\"channels\":[");
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (i) {
			hprintf255(request, ",");
		}
		hprintf255(request, "%d", g_cfg.pins.channels[i]);
	}
	// find max non-zero ch2
	maxNonZero = -1;	
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels2[i] != 0) {
			maxNonZero = i;
		}
	}
	if (maxNonZero != -1) {
		poststr(request, "],\"channels2\":[");
		for (i = 0; i <= maxNonZero; i++) {
			if (i) {
				hprintf255(request, ",");
			}
			hprintf255(request, "%d", g_cfg.pins.channels2[i]);
		}
	}
	poststr(request, "],\"states\":[");
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (i) {
			hprintf255(request, ",");
		}
		hprintf255(request, "%d", CHANNEL_Get(g_cfg.pins.channels[i]));
	}
	poststr(request, "]}");
	poststr(request, NULL);
	return 0;
}


static int http_rest_get_channelTypes(http_request_t* request) {
	int i;
	int maxToPrint;

	maxToPrint = 32;

	http_setup(request, httpMimeTypeJson);
	poststr(request, "{\"typenames\":[");
	for (i = 0; i < ChType_Max; i++) {
		if (i) {
			hprintf255(request, ",\"%s\"", g_channelTypeNames[i]);
		}
		else {
			hprintf255(request, "\"%s\"", g_channelTypeNames[i]);
		}
	}
	poststr(request, "],\"types\":[");

	for (i = 0; i < maxToPrint; i++) {
		if (i) {
			hprintf255(request, ",%d", g_cfg.pins.channelTypes[i]);
		}
		else {
			hprintf255(request, "%d", g_cfg.pins.channelTypes[i]);
		}
	}
	poststr(request, "]}");
	poststr(request, NULL);
	return 0;
}



////////////////////////////
// log config
static int http_rest_get_logconfig(http_request_t* request) {
	int i;
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"level\":%d,", g_loglevel);
	hprintf255(request, "\"features\":%d,", logfeatures);
	poststr(request, "\"levelnames\":[");
	for (i = 0; i < LOG_MAX; i++) {
		if (i) {
			hprintf255(request, ",\"%s\"", loglevelnames[i]);
		}
		else {
			hprintf255(request, "\"%s\"", loglevelnames[i]);
		}
	}
	poststr(request, "],\"featurenames\":[");
	for (i = 0; i < LOG_FEATURE_MAX; i++) {
		if (i) {
			hprintf255(request, ",\"%s\"", logfeaturenames[i]);
		}
		else {
			hprintf255(request, "\"%s\"", logfeaturenames[i]);
		}
	}
	poststr(request, "]}");
	poststr(request, NULL);
	return 0;
}

static int http_rest_post_logconfig(http_request_t* request) {
	int i;
	int r;
	char tmp[64];

	//https://github.com/zserge/jsmn/blob/master/example/simple.c
	//jsmn_parser p;
	jsmn_parser* p = os_malloc(sizeof(jsmn_parser));
	//jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
	jsmntok_t* t = os_malloc(sizeof(jsmntok_t) * TOKEN_COUNT);
	char* json_str = request->bodystart;
	int json_len = strlen(json_str);

	http_setup(request, httpMimeTypeText);
	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t) * 128);

	jsmn_init(p);
	r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
	if (r < 0) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
		poststr(request, NULL);
		os_free(p);
		os_free(t);
		return 0;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
		poststr(request, NULL);
		os_free(p);
		os_free(t);
		return 0;
	}

	//sprintf(tmp,"parsed JSON: %s\n", json_str);
	//poststr(request, tmp);
	//poststr(request, NULL);

		/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		if (jsoneq(json_str, &t[i], "level") == 0) {
			if (t[i + 1].type != JSMN_PRIMITIVE) {
				continue; /* We expect groups to be an array of strings */
			}
			g_loglevel = atoi(json_str + t[i + 1].start);
			i += t[i + 1].size + 1;
		}
		else if (jsoneq(json_str, &t[i], "features") == 0) {
			if (t[i + 1].type != JSMN_PRIMITIVE) {
				continue; /* We expect groups to be an array of strings */
			}
			logfeatures = atoi(json_str + t[i + 1].start);;
			i += t[i + 1].size + 1;
		}
		else {
			ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
				json_str + t[i].start);
			snprintf(tmp, sizeof(tmp), "Unexpected key: %.*s\n", t[i].end - t[i].start,
				json_str + t[i].start);
			poststr(request, tmp);
		}
	}

	poststr(request, NULL);
	os_free(p);
	os_free(t);
	return 0;
}

/////////////////////////////////////////////////


static int http_rest_get_info(http_request_t* request) {
	char macstr[3 * 6 + 1];
	long int* pAllGenericFlags = (long int*)&g_cfg.genericFlags;

	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"uptime_s\":%d,", g_secondsElapsed);
	hprintf255(request, "\"build\":\"%s\",", g_build_str);
	hprintf255(request, "\"ip\":\"%s\",", HAL_GetMyIPString());
	hprintf255(request, "\"mac\":\"%s\",", HAL_GetMACStr(macstr));
	hprintf255(request, "\"flags\":\"%ld\",", *pAllGenericFlags);
	hprintf255(request, "\"mqtthost\":\"%s:%d\",", CFG_GetMQTTHost(), CFG_GetMQTTPort());
	hprintf255(request, "\"mqtttopic\":\"%s\",", CFG_GetMQTTClientId());
	hprintf255(request, "\"chipset\":\"%s\",", PLATFORM_MCU_NAME);
	hprintf255(request, "\"webapp\":\"%s\",", CFG_GetWebappRoot());
	hprintf255(request, "\"shortName\":\"%s\",", CFG_GetShortDeviceName());
	poststr(request, "\"startcmd\":\"");
	// This can be longer than 255
	poststr(request, CFG_GetShortStartupCommand());
	poststr(request, "\",");
#ifndef OBK_DISABLE_ALL_DRIVERS
	hprintf255(request, "\"supportsSSDP\":%d,", DRV_IsRunning("SSDP") ? 1 : 0);
#else
	hprintf255(request, "\"supportsSSDP\":0,");
#endif

	hprintf255(request, "\"supportsClientDeviceDB\":true}");

	poststr(request, NULL);
	return 0;
}

static int http_rest_post_pins(http_request_t* request) {
	int i;
	int r;
	char tmp[64];
	int iChanged = 0;
	char tokenStrValue[MAX_JSON_VALUE_LENGTH + 1];

	//https://github.com/zserge/jsmn/blob/master/example/simple.c
	//jsmn_parser p;
	jsmn_parser* p = os_malloc(sizeof(jsmn_parser));
	//jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
	jsmntok_t* t = os_malloc(sizeof(jsmntok_t) * TOKEN_COUNT);
	char* json_str = request->bodystart;
	int json_len = strlen(json_str);

	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t) * TOKEN_COUNT);

	jsmn_init(p);
	r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
	if (r < 0) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
		sprintf(tmp, "Failed to parse JSON: %d\n", r);
		os_free(p);
		os_free(t);
		return http_rest_error(request, 400, tmp);
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
		sprintf(tmp, "Object expected\n");
		os_free(p);
		os_free(t);
		return http_rest_error(request, 400, tmp);
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		if (tryGetTokenString(json_str, &t[i], tokenStrValue) != true) {
			ADDLOG_DEBUG(LOG_FEATURE_API, "Parsing failed");
			continue;
		}
		//ADDLOG_DEBUG(LOG_FEATURE_API, "parsed %s", tokenStrValue);

		if (strcmp(tokenStrValue, "roles") == 0) {
			int j;
			if (t[i + 1].type != JSMN_ARRAY) {
				continue; /* We expect groups to be an array of strings */
			}
			for (j = 0; j < t[i + 1].size; j++) {
				int roleval, pr;
				jsmntok_t* g = &t[i + j + 2];
				roleval = atoi(json_str + g->start);
				pr = PIN_GetPinRoleForPinIndex(j);
				if (pr != roleval) {
					PIN_SetPinRoleForPinIndex(j, roleval);
					iChanged++;
				}
			}
			i += t[i + 1].size + 1;
		}
		else if (strcmp(tokenStrValue, "channels") == 0) {
			int j;
			if (t[i + 1].type != JSMN_ARRAY) {
				continue; /* We expect groups to be an array of strings */
			}
			for (j = 0; j < t[i + 1].size; j++) {
				int chanval, pr;
				jsmntok_t* g = &t[i + j + 2];
				chanval = atoi(json_str + g->start);
				pr = PIN_GetPinChannelForPinIndex(j);
				if (pr != chanval) {
					PIN_SetPinChannelForPinIndex(j, chanval);
					iChanged++;
				}
			}
			i += t[i + 1].size + 1;
		}
		else if (strcmp(tokenStrValue, "deviceFlag") == 0) {
			int flag;
			jsmntok_t* flagTok = &t[i + 1];
			if (flagTok == NULL || flagTok->type != JSMN_PRIMITIVE) {
				continue;
			}

			flag = atoi(json_str + flagTok->start);
			ADDLOG_DEBUG(LOG_FEATURE_API, "received deviceFlag %d", flag);

			if (flag >= 0 && flag <= 10) {
				CFG_SetFlag(flag, true);
				iChanged++;
			}

			i += t[i + 1].size + 1;
		}
		else if (strcmp(tokenStrValue, "deviceCommand") == 0) {
			if (tryGetTokenString(json_str, &t[i + 1], tokenStrValue) == true) {
				ADDLOG_DEBUG(LOG_FEATURE_API, "received deviceCommand %s", tokenStrValue);
				CFG_SetShortStartupCommand_AndExecuteNow(tokenStrValue);
				iChanged++;
			}

			i += t[i + 1].size + 1;
		}
		else {
			ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
				json_str + t[i].start);
		}
	}
	if (iChanged) {
		CFG_Save_SetupTimer();
		ADDLOG_DEBUG(LOG_FEATURE_API, "Changed %d - saved to flash", iChanged);
	}

	os_free(p);
	os_free(t);
	return http_rest_error(request, 200, "OK");
}

static int http_rest_post_channelTypes(http_request_t* request) {
	int i;
	int r;
	char tmp[64];
	int iChanged = 0;
	char tokenStrValue[MAX_JSON_VALUE_LENGTH + 1];

	//https://github.com/zserge/jsmn/blob/master/example/simple.c
	//jsmn_parser p;
	jsmn_parser* p = os_malloc(sizeof(jsmn_parser));
	//jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
	jsmntok_t* t = os_malloc(sizeof(jsmntok_t) * TOKEN_COUNT);
	char* json_str = request->bodystart;
	int json_len = strlen(json_str);

	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t) * TOKEN_COUNT);

	jsmn_init(p);
	r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
	if (r < 0) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
		sprintf(tmp, "Failed to parse JSON: %d\n", r);
		os_free(p);
		os_free(t);
		return http_rest_error(request, 400, tmp);
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
		sprintf(tmp, "Object expected\n");
		os_free(p);
		os_free(t);
		return http_rest_error(request, 400, tmp);
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		if (tryGetTokenString(json_str, &t[i], tokenStrValue) != true) {
			ADDLOG_DEBUG(LOG_FEATURE_API, "Parsing failed");
			continue;
		}
		//ADDLOG_DEBUG(LOG_FEATURE_API, "parsed %s", tokenStrValue);

		if (strcmp(tokenStrValue, "types") == 0) {
			int j;
			if (t[i + 1].type != JSMN_ARRAY) {
				continue; /* We expect groups to be an array of strings */
			}
			for (j = 0; j < t[i + 1].size; j++) {
				int typeval, pr;
				jsmntok_t* g = &t[i + j + 2];
				typeval = atoi(json_str + g->start);
				pr = CHANNEL_GetType(j);
				if (pr != typeval) {
					CHANNEL_SetType(j, typeval);
					iChanged++;
				}
			}
			i += t[i + 1].size + 1;
		}
		else {
			ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
				json_str + t[i].start);
		}
	}
	if (iChanged) {
		CFG_Save_SetupTimer();
		ADDLOG_DEBUG(LOG_FEATURE_API, "Changed %d - saved to flash", iChanged);
	}

	os_free(p);
	os_free(t);
	return http_rest_error(request, 200, "OK");
}

static int http_rest_error(http_request_t* request, int code, char* msg) {
	request->responseCode = code;
	http_setup(request, httpMimeTypeJson);
	if (code != 200) {
		hprintf255(request, "{\"error\":%d, \"msg\":\"%s\"}", code, msg);
	}
	else {
		hprintf255(request, "{\"success\":%d, \"msg\":\"%s\"}", code, msg);
	}
	poststr(request, NULL);
	return 0;
}

#if PLATFORM_BL602

typedef struct ota_header {
	union {
		struct {
			uint8_t header[16];

			uint8_t type[4];//RAW XZ
			uint32_t len;//body len
			uint8_t pad0[8];

			uint8_t ver_hardware[16];
			uint8_t ver_software[16];

			uint8_t sha256[32];
} s;
		uint8_t _pad[512];
	} u;
} ota_header_t;
#define OTA_HEADER_SIZE (sizeof(ota_header_t))

static int _check_ota_header(ota_header_t *ota_header, uint32_t *ota_len, int *use_xz)
{
	char str[33];//assume max segment size
	int i;

	memcpy(str, ota_header->u.s.header, sizeof(ota_header->u.s.header));
	str[sizeof(ota_header->u.s.header)] = '\0';
	puts("[OTA] [HEADER] ota header is ");
	puts(str);
	puts("\r\n");

	memcpy(str, ota_header->u.s.type, sizeof(ota_header->u.s.type));
	str[sizeof(ota_header->u.s.type)] = '\0';
	puts("[OTA] [HEADER] file type is ");
	puts(str);
	puts("\r\n");
	if (strstr(str, "XZ")) {
		*use_xz = 1;
	}
	else {
		*use_xz = 0;
	}

	memcpy(ota_len, &(ota_header->u.s.len), 4);
	printf("[OTA] [HEADER] file length (exclude ota header) is %lu\r\n", *ota_len);

	memcpy(str, ota_header->u.s.ver_hardware, sizeof(ota_header->u.s.ver_hardware));
	str[sizeof(ota_header->u.s.ver_hardware)] = '\0';
	puts("[OTA] [HEADER] ver_hardware is ");
	puts(str);
	puts("\r\n");

	memcpy(str, ota_header->u.s.ver_software, sizeof(ota_header->u.s.ver_software));
	str[sizeof(ota_header->u.s.ver_software)] = '\0';
	puts("[OTA] [HEADER] ver_software is ");
	puts(str);
	puts("\r\n");

	memcpy(str, ota_header->u.s.sha256, sizeof(ota_header->u.s.sha256));
	str[sizeof(ota_header->u.s.sha256)] = '\0';
	puts("[OTA] [HEADER] sha256 is ");
	for (i = 0; i < sizeof(ota_header->u.s.sha256); i++) {
		printf("%02X", str[i]);
	}
	puts("\r\n");

	return 0;
}
#endif

#if PLATFORM_LN882H
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
			LOG(LOG_LVL_INFO,"failed to alloc 4KB!!!\r\n");
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
	int part_len = 0; // [0, 1, 2, ..., 4K-1], 0, 1, 2, ..., (part_len-1)

	if (!is_persistent_started) {
		return LN_TRUE;
	}

	if (temp4k_offset + buf_len < SECTOR_SIZE_4KB) {
		// just copy all buf data to temp4K_buf
		memcpy(temp4K_buf + temp4k_offset, buf, buf_len);
		temp4k_offset += buf_len;
		part_len = 0;
	}
	else {
		// just copy part of buf to temp4K_buf
		part_len = temp4k_offset + buf_len - SECTOR_SIZE_4KB;
		memcpy(temp4K_buf + temp4k_offset, buf, buf_len - part_len);
		temp4k_offset += buf_len - part_len;
	}
	if (temp4k_offset >= (SECTOR_SIZE_4KB - 1)) {
		// write to flash
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "write at flash: 0x%08x\r\n", flash_ota_start_addr + flash_ota_offset);

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
#endif

#if PLATFORM_ESPIDF
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_pm.h"
#endif

static int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{

#if PLATFORM_XR809 || PLATFORM_W800
	return 0;	//Operation not supported yet
#endif


	int total = 0;
	int towrite = request->bodylen;
	char* writebuf = request->bodystart;
	int writelen = request->bodylen;
	int fsize = 0;

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

#ifdef PLATFORM_W600
	int nRetCode = 0;
	char error_message[256];

	if(writelen < 0)
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "ABORTED: %d bytes to write", writelen);
		return http_rest_error(request, -20, "writelen < 0");
	}

	struct pbuf* p;

	//Data is uploaded in 1024 sized chunks, creating a bigger buffer just in case this assumption changes.
	//The code below is based on sdk\OpenW600\src\app\ota\wm_http_fwup.c
	char* Buffer = (char*)os_malloc(2048 + 3);
	memset(Buffer, 0, 2048 + 3);

	if(request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	int recvLen = 0;
	int totalLen = 0;
	//printf("\ntowrite %d writelen=%d\n", towrite, writelen);

	do
	{
		if(writelen > 0)
		{
			//bk_printf("Copying %d from writebuf to Buffer towrite=%d\n", writelen, towrite);
			memcpy(Buffer + 3, writebuf, writelen);

			if(recvLen == 0)
			{
				T_BOOTER* booter = (T_BOOTER*)(Buffer + 3);
				bk_printf("magic_no=%u, img_type=%u, zip_type=%u\n", booter->magic_no, booter->img_type, booter->zip_type);

				if(TRUE == tls_fwup_img_header_check(booter))
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
				if(nRetCode != ERR_OK)
				{
					sprintf(error_message, "Firmware update startup failed");
					break;
				}
			}

			p = pbuf_alloc(PBUF_TRANSPORT, writelen + 3, PBUF_REF);
			if(!p)
			{
				sprintf(error_message, "Unable to allocate memory for buffer");
				nRetCode = -18;
				break;
			}

			if(recvLen == 0)
			{
				*Buffer = SOCKET_FWUP_START;
			}
			else if(recvLen == (totalLen - writelen))
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
			if(nRetCode != ERR_OK)
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

		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if(writelen < 0)
			{
				sprintf(error_message, "recv returned %d - end of data - remaining %d", writelen, towrite);
				nRetCode = -17;
			}
		}
	} while((nRetCode == 0) && (towrite > 0) && (writelen >= 0));

	tls_mem_free(Buffer);

	if(nRetCode != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, error_message);
		socket_fwup_err(0, nRetCode);
		return http_rest_error(request, nRetCode, error_message);
	}


#elif PLATFORM_BL602
	int sockfd, i;
	int ret;
	struct hostent* hostinfo;
	uint8_t* recv_buffer;
	struct sockaddr_in dest;
	iot_sha256_context ctx;
	uint8_t sha256_result[32];
	uint8_t sha256_img[32];
	bl_mtd_handle_t handle;
	//init_ota(startaddr);


#define OTA_PROGRAM_SIZE (512)
	int ota_header_found, use_xz;
	ota_header_t* ota_header = 0;

	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &handle, BL_MTD_OPEN_FLAG_BACKUP);
	if(ret)
	{
		return http_rest_error(request, -20, "Open Default FW partition failed");
	}

	recv_buffer = pvPortMalloc(OTA_PROGRAM_SIZE);

	unsigned int buffer_offset, flash_offset, ota_addr;
	uint32_t bin_size, part_size;
	uint8_t activeID;
	HALPartition_Entry_Config ptEntry;

	activeID = hal_boot2_get_active_partition();

	printf("Starting OTA test. OTA bin addr is %p, incoming len %i\r\n", recv_buffer, writelen);

	printf("[OTA] [TEST] activeID is %u\r\n", activeID);

	if(hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &ptEntry))
	{
		printf("PtTable_Get_Active_Entries fail\r\n");
		vPortFree(recv_buffer);
		bl_mtd_close(handle);
		return http_rest_error(request, -20, "PtTable_Get_Active_Entries fail");
	}
	ota_addr = ptEntry.Address[!ptEntry.activeIndex];
	bin_size = ptEntry.maxLen[!ptEntry.activeIndex];
	part_size = ptEntry.maxLen[!ptEntry.activeIndex];
	(void)part_size;
	/*XXX if you use bin_size is product env, you may want to set bin_size to the actual
	 * OTA BIN size, and also you need to splilt XIP_SFlash_Erase_With_Lock into
	 * serveral pieces. Partition size vs bin_size check is also needed
	 */
	printf("Starting OTA test. OTA size is %lu\r\n", bin_size);

	printf("[OTA] [TEST] activeIndex is %u, use OTA address=%08x\r\n", ptEntry.activeIndex, (unsigned int)ota_addr);

	printf("[OTA] [TEST] Erase flash with size %lu...", bin_size);
	hal_update_mfg_ptable();

	//Erase in chunks, because erasing everything at once is slow and causes issues with http connection
	uint32_t erase_offset = 0;
	uint32_t erase_len = 0;
	while(erase_offset < bin_size)
	{
		erase_len = bin_size - erase_offset;
		if(erase_len > 0x10000)
		{
			erase_len = 0x10000; //Erase in 64kb chunks
		}
		bl_mtd_erase(handle, erase_offset, erase_len);
		printf("[OTA] Erased:  %lu / %lu \r\n", erase_offset, erase_len);
		erase_offset += erase_len;
		rtos_delay_milliseconds(100);
	}
	printf("[OTA] Done\r\n");

	if(request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	// get header
	// recv_buffer	
	//buffer_offset = 0;
	//do {
	//	int take_len;

	//	take_len = OTA_PROGRAM_SIZE - buffer_offset;

	//	memcpy(recv_buffer + buffer_offset, writebuf, writelen);
	//	buffer_offset += writelen;


	//	if (towrite > 0) {
	//		writebuf = request->received;
	//		writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
	//		if (writelen < 0) {
	//			ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
	//		}
	//	}
	//} while(true)

	buffer_offset = 0;
	flash_offset = 0;
	ota_header = 0;
	use_xz = 0;

	utils_sha256_init(&ctx);
	utils_sha256_starts(&ctx);
	memset(sha256_result, 0, sizeof(sha256_result));
	do
	{
		char* useBuf = writebuf;
		int useLen = writelen;

		if(ota_header == 0)
		{
			int take_len;

			// how much left for header?
			take_len = OTA_PROGRAM_SIZE - buffer_offset;
			// clamp to available len
			if(take_len > useLen)
				take_len = useLen;
			printf("Header takes %i. ", take_len);
			memcpy(recv_buffer + buffer_offset, writebuf, take_len);
			buffer_offset += take_len;
			useBuf = writebuf + take_len;
			useLen = writelen - take_len;

			if(buffer_offset >= OTA_PROGRAM_SIZE)
			{
				ota_header = (ota_header_t*)recv_buffer;
				if(strncmp((const char*)ota_header, "BL60X_OTA", 9))
				{
					return http_rest_error(request, -20, "Invalid header ident");
				}
			}
		}


		if(ota_header && useLen)
		{


			if(flash_offset + useLen >= part_size)
			{
				return http_rest_error(request, -20, "Too large bin");
			}
			//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);
			//add_otadata((unsigned char*)writebuf, writelen);

			printf("Flash takes %i. ", useLen);
			utils_sha256_update(&ctx, (byte*)useBuf, useLen);
			bl_mtd_write(handle, flash_offset, useLen, (byte*)useBuf);
			flash_offset += useLen;
		}

		total += writelen;
		startaddr += writelen;
		towrite -= writelen;


		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if(writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while((towrite > 0) && (writelen >= 0));

	if(ota_header == 0)
	{
		return http_rest_error(request, -20, "No header found");
	}
	utils_sha256_finish(&ctx, sha256_result);
	puts("\r\nCalculated SHA256 Checksum:");
	for(i = 0; i < sizeof(sha256_result); i++)
	{
		printf("%02X", sha256_result[i]);
	}
	puts("\r\nHeader SHA256 Checksum:");
	for(i = 0; i < sizeof(sha256_result); i++)
	{
		printf("%02X", ota_header->u.s.sha256[i]);
	}
	if(memcmp(ota_header->u.s.sha256, sha256_result, sizeof(sha256_img)))
	{
		/*Error found*/
		return http_rest_error(request, -20, "SHA256 NOT Correct");
	}
	printf("[OTA] [TCP] prepare OTA partition info\r\n");
	ptEntry.len = total;
	printf("[OTA] [TCP] Update PARTITION, partition len is %lu\r\n", ptEntry.len);
	hal_boot2_update_ptable(&ptEntry);
	printf("[OTA] [TCP] Rebooting\r\n");
	//close_ota();
	vPortFree(recv_buffer);
	utils_sha256_free(&ctx);
	bl_mtd_close(handle);

#elif PLATFORM_LN882H
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "Ota start!\r\n");
	if(LN_TRUE != ota_persistent_start())
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Ota start error, exit...\r\n");
		return 0;
	}

	if(request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	do
	{
		//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);

		if(LN_TRUE != ota_persistent_write(writebuf, writelen))
		{
			//	ADDLOG_DEBUG(LOG_FEATURE_OTA, "ota write err.\r\n");
			return -1;
		}

		rtos_delay_milliseconds(10);
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "Writelen %i at %i", writelen, total);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if(writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while((towrite > 0) && (writelen >= 0));

	ota_persistent_finish();
	is_ready_to_verify = LN_TRUE;
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "cb info: recv %d finished, no more data to deal with.\r\n", towrite);


	ADDLOG_DEBUG(LOG_FEATURE_OTA, "http client job done, exit...\r\n");
	if(LN_TRUE == is_precheck_ok)
	{
		if((LN_TRUE == is_ready_to_verify) && (LN_TRUE == ota_verify_download()))
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


#elif PLATFORM_ESPIDF

	ADDLOG_DEBUG(LOG_FEATURE_OTA, "Ota start!\r\n");
	esp_err_t err;
	esp_ota_handle_t update_handle = 0;
	const esp_partition_t* update_partition = NULL;
	const esp_partition_t* running = esp_ota_get_running_partition();
	update_partition = esp_ota_get_next_update_partition(NULL);
	if(request->contentLength >= 0)
	{
		fsize = towrite = request->contentLength;
	}

	esp_wifi_set_ps(WIFI_PS_NONE);
	bool image_header_was_checked = false;
	do
	{
		if(image_header_was_checked == false)
		{
			esp_app_desc_t new_app_info;
			if(towrite > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
			{
				memcpy(&new_app_info, &writebuf[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "New firmware version: %s", new_app_info.version);

				esp_app_desc_t running_app_info;
				if(esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
				{
					ADDLOG_DEBUG(LOG_FEATURE_OTA, "Running firmware version: %s", running_app_info.version);
				}

				image_header_was_checked = true;

				err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
				if(err != ESP_OK)
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
		if(err != ESP_OK)
		{
			esp_ota_abort(update_handle);
			return -1;
		}

		ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA in progress: %.1f%%", (100 - ((float)towrite / fsize) * 100));
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;

		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if(writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while((towrite > 0) && (writelen >= 0));

	ADDLOG_INFO(LOG_FEATURE_OTA, "OTA in progress: 100%%, total Write binary data length: %d", total);

	err = esp_ota_end(update_handle);
	if(err != ESP_OK)
	{
		if(err == ESP_ERR_OTA_VALIDATE_FAILED)
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
	if(err != ESP_OK)
	{
		ADDLOG_ERROR(LOG_FEATURE_OTA, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		return -1;
	}

#else

	init_ota(startaddr);

	if(request->contentLength >= 0)
	{
		towrite = request->contentLength;
	}

	if(writelen < 0 || (startaddr + writelen > maxaddr))
	{
		ADDLOG_DEBUG(LOG_FEATURE_OTA, "ABORTED: %d bytes to write", writelen);
		return http_rest_error(request, -20, "writelen < 0 or end > 0x200000");
	}

	do
	{
		//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);
		add_otadata((unsigned char*)writebuf, writelen);
		total += writelen;
		startaddr += writelen;
		towrite -= writelen;
		if(towrite > 0)
		{
			writebuf = request->received;
			writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
			if(writelen < 0)
			{
				ADDLOG_DEBUG(LOG_FEATURE_OTA, "recv returned %d - end of data - remaining %d", writelen, towrite);
			}
		}
	} while((towrite > 0) && (writelen >= 0));
	close_ota();
#endif
	ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d total bytes written", total);
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"size\":%d}", total);
	poststr(request, NULL);
	return 0;
}

static int http_rest_post_reboot(http_request_t* request) {
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"reboot\":%d}", 3);
	ADDLOG_DEBUG(LOG_FEATURE_API, "Rebooting in 3 seconds...");
	RESET_ScheduleModuleReset(3);
	poststr(request, NULL);
	return 0;
}

static int http_rest_get_flash_advanced(http_request_t* request) {
	char* params = request->url + 10;
	int startaddr = 0;
	int len = 0;
	int sres;
	sres = sscanf(params, "%x-%x", &startaddr, &len);
	if (sres == 2) {
		return http_rest_get_flash(request, startaddr, len);
	}
	return http_rest_error(request, -1, "invalid url");
}

static int http_rest_post_flash_advanced(http_request_t* request) {
	char* params = request->url + 10;
	int startaddr = 0;
	int sres;
	sres = sscanf(params, "%x", &startaddr);
	if (sres == 1 && startaddr >= START_ADR_OF_BK_PARTITION_OTA) {
		// allow up to end of flash
		return http_rest_post_flash(request, startaddr, 0x200000);
	}
	return http_rest_error(request, -1, "invalid url");
}

static int http_rest_get_flash(http_request_t* request, int startaddr, int len) {
	char* buffer;
	int res;

	if (startaddr < 0 || (startaddr + len > 0x200000)) {
		return http_rest_error(request, -1, "requested flash read out of range");
	}

	buffer = os_malloc(1024);

	http_setup(request, httpMimeTypeBinary);
	while (len) {
		int readlen = len;
		if (readlen > 1024) {
			readlen = 1024;
		}
#if PLATFORM_XR809
		//uint32_t flash_read(uint32_t flash, uint32_t addr,void *buf, uint32_t size)
#define FLASH_INDEX_XR809 0
		res = flash_read(FLASH_INDEX_XR809, startaddr, buffer, readlen);
#elif PLATFORM_BL602
		res = bl_flash_read(startaddr, (uint8_t *)buffer, readlen);
#elif PLATFORM_W600 || PLATFORM_W800
		res = 0;
#elif PLATFORM_LN882H || PLATFORM_ESPIDF
// TODO:LN882H flash read?
        res = 0;
#else
		res = flash_read((char*)buffer, readlen, startaddr);
#endif
		startaddr += readlen;
		len -= readlen;
		postany(request, buffer, readlen);
	}
	poststr(request, NULL);
	os_free(buffer);
	return 0;
}


static int http_rest_get_dumpconfig(http_request_t* request) {



	http_setup(request, httpMimeTypeText);
	poststr(request, NULL);
	return 0;
}



#ifdef TESTCONFIG_ENABLE
// added for OpenBK7231T
typedef struct item_new_test_config
{
	INFO_ITEM_ST head;
	char somename[64];
}ITEM_NEW_TEST_CONFIG, * ITEM_NEW_TEST_CONFIG_PTR;

ITEM_NEW_TEST_CONFIG testconfig;
#endif

static int http_rest_get_testconfig(http_request_t* request) {
	return http_rest_error(request, 400, "unsupported");
	return 0;
}

static int http_rest_get_flash_vars_test(http_request_t* request) {
	//#if PLATFORM_XR809
	//    return http_rest_error(request, 400, "flash vars unsupported");
	//#elif PLATFORM_BL602
	//    return http_rest_error(request, 400, "flash vars unsupported");
	//#else
	//#ifndef DISABLE_FLASH_VARS_VARS
	//    char *params = request->url + 17;
	//    int increment = 0;
	//    int len = 0;
	//    int sres;
	//    int i;
	//    char tmp[128];
	//    FLASH_VARS_STRUCTURE data, *p;
	//
	//    p = &flash_vars;
	//
	//    sres = sscanf(params, "%x-%x", &increment, &len);
	//
	//    ADDLOG_DEBUG(LOG_FEATURE_API, "http_rest_get_flash_vars_test %d %d returned %d", increment, len, sres);
	//
	//    if (increment == 10){
	//        flash_vars_read(&data);
	//        p = &data;
	//    } else {
	//        for (i = 0; i < increment; i++){
	//            HAL_FlashVars_IncreaseBootCount();
	//        }
	//        for (i = 0; i < len; i++){
	//            HAL_FlashVars_SaveBootComplete();
	//        }
	//    }
	//
	//    sprintf(tmp, "offset %d, boot count %d, boot success %d, bootfailures %d",
	//        flash_vars_offset,
	//        p->boot_count,
	//        p->boot_success_count,
	//        p->boot_count - p->boot_success_count );
	//
	//    return http_rest_error(request, 200, tmp);
	//#else
	return http_rest_error(request, 400, "flash test unsupported");
}


static int http_rest_get_channels(http_request_t* request) {
	int i;
	int addcomma = 0;
	/*typedef struct pinsState_s {
		byte roles[32];
		byte channels[32];
	} pinsState_t;

	extern pinsState_t g_pins;
	*/
	http_setup(request, httpMimeTypeJson);
	poststr(request, "{");

	// TODO: maybe we should cull futher channels that are not used?
	// I support many channels because I plan to use 16x relays module with I2C MCP23017 driver
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		// "i" is a pin index
		// Get channel index and role
		int ch = PIN_GetPinChannelForPinIndex(i);
		int role = PIN_GetPinRoleForPinIndex(i);
		if (role) {
			if (addcomma) {
				hprintf255(request, ",");
			}
			hprintf255(request, "\"%d\":%d", ch, CHANNEL_Get(ch));
			addcomma = 1;
		}
	}
	poststr(request, "}");
	poststr(request, NULL);
	return 0;
}

// currently crashes the MCU - maybe stack overflow?
static int http_rest_post_channels(http_request_t* request) {
	int i;
	int r;
	char tmp[64];

	//https://github.com/zserge/jsmn/blob/master/example/simple.c
	//jsmn_parser p;
	jsmn_parser* p = os_malloc(sizeof(jsmn_parser));
	//jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
	jsmntok_t* t = os_malloc(sizeof(jsmntok_t) * TOKEN_COUNT);
	char* json_str = request->bodystart;
	int json_len = strlen(json_str);

	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t) * 128);

	jsmn_init(p);
	r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
	if (r < 0) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
		sprintf(tmp, "Failed to parse JSON: %d\n", r);
		os_free(p);
		os_free(t);
		return http_rest_error(request, 400, tmp);
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_ARRAY) {
		ADDLOG_ERROR(LOG_FEATURE_API, "Array expected", r);
		sprintf(tmp, "Object expected\n");
		os_free(p);
		os_free(t);
		return http_rest_error(request, 400, tmp);
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		int chanval;
		jsmntok_t* g = &t[i];
		chanval = atoi(json_str + g->start);
		CHANNEL_Set(i - 1, chanval, 0);
		ADDLOG_DEBUG(LOG_FEATURE_API, "Set of chan %d to %d", i,
			chanval);
	}

	os_free(p);
	os_free(t);
	return http_rest_error(request, 200, "OK");
	return 0;
}



static int http_rest_post_cmd(http_request_t* request) {
	commandResult_t res;
	int code;
	const char *reply;
	const char *type;
	const char* cmd = request->bodystart;
	res = CMD_ExecuteCommand(cmd, COMMAND_FLAG_SOURCE_CONSOLE);
	reply = CMD_GetResultString(res);
	if (1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "[WebApp Cmd '%s' Result] %s", cmd, reply);
	}
	if (res != CMD_RES_OK) {
		type = "error";
		if (res == CMD_RES_UNKNOWN_COMMAND) {
			code = 501;
		}
		else {
			code = 400;
		}
	}
	else {
		type = "success";
		code = 200;
	}

	request->responseCode = code;
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "{\"%s\":%d, \"msg\":\"%s\", \"res\":", type, code, reply);
#if ENABLE_TASMOTA_JSON
	JSON_ProcessCommandReply(cmd, skipToNextWord(cmd), request, (jsonCb_t)hprintf255, COMMAND_FLAG_SOURCE_HTTP);
#endif
	hprintf255(request, "}", code, reply);
	poststr(request, NULL);
	return 0;
}


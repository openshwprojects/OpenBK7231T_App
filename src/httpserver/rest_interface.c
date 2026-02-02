#include "../obk_config.h"

#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
#include "../new_pins.h"
#include "../jsmn/jsmn_h.h"
#include "../hal/hal_ota.h"
#include "../hal/hal_wifi.h"
#include "../hal/hal_flashVars.h"
#include "../littlefs/our_lfs.h"
#include "lwip/sockets.h"

#define DEFAULT_FLASH_LEN 0x200000

#if PLATFORM_RTL8710A
#undef DEFAULT_FLASH_LEN
#define DEFAULT_FLASH_LEN 0x400000
#elif PLATFORM_RTL8720D || PLATFORM_REALTEK_NEW
extern uint8_t flash_size_8720;
#undef DEFAULT_FLASH_LEN
#define DEFAULT_FLASH_LEN (flash_size_8720 << 20)
#endif

#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"

#ifndef OBK_DISABLE_ALL_DRIVERS
#include "../driver/drv_local.h"
#endif

#define MAX_JSON_VALUE_LENGTH   128


int http_rest_error(http_request_t* request, int code, char* msg);

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
static int http_rest_run_lfs_file(http_request_t* request);
static int http_rest_post_lfs_file(http_request_t* request);
#endif

static int http_rest_post_reboot(http_request_t* request);
int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr);
static int http_rest_get_flash(http_request_t* request, int startaddr, int len);
static int http_rest_get_flash_advanced(http_request_t* request);
static int http_rest_post_flash_advanced(http_request_t* request);

static int http_rest_get_info(http_request_t* request);

static int http_rest_post_channels(http_request_t* request);
static int http_rest_get_channels(http_request_t* request);

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
	if (!strncmp(request->url, "api/run/", 8)) {
		return http_rest_run_lfs_file(request);
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
		OTA_IncrementProgress(1);
		int r = 0;
#if PLATFORM_BEKEN
		r = http_rest_post_flash(request, START_ADR_OF_BK_PARTITION_OTA, LFS_BLOCKS_END);
#elif PLATFORM_W600
		r = http_rest_post_flash(request, -1, -1);
#elif PLATFORM_W800
		r = http_rest_post_flash(request, -1, -1);
#elif PLATFORM_BL602
		r = http_rest_post_flash(request, -1, -1);
#elif PLATFORM_LN882H || PLATFORM_LN8825
		r = http_rest_post_flash(request, -1, -1);
#elif PLATFORM_ESPIDF || PLATFORM_ESP8266
		r = http_rest_post_flash(request, -1, -1);
#elif PLATFORM_REALTEK
		r = http_rest_post_flash(request, 0, -1);
#elif PLATFORM_ECR6600 || PLATFORM_TR6260
		r = http_rest_post_flash(request, -1, -1);
#elif PLATFORM_XRADIO && !PLATFORM_XR809
		r = http_rest_post_flash(request, 0, -1);
#elif PLATFORM_TXW81X
		r = http_rest_post_flash(request, 0, -1);
#elif PLATFORM_RDA5981
		r = http_rest_post_flash(request, 0, -1);
#else
		// TODO
		ADDLOG_ERROR(LOG_FEATURE_API, "No OTA");
#endif
		OTA_ResetProgress();
		return r;
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
char *my_memmem(const char *haystack, int haystack_len, const char *needle, int needle_len) {
	if (needle_len == 0 || haystack_len < needle_len)
		return NULL;

	for (int i = 0; i <= haystack_len - needle_len; i++) {
		if (memcmp(haystack + i, needle, needle_len) == 0)
			return (char *)(haystack + i);
	}
	return NULL;
}
typedef struct berryBuilder_s {

	char berry_buffer[4096];
	int berry_len;
} berryBuilder_t;

void BB_Start(berryBuilder_t *b)
{
	b->berry_buffer[0] = 0;
	b->berry_len = 0;
}
void BB_AddCode(berryBuilder_t *b, const char *start, const char *end) {
	int len;
	if (end) {
		len = end - start;
	}
	else {
		len = strlen(start);
	}
	memcpy(&b->berry_buffer[b->berry_len], start, len);
	b->berry_len += len;
}
void BB_AddText(berryBuilder_t *b, const char *fname, const char *start, const char *end) {
	BB_AddCode(b, " echo(\"",0);
#if 0
	BB_AddCode(b, start, end);
#else
	const char *p = start;
	const char *limit = end ? end : (start + strlen(start));
	while (p < limit) {
		char c = *p++;
		switch (c) {
		case '\\': BB_AddCode(b, "\\\\", 0); break;
		case '\"': BB_AddCode(b, "\\\"", 0); break;
		case '\n': BB_AddCode(b, "\\n", 0); break;
		case '\r': BB_AddCode(b, "\\r", 0); break;
		case '\t': BB_AddCode(b, "\\t", 0); break;
		default:
			BB_AddCode(b, &c, &c + 1);
			break;
		}
	}

#endif
	BB_AddCode(b, "\")", 0);
}
void eval_berry_snippet(const char *s);
void Berry_SaveRequest(http_request_t *r);
void BB_Run(berryBuilder_t *b)
{
	b->berry_buffer[b->berry_len] = 0;
	eval_berry_snippet(b->berry_buffer);
}
int http_runBerryFile(http_request_t *request, const char *fname) {
	Berry_SaveRequest(request);
	berryBuilder_t bb;
	BB_Start(&bb);
	char *data = (char*)LFS_ReadFile(fname);
	if (data == 0)
		return 0;
	http_setup(request, httpMimeTypeHTML);
	char *p = data;
	while (*p) {
		char *btag = strstr(p, "<?b");
		if (!btag) {
			break;
		}
		BB_AddText(&bb, fname, p, btag);
		char *etag = strstr(btag, "?>");

		BB_AddCode(&bb, btag + 3, etag);

		p = etag + 2;
	}
	const char *s = p;
	while (*p)
		p++;
	BB_AddText(&bb, fname, s, p);
	free(data);
	BB_Run(&bb);
	return 1;
}
static int http_rest_run_lfs_file(http_request_t* request) {
	char* fpath;
	// don't start LFS just because we're trying to read a file -
	// it won't exist anyway
	if (!lfs_present()) {
		request->responseCode = HTTP_RESPONSE_NOT_FOUND;
		http_setup(request, httpMimeTypeText);
		poststr(request, NULL);
		return 0;
	}
#if ENABLE_OBK_BERRY
	const char* base = request->url + strlen("api/lfs/");
	const char* q = strchr(base, '?');
	size_t len = q ? (size_t)(q - base) : strlen(base);
	fpath = os_malloc(len + 1);
	memcpy(fpath, base, len);
	fpath[len] = '\0';
	int ran = http_runBerryFile(request, fpath);
	if (ran==0) 
#endif
	{
		request->responseCode = HTTP_RESPONSE_NOT_FOUND;
		http_setup(request, httpMimeTypeText);
		poststr(request, NULL);
		return 0;
	}
	free(fpath);
	return 0;
}

static int http_rest_get_lfs_file(http_request_t* request) {
	char* fpath;
	char* buff;
	int len;
	int lfsres;
	int total = 0;
	lfs_file_t* file;
	char *args;
	bool isGzip;

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

	isGzip = EndsWith(fpath, "gz");

	ADDLOG_DEBUG(LOG_FEATURE_API, "LFS read of %s", fpath);
	lfsres = lfs_file_open(&lfs, file, fpath, LFS_O_RDONLY);

	if (lfsres == -21) {
		lfs_dir_t* dir;
		ADDLOG_DEBUG(LOG_FEATURE_API, "%s is a folder", fpath);
		dir = os_malloc(sizeof(lfs_dir_t));
		memset(dir, 0, sizeof(*dir));
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
			char* ext = fpath;
			const char *mimetype = httpMimeTypeBinary;

			if (isGzip) {
				// find original extension (e.g., .js from .js.gz)
				char* dot = strrchr(fpath, '.');
				if (dot) {
					*dot = '\0'; // temporarily strip .gz
					if (EndsWith(fpath, ".js")) {
						mimetype = httpMimeTypeJavascript;
					}
					else if (EndsWith(fpath, ".html")) {
						mimetype = httpMimeTypeHTML;
					}
					else if (EndsWith(fpath, ".css")) {
						mimetype = httpMimeTypeCSS;
					}
					else if (EndsWith(fpath, ".json")) {
						mimetype = httpMimeTypeJson;
					}
					else if (EndsWith(fpath, ".ico")) {
						mimetype = "image/x-icon";
					}
					*dot = '.'; // restore .gz
				}
			}
			else {
				if (EndsWith(fpath, ".js") || EndsWith(fpath, ".vue")) {
					mimetype = httpMimeTypeJavascript;
				}
				else if (EndsWith(fpath, ".json")) {
					mimetype = httpMimeTypeJson;
				}
				else if (EndsWith(fpath, ".html")) {
					mimetype = httpMimeTypeHTML;
				}
				else if (EndsWith(fpath, ".css")) {
					mimetype = httpMimeTypeCSS;
				}
				else if (EndsWith(fpath, ".ico")) {
					mimetype = "image/x-icon";
				}
			}

			if (isGzip) {
				http_setup_gz(request, mimetype);
			}
			else {
				http_setup(request, mimetype);
			}
			//#if ENABLE_OBK_BERRY
			//			http_runBerryFile(request, fpath);
			//#else
			do {
				len = lfs_file_read(&lfs, file, buff, 1024);
				total += len;
				if (len) {
					//ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes read", len);
					postany(request, buff, len);
				}
			} while (len > 0);
			//#endif
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
bool HTTP_checkLFSOverride(http_request_t* request, const char *ext) {
	char tmp[64];
	//sprintf_s(tmp, sizeof(tmp), "override/%s", request->url);
	//sprintf_s(tmp, sizeof(tmp), "%s%s", request->url, ext);
	strcpy_safe(tmp, request->url, sizeof(tmp));
	strcat_safe(tmp, ext, sizeof(tmp));
	char *fix = strchr(tmp, '?');
	if (fix) {
		*fix = 0;
	}
	lfs_file_t* file;
	file = os_malloc(sizeof(lfs_file_t));
	memset(file,0, sizeof(lfs_file_t));
	int lfsres = lfs_file_open(&lfs, file, tmp, LFS_O_RDONLY);
	if (lfsres == 0) {
		lfs_file_close(&lfs, file);
		free(file);
		strcpy_safe(tmp, "api/lfs/", sizeof(tmp));
		strcat_safe(tmp, request->url, sizeof(tmp));
		strcat_safe(tmp, ext, sizeof(tmp));
		char *oldURL = request->url;
		request->url = tmp;
		http_rest_get_lfs_file(request);
		request->url = oldURL;
		// "api/lfs/", 8)) {
		// "api/run/", 8)) {
		return 1;
	}
	free(file);
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
		poststr(request, "LittleFS is not available");
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
#if ENABLE_LFS_SPI
			if (loops > 50) {
				loops = 0;
				rtos_delay_milliseconds(1);
			}
#else
			if (loops > 10) {
				loops = 0;
				rtos_delay_milliseconds(10);
			}
#endif
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

	for (i = 0; i < CHANNEL_MAX; i++) {
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
	poststr_escapedForJSON(request, CFG_GetShortStartupCommand());
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

int http_rest_error(http_request_t* request, int code, char* msg) {
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

	if (startaddr < 0 || (startaddr + len > DEFAULT_FLASH_LEN)) {
		return http_rest_error(request, -1, "requested flash read out of range");
	}

	int bufferSize = 1024;
	buffer = os_malloc(bufferSize);
	memset(buffer, 0, bufferSize);

	http_setup(request, httpMimeTypeBinary);
	while (len) {
		int readlen = len;
		if (readlen > 1024) {
			readlen = 1024;
		}
		res = HAL_FlashRead(buffer, readlen, startaddr);
		startaddr += readlen;
		len -= readlen;
		postany(request, buffer, readlen);
	}
	poststr(request, NULL);
	os_free(buffer);
	return 0;
}

static int http_rest_get_channels(http_request_t* request) {
	int i;
	int addcomma = 0;

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


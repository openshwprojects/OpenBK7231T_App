#include "http_basic_auth.h"
#include <stdio.h>
#include "../logging/logging.h"
#include "../base64/base64.h"
#include "../new_pins.h"
#include "../new_cfg.h"

#define LOG_FEATURE LOG_FEATURE_HTTP

int http_basic_auth_eval(http_request_t *request) {
#if ALLOW_WEB_PASSWORD
	if (strlen(g_cfg.webPassword) == 0 || (bSafeMode && CFG_HasFlag(OBK_FLAG_HTTP_DISABLE_AUTH_IN_SAFE_MODE))) {
		return HTTP_BASIC_AUTH_OK;
	}
	char tmp_auth[256];
	for (int i = 0; i < request->numheaders; i++) {
		char *header = request->headers[i];
		if (!my_strnicmp(header, "Authorization: Basic ", 21)) {
			char *basic_token = header + 21;
			size_t decoded_len = b64_decoded_size(basic_token);
			if (decoded_len > 255) {
				break;
			}
			if (!b64_decode(basic_token, (unsigned char *)tmp_auth, decoded_len + 1)) {
				ADDLOGF_ERROR("AUTH: Failed to decode B64 token.");
				break;
			}
			tmp_auth[decoded_len] = 0;
			if (!my_strnicmp(tmp_auth, "admin:", 6)) {
				char *basic_auth_password = tmp_auth + 6;
				if (strncmp(basic_auth_password, g_cfg.webPassword, 32) == 0) {
					return HTTP_BASIC_AUTH_OK;
				}
			}
            break;
		}
	}
    return HTTP_BASIC_AUTH_FAIL;
#else
	return HTTP_BASIC_AUTH_OK;
#endif
}

int http_basic_auth_run(http_request_t *request) {
    int result = http_basic_auth_eval(request);
    if (result == HTTP_BASIC_AUTH_FAIL) {
		poststr(request, "HTTP/1.1 401 Unauthorized\r\n");
        poststr(request, "Connection: close");
        poststr(request, "\r\n");
        poststr(request, "WWW-Authenticate: Basic realm=\"OpenBeken HTTP Server\"");
        poststr(request, "\r\n");
        poststr(request, "\r\n");
		poststr(request, NULL);
    }
    return result;
}
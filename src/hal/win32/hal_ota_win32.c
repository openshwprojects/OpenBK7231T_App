#ifdef WINDOWS

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../httpserver/new_http.h"
#include "../../logging/logging.h"

int HAL_FlashRead(char*buffer, int readlen, int startaddr) {
	int res;
	res = 0;
	return res;
}

int http_rest_post_flash(http_request_t* request, int startaddr, int maxaddr)
{

	return 0;
}

#endif


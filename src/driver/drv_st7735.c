#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../httpclient/utils_net.h"
#include "drv_ntp.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../cJSON/cJSON.h"


static commandResult_t CMD_ST7735_Test(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);


	return CMD_RES_OK;
}

void ST7735_Init() {
	CMD_RegisterCommand("ST7735_Test", CMD_ST7735_Test, NULL);


}


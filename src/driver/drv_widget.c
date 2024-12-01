#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "drv_ntp.h"

/*
startDriver widget

*/
const char *demo =
"<button id='colorToggleButton' style='padding: 10px 20px;' onclick='changeColor(this)'>Click Me</button>"
"<script>"
"const colors = ['red', 'green', 'blue'];"
"let currentColorIndex = 0;"
"function changeColor(button) {"
"    currentColorIndex = (currentColorIndex + 1) % colors.length;"
"    button.style.backgroundColor = colors[currentColorIndex];"
"}"
"</script>";


void DRV_Widget_AddToHtmlPage(http_request_t *request) {
	poststr(request, demo);
}

static commandResult_t CMD_Widget_Create(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if(Tokenizer_GetArgsCount()<=1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	const char *fname = Tokenizer_GetArg(0);

	return CMD_RES_OK;
}
void DRV_Widget_Init() {


	CMD_RegisterCommand("widget_create", CMD_Widget_Create, NULL);

}


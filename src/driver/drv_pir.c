#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

int g_onTime;

void PIR_Init() {

}

void PIR_OnEverySecond() {

}

void PIR_OnChannelChanged(int ch, int value) {


}

void PIR_AppendInformationToHTTPIndexPage(http_request_t *request) {
	char tmpA[32];
	if (http_getArg(request->url, "pirTime", tmpA, sizeof(tmpA))) {
		g_onTime = atoi(tmpA);
	}

	hprintf255(request, "<h3>PIR Sensor Settings</h3>");
	hprintf255(request, "<form action=\"/\" method=\"get\">");
	hprintf255(request, "On Time (seconds): <input type=\"text\" name=\"pirTime\" value=\"%i\"/>", g_onTime);
	hprintf255(request, "<input type=\"submit\" value=\"Save\"/>");
	hprintf255(request, "</form>");
}









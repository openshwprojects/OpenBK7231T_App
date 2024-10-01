
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../mqtt/new_mqtt.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_tuyaMCU.h"
#include "drv_uart.h"
#include <time.h>
#include "drv_ntp.h"

int g_numLEDs = 0;
byte *g_ledStates;
int g_id_states = 104;

// startDriver tmPixLED
void TuyaMCU_PixelLED_Init() {
	g_numLEDs = Tokenizer_GetArgIntegerDefault(1, 12);
	g_ledStates = (byte*)malloc(g_numLEDs);
}
void TuyaMCU_PixelLED_SetOnOff() {
	char tmp[32];
	for (int i = 0; i < g_numLEDs; i++) {
		tmp[i] = g_ledStates[i] ? '1' : '0';
	}
	tmp[g_numLEDs] = 0;
	// dpId=104 Str V=111111111111
	// - Individual bulb on/off as ASCII 1 or 0:
	TuyaMCU_SendString(g_id_states, tmp);
}
void TuyaMCU_PixelLED_AppendInformationToHTTPIndexPage(http_request_t* request) {
	int i;
	char tmp[16];
	if (http_getArg(request->url, "px", tmp, sizeof(tmp))) {
		int px = atoi(tmp);
		http_getArg(request->url, "st", tmp, sizeof(tmp));
		int st = atoi(tmp);
		g_ledStates[px] = st;

		TuyaMCU_PixelLED_SetOnOff();
	}

	hprintf255(request, "<h4>Pixel LEDs</h4>");
	hprintf255(request, "<div style=\"display: flex;\">");
	for (i = 0; i < g_numLEDs; i++) {
		int state = g_ledStates[i];
		const char *cl = state ? "bgrn" : "bred";
		hprintf255(request, "<form method=\"POST\" action=\"/index?px=%i&st=%i\" style=\"margin-right: 10px;\">",
			i, !state);
		hprintf255(request, "<input type=\"hidden\" name=\"led_id\" value=\"%d\">", i);
		hprintf255(request, "<input type=\"submit\" value=\"%i\" style=\"width: 32px;\" class=\"%s\">", i, cl);
		hprintf255(request, "</form>");
	}
	hprintf255(request, "</div>");
}






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
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#ifndef WINDOWS
#include <lwip/dns.h>
#endif

xTaskHandle g_weather_thread = NULL;

static void weather_thread(beken_thread_arg_t arg) {
	struct hostent *he;
	char hostname[] = "api.openweathermap.org";
	struct in_addr **addr_list;
	int s;

	he = gethostbyname(hostname);
	if (he == NULL) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Failed to resolve hostname.");
		return;
	}
	addr_list = (struct in_addr **)he->h_addr_list;
	ADDLOG_ERROR(LOG_FEATURE_HTTP, "Resolved IP address: %s\n", inet_ntoa(*addr_list[0]));

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Could not create socket.");
		return;
	}

	struct sockaddr_in server;
	server.sin_addr = *addr_list[0];
	server.sin_family = AF_INET;
	server.sin_port = htons(80);

	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Connection failed.");
		closesocket(s);
		return 1;
	}
	// let's just assume that you didn't see my key here, ok? 
	const char *request = "GET /data/2.5/weather?lat=40.7128&lon=-74.0060&appid=d6fae53c4278ffb3fe4c17c23fc6a7c6 HTTP/1.1\r\n"
		"Host: api.openweathermap.org\r\n"
		"Connection: close\r\n\r\n";

	if (send(s, request, strlen(request), 0) < 0) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Send failed");
		closesocket(s);
		return 1;
	}

	char buffer[512];
	int recv_size;
	do {
		recv_size = recv(s, buffer, sizeof(buffer) - 1, 0);
		if (recv_size > 0) {
			buffer[recv_size] = '\0';
			ADDLOG_ERROR(LOG_FEATURE_HTTP, buffer);
		}
	} while (recv_size > 0);

	closesocket(s);
}
void startWeatherThread() {
	OSStatus err = kNoErr;

	err = rtos_create_thread(&g_weather_thread, BEKEN_APPLICATION_PRIORITY,
		"OWM",
		(beken_thread_function_t)weather_thread,
		8192,
		(beken_thread_arg_t)0);
	if (err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"OWM\" thread failed with %i!\r\n", err);
	}
}
static commandResult_t CMD_OWM_Request(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	startWeatherThread();

	return CMD_RES_OK;
}
void DRV_OpenWeatherMap_Init() {
	
	CMD_RegisterCommand("owm_request", CMD_OWM_Request, NULL);


}


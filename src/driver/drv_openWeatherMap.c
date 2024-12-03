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
#ifndef WINDOWS
#include <lwip/dns.h>
#endif

#ifdef WINDOWS
typedef int xTaskHandle;
#endif

xTaskHandle g_weather_thread = NULL;

static char g_request[512]; // Adjust size as needed
static char g_reply[1024];

void Weather_SetReply(const char *s) {
	strcpy_safe(g_reply, s, sizeof(g_reply));
}

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
		closesocket(s);
		return 1;
	}
	if (send(s, g_request, strlen(g_request), 0) < 0) {
		closesocket(s);
		return 1;
	}
	char buffer[1024];
	int recv_size = recv(s, buffer, sizeof(buffer) - 1, 0);
	ADDLOG_ERROR(LOG_FEATURE_HTTP, "Rec %i",recv_size);
	if (recv_size > 0) {
		buffer[recv_size] = '\0';
		Weather_SetReply(buffer);
		ADDLOG_ERROR(LOG_FEATURE_HTTP, buffer);
	}

	//HAL_TCP_Destroy(s);
	//lwip_close_force(s);
	closesocket(s);
	rtos_delete_thread(NULL);
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
static void weather_thread2(beken_thread_arg_t arg) {
	int s = HAL_TCP_Establish("api.openweathermap.org", 80);
	if (s == 0) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Connect fail.");
		return;
	}
	int len = strlen(g_request);
	if (HAL_TCP_Write(s, g_request, len, 1000) != len) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Send failed");
		rtos_delay_milliseconds(250);
		closesocket(s);
		return 1;
	}
	char buffer[1024];
	int recv_size = HAL_TCP_Read(s, buffer, sizeof(buffer) - 1, 1000);
	ADDLOG_ERROR(LOG_FEATURE_HTTP, "Rec %i", recv_size);
	if (recv_size > 0) {
		buffer[recv_size] = '\0';
		Weather_SetReply(buffer);
		ADDLOG_ERROR(LOG_FEATURE_HTTP, buffer);
	}

	HAL_TCP_Destroy(s);
	rtos_delete_thread(NULL);
}
void startWeatherThread2() {
	OSStatus err = kNoErr;

	err = rtos_create_thread(&g_weather_thread, BEKEN_APPLICATION_PRIORITY,
		"OWM",
		(beken_thread_function_t)weather_thread2,
		8192,
		(beken_thread_arg_t)0);
	if (err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"OWM\" thread failed with %i!\r\n", err);
	}
}
static commandResult_t CMD_OWM_Request2(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	startWeatherThread2();

	return CMD_RES_OK;
}
static commandResult_t CMD_OWM_Setup(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	const char *lat = Tokenizer_GetArg(0);
	const char *lng = Tokenizer_GetArg(1);
	const char *key = Tokenizer_GetArg(2);

	snprintf(g_request, sizeof(g_request),
		"GET /data/2.5/weather?lat=%s&lon=%s&appid=%s HTTP/1.1\r\n"
		"Host: api.openweathermap.org\r\n"
		"Connection: close\r\n\r\n",
		lat, lng, key);

	ADDLOG_ERROR(LOG_FEATURE_HTTP, g_request);

	return CMD_RES_OK;
}
void OWM_AppendInformationToHTTPIndexPage(http_request_t *request) {

	hprintf255(request, "<h4>OpenWeatherMap Integration</h4>");
	if (1) {
		hprintf255(request, "<h6>Raw Reply (only in DEBUG)</h6>");
		if (strlen(g_reply) > 0) {
			poststr(request, "<textarea style='width:100%%; height:200px;'>");
			poststr(request, g_reply);
			poststr(request, "</textarea>");
		}
	}
}
/*
startDriver OpenWeatherMap
owm_setup 40.7128 -74.0060 d6fae53c4278ffb3fe4c17c23fc6a7c6 
owm_request
*/
void DRV_OpenWeatherMap_Init() {
	CMD_RegisterCommand("owm_setup", CMD_OWM_Setup, NULL);
	CMD_RegisterCommand("owm_request", CMD_OWM_Request, NULL);
	CMD_RegisterCommand("owm_request2", CMD_OWM_Request2, NULL);


}


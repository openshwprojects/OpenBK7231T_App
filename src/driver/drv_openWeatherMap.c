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
#include "lwip/netdb.h"
#include "../cJSON/cJSON.h"

#ifndef WINDOWS
#include <lwip/dns.h>
#endif

#ifdef WINDOWS
typedef int xTaskHandle;
#endif

xTaskHandle g_weather_thread = NULL;

static char g_request[512];
static char g_reply[1024];

typedef struct weatherData_s {
	double lon;
	double lat;
	char main_weather[32];
	char description[32];
	double temp;
	int pressure;
	int humidity;
	int timezone;
	int sunrise;
	int sunset;
} weatherData_t;

typedef struct weatherChannels_s {
	byte bInitialized;
	char temperature;
	char humidity;
	char pressure;
} weatherChannels_t;

weatherData_t g_weather;
weatherChannels_t g_channels;

void Weather_SetReply(const char *s) {
	const char *json_start = strstr(s, "\r\n\r\n");
	if (json_start) {
		json_start += 4;
		strcpy_safe(g_reply, json_start, sizeof(g_reply));

		cJSON *json = cJSON_Parse(json_start);
		if (json) {
			cJSON *coord = cJSON_GetObjectItem(json, "coord");
			if (coord) {
				g_weather.lon = cJSON_GetObjectItem(coord, "lon") ? cJSON_GetObjectItem(coord, "lon")->valuedouble : 0.0;
				g_weather.lat = cJSON_GetObjectItem(coord, "lat") ? cJSON_GetObjectItem(coord, "lat")->valuedouble : 0.0;
			}

			cJSON *weather_array = cJSON_GetObjectItem(json, "weather");
			if (weather_array && cJSON_IsArray(weather_array)) {
				cJSON *weather = cJSON_GetArrayItem(weather_array, 0);
				if (weather) {
					strncpy(g_weather.main_weather, cJSON_GetObjectItem(weather, "main") ? cJSON_GetObjectItem(weather, "main")->valuestring : "Unknown", sizeof(g_weather.main_weather) - 1);
					strncpy(g_weather.description, cJSON_GetObjectItem(weather, "description") ? cJSON_GetObjectItem(weather, "description")->valuestring : "Unknown", sizeof(g_weather.description) - 1);
				}
			}

			cJSON *main = cJSON_GetObjectItem(json, "main");
			if (main) {
				g_weather.temp = cJSON_GetObjectItem(main, "temp") ? cJSON_GetObjectItem(main, "temp")->valuedouble : 0.0;
				g_weather.pressure = cJSON_GetObjectItem(main, "pressure") ? cJSON_GetObjectItem(main, "pressure")->valueint : 0;
				g_weather.humidity = cJSON_GetObjectItem(main, "humidity") ? cJSON_GetObjectItem(main, "humidity")->valueint : 0;
			}

			g_weather.timezone = cJSON_GetObjectItem(json, "timezone") ? cJSON_GetObjectItem(json, "timezone")->valueint : 0;
			g_weather.sunrise = cJSON_GetObjectItem(json, "sys") ? cJSON_GetObjectItem(cJSON_GetObjectItem(json, "sys"), "sunrise")->valueint : 0;
			g_weather.sunset = cJSON_GetObjectItem(json, "sys") ? cJSON_GetObjectItem(cJSON_GetObjectItem(json, "sys"), "sunset")->valueint : 0;

			cJSON_Delete(json);
		}
		else {
			ADDLOG_ERROR(LOG_FEATURE_HTTP, "Failed to parse JSON");
		}
	}
	else {
		g_reply[0] = '\0';
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "No JSON found in reply");
	}
	if (g_channels.bInitialized) {
		if (g_channels.temperature != -1) {
			CHANNEL_SetSmart(g_channels.temperature, g_weather.temp, 0);
		}
		if (g_channels.humidity != -1) {
			CHANNEL_SetSmart(g_channels.humidity, g_weather.humidity, 0);
		}
		if (g_channels.pressure != -1) {
			CHANNEL_SetSmart(g_channels.pressure, g_weather.pressure, 0);
		}
	}
}
#if 0

static void sendQueryThreadInternal() {
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
		return;
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
}
#else

static void sendQueryThreadInternal() {
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
		return;
	}
	if (send(s, g_request, strlen(g_request), 0) < 0) {
		closesocket(s);
		return;
	}
	char buffer[1024];
	int recv_size = recv(s, buffer, sizeof(buffer) - 1, 0);
	ADDLOG_ERROR(LOG_FEATURE_HTTP, "Rec %i", recv_size);
	if (recv_size > 0) {
		buffer[recv_size] = '\0';
		Weather_SetReply(buffer);
		ADDLOG_ERROR(LOG_FEATURE_HTTP, buffer);
	}

	//HAL_TCP_Destroy(s);
	//lwip_close_force(s);
	closesocket(s);

}
#endif
static void weather_thread(beken_thread_arg_t arg) {
	sendQueryThreadInternal();
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
static commandResult_t CMD_OWM_Channels(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	g_channels.bInitialized = 1;
	g_channels.temperature = Tokenizer_GetArgIntegerDefault(0,-1);
	g_channels.humidity = Tokenizer_GetArgIntegerDefault(1, -1);
	g_channels.pressure = Tokenizer_GetArgIntegerDefault(2, -1);



	return CMD_RES_OK;
}
static commandResult_t CMD_OWM_Setup(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	const char *lat = Tokenizer_GetArg(0);
	const char *lng = Tokenizer_GetArg(1);
	const char *key = Tokenizer_GetArg(2);

	snprintf(g_request, sizeof(g_request),
		"GET /data/2.5/weather?lat=%s&lon=%s&appid=%s&units=metric HTTP/1.1\r\n"
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
	if (strlen(g_reply) > 0) {
		char buff[16];

		hprintf255(request, "<h5>Weather: %s (%s)</h5>", g_weather.main_weather, g_weather.description);
		hprintf255(request, "<h5>Temperature: %.2f C, Pressure: %d hPa, Humidity: %d%%</h5>", g_weather.temp, g_weather.pressure, g_weather.humidity);

		struct tm *tm = gmtime(&g_weather.sunrise);
		strftime(buff, sizeof(buff), "%H:%M:%S", tm);
		hprintf255(request, "<h5>Timezone: %d, Sunrise: %s, ", g_weather.timezone, buff);
		tm = gmtime(&g_weather.sunset);
		strftime(buff, sizeof(buff), "%H:%M:%S", tm);
		hprintf255(request, "Sunset: %s</h5>", buff);
	}
}
/*
startDriver OpenWeatherMap
// owm_setup lat long APIkey
owm_setup 40.7128 -74.0060 d6fae53c4278ffb3fe4c17c23fc6a7c6 
setChannelType 3 Temperature_div10
setChannelType 4 Humidity
setChannelType 5 Pressure_div100
// owm_channels temperature humidity pressure
owm_channels 3 4 5
// must have wifi
waitFor WiFiState 4
// this will send a HTTP GET once
owm_request
*/
void DRV_OpenWeatherMap_Init() {
	//cmddetail:{"name":"owm_setup","args":"[lat][lng][api_key]",
	//cmddetail:"descr":"Setups OWM driver for your location and API key",
	//cmddetail:"fn":"NULL);","file":"driver/drv_openWeatherMap.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("owm_setup", CMD_OWM_Setup, NULL);
	//cmddetail:{"name":"owm_request","args":"",
	//cmddetail:"descr":"Sends OWM request to the API. Do not use it too often, as API may have limits",
	//cmddetail:"fn":"NULL);","file":"driver/drv_openWeatherMap.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("owm_request", CMD_OWM_Request, NULL);
	//cmddetail:{"name":"owm_channels","args":"[temperature][humidity][pressure]",
	//cmddetail:"descr":"Sets channels that will be used to store OWM response results",
	//cmddetail:"fn":"NULL);","file":"driver/drv_openWeatherMap.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("owm_channels", CMD_OWM_Channels, NULL);


}


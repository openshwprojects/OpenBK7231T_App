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

xTaskHandle g_weather_thread = NULL;

static void weather_thread(beken_thread_arg_t arg) {

}
void startWeatherThread() {
	OSStatus err = kNoErr;

	err = rtos_create_thread(&g_weather_thread, BEKEN_APPLICATION_PRIORITY,
		"OWM",
		(beken_thread_function_t)weather_thread,
		0x800,
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


#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

static int32_t g_temperature, g_pressure;
static softI2C_t g_softI2C;

unsigned short BMP280_Start(void) {
	Soft_I2C_Start_Internal(&g_softI2C);
	return 0;
}
unsigned short BMP280_Write(byte data_) {
	Soft_I2C_WriteByte(&g_softI2C, data_);
	return 0;
}
unsigned short BMP280_Read(unsigned short ack) {
	byte r;
	r = Soft_I2C_ReadByte(&g_softI2C, !ack);
	return r;
}
void BMP280_Stop(void) {		//manufacturer ID
	Soft_I2C_Stop(&g_softI2C);
}

#include "BMP280.h"

// startDriver BMP280
void BMP280_Init() {

	uint8_t buff[4];

	g_softI2C.pin_clk = 8;
	g_softI2C.pin_data = 14;

	Soft_I2C_PreInit(&g_softI2C);

	usleep(100);
	if (BMP280_begin(MODE_NORMAL, SAMPLING_X1, SAMPLING_X1, FILTER_OFF, STANDBY_0_5) == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "BMP280 failed!");
	} else {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "BMP280 ready!");
	}
}


void BMP280_OnEverySecond() {

	BMP280_readTemperature(&g_temperature);  // read temperature
	BMP280_readPressure(&g_pressure);        // read pressure

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "T %i, P %i!", g_temperature, g_pressure);
}

void BMP280_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>BMP280 Temperature=%f, Pressure=%f</h2>", g_temperature*0.01f, g_pressure*0.1f);
}


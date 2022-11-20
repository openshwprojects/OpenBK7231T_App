
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

static int g_elapsedTime = 0;
static int g_hadMQTT = 0;
static int g_delay_between_publishes = 10;
static int g_cur_delay_publish = 0;

static byte g_hello[] =  { 0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00 };
static byte g_request_state[] =  { 0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x04, 0x06 };

void TuyaMCU_Sensor_Init() {
	g_elapsedTime = 0;
}
void TuyaMCU_Sensor_RunFrame() {

	g_elapsedTime++;

	if(g_elapsedTime == 3) {
		TuyaMCU_Send_RawBuffer(g_hello,sizeof(g_hello));
	}
	else if(g_elapsedTime == 6) {
		TuyaMCU_Send_RawBuffer(g_request_state,sizeof(g_request_state));
	} else if(g_elapsedTime > 10) {
		if(Main_HasMQTTConnected()) {
			g_hadMQTT++;
			if(g_hadMQTT > 2){
				g_cur_delay_publish--;
				if (g_cur_delay_publish <= 0) {

				}
				MQTT_PublishOnlyDeviceChannelsIfPossible();
			}
		} else {
			g_hadMQTT = 0;
		}
	} else if(g_elapsedTime > 50){
		g_elapsedTime = 0;
	}
}



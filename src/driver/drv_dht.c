#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_dht_internal.h"

dht_t *test = 0;

// simplest demo
void DHT_DoMyDHTTest() {
	if (test == 0) {
		test = DHT_Create(24, DHT11);
	}
	if (test) {
		float temp, humid;

		humid = DHT_readHumidity(test, false);
		temp = DHT_readTemperature(test, false, false);

		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "DHT says %f %f", humid, temp);
	}
}

void DHT_OnEverySecond() {

}

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

// test device
static dht_t *test = 0;
// per-pin devices
static dht_t **g_dhts = 0;

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
int translateDHTType(int role) {
	if (role == IOR_DHT11)
		return DHT11;
	if (role == IOR_DHT12)
		return DHT12;
	if (role == IOR_DHT21)
		return DHT21;
	if (role == IOR_DHT22)
		return DHT22;
	return 0;
}
int g_dhtsCount = 0;

void DHT_OnPinsConfigChanged() {
	int i;

	g_dhtsCount = 0;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (IS_PIN_DHT_ROLE(g_cfg.pins.roles[i])) {
			g_dhtsCount++;
		}
	}
	if (g_dhtsCount == 0) {
		if (g_dhts) {
			for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
				if (g_dhts[i]) {
					free(g_dhts[i]);
					g_dhts[i] = 0;
				}
			}
			free(g_dhts);
			g_dhts = 0;
		}
		// nothing to do here
		return;
	}
	if (g_dhts == 0) {
		g_dhts = (dht_t**)malloc(sizeof(dht_t*)*PLATFORM_GPIO_MAX);
		memset(g_dhts, 0, sizeof(dht_t*)*PLATFORM_GPIO_MAX);
	}
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (IS_PIN_DHT_ROLE(g_cfg.pins.roles[i])) {
			if (g_dhts[i] == 0) {
				g_dhts[i] = DHT_Create(i, translateDHTType(g_cfg.pins.roles[i]));
			}
		}
		else {
			if (g_dhts[i] != 0) {
				free(g_dhts[i]);
				g_dhts[i] = 0;
			}
		}
	}
}
void DHT_OnEverySecond() {
	int i;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (IS_PIN_DHT_ROLE(g_cfg.pins.roles[i])) {
			if (g_dhts[i] == 0) {
				g_dhts[i] = DHT_Create(i, translateDHTType(g_cfg.pins.roles[i]));
			}
			if (g_dhts[i]) {
				float temp, humid;

				humid = DHT_readHumidity(g_dhts[i], false);
				temp = DHT_readTemperature(g_dhts[i], false, false);

				if (g_dhts[i]->_lastresult != 0) {
					// don't want to loose accuracy, so multiply by 10
					// We have a channel types to handle that
					CHANNEL_Set(g_cfg.pins.channels[i], (int)(temp * 10), 0);
					CHANNEL_Set(g_cfg.pins.channels2[i], (int)(humid), 0);
				}
			}
		}
		else {
			if (g_dhts[i] != 0) {
				free(g_dhts[i]);
				g_dhts[i] = 0;
			}
		}
	}
}

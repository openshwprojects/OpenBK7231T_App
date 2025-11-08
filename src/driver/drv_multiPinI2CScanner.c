
#include "../obk_config.h"

#include "drv_local.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"

int g_adr = 1;
int g_scl = 0;
int g_sda = 1;
softI2C_t g_scanI2C;

bool canUsePin(int pin) {
#if PLATFORM_ESP8266
	if (pin == 6) {
		return false;
	}
	if (pin == 7) {
		return false;
	}
#endif
	return true;
}
static int nextValidPin(int pin) {
	do {
		pin++;
		if (pin >= PLATFORM_GPIO_MAX)
			pin = 0;
	} while (!canUsePin(pin));
	return pin;
}
// startDriver MultiPinI2CScanner
void MultiPinI2CScanner_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	hprintf255(request, "Scan pins: %i %i, adr %i", g_scl, g_sda, g_adr);

}
void MultiPinI2CScanner_RunFrame() {

	g_scanI2C.pin_clk = g_scl;
	g_scanI2C.pin_data = g_sda;
	Soft_I2C_PreInit(&g_scanI2C);
	bool bOk = Soft_I2C_Start(&g_scanI2C, (g_adr << 1) + 0);
	Soft_I2C_Stop(&g_scanI2C);
	if (bOk) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "Pins SDA = %i, SCL =%i, adr 0x%x (dec %i)\n",
			g_sda, g_scl, g_adr);
	}
	g_adr++;
	if (g_adr >= 128) {
		g_adr = 0;

		// advance SCL
		g_scl = nextValidPin(g_scl);

		// avoid SDA == SCL
		if (g_scl == g_sda)
			g_scl = nextValidPin(g_scl);

		// if SCL wrapped back to start, move SDA
		if (g_scl == 0) {
			g_sda = nextValidPin(g_sda);

			// avoid SDA == SCL after moving SDA
			if (g_scl == g_sda)
				g_sda = nextValidPin(g_sda);
		}

		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "Next SDA = %i, SCL =%i", g_sda, g_scl);
	}
}

void MultiPinI2CScanner_Init() {

}


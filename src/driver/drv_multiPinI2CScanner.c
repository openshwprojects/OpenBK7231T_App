
#include "../obk_config.h"

#include "drv_local.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"

int g_adr = 1;
int g_scl = 0;
int g_sda = 0;
softI2C_t g_scanI2C;

void MultiPinI2CScanner_RunFrame() {


	bool bOk = Soft_I2C_Start(&g_scanI2C, (g_adr << 1) + 0);
	Soft_I2C_Stop(&g_scanI2C);
	if (bOk) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "Pins SDA = %i, SCL =%i, adr 0x%x (dec %i)\n",
			g_sda, g_scl, g_adr);
	}
	g_adr++;
	if (g_adr >= 128) {
		g_scl++;
		if (g_scl == g_sda) {
			g_scl++;
		}
		if (g_scl >= PLATFORM_GPIO_MAX) {
			g_scl = 0;
			g_sda++;
			if (g_sda >= PLATFORM_GPIO_MAX) {
				g_sda = 0;
			}
		}
	}
}

void MultiPinI2CScanner_Init() {

}


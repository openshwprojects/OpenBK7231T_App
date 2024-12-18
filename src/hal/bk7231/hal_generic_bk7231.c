#include "../../new_common.h"
#include "../hal_generic.h"
#include <BkDriverWdg.h>

// from wlan_ui.c
void bk_reboot(void);
extern bool g_powersave;

void HAL_RebootModule() 
{
	bk_reboot();
}

void HAL_Delay_us(int delay)
{
	float adj = 1;
	if(g_powersave) adj = 1.5;
	usleep((17 * delay * adj) / 10); // "1" is to fast and "2" to slow, 1.7 seems better than 1.5 (only from observing readings, no scope involved)
}

void HAL_Configure_WDT()
{
	bk_wdg_initialize(10000);
}

void HAL_Run_WDT()
{
	bk_wdg_reload();
}

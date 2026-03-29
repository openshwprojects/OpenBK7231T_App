#if PLATFORM_BL_NEW

#include "../hal_generic.h"
#include "../../new_common.h"
#include "bflb_common.h"
#include "bflb_wdg.h"

extern int bl_sys_reset_por(void);
struct bflb_device_s* wdg;

void HAL_RebootModule()
{
	//bl_sys_reset_system();
	bl_sys_reset_por();
}

void HAL_Delay_us(int delay)
{
	//arch_delay_us(delay);
	bflb_mtimer_delay_us(delay);
}

void HAL_Configure_WDT()
{
	struct bflb_wdg_config_s wdg_cfg;
	wdg_cfg.clock_source = WDG_CLKSRC_32K;
	wdg_cfg.clock_div = 31;
	wdg_cfg.comp_val = 10000;
	wdg_cfg.mode = WDG_MODE_RESET;
	wdg = bflb_device_get_by_name("watchdog0");
	bflb_wdg_init(wdg, &wdg_cfg);
	bflb_wdg_start(wdg);
}

void HAL_Run_WDT()
{
	bflb_wdg_reset_countervalue(wdg);
}

#endif // PLATFORM_BL602

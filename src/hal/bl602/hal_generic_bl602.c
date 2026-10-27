#if PLATFORM_BL602 || PLATFORM_BL616

#include "../hal_generic.h"
#include "../../new_common.h"
#if !PLATFORM_BL_NEW
#include <hal_sys.h>
#include <bl_wdt.h>
#include <bl_timer.h>
#else
#include "bflb_common.h"
#include "bflb_wdg.h"
extern int bl_sys_reset_por(void);
struct bflb_device_s* wdg;
#define hal_reboot bl_sys_reset_por
#define bl_timer_delay_us bflb_mtimer_delay_us
#endif

void HAL_RebootModule()
{
	hal_reboot();
}

void HAL_Delay_us(int delay)
{
	bl_timer_delay_us(delay);
}

void HAL_Configure_WDT()
{
#if !PLATFORM_BL_NEW
	bl_wdt_init(3000);
#else
	struct bflb_wdg_config_s wdg_cfg;
	wdg_cfg.clock_source = WDG_CLKSRC_32K;
	wdg_cfg.clock_div = 31;
	wdg_cfg.comp_val = 10000;
	wdg_cfg.mode = WDG_MODE_RESET;
	wdg = bflb_device_get_by_name("watchdog0");
	bflb_wdg_init(wdg, &wdg_cfg);
	bflb_wdg_start(wdg);
#endif
}

void HAL_Run_WDT()
{
#if !PLATFORM_BL_NEW
	bl_wdt_feed();
#else
	bflb_wdg_reset_countervalue(wdg);
#endif
}

#endif // PLATFORM_BL602

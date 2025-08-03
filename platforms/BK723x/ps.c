#include "include.h"
#include "co_math.h"
#include "wlan_ui_pub.h"

BK_PS_LEVEL global_ps_level = 0;
int bk_wlan_power_save_set_level(BK_PS_LEVEL level)
{
    if(level & PS_DEEP_SLEEP_BIT)
    {
#if CFG_USE_STA_PS
        if(global_ps_level & PS_RF_SLEEP_BIT)
        {
            bk_wlan_dtim_rf_ps_mode_disable();
        }
#endif
#if CFG_USE_MCU_PS
        if(global_ps_level & PS_MCU_SLEEP_BIT)
        {
            bk_wlan_mcu_ps_mode_disable();
        }
#endif
        rtos_delay_milliseconds(100);
#if CFG_USE_DEEP_PS
        ///  bk_enter_deep_sleep(0xc000,0x0);
#endif
    }

    if((global_ps_level & PS_RF_SLEEP_BIT) ^ (level & PS_RF_SLEEP_BIT))
    {
#if CFG_USE_STA_PS
        if(global_ps_level & PS_RF_SLEEP_BIT)
        {
            bk_wlan_dtim_rf_ps_mode_disable();
        }
        else
        {
            bk_wlan_dtim_rf_ps_mode_enable();
        }
#endif
    }

    if((global_ps_level & PS_MCU_SLEEP_BIT) ^ (level & PS_MCU_SLEEP_BIT))
    {
#if CFG_USE_MCU_PS
        if(global_ps_level & PS_MCU_SLEEP_BIT)
        {
            bk_wlan_mcu_ps_mode_disable();
        }
        else
        {
            bk_wlan_mcu_ps_mode_enable();
        }
#endif
    }

    global_ps_level = level;

    return 0;
}

void __attribute__((weak)) phy_enter_11b_low_power(void) {}
void __attribute__((weak)) phy_exit_11b_low_power(void) {}

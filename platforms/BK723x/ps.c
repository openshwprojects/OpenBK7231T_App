#include "include.h"
#include "rw_pub.h"
#include "vif_mgmt.h"
#include "sa_station.h"
#include "param_config.h"
#include "common/ieee802_11_defs.h"
#include "driver_beken.h"
#include "mac_ie.h"
#include "sa_ap.h"
#include "main_none.h"
#include "sm.h"
#include "mac.h"
#include "sm_task.h"
#include "scan_task.h"
#include "hal_machw.h"
#include "error.h"
#include "lwip_netif_address.h"
#include "lwip/inet.h"
#include <stdbool.h>
#include "rw_pub.h"
#include "power_save_pub.h"
#include "rwnx.h"
#include "net.h"
#include "mm_bcn.h"
#include "mm_task.h"
#include "mcu_ps_pub.h"
#include "manual_ps_pub.h"
#include "gpio_pub.h"
#include "phy_trident.h"
#include "rw_msg_rx.h"
#include "app.h"
#include "ate_app.h"
#include "wdt_pub.h"
#include "start_type_pub.h"
#include "wpa_psk_cache.h"
#include "drv_model_pub.h"
#include "str_pub.h"

#if CFG_WPA_CTRL_IFACE
#include "wlan_defs_pub.h"
#include "wpa_ctrl.h"
#include "flash_pub.h"
#endif

#if CFG_ROLE_LAUNCH
#include "role_launch.h"
#endif

#if (FAST_CONNECT_INFO_ENC_METHOD != ENC_METHOD_NULL)
#include "soft_encrypt.h"
#endif

#if CFG_SUPPORT_OTA_HTTP
#include "utils_httpc.h"
#endif

#include "bk7011_cal_pub.h"
#include "target_util_pub.h"
#include "wlan_ui_pub.h"
#if CFG_WIFI_P2P
#include "video_demo_pub.h"
#include "rw_ieee80211.h"
//#include "sys.h"
#endif
#include "low_voltage_ps.h"
#include "intc_pub.h"
#include "arm_arch.h"

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
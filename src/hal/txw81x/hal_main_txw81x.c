#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../quicktick.h"
#include "sys_config.h"
#include "typesdef.h"
#include "osal/work.h"
#include "lib/skb/skbpool.h"
#include "lib/syscfg/syscfg.h"
#include "syscfg.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "project_config.h"

static struct os_work main_wk;
extern struct system_status sys_status;
extern void TXW_TakeEvents();
uint8_t qc_mode = 0;

uint8 vcam_en()
{
    uint8 ret = TRUE;
#if VCAM_EN
    pmu_vcam_dis();
    os_sleep_ms(1);
    pmu_set_vcam_vol(VCAM_VOL_2V80);
    pmu_vcam_lc_en();
    pmu_vcam_oc_detect_dis();
    pmu_vcam_oc_int_dis();
    pmu_vcam_discharge_dis();
    pmu_vcam_pg_dis();
#ifdef VCAM_33
    pmu_set_vcam_vol(VCAM_VOL_3V25);
    pmu_vcam_en();
    os_sleep_ms(1);
    pmu_vcam_pg_en();
#else
    pmu_vcam_en();
    os_sleep_ms(1);
#endif    
    //    sys_reset_pending_clr();
    pmu_vcam_lc_dis();
#ifndef VCAM_33
    pmu_vcam_oc_detect_en();
#endif
    if(PMU_VCAM_OC_PENDING)
    {
        return FALSE;
    }
    pmu_vcam_oc_detect_dis();
    pmu_vcam_oc_int_dis();
    pmu_lvd_oe_en();
#endif
    return ret;
}

static int32 main_loop(struct os_work* work)
{
    os_run_work_delay(&main_wk, 1000);
    Main_OnEverySecond();
    return 0;
}

int main(void)
{
    uint32 sysheap_freesize(struct sys_heap* heap);
    os_printf("freemem:%d\r\n", sysheap_freesize(&sram_heap));
    void* skb_pool_addr = (void*)os_malloc(30 * 1024);
    if(!skb_pool_addr)
    {
        while(1)
        {
            os_printf("skb malloc fail,malloc size:%d\tremain size:%d\n", 10 * 1024, sysheap_freesize(&sram_heap));
            os_sleep_ms(1000);
        }
    }
    skbpool_init((uint32_t)skb_pool_addr, 30 * 1024, 80, 0);
    syscfg_set_default_val();

    ip_addr_t ipaddr, netmask, gw;
    struct netdev* ndev;
    int offset = sys_cfgs.wifi_mode == WIFI_MODE_STA ? WIFI_MODE_STA : WIFI_MODE_AP;
    tcpip_init(NULL, NULL);

    //ndev = (struct netdev*)dev_get(HG_WIFI0_DEVID + offset - 1);
    //if(ndev)
    //{
    //    ipaddr.addr = sys_cfgs.ipaddr;
    //    netmask.addr = sys_cfgs.netmask;
    //    gw.addr = sys_cfgs.gw_ip;
    //    lwip_netif_add(ndev, "w0", &ipaddr, &netmask, &gw);
    //    lwip_netif_set_default(ndev);
    //    if(sys_cfgs.wifi_mode == WIFI_MODE_STA && sys_cfgs.dhcpc_en)
    //    {
    //        sys_status.dhcpc_done = 0;
    //        lwip_netif_set_dhcp(ndev, 1);
    //        os_printf("start dhcp client ...\r\n");
    //    }
    //    os_printf("add w0 interface!\r\n");
    //}
    //else
    //{
    //    os_printf("**not find wifi device**\r\n");
    //}
    TXW_TakeEvents();
    eloop_init();
    os_task_create("eloop_run", user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    Main_Init();
    OS_WORK_INIT(&main_wk, main_loop, 0);
    os_run_work_delay(&main_wk, 1000);

    uint8 vcam;
    vcam = vcam_en();
    void* custom_buf = (void*)os_malloc(53*1000);
    custom_mem_init(custom_buf, 53 * 1000);
    print_custom_sram();
    stream_work_queue_start();
    jpg_cfg(HG_JPG0_DEVID, VPP_DATA1);
    bool csi_ret;
    bool csi_cfg();
    csi_ret = csi_cfg();
    bool csi_open();
    if(csi_ret)
        csi_open();
    spook_init();
}
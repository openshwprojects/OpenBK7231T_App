/**************************************************************************
 * File Name                   : tls_sys.c
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 * All rights reserved.
 *
 ***************************************************************************/
#include "wm_config.h"
#include "wm_osal.h"
#include "tls_sys.h"
#include "wm_mem.h"
#include "wm_debug.h"
#include "wm_params.h"
#include "wm_regs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm_wifi.h"
#include "wm_netif.h"
#include "wm_sockets.h"
#include "wm_include.h"
#if TLS_CONFIG_AP
#include "wm_dhcp_server.h"
#include "wm_dns_server.h"
#include "wm_cpu.h"
#endif
#include "wm_wl_task.h"
#if TLS_CONFIG_RMMS
#if (GCC_COMPILE==1)
#include "wm_cmdp_hostif_gcc.h"
#else
#include "wm_cmdp_hostif.h"
#endif
#endif


struct tls_sys_msg
{
    u32 msg;
    void *data;
};
#define SYS_TASK_STK_SIZE          384//256

static tls_os_queue_t *msg_queue = NULL;
static tls_os_sem_t *sys_task_sem = NULL;
void tls_sys_task_del(void);
int  tls_sys_task_init(void);


#if TLS_DBG_LEVEL_DUMP
void TLS_DBGPRT_DUMP(char *p, u32 len)
{
    int i;
    if (TLS_DBG_LEVEL_DUMP)
    {
        printf("dump length : %d\n", len);
        for (i = 0; i < len; i++)
        {
            printf("%02X ", p[i]);
            if ((i + 1) % 16 == 0 && (i + 1) % 32 != 0)
            {
                printf("- ");
            }
            if ((i + 1) % 32 == 0)
            {
                printf("\n");
            }
            if (i == 2000)
            {
                printf("\n");
                break;
            }
        }
        printf("\n");
    }
}

//-------------------------------------------------------------------------

#endif

static void sys_net_up()
{
    ip_addr_t ip_addr, net_mask, gateway, dns1, dns2;
    struct tls_param_ip ip_param;
    bool enable = FALSE;	

    tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);
    if (ip_param.dhcp_enable)
    {
        ip_addr_set_zero(&ip_addr);
        ip_addr_set_zero(&net_mask);
        ip_addr_set_zero(&gateway);
        tls_netif_set_addr( &ip_addr, &net_mask, &gateway);
        tls_netif_set_up();
        tls_dhcp_start();
    }
    else
    {
        tls_dhcp_stop();
        tls_netif_set_up();

        MEMCPY((char*)ip_2_ip4(&ip_addr), &ip_param.ip, 4);
        MEMCPY((char*)ip_2_ip4(&net_mask), &ip_param.netmask, 4);
        MEMCPY((char*)ip_2_ip4(&gateway), &ip_param.gateway, 4);
        tls_netif_set_addr( &ip_addr, &net_mask, &gateway);
        MEMCPY((char*)ip_2_ip4(&dns1), &ip_param.dns1, 4);
        MEMCPY((char*)ip_2_ip4(&dns2), &ip_param.dns2, 4);
        tls_netif_dns_setserver(0, &dns1);
        tls_netif_dns_setserver(1, &dns2);
		
		/*when DHCP is disabled, Use static IP without IP_NET_UP Reporting, 
		  set wifi powersaving flag according to TLS_PARAM_ID_PSM here.*/
		tls_param_get(TLS_PARAM_ID_PSM, &enable, TRUE); 	
		tls_wifi_set_psflag(enable, FALSE);
		tls_netif_set_status(1);
    }

    return ;
}

//-------------------------------------------------------------------------

#if TLS_CONFIG_AP
static void sys_net2_up()
{
    ip_addr_t ip_addr, net_mask, gateway;
    struct tls_param_ip ip_param;
    u8 dnsname[32];

    dnsname[0] = '\0';
    tls_param_get(TLS_PARAM_ID_DNSNAME, dnsname, 0);
    tls_param_get(TLS_PARAM_ID_SOFTAP_IP, &ip_param, FALSE);

    tls_netif2_set_up();

    MEMCPY((char*)ip_2_ip4(&ip_addr), &ip_param.ip, 4);
    MEMCPY((char*)ip_2_ip4(&net_mask), &ip_param.netmask, 4);
    MEMCPY((char*)ip_2_ip4(&gateway), &ip_param.gateway, 4);
    tls_netif2_set_addr(&ip_addr, &net_mask, &gateway);

    if (ip_param.dhcp_enable)
    {
        tls_dhcps_start();
    }

    if ('\0' != dnsname[0])
    {
        tls_dnss_start(dnsname);
    }

    return ;
}

//-------------------------------------------------------------------------

static void sys_net2_down()
{
    tls_dnss_stop();

    tls_dhcps_stop();

    tls_netif2_set_down();

    return ;
}
#endif

static void sys_net_down()
{
    struct tls_param_ip ip_param;

#if TLS_CONFIG_RMMS
    tls_rmms_stop();
#endif

    tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);
    if (ip_param.dhcp_enable)
    {
        tls_dhcp_stop();
    }
    tls_netif_set_status(0);
    tls_netif_set_down();

    /* Try to reconnect if auto_connect is set*/
    tls_auto_reconnect(1);

    return ;
}

//-------------------------------------------------------------------------

#if TLS_CONFIG_AP
static void tls_auto_reconnect_softap(void)
{
    struct tls_param_ssid ssid;
    struct tls_softap_info_t *apinfo;
    struct tls_ip_info_t *ipinfo;
    struct tls_param_ip ip_param;
    struct tls_param_key key;

    apinfo = tls_mem_alloc(sizeof(struct tls_softap_info_t));
    if (apinfo == NULL)
    {
        return ;
    }
    ipinfo = tls_mem_alloc(sizeof(struct tls_ip_info_t));
    if (ipinfo == NULL)
    {
        tls_mem_free(apinfo);
        return ;
    }

    tls_param_get(TLS_PARAM_ID_SOFTAP_SSID, (void*) &ssid, TRUE);
    memcpy(apinfo->ssid, ssid.ssid, ssid.ssid_len);
    apinfo->ssid[ssid.ssid_len] = '\0';

    tls_param_get(TLS_PARAM_ID_SOFTAP_ENCRY, (void*) &apinfo->encrypt, TRUE);
    tls_param_get(TLS_PARAM_ID_SOFTAP_CHANNEL, (void*) &apinfo->channel, TRUE);

    tls_param_get(TLS_PARAM_ID_SOFTAP_KEY, (void*) &key, TRUE);
    apinfo->keyinfo.key_len = key.key_length;
    apinfo->keyinfo.format = key.key_format;
    apinfo->keyinfo.index = key.key_index;
    memcpy(apinfo->keyinfo.key, key.psk, key.key_length);

    tls_param_get(TLS_PARAM_ID_SOFTAP_IP, &ip_param, TRUE);
    /*ip配置信息:ip地址，掩码，dns名称*/
    memcpy(ipinfo->ip_addr, ip_param.ip, 4);
    memcpy(ipinfo->netmask, ip_param.netmask, 4);
    tls_param_get(TLS_PARAM_ID_DNSNAME, ipinfo->dnsname, FALSE);

    tls_wifi_softap_create(apinfo, ipinfo);
    tls_mem_free(apinfo);
    tls_mem_free(ipinfo);
    return ;
}

//-------------------------------------------------------------------------

#endif
void tls_auto_reconnect(u8 delayflag)
{
    struct tls_param_ssid ssid;
    u8 auto_reconnect = 0xff;
    u8 wireless_protocol = 0;
#if 0	
    u8 i = 0;
    while (i < 200)
    {
        if (tls_wifi_get_oneshot_flag())
        {
             /*oneshot config ongoing,do not reconnect WiFi*/
            return ;
        } tls_os_time_delay(1);
        i++;
    }
#endif	
    tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET, &auto_reconnect);
    if (auto_reconnect == WIFI_AUTO_CNT_OFF)
    {
        return ;
    }
    else if (auto_reconnect == WIFI_AUTO_CNT_TMP_OFF)
    {
    	auto_reconnect = WIFI_AUTO_CNT_ON;
    	tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET, &auto_reconnect);
    	return ; //tmparary return, for active "DISCONNECT" , such as AT CMD
    }
	if (delayflag)
	{
		tls_os_time_delay(1500);
	}

    tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void*) &wireless_protocol, TRUE);
    switch (wireless_protocol)
    {
        case TLS_PARAM_IEEE80211_INFRA:
#if TLS_CONFIG_AP
            case (TLS_PARAM_IEEE80211_INFRA | TLS_PARAM_IEEE80211_SOFTAP):
#endif
            {
                struct tls_param_original_key origin_key;
                struct tls_param_bssid bssid;
                tls_param_get(TLS_PARAM_ID_BSSID, (void*) &bssid, TRUE);
                tls_param_get(TLS_PARAM_ID_SSID, (void*) &ssid, TRUE);				
                tls_param_get(TLS_PARAM_ID_ORIGIN_KEY, (void*) &origin_key,  TRUE);

                if (bssid.bssid_enable)
                {
                    if (ssid.ssid_len && (ssid.ssid_len <= 32))
                    {
                        tls_wifi_connect_by_ssid_bssid(ssid.ssid, ssid.ssid_len, bssid.bssid, origin_key.psk,
                            origin_key.key_length);
                    }
                    else
                    {
                        tls_wifi_connect_by_bssid(bssid.bssid, origin_key.psk, origin_key.key_length);	
                    }
                }
                else
                {
					if (ssid.ssid_len && (ssid.ssid_len <= 32))
					{
						tls_wifi_connect(ssid.ssid, ssid.ssid_len, origin_key.psk, origin_key.key_length);
					}
                }
            }
            break;
#if TLS_CONFIG_AP
        case TLS_PARAM_IEEE80211_SOFTAP:
            {
                tls_auto_reconnect_softap();
            }
            break;
#endif
        default:
            break;
    }

    return ;
}

//-------------------------------------------------------------------------

#if TLS_CONFIG_RMMS
static void tls_proc_rmms(struct rmms_msg *msg)
{
    int err;
    struct tls_hostif *hif = tls_get_hostif();

    if (0 == hif->rmms_status)
    {
        hif->rmms_status = 1;
        err = tls_hostif_cmd_handler(HOSTIF_RMMS_AT_CMD, (char*)msg, 6+strlen(
            (char*)(msg->CmdStr)));
        if (0 != err)
        {
            tls_mem_free(msg);
            hif->rmms_status = 0;
        }
    }

    return ;
}

//-------------------------------------------------------------------------

#endif

/*
 * sys task stack
 */
static u32 *sys_task_stk = NULL;
tls_os_task_t sys_task_hdl = NULL;

void tls_sys_task(void *data)
{
    u8 err;
    struct tls_sys_msg *msg;
    u8 auto_reconnect = WIFI_AUTO_CNT_OFF;

    //u8 oneshotflag = 0;
    //u8 auto_mode = 0;
    for (;;)
    {
        err = tls_os_queue_receive(msg_queue, (void **) &msg, 0, 0);
        if (!err)
        {
            switch (msg->msg)
            {
                case SYS_MSG_NET_UP:
                    sys_net_up();
                    break;
#if TLS_CONFIG_AP
                case SYS_MSG_NET2_UP:
                    sys_net2_up();
                    break;
                case SYS_MSG_NET2_DOWN:
                    sys_net2_down();
                    break;
                case SYS_MSG_NET2_FAIL:
                    sys_net2_down();
                    tls_auto_reconnect(1);
                    break;
#endif
                case SYS_MSG_NET_DOWN:
                    sys_net_down();
                    break;
                case SYS_MSG_CONNECT_FAILED:
					sys_net_down();
                    break;
                case SYS_MSG_AUTO_MODE_RUN:
                    /*restore WiFi auto reconnect Tmp OFF*/
                    tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET,
                        &auto_reconnect);
                    if (auto_reconnect == WIFI_AUTO_CNT_TMP_OFF)
                    {
                        auto_reconnect = WIFI_AUTO_CNT_ON;
                        tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET,
                            &auto_reconnect);
                    }

                    tls_auto_reconnect(0);
                    break;
#if TLS_CONFIG_RMMS
                case SYS_MSG_RMMS:
                    tls_proc_rmms(msg->data);
                    break;
#endif
                default:
                    break;
            }
            tls_mem_free(msg);
        }
        else
        {

        }
		tls_os_sem_acquire(sys_task_sem, 0);
		if (tls_os_queue_is_empty(msg_queue))
		{
			tls_sys_task_del();
		}
		else
		{
			tls_os_sem_release(sys_task_sem);
		}
    }
}

//-------------------------------------------------------------------------

void tls_sys_send_msg(u32 msg, void *data)
{
    struct tls_sys_msg *pmsg = NULL;
	tls_os_status_t os_status = TLS_OS_ERROR;

    pmsg = tls_mem_alloc(sizeof(struct tls_sys_msg));
    if (NULL != pmsg)
    {
		tls_os_sem_acquire(sys_task_sem, 0);
	    if (0 == tls_sys_task_init())
	    {
	        memset(pmsg, 0, sizeof(struct tls_sys_msg));
	        pmsg->msg = msg;
	        pmsg->data = data;
	        os_status = tls_os_queue_send(msg_queue, pmsg, 0);
			if (os_status != TLS_OS_SUCCESS)
			{
				tls_mem_free(pmsg);
				pmsg = NULL;
			}
	    }
		else
		{
			tls_mem_free(pmsg);
			pmsg = NULL;
		}
		tls_os_sem_release(sys_task_sem);
    }

    return ;
}

//-------------------------------------------------------------------------

void tls_sys_auto_mode_run(void)
{
    tls_sys_send_msg(SYS_MSG_AUTO_MODE_RUN, NULL);
}

//-------------------------------------------------------------------------

static void tls_sys_net_up(void)
{
    tls_sys_send_msg(SYS_MSG_NET_UP, NULL);
}

//-------------------------------------------------------------------------

#if TLS_CONFIG_AP
static void tls_sys_net2_up(void)
{
    tls_sys_send_msg(SYS_MSG_NET2_UP, NULL);
}

//-------------------------------------------------------------------------

static void tls_sys_net2_down(void)
{
    tls_sys_send_msg(SYS_MSG_NET2_DOWN, NULL);
}

static void tls_sys_net2_fail(void)
{
    tls_sys_send_msg(SYS_MSG_NET2_FAIL, NULL);
}
#endif
static void tls_sys_net_down(void)
{
    tls_sys_send_msg(SYS_MSG_NET_DOWN, NULL);
}

//-------------------------------------------------------------------------

static void tls_sys_connect_failed(void)
{
    tls_sys_send_msg(SYS_MSG_CONNECT_FAILED, NULL);
}

//-------------------------------------------------------------------------

static void sys_net_status_changed(u8 status)
{
#if TLS_CONFIG_TLS_DEBUG
    struct tls_ethif *ethif;
#endif
#if TLS_CONFIG_AP
    u8 wireless_protocol = 0;
#endif
    bool enable = FALSE;

    switch (status)
    {
        case NETIF_WIFI_JOIN_SUCCESS:
            TLS_DBGPRT_INFO("join net success\n");
            tls_sys_net_up();
            break;
        case NETIF_WIFI_JOIN_FAILED:
            {
                TLS_DBGPRT_INFO("join net failed\n");
            }
            tls_sys_connect_failed();
            break;
        case NETIF_WIFI_DISCONNECTED:
            {
                TLS_DBGPRT_INFO("net disconnected\n");
            }
            tls_sys_net_down();
            break;
        case NETIF_IP_NET_UP:
            tls_netif_set_status(1);
			/*when DHCP enable, IP_NET_UP Report, set wifi powersaving flag according to  TLS_PARAM_ID_PSM*/
            tls_param_get(TLS_PARAM_ID_PSM, &enable, TRUE);		
            tls_wifi_set_psflag(enable, FALSE);

#if TLS_CONFIG_TLS_DEBUG
            ethif = tls_netif_get_ethif();
            TLS_DBGPRT_INFO("net up ==> ip = %x\n", ethif->ip_addr.addr);
#endif
#if TLS_CONFIG_AP
            tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void*) &wireless_protocol,
                TRUE);
            if ((TLS_PARAM_IEEE80211_SOFTAP &wireless_protocol) &&
                (WM_WIFI_JOINED != tls_wifi_softap_get_state()))
            {
                tls_auto_reconnect_softap();
            }
#endif
//#if TLS_CONFIG_RMMS
//            tls_rmms_start();
//#endif
            break;
#if TLS_CONFIG_AP
        case NETIF_WIFI_SOFTAP_SUCCESS:
            TLS_DBGPRT_INFO("softap create success.\n");
            tls_sys_net2_up();
            break;
        case NETIF_WIFI_SOFTAP_FAILED:
            TLS_DBGPRT_INFO("softap create failed.\n");
			tls_sys_net2_fail();		
            break;
        case NETIF_WIFI_SOFTAP_CLOSED:
            TLS_DBGPRT_INFO("softap closed.\n");
            tls_sys_net2_down();
            break;
        case NETIF_IP_NET2_UP:
#if TLS_CONFIG_TLS_DEBUG
            ethif = tls_netif_get_ethif2();
            TLS_DBGPRT_INFO("net up ==> ip = %d.%d.%d.%d\n" ip4_addr1(ip_2_ip4
                (&ethif->ip_addr)), ip4_addr2(ip_2_ip4(&ethif->ip_addr)),
                ip4_addr3(ip_2_ip4(&ethif->ip_addr)), ip4_addr4(ip_2_ip4(&ethif
                ->ip_addr)));
#endif
            break;
#endif
        default:
            break;
    }
}

//-------------------------------------------------------------------------

int tls_sys_init()
{
	int err;
/* create messge queue */
#define SYS_MSG_SIZE     20

	err = tls_os_queue_create(&msg_queue, SYS_MSG_SIZE);
	if (err)
	{
		return	- 1;
	}
	err = tls_os_sem_create(&sys_task_sem, 1);
	if (err)
	{
		tls_os_queue_delete(msg_queue);
		return -2;
	}
	tls_netif_add_status_event(sys_net_status_changed);

    return 0;
}

static void tls_sys_task_free(void)
{
	if (sys_task_stk)
	{
		tls_mem_free(sys_task_stk);
		sys_task_stk = NULL;
		tls_os_sem_release(sys_task_sem);		
	}
}
void tls_sys_task_del(void)
{
	if (sys_task_hdl)
	{
		tls_os_task_del_by_task_handle(sys_task_hdl,tls_sys_task_free);
	}
}

int tls_sys_task_init(void)
{
	int err;

	if ((sys_task_stk == NULL) && sys_task_sem && msg_queue)
	{
		sys_task_stk = (u32 *)tls_mem_alloc(SYS_TASK_STK_SIZE *sizeof(u32));
		if (sys_task_stk)
		{
			/* create task */
		   err =  tls_os_task_create(&sys_task_hdl, "Sys Task", \
				tls_sys_task, (void*)0, \
				(void *)sys_task_stk,				/* task's stack start address */
				SYS_TASK_STK_SIZE *sizeof(u32),  /* task's stack size, unit:byte */
				TLS_SYS_TASK_PRIO, 0);
			if (err != TLS_OS_SUCCESS)
			{
				tls_mem_free(sys_task_stk);
				sys_task_stk = NULL;
				return -2;
			}
		}
		else
		{
			return -3;
		}

		return 0;
	}
	else
	{
		return 0;
	}

}


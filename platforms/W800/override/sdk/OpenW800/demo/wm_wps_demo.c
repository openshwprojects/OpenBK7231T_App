/*****************************************************************************
*
* File Name : wm_mcast_demo.c
*
* Description: mcast demo function
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : wanghaifang
*
* Date : 2014-6-2
*****************************************************************************/

#include <string.h>
#include "wm_include.h"

#if DEMO_WPS
static void demo_wps_netif_stataus_callbak(u8 status)
{
    switch (status)
    {
    case NETIF_WIFI_JOIN_SUCCESS:
        printf("WiFi JOIN SUCCESS\r\n");
        break;
    case NETIF_WIFI_JOIN_FAILED:
        printf("WiFi JOIN FAILED:%d\r\n", tls_wifi_get_errno());
        break;
    case NETIF_WIFI_DISCONNECTED:
        printf("WiFi DISCONNECT\r\n");
        break;
    case NETIF_IP_NET_UP:
    {
        struct tls_ethif *ethif;
        ethif = tls_netif_get_ethif();
        printf("NET UP OK,Local IP:%d.%d.%d.%d\r\n", ip4_addr1(ip_2_ip4(&ethif->ip_addr)), ip4_addr2(ip_2_ip4(&ethif->ip_addr)),
               ip4_addr3(ip_2_ip4(&ethif->ip_addr)), ip4_addr4(ip_2_ip4(&ethif->ip_addr)));
    }
    break;
    default:
        printf("Not Expected Value:%d\r\n", status);
        break;
    }
}

int demo_wps_pbc(char *buf)
{
    struct tls_param_ip ip_param;
    int ret = WM_FAILED;
    tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);
    ip_param.dhcp_enable = TRUE;
    tls_param_set(TLS_PARAM_ID_IP, &ip_param, FALSE);

#if TLS_CONFIG_WPS
    tls_wifi_set_oneshot_flag(0);
    tls_netif_add_status_event(demo_wps_netif_stataus_callbak);/*register net status event*/
    ret = tls_wps_start_pbc();
#endif
    if(ret == WM_SUCCESS)
        printf("Start WPS pbc mode ... \n");


    return WM_SUCCESS;
}


int demo_wps_pin(char *buf)
{
    int ret = WM_FAILED;
    struct tls_param_ip ip_param;

    tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);
    ip_param.dhcp_enable = TRUE;
    tls_param_set(TLS_PARAM_ID_IP, &ip_param, FALSE);

#if TLS_CONFIG_WPS
    tls_wifi_set_oneshot_flag(0);
    tls_netif_add_status_event(demo_wps_netif_stataus_callbak);/*register net status event*/
    ret = tls_wps_start_pin();
#endif
    if(ret == WM_SUCCESS)
        printf("Start WPS pin mode ... \n");

    return WM_SUCCESS;
}

int demo_wps_get_pin(char *buf)
{
#if TLS_CONFIG_WPS
    u8 pin[WPS_PIN_LEN + 1];

    if(!tls_wps_get_pin(pin))
        printf("Pin code: %s\n", pin);

    if(!tls_wps_set_pin(pin, WPS_PIN_LEN))
        printf("Pin set correctly: %s\n", pin);
#endif

    return WM_SUCCESS;
}

#endif


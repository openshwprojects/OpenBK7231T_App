/*****************************************************************************
*
* File Name : wm_apsta_demo.c
*
* Description: apsta demo function
*
* Copyright (c) 2015 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : LiLimin
*
* Date : 2015-3-24
*****************************************************************************/
#include <string.h>
#include "wm_include.h"
#include "lwip/netif.h"
#include "wm_netif.h"
#include "wm_demo.h"
#include "wm_sockets.h"

#if DEMO_APSTA

#define         APSTA_DEMO_TASK_PRIO             38
#define         APSTA_DEMO_TASK_SIZE             256
#define         APSTA_DEMO_QUEUE_SIZE            4

static bool ApstaDemoIsInit = false;
static OS_STK   ApstaDemoTaskStk[APSTA_DEMO_TASK_SIZE];
static tls_os_queue_t *ApstaDemoTaskQueue = NULL;

#define         APSTA_DEMO_CMD_SOFTAP_CREATE        0x0
#define         APSTA_DEMO_CMD_STA_JOIN_NET         0x1
#define         APSTA_DEMO_CMD_SOFTAP_CLOSE         0x2

#define         APSTA_DEMO_CMD_SOCKET_DEMO          0x3

#define         APSTA_DEMO_SOCKET_DEMO_REMOTE_PORT  65530
#define         APSTA_DEMO_SOCKET_DEMO_LOCAL_PORT   65531

extern u8 *wpa_supplicant_get_mac(void);
extern u8 *hostapd_get_mac(void);


static char apsta_demo_ssid[33];
static char apsta_demo_pwd[65];
static char apsta_demo_apssid[33];
static char apsta_demo_appwd[65];

static void apsta_demo_client_event(u8 *mac, enum tls_wifi_client_event_type event)
{
    wm_printf("client %M is %s\r\n", mac, event ? "offline" : "online");
}

static void apsta_demo_net_status(u8 status)
{
    struct netif *netif = tls_get_netif();

    switch(status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        wm_printf("sta join net failed\n");
        break;
    case NETIF_WIFI_DISCONNECTED:
        wm_printf("sta net disconnected\n");
        tls_os_queue_send(ApstaDemoTaskQueue, (void *)APSTA_DEMO_CMD_SOFTAP_CLOSE, 0);
        break;
    case NETIF_IP_NET_UP:
        wm_printf("\nsta ip: %v\n", netif->ip_addr.addr);
        tls_os_queue_send(ApstaDemoTaskQueue, (void *)APSTA_DEMO_CMD_SOFTAP_CREATE, 0);
        break;
    case NETIF_WIFI_SOFTAP_FAILED:
        wm_printf("softap create failed\n");
        break;
    case NETIF_WIFI_SOFTAP_CLOSED:
        wm_printf("softap closed\n");
        break;
    case NETIF_IP_NET2_UP:
        wm_printf("\nsoftap ip: %v\n", netif->next->ip_addr.addr);
        break;
    default:
        break;
    }
}

static int soft_ap_demo(char *apssid, char *appwd)
{
    struct tls_softap_info_t apinfo;
    struct tls_ip_info_t ipinfo;
    u8 ret = 0;

    memset(&apinfo, 0, sizeof(apinfo));
    memset(&ipinfo, 0, sizeof(ipinfo));

    u8 *ssid = (u8 *)"w600_apsta_demo";
    u8 ssid_len = strlen("w600_apsta_demo");

    if (apssid)
    {
        ssid_len = strlen(apssid);
        MEMCPY(apinfo.ssid, apssid, ssid_len);
        apinfo.ssid[ssid_len] = '\0';
    }
    else
    {
        MEMCPY(apinfo.ssid, ssid, ssid_len);
        apinfo.ssid[ssid_len] = '\0';
    }

    apinfo.encrypt = strlen(appwd) ? IEEE80211_ENCRYT_CCMP_WPA2 : IEEE80211_ENCRYT_NONE;
    apinfo.channel = 11; /*channel*/
    apinfo.keyinfo.format = 1; /*format:0,hex, 1,ascii*/
    apinfo.keyinfo.index = 1;  /*wep index*/
    apinfo.keyinfo.key_len = strlen(appwd); /*key length*/
    MEMCPY(apinfo.keyinfo.key, appwd, strlen(appwd));

    /*ip information:ip address,mask, DNS name*/
    ipinfo.ip_addr[0] = 192;
    ipinfo.ip_addr[1] = 168;
    ipinfo.ip_addr[2] = 8;
    ipinfo.ip_addr[3] = 1;
    ipinfo.netmask[0] = 255;
    ipinfo.netmask[1] = 255;
    ipinfo.netmask[2] = 255;
    ipinfo.netmask[3] = 0;
    MEMCPY(ipinfo.dnsname, "local.wm", sizeof("local.wm"));

    ret = tls_wifi_softap_create((struct tls_softap_info_t * )&apinfo, (struct tls_ip_info_t * )&ipinfo);
    wm_printf("\nap create %s ! \n", (ret == WM_SUCCESS) ? "Successfully" : "Error");

    return ret;
}

static int connect_wifi_demo(char *ssid, char *pwd)
{
    int ret;

    ret = tls_wifi_connect((u8 *)ssid, strlen(ssid), (u8 *)pwd, strlen(pwd));
    if (WM_SUCCESS == ret)
        wm_printf("\nplease wait connect net......\n");
    else
        wm_printf("\napsta connect net failed, please check configure......\n");

    return ret;
}

static void init_wifi_config(void)
{
    u8 ssid_set = 0;
    u8 wireless_protocol = 0;
    struct tls_param_ip *ip_param = NULL;

    tls_wifi_disconnect();

    tls_wifi_softap_destroy();

    tls_wifi_set_oneshot_flag(0);

    tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void *) &wireless_protocol, TRUE);
    if (TLS_PARAM_IEEE80211_INFRA != wireless_protocol)
    {
        wireless_protocol = TLS_PARAM_IEEE80211_INFRA;
        tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void *) &wireless_protocol, FALSE);
    }

    tls_param_get(TLS_PARAM_ID_BRDSSID, (void *)&ssid_set, (bool)0);
    if (0 == ssid_set)
    {
        ssid_set = 1;
        tls_param_set(TLS_PARAM_ID_BRDSSID, (void *)&ssid_set, (bool)1); /*set flag to broadcast BSSID*/
    }

    ip_param = tls_mem_alloc(sizeof(struct tls_param_ip));
    if (ip_param)
    {
        tls_param_get(TLS_PARAM_ID_IP, ip_param, FALSE);
        ip_param->dhcp_enable = TRUE;
        tls_param_set(TLS_PARAM_ID_IP, ip_param, FALSE);

        tls_param_get(TLS_PARAM_ID_SOFTAP_IP, ip_param, FALSE);
        ip_param->dhcp_enable = TRUE;
        tls_param_set(TLS_PARAM_ID_SOFTAP_IP, ip_param, FALSE);

        tls_mem_free(ip_param);
    }

    tls_netif_add_status_event(apsta_demo_net_status);

    tls_wifi_softap_client_event_register(apsta_demo_client_event);
}

static void apsta_demo_socket_demo(void)
{
    int i;
    int ret;
    int skt;
    u8  *mac;
    u8  *mac2;
    struct netif *netif;
    struct sockaddr_in addr;

    netif = tls_get_netif();

    /*broadcast message to current AP tha STA is connected*/
    printf("broadcast send mac in sta's bbs...\n");
    skt = socket(AF_INET, SOCK_DGRAM, 0);
    if (skt < 0)
        return;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip_addr_get_ip4_u32(&netif->ip_addr);
    addr.sin_port = htons(APSTA_DEMO_SOCKET_DEMO_LOCAL_PORT);

    ret = bind(skt, (struct sockaddr *)&addr, sizeof(addr));
    if (0 != ret)
    {
        close(skt);
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(APSTA_DEMO_SOCKET_DEMO_REMOTE_PORT);

    mac   = wpa_supplicant_get_mac();

    for (i = 0; i < 60; i++)
    {
        sendto(skt, mac, ETH_ALEN, 0, (struct sockaddr *)&addr, sizeof(addr));
        tls_os_time_delay(HZ);
    }

    close(skt);


	/*broadcast message at current soft AP*/
    printf("broadcast send mac in softap's bbs...\n");
    skt = socket(AF_INET, SOCK_DGRAM, 0);
    if (skt < 0)
        return;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip_addr_get_ip4_u32(&netif->next->ip_addr);
    addr.sin_port = htons(APSTA_DEMO_SOCKET_DEMO_LOCAL_PORT);

    ret = bind(skt, (struct sockaddr *)&addr, sizeof(addr));
    if (0 != ret)
    {
        close(skt);
        return;
    }

    ret = setsockopt(skt, IPPROTO_IP, IP_MULTICAST_IF, &addr.sin_addr, sizeof(struct in_addr));
    if(0 != ret)
    {
        close(skt);
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(APSTA_DEMO_SOCKET_DEMO_REMOTE_PORT);

    mac2 = hostapd_get_mac();

    for (i = 0; i < 60; i++)
    {
        sendto(skt, mac2, ETH_ALEN, 0, (struct sockaddr *)&addr, sizeof(addr));
        tls_os_time_delay(HZ);
    }

    close(skt);

    printf("apsta socket sent end.\n");

    return;
}

static void apsta_demo_task(void *p)
{
    int ret;
    void *msg;

    for( ; ; )
    {
        ret = tls_os_queue_receive(ApstaDemoTaskQueue, (void **)&msg, 0, 0);
        if (!ret)
        {
            switch((u32)msg)
            {
            case APSTA_DEMO_CMD_STA_JOIN_NET:
                connect_wifi_demo(apsta_demo_ssid, apsta_demo_pwd);
                break;
            case APSTA_DEMO_CMD_SOFTAP_CREATE:
                soft_ap_demo(apsta_demo_apssid, apsta_demo_appwd);
                break;
            case APSTA_DEMO_CMD_SOFTAP_CLOSE:
                tls_wifi_softap_destroy();
                break;
            case APSTA_DEMO_CMD_SOCKET_DEMO:
                apsta_demo_socket_demo();
                break;
            default:
                break;
            }
        }
    }
}


//apsta demo
//Command as:t-apsta("ssid","pwd", "apsta", "appwd");
int apsta_demo(char *ssid, char *pwd, char *apssid, char *appwd)
{
    memset(apsta_demo_ssid, 0, sizeof(apsta_demo_ssid));
    memset(apsta_demo_pwd, 0, sizeof(apsta_demo_pwd));

    strcpy(apsta_demo_ssid, ssid);
    wm_printf("\nsta_ssid=%s\n", apsta_demo_ssid);

    strcpy(apsta_demo_pwd, pwd);
    wm_printf("\nsta_password=%s\n", apsta_demo_pwd);

    strcpy(apsta_demo_apssid, apssid);
    wm_printf("\nap_ssid=%s\n", apsta_demo_apssid);

    strcpy(apsta_demo_appwd, appwd);
    wm_printf("\nap_password=%s\n", apsta_demo_appwd);

    if (!ApstaDemoIsInit)
    {
        init_wifi_config();

        tls_os_task_create(NULL, NULL, apsta_demo_task,
                           (void *)0, (void *)ApstaDemoTaskStk,/* task's stack start address */
                           APSTA_DEMO_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                           APSTA_DEMO_TASK_PRIO, 0);

        tls_os_queue_create(&ApstaDemoTaskQueue, APSTA_DEMO_QUEUE_SIZE);

        ApstaDemoIsInit = true;
    }

    tls_os_queue_send(ApstaDemoTaskQueue, (void *)APSTA_DEMO_CMD_STA_JOIN_NET, 0);

    return WM_SUCCESS;
}

int apsta_socket_demo(void)
{
    if (ApstaDemoIsInit)
    {
        tls_os_queue_send(ApstaDemoTaskQueue, (void *)APSTA_DEMO_CMD_SOCKET_DEMO, 0);
    }
    else
    {
        printf("please run the apsta demo first.\r\n");
    }

    return 0;
}

#endif

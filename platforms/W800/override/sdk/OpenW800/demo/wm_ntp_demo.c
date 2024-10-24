/**
 * @file    wm_ntp_demo.c
 *
 * @brief   ntp demo function
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include "wm_include.h"
#include <string.h>
#include <time.h>
#include "wm_rtc.h"
#include "wm_ntp.h"
#include "wm_demo.h"

#if DEMO_NTP
extern const char DEMO_SET_NTP_S[];

static int isNetworkOk(void)
{
    struct tls_ethif *etherIf = tls_netif_get_ethif();

    return etherIf->status;
}

static void setAutoConnectMode(void)
{
    u8 auto_reconnect = 0xff;

    tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET, &auto_reconnect);
    if(auto_reconnect != WIFI_AUTO_CNT_ON)
    {
        auto_reconnect = WIFI_AUTO_CNT_ON;
        tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET, &auto_reconnect);
    }
}

//You should config ssid and pwd first before run ntp_demo.
int ntp_demo(void)
{
    unsigned int t;	//used to save time relative to 1970
    struct tm *tblock;

    setAutoConnectMode();
    while(1 != isNetworkOk())
    {
        tls_os_time_delay(HZ);
        printf("waiting for wifi connected......\n");
    }

    t = tls_ntp_client();

    printf("now Time :   %s\n", ctime((const time_t *)&t));
    tblock = localtime((const time_t *)&t);	//switch to local time
    //printf(" sec=%d,min=%d,hour=%d,mon=%d,year=%d\n",tblock->tm_sec,tblock->tm_min,tblock->tm_hour,tblock->tm_mon,tblock->tm_year);
    tls_set_rtc(tblock);

    return WM_SUCCESS;
}


int ntp_set_server_demo(char *ipaddr1, char *ipaddr2, char *ipaddr3)
{
    int server_no = 0;
    printf("\n ipaddr1=%x,2=%x,3=%x\n", (u32)ipaddr1, (u32)ipaddr2, (u32)ipaddr3);
    if (ipaddr1)
    {
        tls_ntp_set_server(ipaddr1, server_no++);
        printf("ntp server %d:%s\n", server_no, ipaddr1);
    }
    if (ipaddr2)
    {
        tls_ntp_set_server(ipaddr2, server_no ++);
        printf("ntp server %d:%s\n", server_no, ipaddr2);
    }
    if (ipaddr3)
    {
        tls_ntp_set_server(ipaddr3, server_no ++);
        printf("ntp server %d:%s\n", server_no, ipaddr3);
    }

    return WM_SUCCESS;
}

int ntp_query_cfg(void)
{
    tls_ntp_query_sntpcfg();

    return WM_SUCCESS;
}

#endif

/*****************************************************************************
*
* File Name : wm_scan_demo.c
*
* Description: wifi scan demo function
*
* Copyright (c) 2018 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : muqing
*
* Date : 2018-12-03
*****************************************************************************/


#include <string.h>
#include "wm_include.h"
#include "tls_wireless.h"

#if DEMO_SCAN

static  char *scan_privacy_string(u8 privacy)
{
    char *sec;

    switch (privacy)
    {
    case WM_WIFI_AUTH_MODE_OPEN:
        sec = "NONE";
        break;
    case WM_WIFI_AUTH_MODE_WEP_AUTO:
        sec = "WEP/AUTO";
        break;
    case WM_WIFI_AUTH_MODE_WPA_PSK_TKIP:
        sec = "WPA_PSK/TKIP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_PSK_CCMP:
        sec = "WPA_PSK/CCMP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_PSK_AUTO:
        sec = "WPA_PSK/AUTO";
        break;
    case WM_WIFI_AUTH_MODE_WPA2_PSK_TKIP:
        sec = "WPA2_PSK/TKIP";
        break;
    case WM_WIFI_AUTH_MODE_WPA2_PSK_CCMP:
        sec = "WPA2_PSK/CCMP";
        break;
    case WM_WIFI_AUTH_MODE_WPA2_PSK_AUTO:
        sec = "WPA2_PSK/AUTO";
        break;
    case WM_WIFI_AUTH_MODE_WPA_WPA2_PSK_TKIP:
        sec = "WPA_PSK/WPA2_PSK/TKIP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_WPA2_PSK_CCMP:
        sec = "WPA_PSK/WPA2_PSK/CCMP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_WPA2_PSK_AUTO:
        sec = "WPA_PSK/WPA2_PSK/AUTO";
        break;

    default:
        sec = "Unknown";
        break;
    }
    return sec;
}

static  char *scan_mode_string(u8 mode)
{
    char *ap_mode;

    switch (mode)
    {
    case 1:
        ap_mode = "IBSS";
        break;
    case 2:
        ap_mode = "ESS";
        break;

    default:
        ap_mode = "ESS";
        break;
    }
    return ap_mode;
}


static void wifi_scan_handler(void)
{
    char *buf = NULL;
    char *buf1 = NULL;
    u32 buflen;
    int i, j;
    int err;
    u8 ssid[33];
    struct tls_scan_bss_t *wsr;
    struct tls_bss_info_t *bss_info;

    buflen = 2000;
    buf = tls_mem_alloc(buflen);
    if (!buf)
    {
        goto end;
    }

    buf1 = tls_mem_alloc(300);
    if(!buf1)
    {
        goto end;
    }
    memset(buf1, 0, 300);

    err = tls_wifi_get_scan_rslt((u8 *)buf, buflen);
    if (err)
    {
        goto end;
    }

    wsr = (struct tls_scan_bss_t *)buf;
    bss_info = (struct tls_bss_info_t *)(buf + 8);

    printf("\n");

    for(i = 0; i < wsr->count; i++)
    {
        j = sprintf(buf1, "bssid:%02X%02X%02X%02X%02X%02X, ", bss_info->bssid[0], bss_info->bssid[1],
                    bss_info->bssid[2], bss_info->bssid[3], bss_info->bssid[4], bss_info->bssid[5]);
        j += sprintf(buf1 + j, "ch:%d, ", bss_info->channel);
        j += sprintf(buf1 + j, "rssi:%d, ", (signed char)bss_info->rssi);
        j += sprintf(buf1 + j, "wps:%d, ", bss_info->wps_support);
        j += sprintf(buf1 + j, "max_rate:%dMbps, ", bss_info->max_data_rate);
        j += sprintf(buf1 + j, "%s, ", scan_mode_string(bss_info->mode));
        j += sprintf(buf1 + j, "%s, ", scan_privacy_string(bss_info->privacy));
        memcpy(ssid, bss_info->ssid, bss_info->ssid_len);
        ssid[bss_info->ssid_len] = '\0';
        j += sprintf(buf1 + j, "%s", ssid);

        printf("%s\n", buf1);

        bss_info ++;
    }

end:
    if(buf)
    {
        tls_mem_free(buf);
    }
    if(buf1)
    {
        tls_mem_free(buf1);
    }
}
/*Scan demo*/
int scan_demo(void)
{
    tls_wifi_scan_result_cb_register(wifi_scan_handler);
    tls_wifi_scan();
    return WM_SUCCESS;
}

#endif


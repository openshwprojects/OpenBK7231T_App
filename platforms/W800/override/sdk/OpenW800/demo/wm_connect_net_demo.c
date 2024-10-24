#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_wifi_oneshot.h"

#if DEMO_CONNECT_NET
static void con_net_status_changed_event(u8 status )
{
    switch(status)
    {
    case NETIF_WIFI_JOIN_SUCCESS:
        printf("NETIF_WIFI_JOIN_SUCCESS\n");
        break;
    case NETIF_WIFI_JOIN_FAILED:
        printf("NETIF_WIFI_JOIN_FAILED\n");
        break;
    case NETIF_WIFI_DISCONNECTED:
        printf("NETIF_WIFI_DISCONNECTED\n");
        break;
    case NETIF_IP_NET_UP:
    {
        struct tls_ethif *tmpethif = tls_netif_get_ethif();
        print_ipaddr(&tmpethif->ip_addr);
#if TLS_CONFIG_IPV6
        print_ipaddr(&tmpethif->ip6_addr[0]);
        print_ipaddr(&tmpethif->ip6_addr[1]);
        print_ipaddr(&tmpethif->ip6_addr[2]);
#endif
    }
    break;
    default:
        //printf("UNKONWN STATE:%d\n", status);
        break;
    }
}

int demo_oneshot(void)
{
    printf("waiting for oneshot \n");
    tls_netif_add_status_event(con_net_status_changed_event);
    tls_wifi_set_oneshot_flag(1);
    return 0;
}

int demo_socket_config(void)
{
    printf("AP Mode socket config demo \n");
    tls_netif_add_status_event(con_net_status_changed_event);
    tls_wifi_set_oneshot_flag(2);
    return 0;
}

int demo_webserver_config(void)
{
    printf("AP Mode web server config mode \n");
    tls_netif_add_status_event(con_net_status_changed_event);
    tls_wifi_set_oneshot_flag(3);
    return 0;
}

int demo_ble_config(void)
{
    printf("BLE config mode \n");
#if TLS_CONFIG_BT
	extern int demo_bt_enable();
    tls_netif_add_status_event(con_net_status_changed_event);
	demo_bt_enable();
	tls_os_time_delay(HZ/2);
	tls_wifi_set_oneshot_flag(4);	
#endif
    return 0;
}



//acitve connect to specified AP, use command as: t-connet("ssid","pwd");
int demo_connect_net(char *ssid, char *pwd)
{
    struct tls_param_ip *ip_param = NULL;
    u8 wireless_protocol = 0;

    if (!ssid)
    {
        return WM_FAILED;
    }

    printf("\nssid:%s\n", ssid);
    printf("password=%s\n", pwd);
    tls_wifi_disconnect();

    tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void *) &wireless_protocol, TRUE);
    if (TLS_PARAM_IEEE80211_INFRA != wireless_protocol)
    {
        tls_wifi_softap_destroy();
        wireless_protocol = TLS_PARAM_IEEE80211_INFRA;
        tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void *) &wireless_protocol, FALSE);
    }

    tls_wifi_set_oneshot_flag(0);

    ip_param = tls_mem_alloc(sizeof(struct tls_param_ip));
    if (ip_param)
    {
        tls_param_get(TLS_PARAM_ID_IP, ip_param, FALSE);
        ip_param->dhcp_enable = TRUE;
        tls_param_set(TLS_PARAM_ID_IP, ip_param, FALSE);
        tls_mem_free(ip_param);
    }

    tls_netif_add_status_event(con_net_status_changed_event);
    tls_wifi_connect((u8 *)ssid, strlen(ssid), (u8 *)pwd, strlen(pwd));
    printf("\nplease wait connect net......\n");

    return WM_SUCCESS;
}
#endif


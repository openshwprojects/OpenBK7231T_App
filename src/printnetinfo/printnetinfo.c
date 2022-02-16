

#include "../new_common.h"
#include "str_pub.h"
#include "wlan_ui_pub.h"
#include "net.h"
#include "../logging/logging.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x "

char g_IP[3+3+3+3+1+1+1] = "unknown";
char *getMyIp(){
    IPStatusTypedef ipStatus;

    os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
    bk_wlan_get_ip_status(&ipStatus, STATION);
    
    strcpy(g_IP, ipStatus.ip);
    return g_IP;
}

////////////////////
// NOTE: this gets the STA mac
void getMAC(unsigned char *mac){
    net_get_if_macaddr(mac, net_get_sta_handle());
}

char *getMACStr(char *macstr){
    unsigned char mac[6];
    getMAC(mac);
    sprintf(macstr, MACSTR, MAC2STR(mac));
    return macstr;
}

void print_network_info(){
    IPStatusTypedef ipStatus;

    os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
    bk_wlan_get_ip_status(&ipStatus, STATION);
    
    char *fmt = "dhcp=%d ip=%s gate=%s mask=%s mac=" MACSTR "\r\n";
    addLog(fmt ,
        ipStatus.dhcp, ipStatus.ip, ipStatus.gate, 
        ipStatus.mask, MAC2STR((unsigned char*)ipStatus.mac));

    // print wifi state
    LinkStatusTypeDef linkStatus;
    network_InitTypeDef_ap_st ap_info;
    char ssid[33] = {0};
    #if CFG_IEEE80211N
        bk_printf("sta: %d, softap: %d, b/g/n\r\n",sta_ip_is_start(),uap_ip_is_start());
    #else
        bk_printf("sta: %d, softap: %d, b/g\r\n",sta_ip_is_start(),uap_ip_is_start());
    #endif
    
    if( sta_ip_is_start() )
    {
    os_memset(&linkStatus, 0x0, sizeof(LinkStatusTypeDef));
    bk_wlan_get_link_status(&linkStatus);
        os_memcpy(ssid, linkStatus.ssid, 32);

    char *fmt = "sta:rssi=%d,ssid=%s,bssid=" MACSTR ",channel=%d,cipher_type:";
    bk_printf(fmt,
        linkStatus.wifi_strength, ssid, MAC2STR(linkStatus.bssid), linkStatus.channel);
        switch(bk_sta_cipher_type())
        {
        case SECURITY_TYPE_NONE:
                bk_printf("OPEN\r\n");
                break;
            case SECURITY_TYPE_WEP :
                bk_printf("WEP\r\n");
                break;
            case SECURITY_TYPE_WPA_TKIP:
                bk_printf("TKIP\r\n");
                break;
            case SECURITY_TYPE_WPA2_AES:
                bk_printf("CCMP\r\n");
                break;
            case SECURITY_TYPE_WPA2_MIXED:
                bk_printf("MIXED\r\n");
                break;
            case SECURITY_TYPE_AUTO:
                bk_printf("AUTO\r\n");
                break;
            default:
                bk_printf("Error\r\n");
                break;
        }
    }
    
    if( uap_ip_is_start() )
    {
        os_memset(&ap_info, 0x0, sizeof(network_InitTypeDef_ap_st));
        bk_wlan_ap_para_info_get(&ap_info);
        os_memcpy(ssid, ap_info.wifi_ssid, 32);
        bk_printf("softap:ssid=%s,channel=%d,dhcp=%d,cipher_type:",
        ssid, ap_info.channel,ap_info.dhcp_mode);
        switch(ap_info.security)
        {
        case SECURITY_TYPE_NONE:
                bk_printf("OPEN\r\n");
                break;
            case SECURITY_TYPE_WEP :
                bk_printf("WEP\r\n");
                break;
            case SECURITY_TYPE_WPA_TKIP:
                bk_printf("TKIP\r\n");
                break;
            case SECURITY_TYPE_WPA2_AES:
                bk_printf("CCMP\r\n");
                break;
            case SECURITY_TYPE_WPA2_MIXED:
                bk_printf("MIXED\r\n");
                break;
            case SECURITY_TYPE_AUTO:
                bk_printf("AUTO\r\n");
                break;
            default:
                bk_printf("Error\r\n");
                break;
        }
        addLog("ip=%s,gate=%s,mask=%s,dns=%s\r\n",
            ap_info.local_ip_addr, 
            ap_info.gateway_ip_addr, 
            ap_info.net_mask, 
            ap_info.dns_server_ip_addr);
    }

}
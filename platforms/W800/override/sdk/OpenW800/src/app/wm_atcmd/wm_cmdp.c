/**************************************************************************
 * File Name                   : wm_cmdp.c
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 * All rights reserved.
 *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "wm_config.h"
#include "wm_cmdp.h"
#if (GCC_COMPILE==1)
#include "wm_cmdp_hostif_gcc.h"
#else
#include "wm_cmdp_hostif.h"
#endif
#include "wm_mem.h"
#include "wm_params.h"
//#include "wm_param.h"
#include "wm_debug.h"
#include "wm_uart.h"
#include "wm_internal_flash.h"
#include "wm_netif.h"
#include "utils.h"
#include "wm_watchdog.h"
#include "wm_wifi.h"
#include "wm_sockets.h"
#include "lwip/netif.h"
#include "wm_efuse.h"
#include "wm_dhcp_server.h"
#include "wm_wifi_oneshot.h"
#include "wm_ram_config.h"
#include "wm_pmu.h"

extern const char FirmWareVer[];
extern const char HwVer[];
tls_os_timer_t *RSTTIMER = NULL;
tls_os_timer_t *chippowersavetimer = NULL;
typedef struct chip_lowpower_st{
	u32 lowpowertype;
	u32 lowpowertime;
	u32 wakesource;	
}chip_lowerpower_struct;

chip_lowerpower_struct st_chiplowpower;


u8 gfwupdatemode = 0;
u8 tls_get_fwup_mode(void){
	return gfwupdatemode;
}

int tls_cmd_get_ver( struct tls_cmd_ver_t *ver)
{
	MEMCPY(ver->hw_ver, HwVer, 6);
	MEMCPY(ver->fw_ver, FirmWareVer, 4);
	if(tls_get_fwup_mode())
	{
		ver->fw_ver[0] = 'B';
	}
	return 0;
}

int tls_cmd_get_hw_ver(u8 *hwver)
{
	if (hwver){
//		tls_get_hw_version(hwver);
	}
	return 0;
}


int tls_cmd_set_hw_ver(u8 *hwver)
{
	if (hwver){
//		tls_set_hw_version(hwver);
	}

	return 0;
}


#if TLS_CONFIG_HOSTIF

struct tls_socket_cfg  socket_cfg;
static u8 net_up;
cmd_set_uart0_mode_callback set_uart0_mode_callback;
cmd_get_uart1_port_callback get_uart1_port_callback;
cmd_set_uart1_mode_callback set_uart1_mode_callback;
cmd_set_uart1_sock_param_callback set_uart1_sock_param_callback;
extern struct tls_wif * tls_get_wif_data(void);

void tls_set_fwup_mode(u8 flag){
	gfwupdatemode = flag;
}

u8 tls_cmd_get_auto_mode(void)
{
    u8 auto_mode_set;
    tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode_set, FALSE);
    return auto_mode_set;
}
struct tls_socket_cfg *tls_cmd_get_socket_cfg(void)
{
    return &socket_cfg;
}
void tls_cmd_set_net_up(u8 netup){
	net_up = netup;
}

u8 tls_cmd_get_net_up(void){
	return net_up;
}
void tls_cmd_init_socket_cfg(void)
{
    int timeout = 0, host_len;
    struct tls_param_socket remote_socket_cfg;
    tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
    /* read default socket params */
    socket_cfg.proto = remote_socket_cfg.protocol;
    socket_cfg.client = remote_socket_cfg.client_or_server ? 0 : 1;
    socket_cfg.port = remote_socket_cfg.port_num;
	TLS_DBGPRT_INFO("socket_cfg.proto = %d, socket_cfg.client = %d, socket_cfg.port = %d\n", socket_cfg.proto, socket_cfg.client, socket_cfg.port);
    host_len = strlen((char *)remote_socket_cfg.host);
    if (socket_cfg.client) {
        /*  host name */
		if (host_len){
	        MEMCPY(socket_cfg.host,
	                remote_socket_cfg.host, host_len);
			string_to_ipaddr((char *)remote_socket_cfg.host, &socket_cfg.ip_addr[0]);
		}
    } else if (!socket_cfg.client && socket_cfg.proto == 0) {
	    if (strtodec(&timeout, (char *)remote_socket_cfg.host)<0){
			timeout = 0;
	    }

        socket_cfg.timeout = timeout;
    } else
        ;
}

#if 0
int hostif_cipher2host(int cipher, int proto)
{
    switch (cipher) {
        case WPA_CIPHER_NONE:
            return 0;
        case WPA_CIPHER_WEP40:
            return 1;
        case WPA_CIPHER_TKIP:
            if (proto == WPA_PROTO_WPA)
                return 3;
            else
                return 5;
        case WPA_CIPHER_CCMP:
            if (proto == WPA_PROTO_WPA)
                return 4;
            else
                return 6;
        case WPA_CIPHER_WEP104:
            return 2;
		case WPA_CIPHER_TKIP|WPA_CIPHER_CCMP:
			if (proto == WPA_PROTO_WPA){
				return 7;
			}else{
				return 8;
			}
        default:
            return 0;
    }
}
#endif

static void ResetTimerProc(void *ptmr, void *parg)
{
	tls_sys_set_reboot_reason(REBOOT_REASON_ACTIVE);
    tls_sys_reset();
}

void tls_cmd_reset_sys(void)
{
    int err=0;
	//if(0 == tls_get_fwup_mode())
	{
		if(RSTTIMER == NULL)
		{
			err = tls_os_timer_create(&RSTTIMER,
								ResetTimerProc,
								NULL,
								HZ/10,
								FALSE,
								NULL);
			if(TLS_OS_SUCCESS == err)
			{
				tls_os_timer_start(RSTTIMER);
			}
		}
	}
}

int tls_cmd_pmtf(void)
{
    int err;
    err = tls_param_to_flash(TLS_PARAM_ID_ALL);
    return err;
}

int tls_cmd_reset_flash(void)
{
    int err;

	err = tls_param_to_default();
	#if 0
	struct tls_sys_param cfg_param;
    tls_param_load_factory_default();
    err = tls_param_set(TLS_PARAM_ID_ALL, &cfg_param, 1);
	#endif
    return err;
}

static void delay_enter_chip_lowpower_timeout(void *ptmr, void *parg)
{
    chip_lowerpower_struct *stchiplp = (chip_lowerpower_struct *)parg;
    if (stchiplp->wakesource == 1)
    {
		tls_pmu_timer0_start(stchiplp->lowpowertime/1000);
    }

	switch(stchiplp->lowpowertype)
	{
		case 1:
			tls_pmu_standby_start();
			break;
		case 2:
	    	tls_pmu_sleep_start();
			break;
		default:
			break;
	}
}


void tls_cmd_chip_low_power_function(u32 lowpowertype, u32 wakesource, u32 delaytime, u32 lowpowertime)
{
	int err;

	st_chiplowpower.lowpowertype = lowpowertype;
	st_chiplowpower.wakesource = wakesource;
	st_chiplowpower.lowpowertime = lowpowertime;	

	if(chippowersavetimer == NULL)
	{
		err = tls_os_timer_create(&chippowersavetimer,
							delay_enter_chip_lowpower_timeout,
							&st_chiplowpower,
							delaytime/2,
							FALSE,
							NULL);
		if(TLS_OS_SUCCESS == err)
		{
			tls_os_timer_start(chippowersavetimer);
		}
	}
	else
	{
		tls_os_timer_change(chippowersavetimer,delaytime);
	}
}


int tls_cmd_ps( struct tls_cmd_ps_t *ps)
{
/* TODO: not just close wifi rx&tx,
*	here should mean the whole CPU sleep
*/
#if 1
    if (ps->ps_type > 2 || ps->wake_type > 1 ) {
        return CMD_ERR_INV_PARAMS;
    }

    if ((ps->ps_type == 1)||((ps->ps_type == 2) && (ps->wake_type == 1))) {
        if (((ps->delay_time < 100) || (ps->delay_time > 10000)) ||
            (ps->wake_time < 1000)) {
            return CMD_ERR_INV_PARAMS;
        }
    }

    if (ps->ps_type == 0) {
        if (tls_cmd_get_auto_mode() /*|| wif->priv->scanning*/
		|| tls_wifi_get_oneshot_flag()) {
            return CMD_ERR_NOT_ALLOW;
        }

        if (ps->wake_type == 0) {
            /* wake up */
            tls_wl_if_ps(1);
        } else {
            /* enter sleep */
            tls_wl_if_ps(0);
        }
    } else if (ps->ps_type == 1){/*standby*/
		tls_cmd_chip_low_power_function(ps->ps_type, ps->wake_type, ps->delay_time,
                ps->wake_time);
    }else if (ps->ps_type == 2){/*sleep*/
        tls_cmd_chip_low_power_function(ps->ps_type, ps->wake_type, ps->delay_time,
                ps->wake_time);
    }
#endif
    return CMD_ERR_OK;
}

int tls_cmd_scan( enum tls_cmd_mode mode)
{

    int ret=0;
    struct tls_hostif *hif = tls_get_hostif();

    /* scanning not finished */
    if (hif->last_scan )
        return CMD_ERR_BUSY;

    hif->last_scan = 1;
    hif->last_scan_cmd_mode = mode;
    
    /* register scan complt callback*/
    tls_wifi_scan_result_cb_register(hostif_wscan_cmplt);
    
    /* trigger the scan */
    ret = tls_wifi_scan();
    if(ret == WM_WIFI_SCANNING_BUSY)
    {
        hif->last_scan = 0;
        return CMD_ERR_BUSY;
    }
    else if(ret == WM_FAILED)
    {
        hif->last_scan = 0;
        return CMD_ERR_MEM;
    }

    return CMD_ERR_OK;
}

int tls_cmd_scan_by_param( enum tls_cmd_mode mode, u16 channellist, u32 times, u16 switchinterval, u16 scantype)
{

    int ret=0;
    struct tls_hostif *hif = tls_get_hostif();
    struct tls_wifi_scan_param_t scan_param;

    /* scanning not finished */
    if (hif->last_scan )
        return CMD_ERR_BUSY;

    hif->last_scan = 1;
    hif->last_scan_cmd_mode = mode;
    
    /* register scan complt callback*/
    tls_wifi_scan_result_cb_register(hostif_wscan_cmplt);
    
    /* trigger the scan */
	scan_param.scan_type = scantype;
    scan_param.scan_chanlist = channellist;
    scan_param.scan_chinterval = switchinterval;
    scan_param.scan_times = times;
    ret = tls_wifi_scan_by_param(&scan_param);
    if(ret == WM_WIFI_SCANNING_BUSY)
    {
    	tls_wifi_scan_result_cb_register(NULL);
        hif->last_scan = 0;
        return CMD_ERR_BUSY;
    }
    else if(ret == WM_FAILED)
    {
    	tls_wifi_scan_result_cb_register(NULL);
        hif->last_scan = 0;
        return CMD_ERR_MEM;
    }

    return CMD_ERR_OK;
}


int tls_cmd_join_net(void)
{
	struct tls_cmd_ssid_t ssid;
	struct tls_cmd_key_t  *key;
	struct tls_cmd_bssid_t bssid;
	int ret;

	key = tls_mem_alloc(sizeof(struct tls_cmd_key_t));
	if(!key)
		return -1;
	memset(key, 0, sizeof(struct tls_cmd_key_t));

	tls_cmd_get_bssid(&bssid);
	tls_cmd_get_ssid(&ssid);
	tls_cmd_get_key(key);	

	if(bssid.enable){
		if (ssid.ssid_len)
		{
			ret = tls_wifi_connect_by_ssid_bssid(ssid.ssid, ssid.ssid_len,bssid.bssid, key->key, key->key_len);
		}
		else
		{
			ret = tls_wifi_connect_by_bssid(bssid.bssid, key->key, key->key_len);
		}
	}else{
		ret = tls_wifi_connect(ssid.ssid, ssid.ssid_len, key->key, key->key_len);
	}

	tls_mem_free(key);
	return ret;

}

int tls_cmd_create_net( void )
{
    int ret = CMD_ERR_UNSUPP;
#if TLS_CONFIG_AP
	struct tls_softap_info_t* apinfo;
	struct tls_ip_info_t* ipinfo;
	struct tls_cmd_ssid_t ssid;
	struct tls_cmd_ip_params_t ip_addr;

	apinfo = tls_mem_alloc(sizeof(struct tls_softap_info_t));
	if(apinfo == NULL)
		return CMD_ERR_MEM;
	ipinfo = tls_mem_alloc(sizeof(struct tls_ip_info_t));
	if(ipinfo == NULL){
		tls_mem_free(apinfo);
		return CMD_ERR_MEM;
	}

	tls_cmd_get_softap_ssid(&ssid);
	MEMCPY(apinfo->ssid, ssid.ssid, ssid.ssid_len);
	apinfo->ssid[ssid.ssid_len] = '\0';

	tls_cmd_get_softap_encrypt( &apinfo->encrypt);

	tls_cmd_get_softap_channel( &apinfo->channel);

	tls_cmd_get_softap_key((struct tls_cmd_key_t *)(&apinfo->keyinfo));

	tls_cmd_get_softap_ip_info(&ip_addr);

	MEMCPY(ipinfo->ip_addr, ip_addr.ip_addr, 4);
	MEMCPY(ipinfo->netmask, ip_addr.netmask, 4);
	tls_cmd_get_dnsname( ipinfo->dnsname);

	ret = tls_wifi_softap_create(apinfo, ipinfo);

	tls_mem_free(apinfo);
	tls_mem_free(ipinfo);
#endif

	return ret;
}

int tls_cmd_create_ibss_net( void )
{
	int ret=CMD_ERR_UNSUPP;
	return ret;
}


int tls_cmd_join( enum tls_cmd_mode mode,
        struct tls_cmd_connect_t *conn)
{
    int ret = -1;
    u8 wifi_mode;
	struct tls_hostif *hif = tls_get_hostif();
	u8 auto_reconnect = WIFI_AUTO_CNT_OFF;
    static u8 last_wifi_mode = 0xFF;

    /* supplicant is connect a network */
    if (hif->last_join)
    return CMD_ERR_BUSY;

	hif->last_join_cmd_mode = mode;
	hif->last_join = 1;
	/*restore WiFi auto reconnect Tmp OFF*/
	tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET, &auto_reconnect);
	if(auto_reconnect == WIFI_AUTO_CNT_TMP_OFF){
		auto_reconnect = WIFI_AUTO_CNT_ON;
		tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET, &auto_reconnect);
	} 

	tls_cmd_get_wireless_mode(&wifi_mode);

	tls_wifi_set_oneshot_flag(0);
    switch (wifi_mode) {
        case 0://IEEE80211_MODE_INFRA:
        case 3://IEEE80211_MODE_APSTA:
            if ((last_wifi_mode != 0xFF)&&(last_wifi_mode != wifi_mode))
            {
	           tls_wifi_disconnect();
	           tls_wifi_softap_destroy();
            }  
            ret = tls_cmd_join_net();
            break;
        
        case 1://IEEE80211_MODE_IBSS:
			ret = tls_cmd_create_ibss_net();
			break;
        case 2://IEEE80211_MODE_AP:
            if ((last_wifi_mode != 0xFF)&&(last_wifi_mode != wifi_mode))
            {
                tls_wifi_disconnect();
                tls_wifi_softap_destroy();
            }
            ret = tls_cmd_create_net();
            break;
        default:
            break;
    }
    last_wifi_mode = wifi_mode;
    if (ret)
        hif->last_join = 0;
    return ret;
}

int tls_cmd_disconnect_network(u8 mode)
{
	struct tls_hostif *hif = tls_get_hostif();
	u8 auto_reconnect = 0xff;
	int ret;

    hif->last_join = 0;

	/* notify sys task */
	tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET, &auto_reconnect);
	if(auto_reconnect == WIFI_AUTO_CNT_ON){
		auto_reconnect = WIFI_AUTO_CNT_TMP_OFF;
		ret = tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET, &auto_reconnect);
		if(ret != WM_SUCCESS)
			return ret;
	}
#if TLS_CONFIG_AP
    if (IEEE80211_MODE_AP & mode)
        tls_wifi_softap_destroy();
    if ((~IEEE80211_MODE_AP) & mode)
#endif
	tls_wifi_disconnect();
    return WM_SUCCESS;
}

int tls_cmd_get_link_status(
        struct tls_cmd_link_status_t *lks)
{
    struct tls_ethif *ni;

    ni=tls_netif_get_ethif();

	if (ni == NULL)
	{
		return -1;
	}
	
    if (ni->status)
        lks->status = 1;
    else
        lks->status = 0;
    MEMCPY(lks->ip, (char *)ip_2_ip4(&ni->ip_addr), 4);
    MEMCPY(lks->netmask, (char *)ip_2_ip4(&ni->netmask), 4);
    MEMCPY(lks->gw, (char *)ip_2_ip4(&ni->gw), 4);
    MEMCPY(lks->dns1, (char *)ip_2_ip4(&ni->dns1), 4);
    MEMCPY(lks->dns2, (char *)ip_2_ip4(&ni->dns2), 4);
    return 0;
}

int tls_cmd_wps_start(void)
{
    return -1;
}

int tls_cmd_set_wireless_mode(
        u8 mode, u8 update_flash)
{
    u8 wmode;

    switch (mode) {
        case 0:
            wmode = IEEE80211_MODE_INFRA;
            break;
        case 1:
            wmode = IEEE80211_MODE_IBSS;
            break;
        case 2:
            wmode = IEEE80211_MODE_AP;
            break;
        case 3:
            wmode = IEEE80211_MODE_INFRA | IEEE80211_MODE_AP;
            break;
        default:
            return -1;
    }
    tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void *)&wmode, (bool)update_flash);
    return 0;
}

int tls_cmd_get_wireless_mode(u8 *mode)
{
    int err  = 0;
    u8 wmode = 0;

	tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&wmode, TRUE);
	/*set WPAS_MODE to do*/

	switch (wmode)
	{
        case IEEE80211_MODE_INFRA:
            *mode = 0;
            break;
        case IEEE80211_MODE_IBSS:
            *mode = 1;
            break;
        case IEEE80211_MODE_AP:
            *mode = 2;
            break;
        case (IEEE80211_MODE_INFRA | IEEE80211_MODE_AP):
            *mode = 3;
            break;
        default:
            err  = CMD_ERR_NOT_ALLOW;
            break;
	}
    return err;
}

int tls_cmd_set_ssid(struct tls_cmd_ssid_t *ssid, u8 update_flash)
{
    struct tls_param_ssid params_ssid;

    if (ssid->ssid_len > 32)
        return -1;

    params_ssid.ssid_len = ssid->ssid_len;
    MEMCPY(&params_ssid.ssid, ssid->ssid, ssid->ssid_len);

    tls_param_set(TLS_PARAM_ID_SSID, (void *)&params_ssid, (bool)update_flash);

    return 0;
}

int tls_cmd_get_ssid(struct tls_cmd_ssid_t *ssid)
{
    struct tls_param_ssid params_ssid;

    tls_param_get(TLS_PARAM_ID_SSID, (void *)&params_ssid, 0);
    if (params_ssid.ssid_len > 32){
    	ssid->ssid_len = 0;
	params_ssid.ssid[0] = '\0';
    }else{
	ssid->ssid_len = params_ssid.ssid_len;
	MEMCPY(ssid->ssid, params_ssid.ssid, ssid->ssid_len);
    }
    return 0;
}

#if 0
int tls_set_key_cfg(struct tls_cmd_key_t *key)
{
	struct wpa_supplicant *wpa_s = tls_get_wpa_data();
    struct wpa_ssid *ssid = wpa_s->ssid_conf;
    int i, j;
    u8 c;

    /* check key length */
    if (key->key_len > 64)
        return -1;
    if (key->index > 4)
        return -1;
    /* check hex format */
    if (key->format == 0) {
        for (i = 0; i < key->key_len; i++) {
            c = *(u8 *)(key->key+i);
            if (c > 0xF)
                return -1;
        }
        switch (key->key_len) {
            case 10:
                if (ssid->encrypt != 1 || (key->index < 1) || (key->index > 4))
                    return -1;
                for (i = 0, j=0; j < 5; i+=2,j++) {
                    c = *(u8 *)(key->key + i + 1) | (*(u8 *)(key->key + i) << 4);
                    ssid->wep_key[key->index-1][j] = c;
                }
                ssid->wep_tx_keyidx = (key->index - 1);
                ssid->wep_key_len[key->index-1] = 5;
                break;
            case 26:
                if (ssid->encrypt != 2 || (key->index < 1) || (key->index > 4))
                    return -1;
                for (i = 0,j=0; j < 13; i+=2,j++) {
                    c = *(u8 *)(key->key + i + 1) | (*(u8 *)(key->key + i) << 4);
                    ssid->wep_key[key->index-1][j] = c;
                }
                ssid->wep_tx_keyidx = key->index -1;
                ssid->wep_key_len[key->index-1] = 13;
                break;
            case 64:
                if (ssid->encrypt < 3)
                    return -1;
                for (i = 0,j=0; j < 32; i+=2,j++) {
                    c = *(u8 *)(key->key + i + 1) | (*(u8 *)(key->key + i) << 4);
                    ssid->psk[j] = c;
                    ssid->psk_set = 1;
                }
                if (ssid->passphrase) {
                    tls_mem_free(ssid->passphrase);
                    ssid->passphrase = NULL;
                    ssid->passphrase_len = 0;
                }
                break;
            default:
                return -1;
        }
    } else if (key->format == 1) {
        switch (key->key_len) {
            case 5:
                /* wep40 */
                if (ssid->encrypt != 1 || (key->index < 1) || (key->index > 4))
                    return -1;
                MEMCPY(&ssid->wep_key[key->index-1][0], key->key, key->key_len);
                ssid->wep_tx_keyidx = key->index -1;
                ssid->wep_key_len[key->index-1] = 5;
                break;
            case 13:
                if ((ssid->encrypt == 2) && (key->index >= 1) && (key->index <= 4)) {
                    /* wep104 */
                    MEMCPY(&ssid->wep_key[key->index-1][0], key->key, key->key_len);
                    ssid->wep_tx_keyidx = key->index -1;
                    ssid->wep_key_len[key->index-1] = 13;
                    break;
                } else if (ssid->encrypt > 2) {
                    /* set key in switch-default */
                } else
                    return -1;
            default:
                if ((ssid->encrypt > 2) && (key->key_len < 8))
                    return -1;
                if (ssid->encrypt <= 2)
                    return -1;
                if (key->key_len > 63)
                    return -1;
                /* the key is TKIP or CCMP ASCII */
                if (ssid->passphrase)
                    tls_mem_free(ssid->passphrase);
                ssid->passphrase = tls_mem_alloc(key->key_len + 1);
                if (!ssid->passphrase)
                    return -1;
                MEMCPY(ssid->passphrase, key->key, key->key_len);
                ssid->passphrase[key->key_len] = '\0';
                ssid->psk_set = 0;
                ssid->passphrase_len = key->key_len;
                break;
        }
    } else
        return -1;

    return 0;
}
#endif
int tls_cmd_set_key(struct tls_cmd_key_t *key, u8 update_flash)
{
    struct tls_param_key param_key;
	struct tls_param_original_key* orig_key;
	struct tls_param_sha1* sha1_key;

    MEMCPY(param_key.psk, key->key, 64);
    param_key.key_format = key->format;
    param_key.key_index = key->index;
    param_key.key_length = key->key_len;
    tls_param_set(TLS_PARAM_ID_KEY, (void *)&param_key, (bool)update_flash);


	orig_key = (struct tls_param_original_key*)&param_key;
    MEMCPY(orig_key->psk, key->key, 64);
    orig_key->key_length = key->key_len;
    tls_param_set(TLS_PARAM_ID_ORIGIN_KEY, (void *)orig_key, (bool)update_flash);

	sha1_key = (struct tls_param_sha1*)&param_key;
	memset((u8* )sha1_key, 0, sizeof(struct tls_param_sha1));
	tls_param_set(TLS_PARAM_ID_SHA1, (void *)sha1_key, TRUE);


    return 0;
}

int tls_cmd_get_key(struct tls_cmd_key_t *key)
{
    struct tls_param_key *param_key;
	struct tls_param_original_key* orig_key;

	param_key = tls_mem_alloc(sizeof(struct tls_cmd_key_t));
	if(!param_key)
		return -1;

	orig_key = tls_mem_alloc(sizeof(struct tls_param_original_key));
	if(!orig_key)
	{
		tls_mem_free(param_key);
		return -1;
	}
	
	memset(param_key, 0, sizeof(struct tls_cmd_key_t));
	memset(orig_key, 0, sizeof(struct tls_param_original_key));
	
    tls_param_get(TLS_PARAM_ID_KEY, (void *)param_key, 1);
    key->index = param_key->key_index;
    key->format = param_key->key_format;
	
	tls_param_get(TLS_PARAM_ID_ORIGIN_KEY, (void *)orig_key, 1);
    MEMCPY(key->key, orig_key->psk, 64);
	key->key_len = orig_key->key_length;

	tls_mem_free(param_key);
	tls_mem_free(orig_key);
    return 0;
}
#if 0
int tls_set_encrypt_cfg( u8 encrypt)
{
	struct wpa_supplicant *wpa_s = tls_get_wpa_data();
    struct wpa_ssid *ssid = wpa_s->ssid_conf;
	struct wpa_config *conf = wpa_s->conf;

    switch (encrypt) {
        case 0: /* OPEN */
            ssid->proto = 0;
            ssid->key_mgmt = WPA_KEY_MGMT_NONE;
            ssid->pairwise_cipher = WPA_CIPHER_NONE;
            ssid->group_cipher = WPA_CIPHER_NONE;
            break;
        case 1: /* WEP 64 */
            ssid->proto = 0;
            ssid->key_mgmt = WPA_KEY_MGMT_NONE;
            ssid->pairwise_cipher = WPA_CIPHER_WEP40;
            ssid->group_cipher = WPA_CIPHER_WEP40;
            break;
        case 2: /* WEP 128 */
            ssid->proto = 0;
            ssid->key_mgmt = WPA_KEY_MGMT_NONE;
            ssid->pairwise_cipher = WPA_CIPHER_WEP104;
            ssid->group_cipher = WPA_CIPHER_WEP104;
            break;
        case 3: /* TKIP WPA */
            ssid->proto = WPA_PROTO_WPA;
            ssid->key_mgmt = WPA_KEY_MGMT_PSK;
            ssid->pairwise_cipher = WPA_CIPHER_TKIP;
            ssid->group_cipher = WPA_CIPHER_TKIP;
            break;
        case 4: /* CCMP WPA */
            ssid->proto = WPA_PROTO_WPA;
            ssid->key_mgmt = WPA_KEY_MGMT_PSK;
            ssid->pairwise_cipher = WPA_CIPHER_CCMP;
            ssid->group_cipher = WPA_CIPHER_CCMP;
            break;
        case 5: /* TKIP WPA2 */
            ssid->proto = WPA_PROTO_RSN;
            ssid->key_mgmt = WPA_KEY_MGMT_PSK;
            ssid->pairwise_cipher = WPA_CIPHER_TKIP;
            ssid->group_cipher = WPA_CIPHER_TKIP;
            break;
        case 6: /* CCMP WPA2 */
            ssid->proto = WPA_PROTO_RSN;
            ssid->key_mgmt = WPA_KEY_MGMT_PSK;
            ssid->pairwise_cipher = WPA_CIPHER_CCMP;
            ssid->group_cipher = WPA_CIPHER_CCMP;
            break;
        case 7:
            ssid->proto = WPA_PROTO_WPA;
            ssid->key_mgmt = WPA_KEY_MGMT_PSK;
            ssid->pairwise_cipher = WPA_CIPHER_CCMP|WPA_CIPHER_TKIP;
            ssid->group_cipher = WPA_CIPHER_TKIP;
            break;
        case 8:
            ssid->proto = WPA_PROTO_RSN;
            ssid->key_mgmt = WPA_KEY_MGMT_PSK;
            ssid->pairwise_cipher = WPA_CIPHER_CCMP|WPA_CIPHER_TKIP;
            ssid->group_cipher = WPA_CIPHER_TKIP;
            break;
        default:
            return -1;
    }
	conf->proto = ssid->proto;
	conf->key_mgmt = ssid->key_mgmt;
	conf->pairwise_cipher = ssid->pairwise_cipher;
	conf->group_cipher = ssid->group_cipher;
    return 0;
}
#endif
int tls_cmd_set_encrypt(
        u8 encrypt, u8 update_flash)
{
    struct tls_param_key param_key;
    //int err;
#if 0
    err = tls_set_encrypt_cfg(encrypt);
    if (err)
        return -1;
#endif
	if (0 == encrypt){
	    memset(param_key.psk, 0, 64);
	    param_key.key_format = 0;
	    param_key.key_index = 0;
	    param_key.key_length = 0;
	    tls_param_set(TLS_PARAM_ID_KEY, (void *)&param_key, (bool)update_flash);
	}

    tls_param_set(TLS_PARAM_ID_ENCRY, (void *)&encrypt, (bool)update_flash);

    return 0;
}

int tls_cmd_get_encrypt( u8 *encrypt)
{
	tls_param_get(TLS_PARAM_ID_ENCRY, (void *)encrypt, (bool)0);

    return 0;
}

int tls_cmd_set_bssid(
        struct tls_cmd_bssid_t *bssid,
        u8 update_flash)
{
    struct tls_param_bssid param_bssid;
    int err;

    err = is_zero_ether_addr(bssid->bssid);
    if (err)
        return -1;
    param_bssid.bssid_enable = bssid->enable;
    MEMCPY(param_bssid.bssid, bssid->bssid, ETH_ALEN);

    tls_param_set(TLS_PARAM_ID_BSSID, (void *)&param_bssid,
            (bool)update_flash);

    return 0;
}

int tls_cmd_get_bssid(struct tls_cmd_bssid_t *bssid)
{
    struct tls_param_bssid param_bssid;

	if (bssid){
	    tls_param_get(TLS_PARAM_ID_BSSID, (void *)&param_bssid,
    	        (bool)0);
		MEMCPY(bssid->bssid, param_bssid.bssid, 6);
		bssid->enable = param_bssid.bssid_enable;
	}

    return 0;
}

int tls_cmd_get_original_ssid(struct tls_param_ssid *original_ssid)
{
	tls_param_get(TLS_PARAM_ID_ORIGIN_SSID, (void *)original_ssid, 1);
	if(original_ssid->ssid_len > 32)
       {
           original_ssid->ssid_len = 0;
           original_ssid->ssid[0] = '\0';
	}

	return 0;
}

int tls_cmd_get_original_key(struct tls_param_original_key *original_key)
{
	tls_param_get(TLS_PARAM_ID_ORIGIN_KEY, (void *)original_key, 1);
	return 0;
}

int tls_cmd_set_hide_ssid(
        u8 ssid_set, u8 update_flash)
{

    tls_param_set(TLS_PARAM_ID_BRDSSID, (void *)&ssid_set, (bool)update_flash);

    return 0;
}

int tls_cmd_get_hide_ssid( u8 *ssid_set)
{
	tls_param_get(TLS_PARAM_ID_BRDSSID, (void *)ssid_set, (bool)0);

    return 0;
}

int tls_cmd_set_channel(u8 channel,  u8 channel_en, u8 update_flash)
{
    if (channel > 14)
        return -1;

    tls_param_set(TLS_PARAM_ID_CHANNEL, (void *)&channel, (bool)update_flash);
    tls_param_set(TLS_PARAM_ID_CHANNEL_EN, (void *)&channel_en, (bool)update_flash);

    return 0;
}

int tls_cmd_get_channel( u8 *channel, u8 *channel_en)
{
    tls_param_get(TLS_PARAM_ID_CHANNEL, (void *)channel, (bool)0);
    tls_param_get(TLS_PARAM_ID_CHANNEL_EN, (void *)channel_en, (bool)0);

    /* 未指定信道时默认选择1信道 for BUG #429 */
    if (0 == *channel_en)
    {
        *channel = 1;
    }

    return 0;
}

int tls_cmd_set_channellist( u16 channellist, u8 update_flash)
{
	tls_param_set(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, (bool)update_flash);
	return 0;
}

int tls_cmd_get_channellist( u16 *channellist)
{
	tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)channellist, (bool)1);
	return 0;
}

int tls_cmd_set_region(
        u16 region, u8 update_flash)
{
#if 0
	struct wpa_supplicant *wpa_s = tls_get_wpa_data();
    struct wpa_ssid *ssid = wpa_s->ssid_conf;

    ssid->region = region;
#endif
    tls_param_set(TLS_PARAM_ID_COUNTRY_REGION, (void *)&region, (bool)update_flash);
    return 0;
}

int tls_cmd_get_region(u16 *region)
{
    tls_param_get(TLS_PARAM_ID_COUNTRY_REGION, (void *)region, (bool)0);

    return 0;
}

/*
* 0: 11B/G
* 1: 11B
* 2: 11B/G/N
*/
int tls_cmd_set_hw_mode(
        struct tls_cmd_wl_hw_mode_t *hw_mode, u8 update_flash)
{
    struct tls_param_bgr bgr;

    //int ret;

	if (hw_mode->hw_mode > 2) //wangm:  bgn
		return -1;

    if ((hw_mode->hw_mode == 1) && (hw_mode->max_rate > 3)) {
        return -1;
    }
#if 0
	/* max_rate will be initialized in wl_core_init, if changed this, need reboot(NOT SURE?) */
	struct tls_wif * wif= tls_get_wif_data();
    ret = tls_wl_if_set_max_rate(wif, hw_mode->max_rate);
    if (ret)
        return -1;
#endif
    bgr.bg = hw_mode->hw_mode;
    bgr.max_rate = hw_mode->max_rate;
    tls_param_set(TLS_PARAM_ID_WBGR, (void *)&bgr, (bool)update_flash);
    return 0;
}

int tls_cmd_get_hw_mode(
        struct tls_cmd_wl_hw_mode_t *hw_mode)
{
    struct tls_param_bgr bgr;

    tls_param_get(TLS_PARAM_ID_WBGR, (void *)&bgr, (bool)0);
	hw_mode->hw_mode = bgr.bg;
	hw_mode->max_rate = bgr.max_rate;

    return 0;
}

int tls_cmd_set_adhoc_create_mode(
        u8 mode, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_ADHOC_AUTOCREATE, (void *)&mode, (bool)update_flash);
    return 0;
}

int tls_cmd_get_adhoc_create_mode( u8 *mode)
{
    tls_param_get(TLS_PARAM_ID_ADHOC_AUTOCREATE, (void *)mode, (bool)0);

    return 0;
}

int tls_cmd_set_wl_ps_mode( u8 enable,
        u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_PSM, (void *)&enable, (bool)update_flash);

    return 0;
}

int tls_cmd_get_wl_ps_mode( u8 *enable)
{
    tls_param_get(TLS_PARAM_ID_PSM, (void *)enable, (bool)1);

    return 0;
}

int tls_cmd_set_roaming_mode(u8 enable, u8 update_flash)
{

    tls_param_set(TLS_PARAM_ID_ROAMING, (void *)&enable, (bool)update_flash);

    return 0;
}

int tls_cmd_get_roaming_mode(u8 *enable)
{
    tls_param_get(TLS_PARAM_ID_ROAMING, (void *)enable, (bool)0);
    return 0;
}

int tls_cmd_set_wps_params(struct tls_cmd_wps_params_t *params, u8 update_flash)
{
#if TLS_CONFIG_WPS
    return 0;
#else
    return -1;
#endif
}

int tls_cmd_get_wps_params(struct tls_cmd_wps_params_t *params)
{

#if TLS_CONFIG_WPS
#if 0
	struct wpa_supplicant *wpa_s = tls_get_wpa_data();
    struct wpa_ssid *ssid = wpa_s->ssid_conf;
    if (ssid->pin_start == 1) {
        params->mode = 1;
        params->pin_len = 8;
        MEMCPY(params->pin, ssid->pin, 8);
    } else
        params->mode = 0;
#endif
    return 0;
#else
    return -1;
#endif
}

int tls_cmd_get_ip_info(
        struct tls_cmd_ip_params_t *params)
{
    struct tls_param_ip ip_param;
    tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);

    MEMCPY(params->ip_addr, (char *)ip_param.ip, 4);
    MEMCPY(params->netmask, (char *)ip_param.netmask, 4);
    MEMCPY(params->gateway, (char *)ip_param.gateway, 4);
    MEMCPY(params->dns, (char *)ip_param.dns1, 4);
    params->type = ip_param.dhcp_enable ? 0 : 1;
    return 0;
}

int tls_cmd_set_ip_info(
        struct tls_cmd_ip_params_t *params, u8 update_flash)
{
    struct tls_ethif *ethif;
    struct tls_param_ip param_ip;

    ethif=tls_netif_get_ethif();
	if (ethif == NULL)
	{
		return -1;
	}
	//if(tls_param_get_updp_mode() == 0)
	if (WM_WIFI_JOINED == tls_wifi_get_state())
	{
	    if (params->type == 0) {
	        /* enable dhcp */
	            tls_dhcp_start();
	    } else {
	        tls_dhcp_stop();

			MEMCPY((char *)ip_2_ip4(&ethif->ip_addr) , &params->ip_addr, 4);
            MEMCPY((char *)ip_2_ip4(&ethif->dns1), &params->dns, 4);
	        MEMCPY((char *)ip_2_ip4(&ethif->netmask), &params->netmask, 4);
	        MEMCPY((char *)ip_2_ip4(&ethif->gw), &params->gateway, 4);
	        tls_netif_set_addr(&ethif->ip_addr, &ethif->netmask, &ethif->gw);
	    }
	}

    /* update flash params */
    param_ip.dhcp_enable = params->type ? 0 : 1;
    MEMCPY((char *)param_ip.dns1, &params->dns, 4);
    MEMCPY((char *)param_ip.dns2, param_ip.dns2, 4);
    MEMCPY((char *)param_ip.gateway, &params->gateway, 4);
    MEMCPY((char *)param_ip.ip, &params->ip_addr, 4);
    MEMCPY((char *)param_ip.netmask, &params->netmask, 4);
    tls_param_set(TLS_PARAM_ID_IP, (void *)&param_ip, (bool)update_flash);

    return 0;
}

int tls_cmd_set_work_mode(
        u8 mode, u8 update_flash)
{
	u8 auto_mode;

	switch (mode) {
	       case 0:
	           auto_mode= 1;
	            break;
	        case 1:
	           auto_mode= 0;
	            break;
	        default:
	            return -1;
	}

    tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode,
            (bool)update_flash);

	/* AUTOMODE: transmode, it must work with AUTO_RECONNECT together */
	//auto_mode = (auto_mode == 0) ? 1:0;
	tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_work_mode(u8 *mode)
{
	u8 auto_mode;

    tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, 0);
    if (0 == auto_mode)
        *mode = 1;
    else
        *mode = 0;
    return 0;
}

int tls_cmd_get_hostif_mode(u8 *mode)
{
    tls_param_get(TLS_PARAM_ID_USRINTF, (void *)mode, TRUE);

    return 0;
}

int tls_cmd_set_hostif_mode(u8 mode, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_USRINTF, (void *)&mode,
            (bool)update_flash);
    return 0;
}

int tls_cmd_set_default_socket_params(
        struct tls_cmd_socket_t *params, u8 update_flash)
{
    struct tls_socket_cfg  *skt_cfg = &socket_cfg;
	struct tls_param_socket param_socket_cfg;
    if(tls_param_get_updp_mode()==0)
    {
	    skt_cfg->proto = params->proto;
	    skt_cfg->client = params->client;
	    skt_cfg->port = params->port;
	   	skt_cfg->host_len = params->host_len;
	    MEMCPY(skt_cfg->ip_addr, params->ip_addr, 4);
		strcpy((char *)skt_cfg->host, params->host_name);
	    skt_cfg->timeout = params->timeout;
    }
    param_socket_cfg.client_or_server = params->client ? 0 : 1;
    param_socket_cfg.protocol = params->proto;
    param_socket_cfg.port_num = params->port;
   	strcpy((char *)param_socket_cfg.host, params->host_name);
    tls_param_set(TLS_PARAM_ID_DEFSOCKET, (void *)&param_socket_cfg,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_default_socket_params(
        struct tls_cmd_socket_t *params)
{
    struct tls_socket_cfg  *skt_cfg = &socket_cfg;

    params->proto = skt_cfg->proto;
    params->client = skt_cfg->client;
    params->port = skt_cfg->port;
    params->host_len = skt_cfg->host_len;
    strcpy(params->host_name, (char *)skt_cfg->host);
    MEMCPY(params->ip_addr, skt_cfg->ip_addr, 4);
    params->timeout = skt_cfg->timeout;
    return 0;
}

int tls_cmd_get_uart_params(
        struct tls_cmd_uart_params_t *params)
{
    struct tls_param_uart uart_params;

    tls_param_get(TLS_PARAM_ID_UART,
            (void *)&uart_params, 0);
    params->baud_rate = uart_params.baudrate;
    params->flow_ctrl = uart_params.flow;
    params->parity = uart_params.parity;
	switch (uart_params.stop_bits){
		case TLS_UART_ONE_STOPBITS:
			params->stop_bit = 0;
			break;
		case TLS_UART_TWO_STOPBITS:
			params->stop_bit = 2;
			break;
		default:
			params->stop_bit = 0;
			break;
	}
	switch (uart_params.charsize){
		case TLS_UART_CHSIZE_8BIT:
			params->charlength = 0;
			break;
		case TLS_UART_CHSIZE_7BIT:
			params->charlength = 1;
			break;
		default:
			params->charlength = 0;
	}


    return 0;
}

int tls_cmd_set_uart_params(
        struct tls_cmd_uart_params_t *params, u8 update_flash)
{
    int err;
    TLS_UART_STOPBITS_T stop_bit;
    struct tls_param_uart uart_params;
	TLS_UART_CHSIZE_T charsize;
    struct tls_uart_port *uart1_port = NULL;
	cmd_get_uart1_port_callback callback;

#if TLS_CONFIG_UART
	{
		extern int tls_uart_check_baudrate(u32 baudrate);
    err = tls_uart_check_baudrate(params->baud_rate);
	}
#endif
    if (err < 0)
    return CMD_ERR_INV_PARAMS;
	switch (params->charlength){
		case 0:
			charsize = TLS_UART_CHSIZE_8BIT;
			break;
		case 1:
			charsize = TLS_UART_CHSIZE_7BIT;
			break;
		default:
			return CMD_ERR_INV_PARAMS;
	}
    if (params->flow_ctrl > 1)
        return CMD_ERR_INV_PARAMS;
    if (params->parity > 2)
        return CMD_ERR_INV_PARAMS;
    switch (params->stop_bit) {
        case 0:
            stop_bit = TLS_UART_ONE_STOPBITS;
            break;
        case 2:
            stop_bit = TLS_UART_TWO_STOPBITS;
			break;
        default:
            return CMD_ERR_INV_PARAMS;
    }
    callback = tls_cmd_get_uart1_port();
    if(callback!=NULL)
        callback(&uart1_port);
    if (!uart1_port)
        return CMD_ERR_NOT_ALLOW;
    if(tls_param_get_updp_mode()==0)
    {
		uart1_port->opts.baudrate = params->baud_rate;
		uart1_port->opts.paritytype= (TLS_UART_PMODE_T)params->parity;
		uart1_port->opts.flow_ctrl = (TLS_UART_FLOW_CTRL_MODE_T)params->flow_ctrl;
		uart1_port->opts.stopbits= stop_bit;
		uart1_port->opts.charlength = charsize;
    }
    uart_params.baudrate = params->baud_rate;
    uart_params.flow = params->flow_ctrl;
    uart_params.parity = params->parity;
    uart_params.stop_bits = stop_bit;
	uart_params.charsize = charsize;

    err = tls_param_set(TLS_PARAM_ID_UART,
            (void *)&uart_params,
            (bool)update_flash);
    if (err)
        return CMD_ERR_FLASH;
    return 0;
}

int tls_cmd_get_atlt( u16 *length)
{
	struct tls_hostif *hif = tls_get_hostif();
    *length = hif->uart_atlt;
    return 0;
}

int tls_cmd_set_atlt( u16 length, u8 update_flash)
{
	struct tls_hostif *hif = tls_get_hostif();
    if (length < 32 || length > 1024)
        return -1;
    hif->uart_atlt = length;
    tls_param_set(TLS_PARAM_ID_AUTO_TRIGGER_LENGTH,
            (void *)&length,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_atpt( u16 *period)
{
	struct tls_hostif *hif = tls_get_hostif();
	tls_param_get(TLS_PARAM_ID_AUTO_TRIGGER_PERIOD, (void *)period, 0);
	hif->uart_atpt = *period;
	return 0;
}


int tls_cmd_set_dnsname( u8 *dnsname, u8 update_flash)
{
    if (dnsname == NULL)
        return -1;
    tls_param_set(TLS_PARAM_ID_DNSNAME,
            (void *)dnsname,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_dnsname( u8 *dnsname)
{
	tls_param_get(TLS_PARAM_ID_DNSNAME, dnsname, 0);
	return 0;
}

int tls_cmd_set_atpt( u16 period, u8 update_flash)
{
	struct tls_hostif *hif = tls_get_hostif();
    if (period > 10000)
        return -1;
    hif->uart_atpt = period;
    tls_param_set(TLS_PARAM_ID_AUTO_TRIGGER_PERIOD,
            (void *)&period,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_espc(u8 *escapechar)
{
    tls_param_get(TLS_PARAM_ID_ESCAPE_CHAR,
            (void *)escapechar, 0);
	return 0;
}

int tls_cmd_set_espc( u8 escapechar, u8 update_flash)
{
	struct tls_hostif *hif = tls_get_hostif();
	hif->escape_char = escapechar;
    tls_param_set(TLS_PARAM_ID_ESCAPE_CHAR,
            (void *)&escapechar,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_espt(u16 *escapeperiod)
{
    tls_param_get(TLS_PARAM_ID_ESCAPE_PERIOD,
            (void *)escapeperiod, 0);
	return 0;
}

int tls_cmd_set_espt( u16 escapeperiod, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_ESCAPE_PERIOD,
            (void *)&escapeperiod,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_iom( u8 *iomode)
{
    tls_param_get(TLS_PARAM_ID_IO_MODE, (void *)iomode, 0);

	return 0;
}

int tls_cmd_set_iom( u8 iomode, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_IO_MODE,
            (void *)&iomode,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_cmdm( u8 *cmdmode)
{
    tls_param_get(TLS_PARAM_ID_CMD_MODE,
            (void *)cmdmode,
            (bool)0);

	return 0;
}

int tls_cmd_set_cmdm( u8 cmdmode, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_CMD_MODE,
            (void *)&cmdmode,
            (bool)update_flash);
    return 0;
}

int tls_cmd_set_oneshot( u8 oneshotflag, u8 update_flash)
{
	return tls_wifi_set_oneshot_flag(oneshotflag);
}

int tls_cmd_get_oneshot( u8 *oneshotflag)
{
	*oneshotflag = tls_wifi_get_oneshot_flag();
	return 0;
}

int tls_cmd_set_sha1( u8* psk, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_SHA1,
            (void *)psk,
            (bool)update_flash);
    return 0;
}

int tls_cmd_get_sha1( u8 *psk)
{
	tls_param_get(TLS_PARAM_ID_SHA1, (void *)psk,1);

	return 0;
}
#if TLS_CONFIG_WPS
int tls_cmd_set_wps_pin( struct tls_param_wps* wps, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_WPS, (void *)wps, (bool)update_flash);
    return 0;
}

int tls_cmd_get_wps_pin( struct tls_param_wps *wps)
{
	tls_param_get(TLS_PARAM_ID_WPS, (void *)wps,1);

	return 0;
}
#endif
int tls_cmd_get_pass( u8 *password)
{
    tls_param_get(TLS_PARAM_ID_PASSWORD, (void *)password,(bool)0);
	return 0;
}

int tls_cmd_set_pass( u8* password, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_PASSWORD,
        (void *)password,
        (bool)update_flash);
    return 0;
}

int tls_cmd_get_warc( u8 *autoretrycnt)
{
    tls_param_get(TLS_PARAM_ID_AUTO_RETRY_CNT,
        (void *)autoretrycnt,
        (bool)1);
	return 0;
}

int tls_cmd_set_warc( u8 autoretrycnt, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_AUTO_RETRY_CNT,
        (void *)&autoretrycnt,
        (bool)update_flash);
    return 0;
}

int tls_cmd_set_webs( struct tls_webs_cfg webcfg, u8 update_flash)
{
    tls_param_set(TLS_PARAM_ID_WEBS_CONFIG,
        (void *)&webcfg,
        (bool)update_flash);
	return 0;
}

int tls_cmd_get_webs( struct tls_webs_cfg *webcfg)
{
	tls_param_get(TLS_PARAM_ID_WEBS_CONFIG, (void *)webcfg, 0);
	return 0;
}

int tls_cmd_set_dbg( u32 dbg)
{
#if TLS_CONFIG_LOG_PRINT
    if (dbg)
        tls_wifi_enable_log(true);
    else
        tls_wifi_enable_log(false);
#endif

    return 0;
}

int tls_cmd_wr_flash(
        struct tls_cmd_flash_t *wr_flash)
{
    u8 data[24];
    //TLS_DBGPRT_INFO("ptr = 0x%x\n", wr_flash->value);
    //TLS_DBGPRT_DUMP((char *)wr_flash->value, 24);
    tls_fls_write(wr_flash->flash_addr,
            (u8 *)wr_flash->value,
            sizeof(u32) * wr_flash->word_cnt);


    memset(data, 0, 24);
    tls_fls_read(wr_flash->flash_addr, data, 24);
    TLS_DBGPRT_DUMP((char *)data, 24);

    return 0;
}

void tls_cmd_register_get_uart1_port(cmd_get_uart1_port_callback callback)
{
    get_uart1_port_callback = callback;
}
cmd_get_uart1_port_callback tls_cmd_get_uart1_port(void)
{
    return get_uart1_port_callback;
}
void tls_cmd_register_set_uart1_mode(cmd_set_uart1_mode_callback callback)
{
    set_uart1_mode_callback = callback;
}

cmd_set_uart1_mode_callback tls_cmd_get_set_uart1_mode(void)
{
    return set_uart1_mode_callback;
}
void tls_cmd_register_set_uart1_sock_param(cmd_set_uart1_sock_param_callback callback)
{
    set_uart1_sock_param_callback = callback;
}
cmd_set_uart1_sock_param_callback tls_cmd_get_set_uart1_sock_param(void)
{
    return set_uart1_sock_param_callback;
}

void tls_cmd_register_set_uart0_mode(cmd_set_uart0_mode_callback callback)
{
    set_uart0_mode_callback = callback;
}
cmd_set_uart0_mode_callback tls_cmd_get_set_uart0_mode(void)
{
    return set_uart0_mode_callback;
}


#if TLS_CONFIG_AP
int tls_cmd_set_softap_ssid(struct tls_cmd_ssid_t *ssid, u8 update_flash)
{
    struct tls_param_ssid params_ssid;
    struct tls_param_sha1 apsta_psk;

    if (ssid->ssid_len > 32)
        return -1;

    params_ssid.ssid_len = ssid->ssid_len;
    MEMCPY(&params_ssid.ssid, ssid->ssid, ssid->ssid_len);
    tls_param_set(TLS_PARAM_ID_SOFTAP_SSID, (void *)&params_ssid, (bool)update_flash);

    memset(&apsta_psk, 0, sizeof(apsta_psk));
    tls_param_set(TLS_PARAM_ID_SOFTAP_PSK, (void *)&apsta_psk, (bool)update_flash);

    return 0;
}

int tls_cmd_get_softap_ssid(struct tls_cmd_ssid_t *ssid)
{
    struct tls_param_ssid params_ssid;

    tls_param_get(TLS_PARAM_ID_SOFTAP_SSID, (void *)&params_ssid, 1);
    if (params_ssid.ssid_len > 32){
    	ssid->ssid_len = 0;
		params_ssid.ssid[0] = '\0';
    }else{
		ssid->ssid_len = params_ssid.ssid_len;
	    MEMCPY(ssid->ssid, params_ssid.ssid, ssid->ssid_len);
    }
    return 0;
}

int tls_cmd_set_softap_key(struct tls_cmd_key_t *key, u8 update_flash)
{
    struct tls_param_key param_key;
	struct tls_param_sha1* sha1_key;

    MEMCPY(param_key.psk, key->key, 64);
    param_key.key_format = key->format;
    param_key.key_index = key->index;
    param_key.key_length = key->key_len;
    tls_param_set(TLS_PARAM_ID_SOFTAP_KEY, (void *)&param_key, (bool)update_flash);

	sha1_key = (struct tls_param_sha1*)&param_key;
	memset((u8* )sha1_key, 0, sizeof(struct tls_param_sha1));
	tls_param_set(TLS_PARAM_ID_SOFTAP_PSK, (void *)sha1_key, TRUE);

    return 0;
}

int tls_cmd_get_softap_key(struct tls_cmd_key_t *key)
{
    struct tls_param_key *param_key;

	param_key = tls_mem_alloc(sizeof(struct tls_cmd_key_t));
	if(!param_key)
		return -1;

	memset(param_key, 0, sizeof(struct tls_cmd_key_t));
    tls_param_get(TLS_PARAM_ID_SOFTAP_KEY, (void *)param_key, 1);

    key->index = param_key->key_index;
    key->key_len = param_key->key_length;
    key->format = param_key->key_format;
    MEMCPY(key->key, param_key->psk, 64);

	tls_mem_free(param_key);

    return 0;
}

int tls_cmd_set_softap_encrypt(
        u8 encrypt, u8 update_flash)
{
    struct tls_param_key param_key;

	if (0 == encrypt){
	    memset(param_key.psk, 0, 64);
	    param_key.key_format = 0;
	    param_key.key_index = 0;
	    param_key.key_length = 0;
	    tls_param_set(TLS_PARAM_ID_SOFTAP_KEY, (void *)&param_key, (bool)update_flash);
	}

    tls_param_set(TLS_PARAM_ID_SOFTAP_ENCRY, (void *)&encrypt, (bool)update_flash);

    return 0;
}

int tls_cmd_get_softap_encrypt( u8 *encrypt)
{
	tls_param_get(TLS_PARAM_ID_SOFTAP_ENCRY, (void *)encrypt, (bool)0);

    return 0;
}

int tls_cmd_get_softap_channel( u8 *channel)
{
    tls_param_get(TLS_PARAM_ID_SOFTAP_CHANNEL, (void *)channel, (bool)0);

    return 0;
}

int tls_cmd_set_softap_channel(u8 channel, u8 update_flash)
{
    if (channel > 14)
        return -1;

    tls_param_set(TLS_PARAM_ID_SOFTAP_CHANNEL, (void *)&channel, (bool)update_flash);

    return 0;
}

/*
* 0: 11B/G
* 1: 11B
* 2: 11B/G/N
*/
int tls_cmd_set_softap_hw_mode(
        struct tls_cmd_wl_hw_mode_t *hw_mode, u8 update_flash)
{
    struct tls_param_bgr bgr;

#if TLS_CONFIG_SOFTAP_11N
	if (hw_mode->hw_mode > 2)
#else
    if (hw_mode->hw_mode > 1) //wangm:  bg
#endif
		return -1;

    if ((hw_mode->hw_mode == 1) && (hw_mode->max_rate > 3)) {
        return -1;
    }

    bgr.bg = hw_mode->hw_mode;
    bgr.max_rate = hw_mode->max_rate;
    tls_param_set(TLS_PARAM_ID_SOFTAP_WBGR, (void *)&bgr, (bool)update_flash);

    return 0;
}

int tls_cmd_get_softap_hw_mode(
        struct tls_cmd_wl_hw_mode_t *hw_mode)
{
    struct tls_param_bgr bgr;

    tls_param_get(TLS_PARAM_ID_SOFTAP_WBGR, (void *)&bgr, (bool)0);
	hw_mode->hw_mode = bgr.bg;
	hw_mode->max_rate = bgr.max_rate;

    return 0;
}

int tls_cmd_get_softap_ip_info(
        struct tls_cmd_ip_params_t *params)
{
    struct tls_param_ip ip_param;
    tls_param_get(TLS_PARAM_ID_SOFTAP_IP, &ip_param, FALSE);

    MEMCPY(params->ip_addr, (char *)ip_param.ip, 4);
    MEMCPY(params->netmask, (char *)ip_param.netmask, 4);
    MEMCPY(params->gateway, (char *)ip_param.gateway, 4);
    MEMCPY(params->dns, (char *)ip_param.dns1, 4);
    params->type = ip_param.dhcp_enable;

    return 0;
}

int tls_cmd_set_softap_ip_info(
        struct tls_cmd_ip_params_t *params, u8 update_flash)
{
    struct tls_param_ip param_ip;

    /* update flash params */
    param_ip.dhcp_enable = params->type;
    MEMCPY((char *)param_ip.dns1, &params->dns, 4);
    MEMCPY((char *)param_ip.dns2, param_ip.dns2, 4);
    MEMCPY((char *)param_ip.gateway, &params->gateway, 4);
    MEMCPY((char *)param_ip.ip, &params->ip_addr, 4);
    MEMCPY((char *)param_ip.netmask, &params->netmask, 4);
    tls_param_set(TLS_PARAM_ID_SOFTAP_IP, (void *)&param_ip, (bool)update_flash);

    return 0;
}

int tls_cmd_get_softap_link_status(struct tls_cmd_link_status_t *lks)
{
    struct netif *netif;

    netif = tls_get_netif();
	if (netif == NULL)
		return -1;
	
    netif = netif->next;

    if (netif_is_up(netif))
        lks->status = 1;
    else
        lks->status = 0;

    MEMCPY(lks->ip, (char *)&netif->ip_addr.addr, 4);
    MEMCPY(lks->netmask, (char *)&netif->netmask.addr, 4);
    MEMCPY(lks->gw, (char *)&netif->gw.addr, 4);

    MEMCPY(lks->dns1, (char *)&netif->ip_addr.addr, 4);
    memset(lks->dns2, 0, 4);

    return 0;
}

int tls_cmd_get_sta_detail(u32 *sta_num, u8 *buf)
{
#define STA_MAC_BUF_LEN  64
    int len = 0;
    u32 cnt;
    u8 *sta_buf;
    ip_addr_t *ip_addr;
    struct tls_sta_info_t *sta;

    sta_buf = tls_mem_alloc(STA_MAC_BUF_LEN);
    if (NULL == sta_buf)
    {
        return -1;
    }

    memset(sta_buf, 0, STA_MAC_BUF_LEN);
    tls_wifi_get_authed_sta_info(sta_num, sta_buf, STA_MAC_BUF_LEN);
    sta = (struct tls_sta_info_t *)sta_buf;
    for (cnt = 0; cnt < *sta_num; cnt++)
    {
        ip_addr = tls_dhcps_getip(sta->mac_addr);
        if (NULL == ip_addr)
        {
            len += sprintf((char *)(buf+len), ",%02X-%02X-%02X-%02X-%02X-%02X,-",
                                               MAC2STR(sta->mac_addr));
        }
        else
        {     
            len += sprintf((char *)(buf+len), ",%02X-%02X-%02X-%02X-%02X-%02X,%d.%d.%d.%d",
                                               MAC2STR(sta->mac_addr),
                                               ip4_addr1(ip_2_ip4(ip_addr)),
                                               ip4_addr2(ip_2_ip4(ip_addr)),
                                               ip4_addr3(ip_2_ip4(ip_addr)),
                                               ip4_addr4(ip_2_ip4(ip_addr)));
        }
        sta++;
    }
    tls_mem_free(sta_buf);

    return 0;
}
#endif
#endif //TLS_CONFIG_HOSTIF

/**************************************************************************
 * File Name                    : wm_wifi_oneshot.c
 * Author                       : WinnerMicro
 * Version                      :
 * Date                         : 05/30/2014
 * Description                  : Wifi one shot sample(UDP, PROBEREUEST)
 *
 * Copyright (C) 2014 Beijing Winner Micro Electronics Co.,Ltd.
 * All rights reserved.
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm_include.h"
#include "wm_mem.h"
#include "wm_type_def.h"

#include "wm_ieee80211_gcc.h"

#include "wm_wifi.h"
#include "wm_wifi_oneshot.h"
#include "utils.h"
#include "wm_params.h"
#include "wm_osal.h"
#include "tls_wireless.h"
#include "wm_wl_task.h"
#include "wm_webserver.h"
#include "wm_timer.h"
#include "wm_cpu.h"
#include "wm_oneshot_lsd.h"
#include "wm_efuse.h"
#include "wm_bt_config.h"

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#define ONESHOT_DEBUG 0
#if ONESHOT_DEBUG
#define ONESHOT_DBG printf
#else
#define ONESHOT_DBG(s, ...)
#endif

#define ONESHOT_INFO 1
#if ONESHOT_INFO
#define ONESHOT_INF printf
#else
#define ONESHOT_INF(s, ...)
#endif

u32 oneshottime = 0;

volatile u8 guconeshotflag = 0;

static u8 glast_ucOneshotPsFlag = 0;
volatile u8 gucOneshotPsFlag = 0;
volatile u8 gucOneshotErr = 0;


/*Networking necessary information*/
volatile u8 gucssidokflag = 0;
u8 gucssidData[33] = {0};

static u8 gucbssidData[ETH_ALEN] = {0};
volatile u8 gucbssidokflag = 0;

volatile u8 gucpwdokflag = 0;
u8 gucpwdData[65] ={0};

static u8 gucCustomData[3][65] ={{'\0'},{'\0'}, {'\0'}};

static tls_wifi_oneshot_result_callback gpfResult = NULL;
static 	int gchanLock = 0;

#define ONESHOT_MSG_QUEUE_SIZE 32
tls_os_queue_t *oneshot_msg_q = NULL;

#define    ONESHOT_TASK_SIZE      1024

static u32 *OneshotTaskStk;

extern bool is_airkiss;

#if TLS_CONFIG_UDP_ONE_SHOT
#define TLS_ONESHOT_RESTART_TIME  5000*HZ/1000
#define TLS_ONESHOT_SYNC_TIME	6000*HZ/1000
#define TLS_ONESHOT_RETRY_TIME  10000*HZ/1000
#define TLS_ONESHOT_RECV_TIME   50000*HZ/1000
#define TLS_ONESHOT_SWITCH_TIMER_MAX (80*HZ/1000)
static tls_os_timer_t *gWifiSwitchChanTim = NULL;
static tls_os_timer_t *gWifiHandShakeTimOut = NULL;
static tls_os_timer_t *gWifiRecvTimOut = NULL;


static u8 gSrcMac[ETH_ALEN] = {0,0,0,0,0,0};

#define HANDSHAKE_CNT 3
volatile u8 guchandshakeflag = 0;

#define TOTAL_CHAN_NUM 17
static u8 airwifichan[TOTAL_CHAN_NUM]={0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF, 0xF,0xF, 0xF};
static u8 airchantype[TOTAL_CHAN_NUM]={0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0, 0};
static u8 uctotalchannum = 0;
static u8 scanChanErr = 0;

static tls_os_sem_t	*gWifiRecvSem = NULL;

#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL
#define ONESHOT_SPECIAL_DELAY_TIME  (8 * HZ)
static u32 g_oneshot_dur_time = 0;
#endif

#endif

#if TLS_CONFIG_AP_MODE_ONESHOT
static u8 gucRawValid = 0;
static u8 *gaucRawData = NULL;

#define APSKT_MAX_ONESHOT_NUM (8)
#define APSKT_SSID_MAX_LEN (32)
#define ONESHOT_AP_NAME "softap"
#define SOCKET_SERVER_PORT 65532
#define SOCKET_RX_DATA_BUFF_LEN 255

struct tls_socket_desc *skt_descp = NULL;
typedef struct sock_recive{
    int socket_num;
	char *sock_rx_data;
	u8 sock_data_len;
}ST_Sock_Recive;
ST_Sock_Recive *sock_rx = NULL;
#endif

extern void tls_wl_change_chanel(u32 chanid);
extern void tls_wl_change_channel_info(u32 chanid, u32 channel_type);
extern int tls_wifi_decode_new_oneshot_data(const u8 *encodeStr, u8 *outKey, u8 *outBssid, u8 *outSsid, u8 *outCustData);
extern void tls_oneshot_recv_err(void);

extern void tls_wl_plcp_cb_register(tls_wifi_data_recv_callback callback);
extern void tls_wl_plcp_stop(void);
extern void tls_wl_plcp_start(void);


#if TLS_CONFIG_BLE_WIFI_ONESHOT
#include "wm_bt_def.h"
extern tls_bt_status_t tls_ble_wifi_cfg_init(void);
extern tls_bt_status_t tls_ble_wifi_cfg_deinit(int reason);
#endif

#if (CONFIG_ONESHOT_MAC_FILTER || TLS_CONFIG_AP_MODE_ONESHOT)
static __inline int tls_is_zero_ether_addr(const u8 *a)
{
	return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}
#endif

void tls_wifi_oneshot_result_cb_register(tls_wifi_oneshot_result_callback callback)
{
    gpfResult = callback;

    return;
}

void tls_wifi_get_oneshot_ssidpwd(u8 *ssid, u8 *pwd)
{
	if (ssid && (gucssidData[0] != '\0')){
		strcpy((char *)ssid, (char *)gucssidData);
	}

	if (pwd && (gucpwdData[0] != '\0')){
		strcpy((char *)pwd, (char *)gucpwdData);
	}
}

void tls_wifi_get_oneshot_customdata(u8 *data)
{
	if (guconeshotflag){
	  	if (data && (gucCustomData[0][0] != '\0')){
	  		strcpy((char *)data, (char *)gucCustomData[0]);
	  	}
	}else{
		gucCustomData[0][0]  = '\0';
	}
}

void tls_wifi_set_oneshot_customdata(u8 *data)
{
	memset(gucCustomData[0], 0, 65);
	
	strcpy((char *)gucCustomData[0], (char *)data);
	if (gpfResult)
	{
		gpfResult(WM_WIFI_ONESHOT_TYPE_CUSTOMDATA);	
	}
}


void tls_wifi_wait_disconnect(void)
{
//#if !CONFIG_UDP_ONE_SHOT
	struct tls_ethif *netif = NULL;

	netif = tls_netif_get_ethif();
	if (netif && (1 == netif->status)){
		tls_wifi_disconnect();
	}

	for(;;){
		netif = tls_netif_get_ethif();
		if (netif && (0 == netif->status)){
			tls_os_time_delay(50);
			break;
		}
		tls_os_time_delay(10);
	}
	//tls_os_time_delay(210);
//#endif
}

u8 tls_wifi_oneshot_connect_by_ssid_bssid(u8 *ssid, u8 *bssid, u8 *pwd)
{
    if (gpfResult)
        gpfResult(WM_WIFI_ONESHOT_TYPE_SSIDPWD);
#if TLS_CONFIG_AP_MODE_ONESHOT	
	if((2 == guconeshotflag)||(3 == guconeshotflag))
	{
		u8 wireless_protocol = IEEE80211_MODE_INFRA;

		tls_wifi_softap_destroy();
		tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void*) &wireless_protocol, TRUE);
	}
	else
#endif	
	{	
		tls_netif_add_status_event(wm_oneshot_netif_status_event);
	}

	tls_wifi_set_oneshot_flag(0);

#if TLS_CONFIG_UDP_ONE_SHOT
	if (1 == guconeshotflag)
		tls_os_time_delay(TLS_ONESHOT_SWITCH_TIMER_MAX);
#endif


	return tls_wifi_connect_by_ssid_bssid(ssid, strlen((char *)ssid), bssid, pwd, (pwd == NULL) ? 0 : strlen((char *)pwd));
}
u8 tls_wifi_oneshot_connect_by_bssid(u8 *bssid, u8 *pwd)
{

    if (gpfResult)
        gpfResult(WM_WIFI_ONESHOT_TYPE_SSIDPWD);

#if TLS_CONFIG_AP_MODE_ONESHOT
	if((2 == guconeshotflag)||(3 == guconeshotflag))
	{
		u8 wireless_protocol = IEEE80211_MODE_INFRA;

		tls_wifi_softap_destroy();
		tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void*) &wireless_protocol, TRUE);
	}else
#endif
	{
		tls_netif_add_status_event(wm_oneshot_netif_status_event);
	}

	tls_wifi_set_oneshot_flag(0);

#if TLS_CONFIG_UDP_ONE_SHOT
	if (1 == guconeshotflag)
		tls_os_time_delay(TLS_ONESHOT_SWITCH_TIMER_MAX);
#endif

	return tls_wifi_connect_by_bssid(bssid, pwd, (pwd == NULL) ? 0 : strlen((char *)pwd));
}

u8 tls_wifi_oneshot_connect(u8 *ssid, u8 *pwd)
{
    if (gpfResult)
        gpfResult(WM_WIFI_ONESHOT_TYPE_SSIDPWD);

#if TLS_CONFIG_AP_MODE_ONESHOT
	if((2 == guconeshotflag)||(3 == guconeshotflag))
	{
		u8 wireless_protocol = IEEE80211_MODE_INFRA;

		tls_wifi_softap_destroy();
		tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void*) &wireless_protocol, TRUE);

		
	}else
#endif	
	{
		tls_netif_add_status_event(wm_oneshot_netif_status_event);
	}

	tls_wifi_set_oneshot_flag(0);

#if TLS_CONFIG_UDP_ONE_SHOT
	if (1 == guconeshotflag)
		tls_os_time_delay(TLS_ONESHOT_SWITCH_TIMER_MAX);
#endif

	return tls_wifi_connect(ssid, strlen((char *)ssid), pwd, (pwd==NULL) ? 0 : strlen((char *)pwd));
}


#if TLS_CONFIG_AP_MODE_ONESHOT
void tls_wifi_send_oneshotinfo(const u8 * ssid,u8 len, u32 send_cnt)
{
	int i = 0;
	int j = 0;
	u8 lenNum =0;
	u8 lenremain = 0;
	if (gaucRawData == NULL){
		gaucRawData = tls_mem_alloc(len+1);
	}

	if (gaucRawData){
		memcpy(gaucRawData, ssid, len);
		lenNum = len/APSKT_SSID_MAX_LEN;
		lenremain = len%APSKT_SSID_MAX_LEN;
		for (j = 0; j< send_cnt; j++){
			for (i = 0; i < lenNum; i++){
				tls_wifi_send_oneshotdata(NULL, (const u8 *)(&(gaucRawData[i*APSKT_SSID_MAX_LEN])), APSKT_SSID_MAX_LEN);
				tls_os_time_delay(10);
			}
			if (lenremain){
				tls_wifi_send_oneshotdata(NULL, (const u8 *)(&(gaucRawData[i*APSKT_SSID_MAX_LEN])), lenremain);
				tls_os_time_delay(10);
			}
		}
		tls_mem_free(gaucRawData);
		gaucRawData = NULL;
	}
}
#endif

u8 tls_wifi_decrypt_data(u8 *data){
	u16 datatype;
	u32 tagid = 0;
	u16 typelen[6]={0,0,0,0,0,0};
	volatile u16 rawlen = 0;
    u16 hdrlen = sizeof(struct ieee80211_hdr);
	int i = 0;
	int tmpLen = 0;
	u8 ret = 0;
	//u8 ucChanId = 0;


	//ucChanId = *(u16*)(data+hdrlen+4);/*Channel ID*/
	tagid = *(u16*)(data+hdrlen+6);/*TAG*/
	if (0xA55A == tagid){
		datatype = *(u16 *)(data+hdrlen+8); /*DataType*/
		tmpLen = hdrlen + 10;
		for (i = 0; i < 6; i++){
			if ((datatype>>i)&0x1){
				typelen[i] = *((u16*)(data+tmpLen));
				tmpLen += 2;
			}

		}
		rawlen = *((u16 *)(data+tmpLen));
		tmpLen += 2;

		gucssidokflag = 0;
		gucbssidokflag = 0;
		gucpwdokflag = 0;
		memset(gucssidData, 0, 33);
		memset(gucbssidData, 0, 6);
		memset(gucpwdData, 0, 65);
		for (i = 0; i < 6; i++){
			if ((datatype>>i)&0x1){
				if (i == 0){ /*PWD*/
					strncpy((char *)gucpwdData,(char *)(data+tmpLen), typelen[i]);
					ONESHOT_DBG("PWD:%s\n", gucpwdData);
					gucpwdokflag = 1;
					ret = 1;
				}else if (i == 1){/*BSSID*/
					memcpy((char *)gucbssidData,(char *)(data+tmpLen), typelen[i]);
					ONESHOT_DBG("gucbssidData:%x:%x:%x:%x:%x:%x\n", MAC2STR(gucbssidData));
					gucbssidokflag = 1;
					ret = 1;
				}else if (i == 2){/*SSID*/
					gucssidData[0] = '\0';
					memcpy((char *)gucssidData,(char *)(data+tmpLen), typelen[i]);
					ONESHOT_DBG("gucssidData:%s\r\n", gucssidData);
					gucssidokflag = 1;
					ret = 1;
				}else{/*3-5 USER DEF*/
					memcpy((char *)gucCustomData[i - 3], (char *)(data+tmpLen), typelen[i]);
					gucCustomData[i - 3][typelen[i]] = '\0';
					ret = 0;
					if (gpfResult)
					{
						gpfResult(WM_WIFI_ONESHOT_TYPE_CUSTOMDATA);
						tls_wifi_set_oneshot_flag(0);
				     }
				}
				tmpLen += typelen[i];
			}
		}
		if(2 == guconeshotflag)
		{
#if TLS_CONFIG_AP_MODE_ONESHOT
			if (ret && rawlen&&(gucRawValid==0)){
				gucRawValid = 1;
				tls_wifi_send_oneshotinfo((const u8 *)(data+tmpLen), rawlen, APSKT_MAX_ONESHOT_NUM);
			}
#endif
		}
	}
	return ret;
}


#if TLS_CONFIG_UDP_ONE_SHOT
static u8 *oneshot_bss = NULL;
#define ONESHOT_BSS_SIZE 4096

void tls_wifi_clear_oneshot_data(u8 iscleardata)
{

	gucbssidokflag = 0;
	gucssidokflag = 0;
	gucpwdokflag = 0;
}

#if CONFIG_ONESHOT_MAC_FILTER
//User should define the source mac address purposely.
static u8 gauSrcmac[ETH_ALEN]= {0xC4,0x07,0x2F,0x04,0x7A,0x69};
void tls_filter_module_srcmac_show(void){
	printf("num:%d\n", sizeof(gauSrcmac)/ETH_ALEN);
}

//only receive info from devices whose mac address is gauSrcmac
int tls_filter_module_srcmac(u8 *mac){
	int ret = 0;
	u8 localmac[6];

	if (0 == tls_is_zero_ether_addr(gauSrcmac)){
		tls_get_mac_addr((u8 *)(&localmac));
		if ((0 == memcmp(gauSrcmac, mac, ETH_ALEN))&&(0 != memcmp(localmac, mac, ETH_ALEN))){
			ret = 1;
			//break;
		}
	}else{
		ret = 1;
	}

	return ret;
}
#endif

static void wifi_change_chanel(u32 chanid, u8  bandwidth)
{
	tls_wl_change_channel_info(chanid, bandwidth);
}

#if TLS_CONFIG_UDP_LSD_SPECIAL
static void oneshot_lsd_finish(void)
{
	if (lsd_param)
	{
		printf("lsd connect, ssid:%s, pwd:%s, time:%d\n", lsd_param->ssid, lsd_param->pwd, (tls_os_get_time()-oneshottime)*1000/HZ);
		
	    tls_netif_add_status_event(wm_oneshot_netif_status_event);
	    if (tls_oneshot_if_use_bssid(lsd_param->ssid, &lsd_param->ssid_len, lsd_param->bssid))
	    {
			ONESHOT_DBG("connect_by_ssid_bssid\n");
			memcpy(gucssidData, lsd_param->ssid, lsd_param->ssid_len);
			gucssidData[lsd_param->ssid_len] = '\0';
			memcpy(gucbssidData, lsd_param->bssid, 6);
			memcpy(gucpwdData, lsd_param->pwd, lsd_param->pwd_len);	
			gucpwdData[lsd_param->pwd_len] = '\0';
			tls_wifi_set_oneshot_flag(0);
			tls_wifi_connect_by_ssid_bssid(gucssidData, strlen((char *)gucssidData), gucbssidData, gucpwdData, strlen((char *)gucpwdData));
	    }
	    else
	    {
			ONESHOT_DBG("connect_by_ssid\n");
			memcpy(gucssidData, lsd_param->ssid, lsd_param->ssid_len);
			gucssidData[lsd_param->ssid_len] = '\0';
			memcpy(gucpwdData, lsd_param->pwd, lsd_param->pwd_len);	
			gucpwdData[lsd_param->pwd_len] = '\0';		
			tls_wifi_set_oneshot_flag(0);
			tls_wifi_connect(gucssidData, strlen((char *)gucssidData), gucpwdData, strlen((char *)gucpwdData));
	    }
	}
}

int tls_wifi_lsd_oneshot_special(u8 *data, u16 data_len)
{
	int ret;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr*)data;

	ret = tls_lsd_recv(data, data_len);	
	if(ret == LSD_ONESHOT_ERR)
	{
		ONESHOT_DBG("lsd oneshot err, ssid or pwd len err\n");
		tls_oneshot_recv_err();
	}
	else if(ret == LSD_ONESHOT_CHAN_TEMP_LOCKED)
	{	
		ONESHOT_DBG("LSD_ONESHOT_CHAN_TEMP_LOCKED:%d\r\n", tls_os_get_time());
		tls_oneshot_switch_channel_tim_temp_stop();
	}
	else if(ret == LSD_ONESHOT_CHAN_LOCKED_BW20)
	{
		ONESHOT_DBG("LSD_ONESHOT_CHAN_LOCKED_BW20:%d,%x\r\n", tls_os_get_time(), hdr->duration_id);
		tls_oneshot_switch_channel_tim_stop((struct ieee80211_hdr *)data);
	}
	else if(ret == LSD_ONESHOT_CHAN_LOCKED_BW40)
	{
		hdr->duration_id |= 0x0001;			////force change to bw40
		tls_oneshot_switch_channel_tim_stop((struct ieee80211_hdr *)data);
	}
	else if(ret == LSD_ONESHOT_COMPLETE)
	{
		if (lsd_param)
		{
			if(lsd_param->user_len > 0)
			{
				tls_wifi_set_oneshot_customdata(lsd_param->user_data);

			}
			else if(lsd_param->ssid_len > 0)
			{
				oneshot_lsd_finish();
			}
		}
	}
	return 0;
}
#endif

/*END CONFIG_UDP_ONE_SHOT*/
#endif
#if TLS_CONFIG_AP_MODE_ONESHOT
int soft_ap_create(void)
{
	struct tls_softap_info_t apinfo;
	struct tls_ip_info_t ipinfo;
	u8 ret=0;
	u8 ssid_set = 0;
	char ssid[33];
	u8 mac_addr[6];

    tls_get_mac_addr(mac_addr);
    ssid[0]='\0';
    u8 ssid_len = sprintf(ssid, "%s_%02x%02x", ONESHOT_AP_NAME, mac_addr[4], mac_addr[5]);


	tls_param_get(TLS_PARAM_ID_BRDSSID, (void *)&ssid_set, (bool)0);
	if (0 == ssid_set)
	{
		ssid_set = 1;
		tls_param_set(TLS_PARAM_ID_BRDSSID, (void *)&ssid_set, (bool)1); /*Set BSSID broadcast flag*/
	}
	memset(&apinfo, 0, sizeof(struct tls_softap_info_t));
	MEMCPY(apinfo.ssid, ssid, ssid_len);
	apinfo.ssid[ssid_len]='\0';

	apinfo.encrypt = 0;  /*0:open, 1:wep64, 2:wep128*/
	apinfo.channel = 5; /*channel random*/
	/*ip information: ip address?ê?netmask?ê?dns*/
	ipinfo.ip_addr[0] = 192;
	ipinfo.ip_addr[1] = 168;
	ipinfo.ip_addr[2] = 1;
	ipinfo.ip_addr[3] = 1;
	ipinfo.netmask[0] = 255;
	ipinfo.netmask[1] = 255;
	ipinfo.netmask[2] = 255;
	ipinfo.netmask[3] = 0;
	MEMCPY(ipinfo.dnsname, "local.wm", sizeof("local.wm"));
	ret = tls_wifi_softap_create((struct tls_softap_info_t* )&apinfo, (struct tls_ip_info_t* )&ipinfo);
	//printf("\n ap create %s ! \n", (ret == WM_SUCCESS)? "Successfully" : "Error");

	return ret;
}
#if TLS_CONFIG_SOCKET_MODE
err_t  socket_recive_cb(u8 skt_num, struct pbuf *p, err_t err)
{
	int len = p->tot_len;
	int datalen = 0;
	char *pStr = NULL;
	char *pEnd;
	char *LenStr = NULL;
	int ret  = 0;
    //printf("socket recive data\n");
	if (0 == gucRawValid){
		gucRawValid = 1;
	    if(p->tot_len > SOCKET_RX_DATA_BUFF_LEN)
	    {
	    	len = SOCKET_RX_DATA_BUFF_LEN;
	    }
		pStr = tls_mem_alloc(len+1);
		if (pStr){
		    pbuf_copy_partial(p, pStr, len, 0);
			//printf("pStr:%s\n", pStr);
			pEnd = strstr(pStr, "\r\n");
			if (pEnd){
				datalen = pEnd - pStr;
				LenStr = tls_mem_alloc(datalen+1);
				memcpy(LenStr, pStr, datalen);
				LenStr[datalen] = '\0';
				ret = strtodec(&datalen,LenStr);
				tls_mem_free(LenStr);
				LenStr = NULL;
				if (ret == 0){
					//printf("trans datalen:%d\n", datalen);
					strncpy(sock_rx->sock_rx_data, pEnd + 2, datalen);
					sock_rx->sock_rx_data[datalen] = '\0';
					pEnd = NULL;
				    sock_rx->sock_data_len = datalen;
				   // printf("\nsock recive data = %s\n",sock_rx->sock_rx_data);
				   if (oneshot_msg_q)
				   {
				       tls_os_queue_send(oneshot_msg_q, (void *)AP_SOCK_S_MSG_SOCKET_RECEIVE_DATA, 0);
				   }
				}
	   		}
			tls_mem_free(pStr);
			pStr = NULL;
		}
	    if (p){
	       pbuf_free(p);
	    }
	}
    return ERR_OK;
}

int create_tcp_server_socket(void)
{
    skt_descp = (struct tls_socket_desc *)tls_mem_alloc(sizeof(struct tls_socket_desc));
    if(skt_descp == NULL)
    {
        return -1;
    }
    memset(skt_descp, 0, sizeof(struct tls_socket_desc));

    sock_rx = (ST_Sock_Recive *)tls_mem_alloc(sizeof(ST_Sock_Recive));
    if(sock_rx == NULL)
    {
        tls_mem_free(skt_descp);
        skt_descp = NULL;
        return -1;
    }
    memset(sock_rx, 0, sizeof(ST_Sock_Recive));

    sock_rx->sock_rx_data = tls_mem_alloc(SOCKET_RX_DATA_BUFF_LEN*sizeof(char));
    if(sock_rx->sock_rx_data == NULL)
    {
        tls_mem_free(sock_rx);
        tls_mem_free(skt_descp);
        sock_rx = NULL;
        skt_descp = NULL;
        return -1;
    }
    memset(sock_rx->sock_rx_data, 0, sizeof(255*sizeof(char)));

	skt_descp->protocol = SOCKET_PROTO_TCP;
	skt_descp->cs_mode = SOCKET_CS_MODE_SERVER;
	skt_descp->port = SOCKET_SERVER_PORT;
    skt_descp->recvf = socket_recive_cb;
	sock_rx->socket_num = tls_socket_create(skt_descp);
	//printf("sck_num =??%d\n",sock_rx->socket_num);
    return WM_SUCCESS;
}

void free_socket(void)
{
	if (sock_rx == NULL){
		return;
	}
	if (sock_rx->socket_num == 0){
		return ;
	}
    tls_socket_close(sock_rx->socket_num);
	sock_rx->socket_num = 0;
    if(NULL != skt_descp)
    {
        tls_mem_free(skt_descp);
        skt_descp = NULL;
    }

    if(NULL != sock_rx->sock_rx_data)
    {
        tls_mem_free(sock_rx->sock_rx_data);
        sock_rx->sock_rx_data = NULL;
		sock_rx->sock_data_len = 0;
    }

        tls_mem_free(sock_rx);
        sock_rx = NULL;
}
#endif
#endif

//need call after scan
void tls_oneshot_callback_start(void)
{
#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
 	tls_airkiss_start();
#endif

#if TLS_CONFIG_UDP_LSD_SPECIAL
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL
	tls_wl_plcp_stop();
#endif

#if LSD_ONESHOT_DEBUG
	lsd_printf = printf;
#endif
	tls_lsd_init(oneshot_bss);
#endif
}


u8 tls_wifi_dataframe_recv(struct ieee80211_hdr *hdr, u32 data_len)
{
	if (tls_wifi_get_oneshot_flag()== 0){
		return 1;
	}

    //only receive data frame
	if (0 == ieee80211_is_data(hdr->frame_control)){
		return 1;
	}
#if TLS_CONFIG_UDP_ONE_SHOT
	tls_os_sem_acquire(gWifiRecvSem, 0);
#endif

#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
    tls_airkiss_recv((u8 *)hdr, data_len);
#endif

#if TLS_CONFIG_UDP_LSD_SPECIAL
	tls_wifi_lsd_oneshot_special((u8 *)hdr, data_len);
#endif

#if TLS_CONFIG_UDP_ONE_SHOT    
	tls_os_sem_release(gWifiRecvSem);
#endif
	return 1;
}


void tls_oneshot_stop_clear_data(void)
{
#if TLS_CONFIG_UDP_ONE_SHOT
    {
        if (gWifiSwitchChanTim)
        {
            tls_os_timer_stop(gWifiSwitchChanTim);
        }
        if (gWifiHandShakeTimOut)
        {
            tls_os_timer_stop(gWifiHandShakeTimOut);
        }
        if (gWifiRecvTimOut)
        {
            tls_os_timer_stop(gWifiRecvTimOut);
        }
    }

	if (oneshot_bss){
		tls_mem_free(oneshot_bss);
		oneshot_bss = NULL;
	}
	
		uctotalchannum = 0;
		memset(airwifichan, 0xF, TOTAL_CHAN_NUM);
		memset(airchantype, 0x0, TOTAL_CHAN_NUM);

	guchandshakeflag = 0;

	memset(gSrcMac, 0, ETH_ALEN);
	tls_wifi_clear_oneshot_data(1);

	tls_lsd_deinit();
#endif

 	gucssidokflag = 0;
	gucbssidokflag = 0;
	gucpwdokflag = 0;

#if TLS_CONFIG_AP_MODE_ONESHOT
#if TLS_CONFIG_SOCKET_MODE
//	if(2 == guconeshotflag)
	{
		free_socket();
	}
#endif
#endif	

	tls_wifi_data_recv_cb_register(NULL);
	tls_wifi_scan_result_cb_register(NULL);
	tls_wl_plcp_cb_register(NULL);
#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
    tls_airkiss_stop();
#endif

}

void tls_oneshot_init_data(void)
{
	gucssidokflag = 0;
	gucbssidokflag = 0;
	gucpwdokflag = 0;
	memset(gucssidData, 0, 33);
	memset(gucbssidData, 0, 6);
	memset(gucpwdData, 0, 65);

#if TLS_CONFIG_UDP_ONE_SHOT
	guchandshakeflag = 0;
	uctotalchannum = 0;
	memset(airwifichan, 0xF, TOTAL_CHAN_NUM);
	memset(airchantype, 0x0, TOTAL_CHAN_NUM);		

	memset(gSrcMac, 0, ETH_ALEN);

	tls_wifi_clear_oneshot_data(1);
#endif	
}

#if TLS_CONFIG_UDP_ONE_SHOT
void tls_oneshot_scan_result_cb(void)
{
	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_SCAN_FINISHED, 0);
	}
}
void tls_oneshot_scan_start(void)
{
	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_SCAN_START, 0);
	}
}
void tls_oneshot_scan_result_deal(void)
{
	int i = 0, j = 0;
	struct tls_scan_bss_t *bss = NULL;
	static u16 lastchanmap = 0;
	lastchanmap = 0; /*clear map*/
	uctotalchannum = 0;	
    /*scan chan to cfm chan switch*/
	if (NULL == oneshot_bss)
	{
		oneshot_bss = tls_mem_alloc(ONESHOT_BSS_SIZE);
	}else{
		memset(oneshot_bss, 0, sizeof(ONESHOT_BSS_SIZE));
	}


	if (oneshot_bss)
	{
		tls_wifi_get_scan_rslt(oneshot_bss, ONESHOT_BSS_SIZE);
		bss = (struct tls_scan_bss_t *)oneshot_bss;
		for (j = 1; j < 15; j++)
		{
			for (i = 0;i < bss->count; i++)
			{
				if ((((lastchanmap>>(j-1))&0x1)==0)&&(j == bss->bss[i].channel))
				{
					lastchanmap |= 1<<(j-1);
					if (j < 5)
					{
						airwifichan[uctotalchannum] = j-1;
						airchantype[uctotalchannum] = 3;
						uctotalchannum++;
					}else if (j < 8)
					{
						airwifichan[uctotalchannum] = j-1;
						airchantype[uctotalchannum] = 3;	
						uctotalchannum++;
						airwifichan[uctotalchannum] = j-1;
						airchantype[uctotalchannum] = 2;						
						uctotalchannum++;
					}else if (j < 14){
						airwifichan[uctotalchannum] = j-1;
						airchantype[uctotalchannum] = 2;	
						uctotalchannum++;
					}else{
						airwifichan[uctotalchannum] = j-1;
						airchantype[uctotalchannum] = 0;	
						uctotalchannum++;
					}
					break;
				}
			}
		}
	}
	
	if ((uctotalchannum == 0) || (scanChanErr == 1))
	{
		uctotalchannum = 0;
		for (i = 0 ; i < 14; i++)
		{
			if (i < 4)
			{
				airwifichan[uctotalchannum] = i;
				airchantype[uctotalchannum] = 3;
				uctotalchannum++;
			}else if (i < 7)
			{
				airwifichan[uctotalchannum] = i;
				airchantype[uctotalchannum] = 3;	
				uctotalchannum++;
				airwifichan[uctotalchannum] = i;
				airchantype[uctotalchannum] = 2;						
				uctotalchannum++;
			}else if (i < 13){
				airwifichan[uctotalchannum] = i;
				airchantype[uctotalchannum] = 2;	
				uctotalchannum++;
			}else{
				airwifichan[uctotalchannum] = i;
				airchantype[uctotalchannum] = 0;	
				uctotalchannum++;
			}
		}
	}	

}


static void tls_find_ssid_nonascII_pos_and_count(u8 *ssid, u8 ssid_len, u8 *nonascii_cnt)
{
    int i = 0;
    int cnt = 0;

    if (ssid == NULL)
    {
    	return;
    }

    for (i = 0; i < ssid_len; i++)
    {
        if ( ssid[i] >= 0x80 )
        {
            cnt++;
        }
    }

    if (nonascii_cnt)
    {   
        *nonascii_cnt = cnt;
    }
}


/**
 * @brief          handle if use bssid to connect wifi.
 *
 * @param[in]      *ssid      : ap name to connect
 * @param[in]      *ssid_len: ap name's length to connect
 * @param[in]      *bssid    : ap bssid
 *
 * @retval         no mean
 *
 * @note           None
 */
int tls_oneshot_if_use_bssid(u8 *ssid, u8 *ssid_len, u8 *bssid)
{
    int i = 0;
    u8  bssidmatch = 0;
    u8  cfgssid_not_ascii_cnt = 0;	
	u8  copyflag = 0;
    struct tls_scan_bss_t *bss = NULL;

	if (0 == *ssid_len)
	{
		return 1;
	}

    if (oneshot_bss)
    {
        bss = (struct tls_scan_bss_t *)oneshot_bss;
        tls_find_ssid_nonascII_pos_and_count(ssid, *ssid_len , &cfgssid_not_ascii_cnt);
        if (cfgssid_not_ascii_cnt) 
        {
            for (i = 0; i < bss->count; i++)
            {
            	if (memcmp(bss->bss[i].bssid, bssid, ETH_ALEN) == 0) /*Find match bssid for ssid not match*/
            	{    
            	    bssidmatch = 1;
            	    break;
            	}
            }
            
            if (bssidmatch)  /*For bssid match and non-zero len ssid, update ssid info*/
            {
            	if (bss->bss[i].ssid_len && (bss->bss[i].ssid_len <= 32))
            	{
            		copyflag = *ssid_len != bss->bss[i].ssid_len ? 1: 0;
            		if (copyflag)
            		{
            			printf("org ssid info[%d]:%s\n", *ssid_len, ssid);
		            	MEMCPY(ssid, bss->bss[i].ssid, bss->bss[i].ssid_len);
		            	*(ssid + bss->bss[i].ssid_len) = '\0';
						*ssid_len = bss->bss[i].ssid_len;
            		}
	            	return 0;
            	}
				else
            	{
            		return 1; /*only ssid can not get, use bssid*/
            	}
            }
        }
    }
    return 0;
}

#if AIRKISS_USE_SELF_WRITE
extern u8 get_crc_8(u8 *ptr, u32 len);
u8 tls_oneshot_is_ssid_crc_match(u8 crc, u8 *ssid, u8 *ssid_len)
{
    int i = 0;
    struct tls_scan_bss_t *bss = NULL;

    if (oneshot_bss)
    {
        bss = (struct tls_scan_bss_t*)oneshot_bss;
        for (i = 0; i < bss->count; i++)
        {
            if ((crc == get_crc_8(bss->bss[i].ssid, bss->bss[i].ssid_len))
            	&& (*ssid_len ==  bss->bss[i].ssid_len))
            {
            	MEMCPY(ssid, bss->bss[i].ssid, bss->bss[i].ssid_len);
            	*(ssid + bss->bss[i].ssid_len) = '\0';
//              *ssid_len = bss->bss[i].ssid_len;
            	return 1;
            }
        }
    }
    return 0;
}
#endif
void tls_oneshot_find_chlist(u8 *ssid, u8 ssid_len, u16 *chlist)
{
    int i = 0;
    struct tls_scan_bss_t *bss = NULL;

    if (oneshot_bss)
    {
        bss = (struct tls_scan_bss_t*)oneshot_bss;
        for (i = 0; i < bss->count; i++)
        {
            if ((ssid_len == bss->bss[i].ssid_len) && (memcmp(bss->bss[i].ssid, ssid, ssid_len) == 0))
            {
                *chlist |= 1<<(bss->bss[i].channel -1);
            }
        }
    }
}

void tls_oneshot_switch_channel_tim_start(void *ptmr, void *parg)
{
	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_SWITCH_CHANNEL, 0);
	}
}

int tls_oneshot_find_ch_by_bssid(u8 *bssid)
{
    int i = 0;
    struct tls_scan_bss_t *bss = NULL;

    if (oneshot_bss)
    {
        bss = (struct tls_scan_bss_t*)oneshot_bss;
        for (i = 0; i < bss->count; i++)
        {
            if ((memcmp(bss->bss[i].bssid, bssid, ETH_ALEN) == 0))
            {
                return bss->bss[i].channel - 1;
            }
        }
    }
    return -1;
}


void tls_oneshot_switch_channel_tim_stop(struct ieee80211_hdr *hdr)
{
	int ch;

	if (gWifiSwitchChanTim)
	{
		tls_os_timer_stop(gWifiSwitchChanTim);
	}

	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_STOP_CHAN_SWITCH, 0);
	}
	if (ieee80211_has_tods(hdr->frame_control))
	{
		 ch = tls_oneshot_find_ch_by_bssid(hdr->addr1);
	}
	else
	{
		 ch = tls_oneshot_find_ch_by_bssid(hdr->addr2);
	}
	if (((hdr->duration_id&0x01) == 0) && (ch >= 0))
	{
		ONESHOT_DBG("change to BW20 ch:%d\n", ch);
		tls_wifi_change_chanel(ch);
	}
	else if(hdr->duration_id == 0)
	{
		ONESHOT_DBG("special frame!!!!!!!!!!!!!!\n");
	}	
}

void tls_oneshot_switch_channel_tim_temp_stop(void)
{
	if (gWifiSwitchChanTim)
	{
		tls_os_timer_stop(gWifiSwitchChanTim);
	}
	if (oneshot_msg_q)
	{	
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_STOP_TMP_CHAN_SWITCH, 0);
	}
}

void tls_oneshot_handshake_timeout(void *ptmr, void *parg)
{
	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_HANDSHAKE_TIMEOUT, 0);
	}
}

void tls_oneshot_recv_timeout(void *ptmr, void *parg)
{
	if (oneshot_msg_q)
	{
	    tls_os_queue_send(oneshot_msg_q, (void*)ONESHOT_RECV_TIMEOUT, 0);
	}
}
#endif
void tls_oneshot_data_clear(void)
{
	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_STOP_DATA_CLEAR, 0);
	}
}


void tls_oneshot_recv_err(void)
{
	if (oneshot_msg_q && (0 == gucOneshotErr))
	{
        gucOneshotErr = 1;
		tls_os_queue_send(oneshot_msg_q, (void *)ONESHOT_RECV_ERR, 0);
	}
}


#if TLS_CONFIG_WEB_SERVER_MODE
void tls_oneshot_send_web_connect_msg(void)
{
	if (oneshot_msg_q)
	{
		tls_os_queue_send(oneshot_msg_q, (void *)AP_WEB_S_MSG_RECEIVE_DATA, 0);
	}
}
#endif
void wm_oneshot_netif_status_event(u8 status )
{

	if (oneshot_msg_q)
	{
		switch(status)
		{
			case NETIF_IP_NET2_UP:
				tls_os_queue_send(oneshot_msg_q, (void *)AP_SOCK_S_MSG_SOCKET_CREATE, 0);
				break;

			case NETIF_WIFI_SOFTAP_FAILED:
				tls_os_queue_send(oneshot_msg_q, (void *)AP_SOCK_S_MSG_WJOIN_FAILD, 0);
				break;

			case NETIF_IP_NET_UP:
				tls_os_queue_send(oneshot_msg_q,(void *)ONESHOT_NET_UP,0);
				break;
				
			default:
				break;
		}
	}

}
#if TLS_CONFIG_SOCKET_RAW
void wm_oneshot_send_mac(void)
{
	int idx;
	int socket_num = 0;
	u8 mac_addr[8];
	struct tls_socket_desc socket_desc = {SOCKET_CS_MODE_CLIENT};
	socket_desc.cs_mode = SOCKET_CS_MODE_CLIENT;
	socket_desc.protocol = SOCKET_PROTO_UDP;
	IP_ADDR4(&socket_desc.ip_addr, 255, 255, 255, 255);
	socket_desc.port = 65534;
	socket_num = tls_socket_create(&socket_desc);
	memset(mac_addr,0,sizeof(mac_addr));
	tls_get_mac_addr(mac_addr);
	tls_os_time_delay(50);
	for(idx = 0;idx < 50;idx ++)
	{
		if (tls_wifi_get_oneshot_flag())
		{
			break;
		}
		tls_socket_send(socket_num,mac_addr, 6);
		tls_os_time_delay(50);
	}
	tls_socket_close(socket_num);
	socket_num = 0;
}
#else
void wm_oneshot_send_mac(void)
{
	int idx;
	int sock = 0;
	u8 mac_addr[8];
	struct sockaddr_in sock_addr;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0)
	{
		return;
	}
	memset(&sock_addr, 0, sizeof(struct sockaddr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = IPADDR_BROADCAST;
	sock_addr.sin_port = htons(65534);

	memset(mac_addr,0,sizeof(mac_addr));
	tls_get_mac_addr(mac_addr);
	tls_os_time_delay(50);
	for(idx = 0;idx < 50;idx ++)
	{
		if (tls_wifi_get_oneshot_flag())
		{
			break;
		}
		sendto(sock, mac_addr, 6, 0, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr));
		tls_os_time_delay(50);
	}
	closesocket(sock);
}
#endif


void tls_oneshot_task_handle(void *arg)
{
    void *msg;
#if TLS_CONFIG_UDP_ONE_SHOT
    static int chanCnt = 0;
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL
    static int chanRepeat = 0;
#endif
#endif
    for(;;)
    {
        tls_os_queue_receive(oneshot_msg_q, (void **)&msg, 0, 0);
        switch((u32)msg)
        {
#if TLS_CONFIG_UDP_ONE_SHOT
            case ONESHOT_SCAN_START:
			gchanLock = 0;
			scanChanErr = 0;
	    	tls_wifi_scan_result_cb_register(tls_oneshot_scan_result_cb);
	    	if(WM_SUCCESS != tls_wifi_scan())
	    	{
	    	    tls_os_time_delay(3*HZ);
			    tls_oneshot_scan_result_cb();
				scanChanErr = 1;
				break;
	    	}
            break;
			
            case ONESHOT_SCAN_FINISHED:
			gchanLock = 0;
		    tls_oneshot_scan_result_deal();

            chanCnt = 0;
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL			
			chanRepeat = 0;
			g_oneshot_dur_time = tls_os_get_time();
#endif
            wifi_change_chanel(airwifichan[chanCnt], airchantype[chanCnt]);
			
			tls_oneshot_callback_start();

            tls_wifi_data_recv_cb_register((tls_wifi_data_recv_callback)tls_wifi_dataframe_recv);
			tls_wl_plcp_cb_register((tls_wifi_data_recv_callback)tls_wifi_dataframe_recv);

            ONESHOT_DBG("scan finished time:%d,%d,%d\n",chanCnt , uctotalchannum,(tls_os_get_time() - oneshottime)*1000/HZ);
            /*start ONESHOT_TIMER_START*/
            if (gWifiSwitchChanTim)
            {
			    tls_os_timer_stop(gWifiSwitchChanTim);
			    tls_os_timer_change(gWifiSwitchChanTim, TLS_ONESHOT_SWITCH_TIMER_MAX);
            }
			
            if (gWifiRecvTimOut)
            {
                tls_os_timer_stop(gWifiRecvTimOut);
                tls_os_timer_change(gWifiRecvTimOut, TLS_ONESHOT_RECV_TIME);
            }
            break;

            case ONESHOT_SWITCH_CHANNEL:
			gchanLock = 0;

			chanCnt ++;

			if (chanCnt >= uctotalchannum)
			{
				chanCnt = 0;		
			}
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL
			if (chanRepeat)
			{
				if((tls_os_get_time() - g_oneshot_dur_time) >= ONESHOT_SPECIAL_DELAY_TIME)
				{
					ONESHOT_DBG("plcp stop\r\n");
					tls_wl_plcp_stop();
					chanRepeat = 0;				
					g_oneshot_dur_time = tls_os_get_time();
				}
				wifi_change_chanel(airwifichan[chanCnt], 0);
				ONESHOT_DBG("@chan:%d,bandwidth:%d,%d\n", airwifichan[chanCnt], 0, tls_os_get_time());
			}
			else
#endif				
			{
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL			
				if((tls_os_get_time() - g_oneshot_dur_time) >= ONESHOT_SPECIAL_DELAY_TIME)
				{
				    ONESHOT_DBG("plcp start\r\n");
					tls_wl_plcp_start();
					chanRepeat = 1;
					g_oneshot_dur_time = tls_os_get_time();
				}
#endif				
				wifi_change_chanel(airwifichan[chanCnt], airchantype[chanCnt]);
				ONESHOT_DBG("chan:%d,bandwidth:%d,%d\n", airwifichan[chanCnt], airchantype[chanCnt], tls_os_get_time());
			}

#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
            tls_oneshot_airkiss_change_channel();
#endif

#if TLS_CONFIG_UDP_LSD_SPECIAL
            tls_lsd_init(oneshot_bss);
#endif

		    if (gWifiSwitchChanTim)
		    {
			    tls_os_timer_stop(gWifiSwitchChanTim);
			    tls_os_timer_change(gWifiSwitchChanTim, TLS_ONESHOT_SWITCH_TIMER_MAX);
		    }
            break;

            case ONESHOT_STOP_TMP_CHAN_SWITCH:
		    {
				ONESHOT_DBG("ONESHOT_STOP_TMP_CHAN_SWITCH:%d\r\n", tls_os_get_time());
				if (gWifiSwitchChanTim)
				{
				    tls_os_timer_stop(gWifiSwitchChanTim);
				}
				if (gWifiHandShakeTimOut && (gchanLock == 0))
				{
				    tls_os_timer_stop(gWifiHandShakeTimOut);
				    tls_os_timer_change(gWifiHandShakeTimOut, TLS_ONESHOT_RESTART_TIME);
				}
            }
            break;	

            case ONESHOT_STOP_CHAN_SWITCH:
			gchanLock = 1;
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL			
			tls_wl_plcp_start();
#endif
			ONESHOT_DBG("stop channel ch:%d time:%d\n",airwifichan[chanCnt], (tls_os_get_time()-oneshottime)*1000/HZ);
		    if (gWifiSwitchChanTim)
		    {
			    tls_os_timer_stop(gWifiSwitchChanTim);
		    }
				
		    if (gWifiHandShakeTimOut)
		    {
		        tls_os_timer_stop(gWifiHandShakeTimOut);
		    }
				
			if (gWifiRecvTimOut)
			{
				tls_os_timer_stop(gWifiRecvTimOut);
				tls_os_timer_change(gWifiRecvTimOut, TLS_ONESHOT_RECV_TIME);
			}			
            break;
            
            case ONESHOT_HANDSHAKE_TIMEOUT:
				gchanLock = 0;
				ONESHOT_DBG("handshake time out:%d\r\n", tls_os_get_time());
                if (gWifiSwitchChanTim)
                {
                    tls_os_timer_stop(gWifiSwitchChanTim);
                    tls_os_timer_change(gWifiSwitchChanTim, TLS_ONESHOT_SWITCH_TIMER_MAX);
                }
           break;

           case ONESHOT_RECV_TIMEOUT:
		   	gchanLock = 0;
           ONESHOT_DBG("timeout to oneshot:%d\n", tls_os_get_time());
#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
           tls_oneshot_airkiss_change_channel();
#endif
           tls_wifi_set_listen_mode(0);
           tls_oneshot_stop_clear_data();
           tls_wifi_set_oneshot_flag(1);
           break;

		   case ONESHOT_RECV_ERR:	
		   	gchanLock = 0;
		   	ONESHOT_DBG("timeout to recv err:%d\n", tls_os_get_time());
#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
           tls_oneshot_airkiss_change_channel();
#endif
           tls_wifi_set_listen_mode(0);
           tls_oneshot_stop_clear_data();
           tls_wifi_set_oneshot_flag(1);		   	
		   break;
#endif     
           case ONESHOT_STOP_DATA_CLEAR:
		   	gchanLock  = 0;
           ONESHOT_DBG("stop oneshot to connect:%d\n", (tls_os_get_time() - oneshottime)*1000/HZ);
           tls_oneshot_stop_clear_data();
           break;
		
           case ONESHOT_NET_UP:
		   	gchanLock = 0;
           printf("oneshot net up, time:%d\n", (tls_os_get_time()-oneshottime)*1000/HZ);
           tls_netif_remove_status_event(wm_oneshot_netif_status_event);
           if (1 == glast_ucOneshotPsFlag)
		   {
#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
				if (is_airkiss)
				{
					oneshot_airkiss_send_reply();
				}else
#endif
				{		
					wm_oneshot_send_mac();
				}
           }

           break;

#if TLS_CONFIG_AP_MODE_ONESHOT
#if TLS_CONFIG_SOCKET_MODE
           case AP_SOCK_S_MSG_SOCKET_RECEIVE_DATA:
           if (2 == guconeshotflag)
           {
               int ret = 0;
               /*Receive data, self processing*/
               gucssidData[0] = '\0';
               memset(gucbssidData, 0, 6);
               ret = tls_wifi_decode_new_oneshot_data((const u8 *)sock_rx->sock_rx_data,gucpwdData, gucbssidData, gucssidData, NULL);
               if (0 == ret){
                   if ((0 == tls_is_zero_ether_addr(gucbssidData))&&(gucssidData[0] == '\0')){
                       gucbssidokflag = 1;
                       gucpwdokflag = 1;
                   }else{
                       gucssidokflag = 1;
                       gucpwdokflag = 1;
                   }
    
                   tls_wifi_send_oneshotinfo((const u8 *)sock_rx->sock_rx_data, sock_rx->sock_data_len, APSKT_MAX_ONESHOT_NUM);
                   if (((1== gucssidokflag)||(1 == gucbssidokflag)) && (1 == gucpwdokflag)){
                       if (gucbssidokflag){
                           ONESHOT_INF("[SOCKB]BSSID:%x:%x:%x:%x:%x:%x\n",  gucbssidData[0],  gucbssidData[1],  gucbssidData[2],  gucbssidData[3],  gucbssidData[4],  gucbssidData[5]);
                           ONESHOT_INF("[SOCKB]PASSWORD:%s\n", gucpwdData);
                           tls_wifi_oneshot_connect_by_bssid(gucbssidData, gucpwdData);
                       }else {
                           ONESHOT_INF("[SOCKS]SSID:%s\n", gucssidData);
                           ONESHOT_INF("[SOCKS]PASSWORD:%s\n", gucpwdData);
                           tls_wifi_oneshot_connect(gucssidData, gucpwdData);
                       }
                   }
               }
               gucRawValid = 0;
           }
           break;
#endif	

#if TLS_CONFIG_WEB_SERVER_MODE
           case AP_WEB_S_MSG_RECEIVE_DATA:        
           if (3 == guconeshotflag)
           {
               tls_os_time_delay(HZ*5);
               tls_webserver_deinit();
            
               ONESHOT_INF("[WEB]SSID:%s\n", gucssidData);
               ONESHOT_INF("[WEB]PASSWORD:%s\n", gucpwdData);
               tls_wifi_oneshot_connect(gucssidData, gucpwdData);
           }
           break;
#endif

           case AP_SOCK_S_MSG_SOCKET_CREATE:
#if TLS_CONFIG_WEB_SERVER_MODE
           if (3 == guconeshotflag)
           {
               tls_webserver_init();
           }
#endif

#if TLS_CONFIG_SOCKET_MODE
           if (2 == guconeshotflag)
           {
               create_tcp_server_socket();
           }
#endif
           break;
#if  TLS_CONFIG_SOCKET_MODE
           case AP_SOCK_S_MSG_WJOIN_FAILD:
           if (2 == guconeshotflag)
           {
               if((sock_rx)&&(sock_rx->socket_num > 0))
               {
                   free_socket();
                   sock_rx->socket_num = 0;
               }
           }
           break;
#endif		
#endif		
           default:
           break;	   
        }
    
    }
}


void tls_oneshot_task_create(void)
{
	tls_os_status_t err = 0;
	if (NULL == oneshot_msg_q)
	{
		tls_os_queue_create(&oneshot_msg_q, ONESHOT_MSG_QUEUE_SIZE);

		OneshotTaskStk = (u32 *)tls_mem_alloc(sizeof(OS_STK)*ONESHOT_TASK_SIZE);
		memset(OneshotTaskStk, 0, sizeof(u32)*ONESHOT_TASK_SIZE);
		if (OneshotTaskStk)
		{
			err = tls_os_task_create(NULL, NULL,
					tls_oneshot_task_handle,
					NULL,
					(void *)OneshotTaskStk, 		 /* 任务栈的起始地址 */
					ONESHOT_TASK_SIZE * sizeof(u32), /* 任务栈的大小	   */
					TLS_ONESHOT_TASK_PRIO,
					0);
			if (err != TLS_OS_SUCCESS)
			{
				tls_os_queue_delete(oneshot_msg_q);
				oneshot_msg_q = NULL;
				tls_mem_free(OneshotTaskStk);
				OneshotTaskStk = NULL;
			}
		}
		else
		{
			if (oneshot_msg_q)
			{
				tls_os_queue_delete(oneshot_msg_q);
				oneshot_msg_q = NULL;
			}
		}
	}
}

void tls_wifi_start_oneshot(void)
{
	tls_oneshot_stop_clear_data();
	tls_oneshot_init_data();
	tls_oneshot_task_create();
	tls_netif_remove_status_event(wm_oneshot_netif_status_event);

	if(1 == guconeshotflag)
	{
#if TLS_CONFIG_UDP_ONE_SHOT	
		if (NULL == gWifiSwitchChanTim){
			tls_os_timer_create(&gWifiSwitchChanTim,tls_oneshot_switch_channel_tim_start, NULL,TLS_ONESHOT_SWITCH_TIMER_MAX,FALSE,NULL);
		}

		if (NULL == gWifiHandShakeTimOut)
		{
			tls_os_timer_create(&gWifiHandShakeTimOut,tls_oneshot_handshake_timeout, NULL,TLS_ONESHOT_RETRY_TIME,FALSE,NULL);
		}
		if (NULL == gWifiRecvTimOut)
		{
		    tls_os_timer_create(&gWifiRecvTimOut, tls_oneshot_recv_timeout, NULL, TLS_ONESHOT_RETRY_TIME, FALSE, NULL);        
		}	
		if(NULL == gWifiRecvSem)	
		{
			tls_os_sem_create(&gWifiRecvSem, 1);
		}

		tls_oneshot_scan_start();
#endif		
	}
	else{
#if TLS_CONFIG_AP_MODE_ONESHOT
		tls_netif_add_status_event(wm_oneshot_netif_status_event);
		soft_ap_create();
#endif
	}

}


/***************************************************************************
* Function: tls_wifi_set_oneshot_flag
*
* Description: This function is used to set oneshot flag.
*
* Input: flag 0: one shot closed
*             1: one shot open
*             2: AP+socket
*             3: AP+WEBSERVER
*             4: bt
*
* Output: None
*
* Return: None
*
* Date : 2014-6-11
****************************************************************************/
int tls_wifi_set_oneshot_flag(u8 flag)
{
    bool enable = FALSE;
#if TLS_CONFIG_BLE_WIFI_ONESHOT
	tls_bt_status_t status;
    if (flag > 4)
#else
    if (flag > 3)
#endif
    {
        printf("net cfg mode not support\n");
        return -1;
    }

	if (0 != flag)
	{
		oneshottime = tls_os_get_time();
		ONESHOT_DBG("wait oneshot[%d] ...\n",oneshottime);

		guconeshotflag = flag;
		glast_ucOneshotPsFlag = flag;
        gucOneshotErr = 0;
		tls_wifi_disconnect();
		tls_wifi_softap_destroy();

        tls_param_get(TLS_PARAM_ID_PSM, &enable, TRUE);
        if (enable)
		{
			gucOneshotPsFlag = 1;
			tls_wifi_set_psflag(0, 0);
			tls_wl_if_ps(1);
		}

		if ((2 == flag) ||(3 == flag)) /*ap mode*/
		{
			tls_wifi_set_listen_mode(0);
			tls_wl_plcp_stop();
		}
#if TLS_CONFIG_BLE_WIFI_ONESHOT
		else if (4 == flag)
		{
			status = tls_ble_wifi_cfg_init();
			if(status != TLS_BT_STATUS_SUCCESS)
			{
			    if (gucOneshotPsFlag)
			    {
                    gucOneshotPsFlag = 0;
    		        tls_wifi_set_psflag(TRUE, FALSE);
    		        tls_wl_if_ps(0);
			    }
				return -1;
			}
			else
			{
                return 0;
			}
		}
#endif
		else /*udp mode*/
		{
			tls_wifi_set_listen_mode(1);
#if TLS_CONFIG_ONESHOT_DELAY_SPECIAL			
#else
			tls_wl_plcp_start();
#endif

		}
		tls_wifi_start_oneshot();
	}
	else
	{
		if((2 == guconeshotflag) ||(3 == guconeshotflag))
		{
#if TLS_CONFIG_AP_MODE_ONESHOT
			if (guconeshotflag)
			{
				u8 wireless_protocol = IEEE80211_MODE_INFRA;		
				tls_wifi_softap_destroy();
				tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void*) &wireless_protocol, TRUE);
			}
#endif
		}
#if TLS_CONFIG_BLE_WIFI_ONESHOT
		else if (4 == guconeshotflag)
		{
			status = tls_ble_wifi_cfg_deinit(1);
		    if (gucOneshotPsFlag)
		    {
                gucOneshotPsFlag = 0;
		        tls_wifi_set_psflag(TRUE, FALSE);
                if (WM_WIFI_DISCONNECTED == tls_wifi_get_state())
		            tls_wl_if_ps(0);
		    }
            guconeshotflag = flag;
            if(status != TLS_BT_STATUS_SUCCESS && status != 2 /*BLE_HS_EALREADY*/)
            {
            	return -1;
            }
            else
            {
                return 0;
            }
		}
#endif
		guconeshotflag = flag;
		tls_wifi_set_listen_mode(0);
		tls_oneshot_data_clear();
		tls_wl_plcp_stop();
		
		if (gucOneshotPsFlag)
		{
			gucOneshotPsFlag = 0;
			tls_wifi_set_psflag(1, 0);
			tls_wl_if_ps(0);
		}
	}
	return 0;
}

/***************************************************************************
* Function: 	tls_wifi_get_oneshot_flag
*
* Description:  This function is used to get oneshot flag.
*
* Input:  		None
*
* Output: 	    None
*
* Return:       0: oneshot closed
*               1: one shot open
*               2: AP+socket
*               3: AP+WEBSERVER
*               4: bt
*
* Date : 2014-6-11
****************************************************************************/
int tls_wifi_get_oneshot_flag(void)
{
	return guconeshotflag;
}


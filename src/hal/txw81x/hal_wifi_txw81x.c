#ifdef PLATFORM_TXW81X

#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "sys_config.h"
#include "typesdef.h"
#include "lib/common/common.h"
#include "lib/common/sysevt.h"
#include "lib/syscfg/syscfg.h"
#include "syscfg.h"
#include "lib/lmac/lmac.h"
#include "netif/ethernetif.h"
#include "lib/net/eloop/eloop.h"
#include "lib/net/dhcpd/dhcpd.h"

static void (*g_wifiStatusCallback)(int code) = NULL;
static int g_bOpenAccessPointMode = 0;
struct system_status sys_status;
extern u8_t netif_num;

const char* HAL_GetMyIPString()
{
	ip_addr_t ip = lwip_netif_get_ip2("w0");
	return ip4addr_ntoa(&ip);
}

const char* HAL_GetMyGatewayString()
{
	ip_addr_t gw = lwip_netif_get_gw2("w0");
	return ip4addr_ntoa(&gw);
}

const char* HAL_GetMyDNSString()
{
	return "error";
}

const char* HAL_GetMyMaskString()
{
	ip_addr_t mask = lwip_netif_get_netmask2("w0");
	return ip4addr_ntoa(&mask);
}

void WiFI_GetMacAddress(char* mac)
{
	memcpy(mac, (char*)sys_cfgs.mac, sizeof(sys_cfgs.mac));
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
	WiFI_GetMacAddress((char*)mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+\r\n");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |\r\n", g_bOpenAccessPointMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |\r\n", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : ["MACSTR"] %-6s  |\r\n", MAC2STR(mac), "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	struct ieee80211_stainfo stainfo;
	if(!g_bOpenAccessPointMode) ieee80211_conf_get_stainfo(WIFI_MODE_STA, 0, NULL, &stainfo);
	return stainfo.rssi;
}

sysevt_hdl_res sysevt_wifi_event(uint32 event_id, uint32 data, uint32 priv)
{
	uint8 mac[6];
	switch(event_id & 0xffff)
	{
		case SYSEVT_WIFI_CONNECT_START:
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTING);
			}
			break;
		case SYSEVT_WIFI_WRONG_KEY:
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
			}
			break;
		case SYSEVT_WIFI_UNPAIR:
		case SYSEVT_WIFI_STA_DISCONNECT:
		case SYSEVT_WIFI_DISCONNECT:
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
			break;
		case SYSEVT_WIFI_CONNECTTED:
			if(sys_cfgs.wifi_mode == WIFI_MODE_STA)
			{
				if(sys_cfgs.dhcpc_en)
				{//dynamic ip, 
					lwip_netif_set_dhcp2("w0", 1);
				}
				else
				{ //static ip
					ip_addr_t addr;
					addr.addr = 0x0101A8C0 + ((data & 0xff) << 24);
					lwip_netif_set_ip2("w0", &addr, NULL, NULL);
					os_printf("w0_ip= %x\r\n", lwip_netif_get_ip2("w0"));
				}
			}
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
			break;
		case SYSEVT_WIFI_STA_CONNECTTED:
		case SYSEVT_WIFI_STA_PS_START:
		case SYSEVT_WIFI_STA_PS_END:
		case SYSEVT_WIFI_SCAN_DONE:
			break;
		case SYSEVT_WIFI_PAIR_DONE:
			if(sys_status.pair_success)
			{
				ieee80211_conf_get_ssid(sys_cfgs.wifi_mode, sys_cfgs.ssid);
				ieee80211_conf_get_psk(sys_cfgs.wifi_mode, sys_cfgs.psk);
				sys_cfgs.key_mgmt = ieee80211_conf_get_keymgmt(sys_cfgs.wifi_mode);

				if((int32)data == 1 && sys_cfgs.wifi_mode == WIFI_MODE_STA)
				{
					os_printf(KERN_NOTICE"wifi pair done, ngo role AP!\r\n");
					sys_cfgs.wifi_mode = WIFI_MODE_AP;
					ieee80211_iface_stop(WIFI_MODE_STA);
					wificfg_flush(WIFI_MODE_AP);
					ieee80211_iface_start(WIFI_MODE_AP);
				}

				if((int32)data == -1 && sys_cfgs.wifi_mode == WIFI_MODE_AP)
				{
					os_printf(KERN_NOTICE"wifi pair done, ngo role STA!\r\n");
					sys_cfgs.wifi_mode = WIFI_MODE_STA;
					ieee80211_iface_stop(WIFI_MODE_AP);
					wificfg_flush(WIFI_MODE_STA);
					ieee80211_iface_start(WIFI_MODE_STA);
				}
			}
			break;
		default:
			os_printf("no this event(%x)...\r\n", event_id);
			break;
	}
	return SYSEVT_CONTINUE;
}

sysevt_hdl_res sysevt_network_event(uint32 event_id, uint32 data, uint32 priv)
{
	struct netif* nif;

	switch(event_id)
	{
		case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
			nif = netif_find("w0");
			sys_status.dhcpc_done = 1;
#ifdef CONFIG_SLEEP
			err_t etharp_set_static(ip4_addr_t * ipaddr);
			err_t etharp_request(struct netif* netif, const ip4_addr_t * ipaddr);

			if(etharp_set_static(&nif->gw))
			{
				etharp_request(nif, &nif->gw);
			}
#endif
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
			break;
	}
	return SYSEVT_CONTINUE;
}

int32 sys_wifi_event(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
	switch(evt)
	{
		case IEEE80211_EVENT_PAIR_SUCCESS:
			sys_status.pair_success = 1;
			if(WIFI_MODE_STA == ifidx)
			{
				sys_cfgs.net_pair_switch = 0;
				ieee80211_pairing(sys_cfgs.wifi_mode, sys_cfgs.net_pair_switch);
				sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE), 1);
			}
			break;
		case IEEE80211_EVENT_PAIR_DONE:
			os_printf("inteface%d pair done, bssid: "MACSTR"\r\n", ifidx, MAC2STR((uint8*)param1));
			sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE), param2);
			break;
		default:
			break;
	}
	return RET_OK;
}
__init static void sys_wifi_parameter_init(void* ops)
{
	lmac_set_rf_pwr_level(ops, WIFI_RF_PWR_LEVEL);
#if WIFI_FEM_CHIP != LMAC_FEM_NONE
	lmac_set_fem(ops, WIFI_FEM_CHIP);   //初始化FEM之后，不能进行RF档位选择！
#endif
#if WIFI_MODULE_80211W_EN
	lmac_bgn_module_80211w_init(ops);
#endif
	lmac_set_rts(ops, WIFI_RTS_THRESHOLD);
	lmac_set_retry_cnt(ops, WIFI_TX_MAX_RETRY, WIFI_RTS_MAX_RETRY);
	lmac_set_txpower(ops, WIFI_TX_MAX_POWER);
	lmac_set_supp_rate(ops, WIFI_TX_SUPP_RATE);
	lmac_set_max_sta_cnt(ops, WIFI_MAX_STA_CNT);
	lmac_set_mcast_dup_txcnt(ops, WIFI_MULICAST_RETRY);
	lmac_set_max_ps_frame(ops, WIFI_MAX_PS_CNT);
	lmac_set_tx_duty_cycle(ops, WIFI_TX_DUTY_CYCLE);
#if WIFI_SSID_FILTER_EN
	lmac_set_ssid_filter(ops, sys_cfgs.ssid, strlen(sys_cfgs.ssid));
#endif
#if WIFI_PREVENT_PS_MODE_EN
	lmac_set_prevent_ps_mode(ops, WIFI_PREVENT_PS_MODE_EN);
#endif
#ifdef LMAC_BGN_PCF
	lmac_set_prevent_ps_mode(ops, 0);//bbm允许sta休眠
#endif

#ifdef RATE_CONTROL_SELECT
	//lmac_rate_ctrl_mcs_mask(ops, 0);
	lmac_rate_ctrl_type(ops, RATE_CONTROL_SELECT);
#endif
	//使用小电流时，就不使用增大发射功率的功率表
#if (WIFI_RF_PWR_LEVEL != 1) && (WIFI_RF_PWR_LEVEL != 2)
#if (WIFI_FEM_CHIP == LMAC_FEM_GSR2701) || (WIFI_FEM_CHIP == LMAC_FEM_KCT8227D) || (WIFI_FEM_CHIP == LMAC_FEM_GSR2701_5V)
	uint8 gain_table[] = {
		96, 96, 96, 96, 88, 88, 64, 64,         // NON HT OFDM
		96, 96, 96, 88, 88, 64, 64, 64,         // HT
		64, 64, 64, 64,                         // DSSS
	};
#else
	uint8 gain_table[] = {
		125, 125, 105, 100, 85, 85, 64, 64,     // NON HT OFDM
		125, 105, 105, 85,  85, 64, 64, 64,     // HT
		80,  80,  80,  80,                      // DSSS
	};
#endif
	lmac_set_tx_modulation_gain(ops, gain_table, sizeof(gain_table));
#endif
	lmac_set_temperature_compesate_en(ops, WIFI_TEMPERATURE_COMPESATE_EN);
	lmac_set_freq_offset_track_mode(ops, WIFI_FREQ_OFFSET_TRACK_MODE);

#if WIFI_HIGH_PRIORITY_TX_MODE_EN
	struct lmac_txq_param txq_max;
	struct lmac_txq_param txq_min;

	txq_max.aifs = 0xFF;                  //不限制aifs
	txq_max.txop = 0xFFFF;                //不限制txop
	txq_max.cw_min = 1;                     //cwmin最大值。如果觉得冲突太厉害，可以改成3
	txq_max.cw_max = 3;                     //cwmax最大值。如果觉得冲突太厉害，可以改成7
	lmac_set_edca_max(ops, &txq_max);
	lmac_set_tx_edca_slot_time(ops, 6);     //6us是其他客户推荐的值
	lmac_set_nav_max(ops, 0);               //完全关闭NAV功能

	txq_min.aifs = 0;
	txq_min.txop = 100;                   //txop最小值限制为100
	txq_min.cw_min = 0;
	txq_min.cw_max = 0;
	lmac_set_edca_min(ops, &txq_min);
#endif
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

void TXW_TakeEvents()
{
	sys_event_init(32);
	sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, 0), sysevt_wifi_event, 0);
	sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), sysevt_network_event, 0);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	int32 ret;
	os_sprintf(sys_cfgs.ssid, "%s", oob_ssid);
	os_sprintf(sys_cfgs.passwd, "%s", connect_key);
	void* ops;
	struct lmac_init_param lparam;

#if DCDC_EN
	pmu_dcdc_open();
#endif
	void* rxbuf = (void*)os_malloc(16 * 1024);
	lparam.rxbuf = (uint32_t)rxbuf;
	lparam.rxbuf_size = 16 * 1024;
	ops = lmac_bgn_init(&lparam);
#ifdef CONFIG_SLEEP
	void* bgn_dsleep_init(void* ops);
	bgn_dsleep_init(ops);
#endif

#ifdef LMAC_BGN_PCF
	lmac_set_bbm_mode_init(ops, 1); //init before ieee80211_support_txw80x
#endif

	lmac_set_aggcnt(ops, 0);
	lmac_set_rx_aggcnt(ops, 0);

	//ieee80211_crypto_bignum_support();
	//ieee80211_crypto_ec_support();

	struct ieee80211_initparam param;
	os_memset(&param, 0, sizeof(param));
	param.vif_maxcnt = 2;
	param.sta_maxcnt = 2;
	param.bss_maxcnt = 2;
	param.bss_lifetime = 300; //300 seconds
	param.no_rxtask = 1;
	param.evt_cb = sys_wifi_event;
	ieee80211_init(&param);
	ieee80211_support_txw80x(ops);

	sys_wifi_parameter_init(ops);
	ieee80211_iface_create_sta(WIFI_MODE_STA, IEEE80211_BAND_2GHZ);

	wificfg_flush(WIFI_MODE_AP);
	wificfg_flush(WIFI_MODE_STA);
	//eloop_init();
	//os_task_create("eloop_run", user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
	os_sleep_ms(1);

	{
		struct netdev* ndev;
		ip_addr_t ipaddr, netmask, gw;
		ndev = (struct netdev*)dev_get(HG_WIFI0_DEVID);
		ipaddr.addr = sys_cfgs.ipaddr;
		netmask.addr = sys_cfgs.netmask;
		gw.addr = sys_cfgs.gw_ip;
		lwip_netif_add(ndev, "w0", &ipaddr, &netmask, &gw);
		lwip_netif_remove_register(ndev);
		lwip_netif_set_default(ndev);
		lwip_netif_set_dhcp(ndev, 1);
	}

	wifi_create_station(oob_ssid, connect_key, WPA_KEY_MGMT_PSK);
	ieee80211_iface_start(WIFI_MODE_STA);
}

void HAL_DisconnectFromWifi()
{
	struct netdev* ndev;
	ndev = (struct netdev*)dev_get(HG_WIFI0_DEVID);
	if(ndev)
	{
		lwip_netif_set_dhcp(ndev, 0);
		os_sleep_ms(1);
		lwip_netif_remove(ndev);
		netif_num--;
	}
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	int32 ret;
	g_bOpenAccessPointMode = 1;

	os_sprintf(sys_cfgs.ssid, "%s", ssid);
	void* ops;
	struct lmac_init_param lparam;

#if DCDC_EN
	pmu_dcdc_open();
#endif
	void* rxbuf = (void*)os_malloc(16 * 1024);
	lparam.rxbuf = (uint32_t)rxbuf;
	lparam.rxbuf_size = 16 * 1024;
	ops = lmac_bgn_init(&lparam);
#ifdef CONFIG_SLEEP
	void* bgn_dsleep_init(void* ops);
	bgn_dsleep_init(ops);
#endif

#ifdef LMAC_BGN_PCF
	lmac_set_bbm_mode_init(ops, 1); //init before ieee80211_support_txw80x
#endif

	lmac_set_aggcnt(ops, 0);
	lmac_set_rx_aggcnt(ops, 0);

	struct ieee80211_initparam param;
	os_memset(&param, 0, sizeof(param));
	param.vif_maxcnt = 2;
	param.sta_maxcnt = 2;
	param.bss_maxcnt = 2;
	param.bss_lifetime = 300; //300 seconds
	param.no_rxtask = 1;
	param.evt_cb = sys_wifi_event;
	ieee80211_init(&param);
	ieee80211_support_txw80x(ops);

	sys_wifi_parameter_init(ops);
	ieee80211_iface_create_ap(WIFI_MODE_AP, IEEE80211_BAND_2GHZ);
	//ieee80211_iface_create_sta(WIFI_MODE_STA, IEEE80211_BAND_2GHZ);

	wificfg_flush(WIFI_MODE_STA);

	//sys_wifi_start_acs(ops);

	struct lmac_acs_ctl acs_ctl;
	if(sys_cfgs.wifi_mode == WIFI_MODE_AP)
	{
		if(sys_cfgs.channel == 0)
		{
			acs_ctl.enable = 1;
			acs_ctl.scan_ms = WIFI_ACS_SCAN_TIME;;
			acs_ctl.chn_bitmap = WIFI_ACS_CHAN_LISTS;

			ret = lmac_start_acs(ops, &acs_ctl, 1);  //阻塞式扫描
			if(ret != RET_ERR)
			{
				sys_cfgs.channel = ret;
			}
		}
	}

	wificfg_flush(WIFI_MODE_AP);

	uint8 txq[][8] = {
		{0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,},
		{0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01,},
		{0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02,},
		{0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x03,},
	};


	ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf0, (struct ieee80211_wmm_param*)&txq[0]);
	ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf1, (struct ieee80211_wmm_param*)&txq[1]);
	ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf2, (struct ieee80211_wmm_param*)&txq[2]);
	ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf3, (struct ieee80211_wmm_param*)&txq[3]);
	os_sleep_ms(1);

	{
		struct netdev* ndev;
		ip_addr_t ipaddr, netmask, gw;
		ndev = (struct netdev*)dev_get(HG_WIFI1_DEVID);
		ipaddr.addr = sys_cfgs.ipaddr;
		netmask.addr = sys_cfgs.netmask;
		gw.addr = sys_cfgs.gw_ip;
		lwip_netif_add(ndev, "w0", &ipaddr, &netmask, &gw);
		lwip_netif_remove_register(ndev);
		lwip_netif_set_default(ndev);

		struct dhcpd_param param;
		os_memset(&param, 0, sizeof(param));
		param.start_ip = sys_cfgs.dhcpd_startip;
		param.end_ip = sys_cfgs.dhcpd_endip;
		param.netmask = sys_cfgs.netmask;
		param.router = sys_cfgs.gw_ip;
		param.dns1 = sys_cfgs.gw_ip;
		param.lease_time = sys_cfgs.dhcpd_lease_time;
		if(dhcpd_start_eloop("w0", &param))
		{
			os_printf("dhcpd start error\r\n");
		}
		dns_start_eloop("w0");
	}

	wifi_create_ap(ssid, NULL, WPA_KEY_MGMT_NONE, 1);
	ieee80211_iface_start(WIFI_MODE_AP);
}

#endif // PLATFORM_TXW81X

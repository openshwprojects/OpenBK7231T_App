/**************************************************************************
 * File Name                   : tls_cmdp.h
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef TLS_CMDP_H
#define TLS_CMDP_H
#include "wm_type_def.h"
#include "wm_params.h"
#include "wm_uart.h"
#include "wm_config.h"

/* error code */
#define CMD_ERR_OK              0
#define CMD_ERR_INV_FMT         1
#define CMD_ERR_UNSUPP          2
#define CMD_ERR_OPS             3
#define CMD_ERR_INV_PARAMS      4
#define CMD_ERR_NOT_ALLOW       5
#define CMD_ERR_MEM             6
#define CMD_ERR_FLASH           7
#define CMD_ERR_BUSY            8
#define CMD_ERR_SLEEP           9
#define CMD_ERR_JOIN            10
#define CMD_ERR_NO_SKT          11
#define CMD_ERR_INV_SKT         12
#define CMD_ERR_SKT_CONN        13
#define CMD_ERR_SKT_SND         62
#define CMD_ERR_SKT_RPT         63
#define CMD_ERR_UNDEFINE        64

struct tls_socket_cfg {
    u8   proto;   /* 0: tcp, 1: udp */
    u8   client;
    u16  port;
    u8   host[32]; /* host name */
    u16  host_len;
    u8   ip_addr[4];
    u32  timeout;
};
struct tls_cmd_rsp_t {
    u8   *rsp_ptr;
    u32    max_size;
    u32    rsp_len;
    u8    res[0]; 
};

enum tls_cmd_mode {
    CMD_MODE_HSPI_ATCMD,
    CMD_MODE_HSPI_RICMD,
    CMD_MODE_UART0_ATCMD,
    CMD_MODE_UART1_ATCMD,
    CMD_MODE_UART1_RICMD,
#if TLS_CONFIG_RMMS
    CMD_MODE_RMMS_ATCMD,
#endif
    CMD_MODE_INTERNAL, 
};

enum {
    UART_ATCMD_MODE = 0,
    UART_RICMD_MODE = 1,
    UART_TRANS_MODE = 2, 
    UART_ATDATA_MODE = 3,
    UART_ATSND_MODE  =4,
};

struct tls_cmd_ps_t {
    u8   ps_type;
    u8   wake_type;
    u32  delay_time;
    u32  wake_time; 
};

struct tls_cmd_ver_t {
    u8 hw_ver[6];
    u8 fw_ver[4];
};

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
struct tls_cmd_connect_t {
    u8 bssid[ETH_ALEN];
    u8 type;
    u8 channel;
    u8 encrypt;
    u8 ssid_len;
    u8 ssid[32];
    u8 rssi;  /* 只取值，不取符号 */
    u8 res;   /* 1: 返回参数, 0: 没有参数 */
};

struct tls_cmd_link_status_t {
    u8 ip[4];
    u8 netmask[4];
    u8 gw[4];
    u8 dns1[4];
    u8 dns2[4];
    u8 status; 
};

struct tls_cmd_socket_t {
    u32 timeout; 
    u8  ip_addr[4];
    u8 proto;
    u8 client;
    u16 port;
    char host_name[32];
    u8  host_len;
    u16 localport;
};

struct tls_cmd_ssid_t {
    u8 ssid_len;
    u8 ssid[32];
};

struct tls_cmd_tem_t {
    u8 offsetLen;
    s32 offset;
};

struct tls_cmd_key_t {
    u8 format;
    u8 index;
    u8 key_len;
    u8 key[64];
};

struct tls_cmd_bssid_t {
    u8 bssid[6];
    u8 enable; 
};

struct tls_cmd_wl_hw_mode_t {
    u8  hw_mode;
    u8  max_rate;
};

struct tls_cmd_wps_params_t {
    u8 mode;
    u8 pin_len;
    u8 pin[8];
};

struct tls_cmd_ip_params_t {
    u8 ip_addr[4];
    u8 netmask[4];
    u8 gateway[4];
    u8 dns[4];
    u8 type; 
};

struct tls_cmd_uart_params_t {
    u32 baud_rate;
    u32  stop_bit;
    u32  parity;
    u32  flow_ctrl;
    u32  charlength; 
};

struct tls_cmd_flash_t {
    u32  word_cnt;
    u32  flash_addr;
    u32  value[8];
};
void tls_cmd_set_net_up(u8 netup);
u8 tls_cmd_get_net_up(void);
u8 tls_cmd_get_auto_mode(void);
struct tls_socket_cfg *tls_cmd_get_socket_cfg(void);
void tls_cmd_init_socket_cfg(void);
int hostif_cipher2host(int cipher, int proto);
int tls_cmd_ps( struct tls_cmd_ps_t *ps);
#if 0
int tls_set_encrypt_cfg( u8 encrypt);
int tls_set_key_cfg(struct tls_cmd_key_t *key);
#endif
int tls_cmd_reset_flash(void);
int tls_cmd_pmtf(void);
void tls_cmd_reset_sys(void);
int tls_cmd_get_ver( struct tls_cmd_ver_t *ver);
int tls_cmd_scan( enum tls_cmd_mode mode);
int tls_cmd_scan_by_param( enum tls_cmd_mode mode, u16 channellist, u32 times, u16 switchinterval);
int tls_cmd_join( enum tls_cmd_mode mode,struct tls_cmd_connect_t *conn);
int tls_cmd_disconnect_network(u8 mode);
int tls_cmd_get_link_status(struct tls_cmd_link_status_t *lks);
int tls_cmd_wps_start(void);
int tls_cmd_set_wireless_mode(u8 mode, u8 update_flash);
int tls_cmd_get_wireless_mode(u8 *mode);
int tls_cmd_set_ssid(struct tls_cmd_ssid_t *ssid, u8 update_flash);
int tls_cmd_get_ssid(struct tls_cmd_ssid_t *ssid);
int tls_cmd_set_key(struct tls_cmd_key_t *key, u8 update_flash);
int tls_cmd_get_key(struct tls_cmd_key_t *key);
int tls_cmd_set_encrypt(u8 encrypt, u8 update_flash);
int tls_cmd_get_encrypt( u8 *encrypt);
int tls_cmd_set_bssid(struct tls_cmd_bssid_t *bssid, u8 update_flash);
int tls_cmd_get_bssid(struct tls_cmd_bssid_t *bssid);
int tls_cmd_get_original_ssid(struct tls_param_ssid *original_ssid);
int tls_cmd_get_original_key(struct tls_param_original_key *original_key);
int tls_cmd_set_hide_ssid(u8 ssid_set, u8 update_flash);
int tls_cmd_get_hide_ssid( u8 *ssid_set);
int tls_cmd_set_channel(u8 channel,  u8 channel_en, u8 update_flash);
int tls_cmd_get_channel( u8 *channel, u8 *channel_en);
int tls_cmd_set_channellist( u16 channellist, u8 update_flash);
int tls_cmd_get_channellist( u16 *channellist);
int tls_cmd_set_region(u16 region, u8 update_flash);
int tls_cmd_get_region(u16 *region);
int tls_cmd_set_hw_mode(struct tls_cmd_wl_hw_mode_t *hw_mode, u8 update_flash);
int tls_cmd_get_hw_mode(struct tls_cmd_wl_hw_mode_t *hw_mode);
int tls_cmd_set_adhoc_create_mode(u8 mode, u8 update_flash);
int tls_cmd_get_adhoc_create_mode( u8 *mode);
int tls_cmd_set_wl_ps_mode( u8 enable,
        u8 update_flash);
int tls_cmd_get_wl_ps_mode( u8 *enable);
int tls_cmd_set_roaming_mode(u8 enable, u8 update_flash);
int tls_cmd_get_roaming_mode(u8 *enable);
int tls_cmd_set_wps_params(struct tls_cmd_wps_params_t *params, u8 update_flash);
int tls_cmd_get_wps_params(struct tls_cmd_wps_params_t *params);
int tls_cmd_get_ip_info(struct tls_cmd_ip_params_t *params);
int tls_cmd_set_ip_info(struct tls_cmd_ip_params_t *params, u8 update_flash);
int tls_cmd_set_work_mode(u8 mode, u8 update_flash);
int tls_cmd_get_work_mode(u8 *mode);
int tls_cmd_get_hostif_mode(u8 *mode);
int tls_cmd_set_hostif_mode(u8 mode, u8 update_flash);
int tls_cmd_set_default_socket_params(struct tls_cmd_socket_t *params, u8 update_flash);
int tls_cmd_get_default_socket_params(struct tls_cmd_socket_t *params);
int tls_cmd_get_uart_params(struct tls_cmd_uart_params_t *params);
int tls_cmd_set_uart_params(struct tls_cmd_uart_params_t *params, u8 update_flash);
int tls_cmd_get_atlt( u16 *length);
int tls_cmd_set_atlt( u16 length, u8 update_flash);
int tls_cmd_get_atpt( u16 *period);
int tls_cmd_set_atpt( u16 period, u8 update_flash);
int tls_cmd_get_espc(u8 *escapechar);
int tls_cmd_set_espc( u8 escapechar, u8 update_flash);
int tls_cmd_get_espt(u16 *escapeperiod);
int tls_cmd_set_espt( u16 escapeperiod, u8 update_flash);
int tls_cmd_get_warc( u8 *autoretrycnt);
int tls_cmd_set_warc( u8 autoretrycnt, u8 update_flash);
int tls_cmd_set_dnsname( u8 *dnsname, u8 update_flash);
int tls_cmd_get_dnsname( u8 *dnsname);
int tls_cmd_set_webs( struct tls_webs_cfg webcfg, u8 update_flash);
int tls_cmd_get_webs( struct tls_webs_cfg *webcfg);
int tls_cmd_get_cmdm( u8 *cmdmode);
int tls_cmd_set_cmdm( u8 cmdmode, u8 update_flash);
int tls_cmd_get_iom( u8 *iomode);
int tls_cmd_set_iom( u8 iomode, u8 update_flash);
int tls_cmd_set_oneshot( u8 oneshotflag, u8 update_flash);
int tls_cmd_get_oneshot( u8 *oneshotflag);
int tls_cmd_get_pass( u8 *password);
int tls_cmd_set_pass( u8* password, u8 update_flash);

int tls_cmd_set_dbg( u32 dbg);
int tls_cmd_wr_flash(struct tls_cmd_flash_t *wr_flash);
int tls_cmd_get_sha1( u8 *psk);
int tls_cmd_set_sha1( u8* psk, u8 update_flash);

void tls_set_fwup_mode(u8 flag);
u8   tls_get_fwup_mode(void);

int tls_cmd_set_wps_pin( struct tls_param_wps* wps, u8 update_flash);
int tls_cmd_get_wps_pin( struct tls_param_wps *wps);
typedef void  (*cmd_get_uart1_port_callback)(struct tls_uart_port ** uart1_port);
void tls_cmd_register_get_uart1_port(cmd_get_uart1_port_callback callback);
cmd_get_uart1_port_callback tls_cmd_get_uart1_port(void);

typedef void  (*cmd_set_uart1_mode_callback)(u32 cmd_mode);
void tls_cmd_register_set_uart1_mode(cmd_set_uart1_mode_callback callback);
cmd_set_uart1_mode_callback tls_cmd_get_set_uart1_mode(void);

typedef void (*cmd_set_uart1_sock_param_callback)(u16 sksnd_cnt, bool rx_idle);
void tls_cmd_register_set_uart1_sock_param(cmd_set_uart1_sock_param_callback callback);
cmd_set_uart1_sock_param_callback tls_cmd_get_set_uart1_sock_param(void);

typedef void  (*cmd_set_uart0_mode_callback)(u32 cmd_mode);
cmd_set_uart0_mode_callback tls_cmd_get_set_uart0_mode(void);
void tls_cmd_register_set_uart0_mode(cmd_set_uart0_mode_callback callback);

int tls_cmd_get_hw_ver(u8 *hwver);
int tls_cmd_set_hw_ver(u8 *hwver);

#if TLS_CONFIG_AP
int tls_cmd_set_softap_ssid(struct tls_cmd_ssid_t *ssid, u8 update_flash);
int tls_cmd_get_softap_ssid(struct tls_cmd_ssid_t *ssid);
int tls_cmd_set_softap_key(struct tls_cmd_key_t *key, u8 update_flash);
int tls_cmd_get_softap_key(struct tls_cmd_key_t *key);
int tls_cmd_set_softap_encrypt(u8 encrypt, u8 update_flash);
int tls_cmd_get_softap_encrypt( u8 *encrypt);
int tls_cmd_get_softap_channel( u8 *channel);
int tls_cmd_set_softap_channel(u8 channel, u8 update_flash);
int tls_cmd_set_softap_hw_mode(struct tls_cmd_wl_hw_mode_t *hw_mode, u8 update_flash);
int tls_cmd_get_softap_hw_mode(struct tls_cmd_wl_hw_mode_t *hw_mode);
int tls_cmd_get_softap_ip_info(struct tls_cmd_ip_params_t *params);
int tls_cmd_set_softap_ip_info(struct tls_cmd_ip_params_t *params, u8 update_flash);
int tls_cmd_get_softap_link_status(struct tls_cmd_link_status_t *lks);
int tls_cmd_get_sta_detail(u32 *sta_num, u8 *buf);

#endif

#endif /* end of TLS_CMDP_H */

#ifndef __WM_BT_API_H__
#define __WM_BT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#if (WM_NIMBLE_INCLUDED == CFG_ON)
#include "wm_bt.h"
#include "wm_ble.h"

extern tls_bt_status_t enable_bt_test_mode(tls_bt_hci_if_t *p_hci_if);
extern tls_bt_status_t exit_bt_test_mode();

extern int tls_at_bt_enable(int uart_no, tls_bt_log_level_t log_level);
extern int tls_at_bt_destroy(void);
extern void tls_rf_bt_mode(uint8_t mode);
extern int tls_ble_client_demo_api_init(tls_ble_uart_output_ptr output_func_ptr,tls_ble_uart_sent_ptr uart_in_and_sent_ptr);
extern int tls_ble_server_demo_api_init(tls_ble_uart_output_ptr output_func_ptr,tls_ble_uart_sent_ptr uart_in_and_sent_ptr);
extern int tls_ble_client_demo_api_deinit();
extern int tls_ble_server_demo_api_deinit();
extern int tls_ble_client_multi_conn_demo_api_init();
extern int tls_ble_client_multi_conn_demo_api_deinit();
extern int tls_ble_uart_init(tls_ble_uart_mode_t mode, uint8_t uart_id, tls_uart_options_t *p_hci_if);
extern int tls_ble_uart_deinit(tls_ble_uart_mode_t mode,uint8_t uart_id);
extern int tls_ble_demo_adv(uint8_t type);
extern int tls_ble_demo_scan(uint8_t start);
extern tls_bt_status_t tls_ble_set_scan_chnl_map(uint8_t map);
extern int tls_ble_client_demo_api_send_msg(uint8_t *ptr, int length);
extern int tls_ble_server_demo_api_send_msg(uint8_t *data, int data_len);


extern int tls_ble_gap_set_name(const char *dev_name, uint8_t update_flash);
extern int tls_ble_gap_get_name(char *dev_name);
extern int tls_ble_gap_set_data(wm_ble_gap_data_t type, uint8_t *data, uint8_t data_len);
extern int tls_ble_gap_set_adv_param(uint8_t adv_type, uint32_t min, uint32_t max, uint8_t chn_map, uint8_t filter_policy, uint8_t *dir_mac, uint8_t dir_mac_type);
extern int tls_ble_gap_set_scan_param(uint32_t intv, uint32_t window, uint8_t filter_policy, bool limited, bool passive, bool filter_duplicate);
extern int tls_nimble_gap_adv(wm_ble_adv_type_t type, int duration);
extern void tls_ble_demo_scan_at_cmd_register(void (*scan_resp_cb_fn)(int type, int8_t rssi, uint8_t *addr, const uint8_t *name, int name_len, const uint8_t *raw_scan_resp, int raw_scan_resp_length));
extern void tls_ble_demo_scan_at_cmd_unregister();
extern int tls_ble_gap_scan(wm_ble_scan_type_t type, bool filter_duplicate);


#if (WM_MESH_INCLUDED == CFG_ON) 
extern int tls_ble_mesh_init(tls_bt_mesh_at_callback_t at_cb, tls_bt_mesh_role_t role, bool running);
extern int tls_ble_mesh_deinit(void);
extern int tls_ble_mesh_get_cfg(tls_mesh_primary_cfg_t *cfg);
extern int tls_ble_mesh_get_primary_addr(uint16_t *primary_addr);
extern int tls_ble_mesh_change_ttl(uint8_t ttl);
extern int tls_ble_mesh_change_primary_addr(uint16_t primary_addr);
extern int tls_ble_mesh_clear_local_rpl(void);
extern int tls_ble_mesh_gen_level_set(uint16_t net_idx, uint16_t dst, uint16_t app_idx, int16_t val,
                               int16_t *state);
extern int tls_ble_mesh_gen_level_get(uint16_t net_idx, uint16_t dst, uint16_t app_idx, int16_t *state);
extern int tls_ble_mesh_gen_off_publish(uint8_t onoff_state);
extern int tls_ble_mesh_gen_onoff_get(uint16_t net_idx, uint16_t dst, uint16_t app_idx, uint8_t *state);
extern int tls_ble_mesh_gen_onoff_set(uint16_t net_idx, uint16_t dst, uint16_t app_idx, uint8_t val,
                               uint8_t *state);
extern int tls_ble_mesh_pub_set(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t mod_id,
                         uint16_t cid,
                         tls_bt_mesh_cfg_mod_pub *pub, uint8_t *status);
extern int tls_ble_mesh_pub_get(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t mod_id,
                         uint16_t cid, tls_bt_mesh_cfg_mod_pub *pub, uint8_t *status);
extern int tls_ble_mesh_sub_add(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t sub_addr,
                         uint16_t mod_id, uint16_t cid, uint8_t *status);

extern int tls_ble_mesh_sub_del(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t sub_addr,
                         uint16_t mod_id, uint16_t cid, uint8_t *status);
extern int tls_ble_mesh_sub_add(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t sub_addr,
                         uint16_t mod_id, uint16_t cid, uint8_t *status);
extern int tls_ble_mesh_sub_get(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t mod_id,
                         uint16_t cid,
                         uint8_t *status, uint16_t *subs, uint32_t *sub_cnt);
extern int tls_ble_mesh_friend_set(uint16_t net_idx, uint16_t dst, uint8_t val, uint8_t *status);
extern int tls_ble_mesh_friend_get(uint16_t net_idx, uint16_t dst, uint8_t *val);
extern int tls_ble_mesh_proxy_get(uint16_t net_idx, uint16_t dst, uint8_t *proxy);
extern int tls_ble_mesh_proxy_set(uint16_t net_idx, uint16_t dst, uint8_t val, uint8_t *proxy);
extern int tls_ble_mesh_relay_get(uint16_t net_idx, uint16_t dst, uint8_t *relay, uint8_t *transmit);
extern int tls_ble_mesh_relay_set(uint16_t net_idx, uint16_t dst, uint8_t relay, uint8_t count,
                           uint8_t interval, uint8_t *status, uint8_t *transmit);
extern int tls_ble_mesh_unbind_app_key(uint16_t net_idx, uint16_t dst, uint16_t elem_addr,
                                uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid, uint8_t *status);
extern int tls_ble_mesh_bind_app_key(uint16_t net_idx, uint16_t dst, uint16_t elem_addr,
                              uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid, uint8_t *status);
extern int tls_ble_mesh_add_app_key(uint16_t net_idx, uint16_t dst, uint16_t key_net_idx,
                             uint16_t key_app_idx, uint8_t app_key[16], uint8_t *status);
extern int tls_ble_mesh_add_local_app_key(uint16_t net_idx, uint16_t app_idx, uint8_t app_key[16]);
extern int tls_ble_mesh_input_oob_number(uint32_t number);
extern int tls_ble_mesh_input_oob_string(const char *string);
extern int tls_ble_mesh_node_reset(uint16_t net_idx, uint16_t addr, uint8_t *status);
extern int tls_ble_mesh_provisioner_prov_adv(uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
                                      uint8_t duration);
extern int tls_ble_mesh_provisioner_scan(bool enable);
extern int tls_ble_mesh_clear_local_rpl(void);
extern int tls_ble_mesh_change_primary_addr(uint16_t primary_addr);
extern int tls_ble_mesh_get_primary_addr(uint16_t *primary_addr);
extern int tls_ble_mesh_change_ttl(uint8_t ttl);
extern int tls_ble_mesh_get_cfg(tls_mesh_primary_cfg_t *cfg);
extern int tls_ble_mesh_get_comp(uint16_t net_idx, uint16_t dst, uint8_t *status, char *rsp_data,
                          uint32_t *data_len);
extern int tls_ble_mesh_vnd_send_msg(uint8_t *msg, int len);

extern int tls_ble_mesh_hb_sub_set(uint16_t net_idx, uint16_t dst, tls_bt_mesh_cfg_hb_sub *hb_sub, uint8_t *status);
extern int tls_ble_mesh_hb_sub_get(uint16_t net_idx, uint16_t dst, tls_bt_mesh_cfg_hb_sub *hb_sub, uint8_t *status);

extern int tls_ble_mesh_hb_pub_set(uint16_t net_idx, uint16_t dst, const tls_bt_mesh_cfg_hb_pub *hb_pub, uint8_t *status);
extern int tls_ble_mesh_hb_pub_get(uint16_t net_idx, uint16_t dst, tls_bt_mesh_cfg_hb_pub *hb_pub, uint8_t *status);
/**node demo api for at command*/
extern int tls_ble_mesh_node_deinit(int reason);
extern int tls_ble_mesh_node_init(void);
extern int tls_ble_mesh_erase_cfg(void);

#endif

#else
#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON)
#include "wm_bt.h"
#include "wm_ble.h"
#include "wm_ble_gatt.h"

extern tls_bt_status_t enable_bt_test_mode(tls_bt_hci_if_t *p_hci_if);
extern tls_bt_status_t exit_bt_test_mode();

extern int tls_at_bt_enable(int uart_no, tls_bt_log_level_t log_level, tls_bt_host_callback_t at_callback_ptr);
extern int tls_at_bt_destroy(void);
extern int tls_at_bt_cleanup_host(void);
extern void tls_rf_bt_mode(uint8_t mode);
#if (WM_BT_INCLUDED == CFG_ON)
extern int demo_bt_scan_mode(int type);
extern int demo_bt_inquiry(int type);
extern int demo_bt_app_on();
extern int demo_bt_app_off();
#endif

#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)
extern tls_bt_status_t tls_bt_enable_a2dp_sink();
extern tls_bt_status_t tls_bt_disable_a2dp_sink();
#endif

#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)
extern tls_bt_status_t tls_bt_enable_hfp_client();
extern tls_bt_status_t tls_bt_disable_hfp_client();
extern tls_bt_status_t tls_bt_dial_number(const char* number);
#endif

#if (WM_BTA_SPPS_INCLUDED == CFG_ON)
extern tls_bt_status_t tls_bt_enable_spp_server();
extern tls_bt_status_t tls_bt_disable_spp_server();
#endif
#if (WM_BTA_SPPC_INCLUDED == CFG_ON)
extern tls_bt_status_t tls_bt_enable_spp_client();
extern tls_bt_status_t tls_bt_disable_spp_client();
#endif

#if (WM_BLE_INCLUDED == CFG_ON)
extern int tls_ble_client_demo_api_init(tls_ble_output_func_ptr output_func_ptr);
extern int tls_ble_server_demo_api_init(tls_ble_output_func_ptr output_func_ptr);
extern int tls_ble_client_demo_api_deinit();
extern int tls_ble_server_demo_api_deinit();

extern int tls_ble_client_multi_conn_demo_api_init();
extern int tls_ble_client_multi_conn_demo_api_deinit();
extern int tls_ble_uart_init(tls_ble_uart_mode_t mode, uint8_t uart_id, tls_uart_options_t *p_hci_if);
extern int tls_ble_uart_deinit(tls_ble_uart_mode_t mode,uint8_t uart_id);
extern int tls_ble_demo_adv(uint8_t type);
extern int tls_ble_demo_scan(uint8_t start);
extern tls_bt_status_t tls_ble_set_scan_chnl_map(uint8_t map);
extern int tls_ble_client_demo_api_send_msg(uint8_t *ptr, int length);
extern int tls_ble_server_demo_api_send_msg(uint8_t *data, int data_len);

extern int tls_ble_register_report_evt(tls_ble_dm_evt_t rpt_evt,  tls_ble_dm_callback_t rpt_callback);
extern int tls_ble_deregister_report_result(tls_ble_dm_evt_t rpt_evt,  tls_ble_dm_callback_t rpt_callback);
extern int tls_ble_demo_prof_init(uint16_t uuid, tls_ble_callback_t at_cb_ptr);
extern int tls_ble_demo_prof_deinit(int server_if);
extern int tls_ble_demo_cli_init(uint16_t uuid, tls_ble_callback_t at_cb_ptr);
extern int tls_ble_demo_cli_deinit(int client_if);
extern tls_bt_uuid_t * app_uuid16_to_uuid128(uint16_t uuid16);

#endif

#endif


#endif

#ifdef __cplusplus
}
#endif


#endif

#ifndef __WM_BLE_SERVER_DEMO_H__
#define __WM_BLE_SERVER_DEMO_H__
#include "wm_bt.h"

int tls_ble_server_demo_api_init(tls_ble_output_func_ptr output_func_ptr);
int tls_ble_server_demo_api_deinit();
int tls_ble_server_demo_api_connect(int status);
int tls_ble_server_demo_api_disconnect(int status);
int tls_ble_server_demo_api_send_msg(uint8_t *ptr, int length);
int tls_ble_server_demo_api_send_response(uint8_t *ptr, int length);
int tls_ble_server_demo_api_clean_up(int status);
int tls_ble_server_demo_api_disable(int status);
int tls_ble_server_demo_api_read_remote_rssi();
uint32_t tls_ble_server_demo_api_get_mtu();


#endif


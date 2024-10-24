#ifndef __WM_BLE_SERVER_DEMO_H__
#define __WM_BLE_SERVER_DEMO_H__
#include "wm_bt.h"

#ifdef __cplusplus
extern "C" {
#endif


int tls_ble_server_demo_api_init(tls_ble_uart_output_ptr uart_output_ptr, tls_ble_uart_sent_ptr uart_in_and_sent_ptr);
int tls_ble_server_demo_api_deinit();
uint32_t tls_ble_server_demo_api_get_mtu();
int tls_ble_server_demo_api_send_msg(uint8_t *data, int data_len);
int tls_ble_server_demo_api_send_msg_notify(uint8_t *ptr, int length);
int tls_ble_server_demo_api_set_work_mode(int work_mode);

#ifdef __cplusplus
}
#endif

#endif


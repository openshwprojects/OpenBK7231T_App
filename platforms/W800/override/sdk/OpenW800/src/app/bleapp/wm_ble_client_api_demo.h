#ifndef __WM_BLE_CLIENT_DEMO_H__
#define __WM_BLE_CLIENT_DEMO_H__

#include "wm_bt_def.h"

#ifdef __cplusplus
extern "C" {
#endif


int tls_ble_client_demo_api_init(tls_ble_uart_output_ptr uart_output_ptr, tls_ble_uart_sent_ptr uart_in_and_sent_ptr);

int tls_ble_client_demo_api_deinit();

int tls_ble_client_demo_api_send_msg(uint8_t *ptr, int length);

#ifdef __cplusplus
}
#endif

#endif


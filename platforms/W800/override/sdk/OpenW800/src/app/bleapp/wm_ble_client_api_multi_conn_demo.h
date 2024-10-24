#ifndef __WM_BLE_CLIENT_DEMO_MULTI_CONN_H__
#define __WM_BLE_CLIENT_DEMO_MULTI_CONN_H__

#include "wm_bt_def.h"

#ifdef __cplusplus
extern "C" {
#endif

int tls_ble_client_multi_conn_demo_api_init();
int tls_ble_client_multi_conn_demo_api_deinit();
int tls_ble_client_multi_conn_demo_send_msg(uint8_t *ptr, int length);

#ifdef __cplusplus
}
#endif

#endif


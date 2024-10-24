#ifndef __WM_BLE_CLIENT_DEMO_HUAWEI_H__
#define __WM_BLE_CLIENT_DEMO_HUAWEI_H__

#include "wm_bt_def.h"

int tls_ble_client_demo_api_init(tls_ble_output_func_ptr output_func_ptr);

int tls_ble_client_demo_api_deinit();

int tls_ble_client_demo_api_send_msg(uint8_t *ptr, int length);

#endif


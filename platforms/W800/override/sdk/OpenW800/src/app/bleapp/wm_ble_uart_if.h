#ifndef __WM_UART_BLE_IF_H__
#define __WM_UART_BLE_IF_H__
#include "wm_uart.h"
#include "wm_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

int tls_ble_uart_init(tls_ble_uart_mode_t mode, uint8_t uart_id, tls_uart_options_t *p_hci_if);
int tls_ble_uart_deinit(tls_ble_uart_mode_t mode, uint8_t uart_id);

#ifdef __cplusplus
}
#endif


#endif

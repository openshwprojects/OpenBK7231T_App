#ifndef __WM_UART_BLE_IF_H__
#define __WM_UART_BLE_IF_H__
#include "wm_uart.h"
#include "wm_bt.h"

int tls_ble_uart_init(tls_ble_uart_mode_t mode, uint8_t uart_id, tls_uart_options_t *p_hci_if);
int tls_ble_uart_deinit(tls_ble_uart_mode_t mode, uint8_t uart_id);

uint32_t tls_ble_uart_buffer_size();
uint32_t tls_ble_uart_buffer_available();
uint32_t tls_ble_uart_buffer_read(uint8_t *ptr, uint32_t length);
uint32_t tls_ble_uart_buffer_delete(uint32_t length);
uint32_t tls_ble_uart_buffer_peek(uint8_t *ptr, uint32_t length);

#endif

#ifdef PLATFORM_ECR6600

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "uart.h"
#include "chip_pinmux.h"

E_DRV_UART_NUM uart = E_UART_NUM_0;
unsigned int uart_base;

static void uart_cb(void* data)
{
	while(drv_uart_rx_tstc(uart_base))
	{
		if(drv_uart_rx_tstc(uart_base))
		{
			UART_AppendByteToReceiveRingBuffer(drv_uart_rx_getc(uart_base));
		}
	}
}

void HAL_UART_SendByte(byte b)
{
	drv_uart_send_poll(uart, &b, 1);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc)
{
	drv_uart_close(uart);
	T_DRV_UART_CONFIG config = 
	{
		.uart_baud_rate = baud,
		.uart_data_bits = UART_DATA_BITS_8,
		.uart_stop_bits = UART_STOP_BITS_1,
		.uart_flow_ctrl = hwflowc == true ? UART_FLOW_CONTROL_ENABLE : UART_FLOW_CONTROL_DISABLE,
		.uart_tx_mode = UART_TX_MODE_STREAM,
		.uart_rx_mode = UART_RX_MODE_USER,
	};
	switch(parity)
	{
		case 1:  config.uart_parity_bit = UART_PARITY_BIT_ODD; break;
		case 2:  config.uart_parity_bit = UART_PARITY_BIT_EVEN; break;
		default: config.uart_parity_bit = UART_PARITY_BIT_NONE; break;
	}
	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		uart = E_UART_NUM_1;
		chip_uart1_pinmux_cfg(hwflowc);
	}
	else
	{
		chip_uart0_pinmux_cfg(UART0_RX_USED_GPIO5, UART0_TX_USED_GPIO6, hwflowc);
	}
	drv_uart_open(uart, &config);
	T_UART_ISR_CALLBACK callback;
	callback.uart_callback = uart_cb;
	drv_uart_ioctrl(uart, DRV_UART_CTRL_REGISTER_RX_CALLBACK, &callback);
	drv_uart_ioctrl(uart, DRV_UART_CTRL_GET_REG_BASE, (void*)(&uart_base));
	return 1;
}

#endif
#if PLATFORM_ARMINO

#include "../hal_uart.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"
#include "driver/uart.h"
#include "driver/hal/hal_uart_types.h"

extern bk_err_t uart_write_byte(uart_id_t id, uint8_t data);
extern int uart_read_byte(uart_id_t id);

static void uart_isr(uart_id_t port, void* param)
{
	int rc = 0;
	int fbufindex = UART_GetBufIndexFromPort(port);

	while((rc = uart_read_byte(port)) != -1)
		UART_AppendByteToReceiveRingBufferEx(fbufindex, rc);
}

void HAL_UART_SendByteEx(int auartindex, byte b)
{
	uart_write_byte(auartindex, b);
}

int HAL_UART_InitEx(int auartindex, int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	uart_config_t config = { 0 };

	config.baud_rate = baud;
	config.data_bits = UART_DATA_8_BITS;
	config.parity = parity;
	config.stop_bits = 0;
	config.flow_ctrl = hwflowc == false ? UART_FLOWCTRL_DISABLE : UART_FLOWCTRL_CTS_RTS;
	config.src_clk = UART_SCLK_XTAL_26M;

	bk_uart_init(auartindex, &config);
	bk_uart_disable_sw_fifo(auartindex);
	bk_uart_register_rx_isr(auartindex, uart_isr, NULL);
	bk_uart_enable_rx_interrupt(auartindex);
	return 1;
}

#endif

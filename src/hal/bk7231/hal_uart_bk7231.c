#include "../hal_uart.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"
#include "../../../beken378/func/user_driver/BkDriverUart.h"

// from uart_bk.c
extern void bk_send_byte(UINT8 uport, UINT8 data);
int g_chosenUART = BK_UART_1;

void test_ty_read_uart_data_to_buffer(int port, void* param)
{
	int rc = 0;

	while((rc = uart_read_byte(port)) != -1)
		UART_AppendByteToReceiveRingBuffer(rc);
}

void HAL_UART_SendByte(byte b)
{
	// BK_UART_1 is defined to 0
	bk_send_byte(g_chosenUART, b);
}

int HAL_UART_Init(int baud, int parity)
{
    bk_uart_config_t config;

    config.baud_rate = baud;
    config.data_width = 0x03;
    config.parity = parity;    //0:no parity,1:odd,2:even
    config.stop_bits = 0;   //0:1bit,1:2bit
    config.flow_control = 0;   //FLOW_CTRL_DISABLED
    config.flags = 0;


    // BK_UART_1 is defined to 0
    if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
    {
        g_chosenUART = BK_UART_2;
    }
    else
    {
        g_chosenUART = BK_UART_1;
    }
    bk_uart_initialize(g_chosenUART, &config, NULL);
    bk_uart_set_rx_callback(g_chosenUART, test_ty_read_uart_data_to_buffer, NULL);
	return 1;
}
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "wm_regs.h"
#include "wm_7816.h"
#include "wm_gpio.h"
#include "wm_uart.h"
#include "wm_osal.h"
#include "wm_demo.h"
#include "wm_pmu.h"

#if DEMO_7816

struct sc_rx 
{
	uint8_t *buf;
	u16	buf_len;
	u16 require_len;
	u16 current_len;
	tls_os_sem_t *  sem_rx;
	uint8_t timeout;
}sc_rx;

extern void tls_uart_rx_callback_register(u16 uart_no, s16(*rx_callback) (u16 len, void* user_data), void *priv_data);
extern void tls_uart_free_tx_sent_data(struct tls_uart_port *port);

const char i2d_cmd[] = {	0x00,0xa4,0x04,0x04,0x10,0xa0,0x00,
							0x00,0x00,0x30,0x50,0x00,0x00,0x00,
							0x00,0x00,0x00,0x00,0x49,0x64,0x32 };

s16 uart2_rx_cb(uint16_t len)
{	
	if (len == 0xFFFF)
	{
		sc_rx.timeout = 1;
		tls_os_sem_release(sc_rx.sem_rx);
		return 0 ;
	}
	if(sc_rx.current_len + len < sc_rx.require_len)
	{
		tls_uart_read(2, &sc_rx.buf[sc_rx.current_len], len);
		sc_rx.current_len += len;
		sc_rx.timeout = 0;
	}
	else
	{
		tls_uart_read(2, &sc_rx.buf[sc_rx.current_len], len);
		sc_rx.current_len += len;
		sc_rx.timeout = 0;
		tls_os_sem_release(sc_rx.sem_rx);
	}
	return 1;
}

void wm_sc_atr_test()
{
	char sdata[4] = {0xff, 0x10, 0x95};
	//char rdata[4] = {0xa,0xb,0xc,0xd};
	char parity_byte = (uint8_t)(0xff ^ (uint8_t)0x10 ^ 0x95);
	sdata[3] = parity_byte;	
	char  test[32];

	uint16_t sc_state = 0;
	/*open 7816 clk*/
	tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART2);
	sc_io.clk_pin_num = WM_IO_PB_04;
	sc_io.clk_opt = WM_IO_OPTION2;

	sc_io.io_pin_num = WM_IO_PB_02;
	sc_io.io_opt = WM_IO_OPTION3;
	sc_io.initialed = 1;

	sc_rx.buf = (uint8_t *)test;
	sc_rx.buf_len = 32;
	sc_rx.require_len = 20;
	sc_rx.current_len = 0;
	sc_rx.timeout = 0;
	tls_os_sem_create(&sc_rx.sem_rx, 0);

	wm_sc_powerInit();
	tls_uart_port_init(2, NULL, 1);
	tls_uart_set_parity(2, TLS_UART_PMODE_EVEN);
	tls_uart_set_stop_bits(2, TLS_UART_TWO_STOPBITS);
	wm_sc_set_bcwt(0x1ff);
	
	tls_uart_rx_callback_register((u16) TLS_UART_2, (void *)uart2_rx_cb, NULL);
	tls_uart_tx_callback_register(2, (s16(*) (struct tls_uart_port *))tls_uart_free_tx_sent_data);
	
	while(1)
	{
		switch(sc_state)
		{
			case 0:
				wm_sc_colreset();
				break;
			case 1:
				if (test[0] == 0x3b)
				{
					sc_rx.require_len = 4;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					tls_uart_write(2, sdata, 4);
				}
				else
				{
					sc_state = 0;
					continue;
				}
				break;
			case 2:
				if (test[0] == 0xff)
				{
					if (!memcmp(sdata, test, 4))
					{
						wm_sc_set_etu(32);
					}
					sc_rx.require_len = 1;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					tls_uart_write(2, (char *)i2d_cmd, 5);
				}
				break;
			case 3:
				if (test[0] == 0xA4)
				{
					sc_rx.require_len = 2;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					tls_uart_write(2, (char *)&i2d_cmd[5], 16);
				}
				break;
			case 4:
				if (test[0] == 0x90)
				{
					test[0] = 0x00;
					test[1] = 0x36;
					test[2] = 0x00;
					test[3] = 0x00;
					test[4] = 0x03;
					sc_rx.require_len = 1;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					tls_uart_write(2, test, 5);
				}
				break;
			case 5:
				if (test[0] == 0x36)
				{
					test[0] = 0x41;
					test[1] = 0x00;
					test[2] = 0x41;
					sc_rx.require_len = 0x2;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					tls_uart_write(2, test, 3);
				}
				break;
			case 6:
				if (test[0] == 0x61)
				{
					test[0] = 0x00;
					test[1] = 0xc0;
					test[2] = 0x00;
					test[3] = 0x00;
					test[4] = 0x16;
					sc_rx.require_len = 0x19;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					tls_uart_write(2, test, 5);
				}
				break;	
			case 7:
				if (test[0] == 0xc0)
				{
					test[19] = 0;
					printf("id2:%s\r\n", &test[1]);
					printf("\nhot reset \n");
					sc_rx.require_len = 20;
					sc_rx.current_len = 0;
					sc_rx.timeout = 0;
					wm_sc_hotreset();
					sc_state = 0;					
				}
				break;
		}
		tls_os_sem_acquire(sc_rx.sem_rx, 0);
		if (!sc_rx.timeout)
		{
			sc_state++;
		}
		else
		{
			printf("timeout\r\n");
		}		
	}
}

#define    DEMO_TASK_SIZE      768
static OS_STK DemoTaskStk7816[DEMO_TASK_SIZE]; 
#define DEMO_CONSOLE_BUF_SIZE   512

int wm_7816_demo(void)
{
	tls_os_task_create(NULL, NULL,
			wm_sc_atr_test,
                    NULL,
                    (void *)DemoTaskStk7816,
                    DEMO_TASK_SIZE * sizeof(u32),
                    61,
                    0);
    return WM_SUCCESS;
}

#endif


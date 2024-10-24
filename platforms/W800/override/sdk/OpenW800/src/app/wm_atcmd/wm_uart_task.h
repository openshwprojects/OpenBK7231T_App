/***************************************************************************** 
* 
* File Name : wm_uart.h 
* 
* Description: uart Driver Module 
* 
* Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-6-4
*****************************************************************************/ 

#ifndef WM_UART_TASK_H
#define WM_UART_TASK_H

#include "wm_cmdp.h"
#include "wm_uart.h"
#include "wm_osal.h"
#define INS_SYNC_CHAR       0x01
#define INS_RICMD           0x02
#define INS_DATA            0x04

#define RICMD_SYNC_FLAG     0xAA

struct uart_ricmd_info {
    u8  sync_head[20];
    u32 cbytes;
    u16 length;
    u8  dest;
};

typedef struct tls_uart{
	struct tls_uart_port *uart_port;
	/** uart rx semaphore, notify receive a char */
//	tls_os_mailbox_t              *rx_mailbox;
	/** uart tx semaphore, notify tx empty */
//	tls_os_mailbox_t              *tx_mailbox;
	u32                         cmd_mode;

	//bool	rx_idle;
	u8	inputstate;
    
	    /** 
	     * tx callbak, notify user application tx complete, 
	     * user can use it, write new data to uart for transmit
	     */
	void (*tx_cb)(struct tls_uart *uart);
	struct uart_ricmd_info ricmd_info;
	u16 sksnd_cnt;
} tls_uart_t;

struct tls_uart *tls_uart_open(u32 uart_no, TLS_UART_MODE_T uart_mode);

void tls_uart_set_at_cmd_port(int at_cmd_port);
int tls_uart_get_at_cmd_port(void);


#endif /* WM_UART_TASK_H */

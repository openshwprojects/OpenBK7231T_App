/**************************************************************************//**
 * @file     wm_i2s_demo.c
 * @version  
 * @date 
 * @author    
 * @note
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. All rights reserved.
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wm_i2s.h"
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_gpio_afsel.h"


#if DEMO_I2S

#define DEMO_DATA_SIZE        (1024)

extern int wm_i2s_tranceive_dma(uint32_t i2s_mode, wm_dma_handler_type *hdma_tx, wm_dma_handler_type *hdma_rx, uint16_t *data_tx, uint16_t *data_rx, uint16_t len);

enum 
{
    WM_I2S_MODE_INT,
    WM_I2S_MODE_DMA
};

enum
{
    WM_I2S_TX = 1,
    WM_I2S_RX = 2,
    WM_I2S_TR = 3,
    WM_I2S_TR_SLAVE = 4,
};

uint32_t i2s_demo_rx[DEMO_DATA_SIZE] = { 0 };
uint32_t i2s_demo_tx[DEMO_DATA_SIZE] = { 0 };

static void show_rx_data(u16 len)
{
    printf("recv %d\r\n", len);
    for(u16 i=0; i<len; i++) 
    {
        printf("%08X ", i2s_demo_rx[i]);
        if(i % 16 == 15)
        {
        	printf("\n");
        }
    }
}

static void tls_i2s_tx_int_demo()
{
    for(u16 len = 0; len < DEMO_DATA_SIZE; len++)
    {
        i2s_demo_tx[len] = 0xA55A55A0+len;
    }

	wm_i2s_tx_int((int16_t *)i2s_demo_tx, DEMO_DATA_SIZE, NULL);
    printf("send %d\r\n", DEMO_DATA_SIZE);
}

static void tls_i2s_rx_int_demo()
{
    memset((u8 *)i2s_demo_rx, 0, DEMO_DATA_SIZE*sizeof(i2s_demo_rx[0]));
	wm_i2s_rx_int((int16_t *)i2s_demo_rx, DEMO_DATA_SIZE);
	show_rx_data(DEMO_DATA_SIZE);
}

static void tls_i2s_tr_int_demo(I2S_InitDef *opts)
{
    for(u16 len = 0; len < DEMO_DATA_SIZE; len++)
    {
        i2s_demo_tx[len] = 0xA55A55A0+len;
    }
    memset((u8 *)i2s_demo_rx, 0, DEMO_DATA_SIZE*sizeof(i2s_demo_rx[0]));
	wm_i2s_tx_rx_int(opts, (int16_t *)i2s_demo_tx, (int16_t *)i2s_demo_rx, DEMO_DATA_SIZE);
	show_rx_data(DEMO_DATA_SIZE);
}
static uint32_t g_tx_buff_val = 0xA55A55A0;
static wm_dma_handler_type hdma_tx;
static void i2sDmaSendCpltCallback(wm_dma_handler_type *hdma)
{
//	int i = DEMO_DATA_SIZE/2;
//	for(; i < DEMO_DATA_SIZE; i++)
//	{
//		i2s_demo_tx[i] = g_tx_buff_val++;
//	}
}
static void i2sDmaSendHalfCpltCallback(wm_dma_handler_type *hdma)
{
//	int i = 0;
//	for(; i < DEMO_DATA_SIZE/2; i++)
//	{
//		i2s_demo_tx[i] = g_tx_buff_val++;
//	}
}
static void tls_i2s_tx_dma_demo()
{	
    for(u16 len = 0; len < DEMO_DATA_SIZE; len++)
    {
        i2s_demo_tx[len] = g_tx_buff_val++;
    }
#if 0
	wm_i2s_tx_dma((int16_t *)i2s_demo_tx, DEMO_DATA_SIZE, NULL);
	printf("send %d\r\n", DEMO_DATA_SIZE);
#else
	memset(&hdma_tx, 0, sizeof(wm_dma_handler_type));
	hdma_tx.XferCpltCallback = i2sDmaSendCpltCallback;
	hdma_tx.XferHalfCpltCallback = i2sDmaSendHalfCpltCallback;
	wm_i2s_transmit_dma(&hdma_tx, (uint16_t *)i2s_demo_tx, DEMO_DATA_SIZE * 2);
	printf("dma transmit start\n");
#endif
}

static wm_dma_handler_type hdma_rx;
static volatile uint8_t g_recv_state = 0; //1: recv half cplt; 2: recv cplt;
static volatile uint32_t g_recv_count = 0;
static void i2sDmaRecvCpltCallback(wm_dma_handler_type *hdma)
{
	g_recv_state = 2;
	g_recv_count++;
//	printf("%d\n", g_recv_count);
}
static void i2sDmaRecvHalfCpltCallback(wm_dma_handler_type *hdma)
{
	g_recv_state = 1;
}
static void tls_i2s_rx_dma_demo()
{
    memset((u8 *)i2s_demo_rx, 0, DEMO_DATA_SIZE*sizeof(i2s_demo_rx[0]));
#if 0
	wm_i2s_rx_dma((int16_t *)i2s_demo_rx, DEMO_DATA_SIZE);
	show_rx_data(DEMO_DATA_SIZE);
#else
	memset(&hdma_rx, 0, sizeof(wm_dma_handler_type));
	hdma_rx.XferCpltCallback = i2sDmaRecvCpltCallback;
	hdma_rx.XferHalfCpltCallback = i2sDmaRecvHalfCpltCallback;
	wm_i2s_receive_dma(&hdma_rx, (uint16_t *)i2s_demo_rx, DEMO_DATA_SIZE*2);
#endif
}
static void tls_i2s_tr_dma_demo(I2S_InitDef *opts)
{
	for(u16 len = 0; len < DEMO_DATA_SIZE; len++)
    {
        i2s_demo_tx[len] = 0xA55A55A0+len;
    }
    memset((u8 *)i2s_demo_rx, 0, DEMO_DATA_SIZE*sizeof(i2s_demo_rx[0]));
#if 0
	wm_i2s_tx_rx_dma(opts, (int16_t *)i2s_demo_tx, (int16_t *)i2s_demo_rx, DEMO_DATA_SIZE);
	show_rx_data(DEMO_DATA_SIZE);
#else
	memset(&hdma_tx, 0, sizeof(wm_dma_handler_type));
	hdma_tx.XferCpltCallback = i2sDmaSendCpltCallback;
	hdma_tx.XferHalfCpltCallback = i2sDmaSendHalfCpltCallback;
	memset(&hdma_rx, 0, sizeof(wm_dma_handler_type));
	hdma_rx.XferCpltCallback = i2sDmaRecvCpltCallback;
	hdma_rx.XferHalfCpltCallback = i2sDmaRecvHalfCpltCallback;
	wm_i2s_tranceive_dma(opts->I2S_Mode_MS, &hdma_tx, &hdma_rx, (uint16_t *)i2s_demo_tx, (uint16_t *)i2s_demo_rx, DEMO_DATA_SIZE*2);
#endif
}

/**
 * @brief              
 *
 * @param[in]  format
 *	- \ref 0: i2s
 *	- \ref 1: msb
 *	- \ref 2: pcma
 *	- \ref 3: pcmb 
 *
 * @param[in]  tx_rx
 *    - \ref 1: transmit
 *    - \ref 2: receive
 *    - \ref 3: duplex master
 *    - \ref 4: duplex slave
 *
 * @param[in]  freq
 *    sample rate 
 *
 * @param[in]  datawidth 
 *    - \ref 8: 8 bit
 *    - \ref 16: 16 bit
 *    - \ref 24: 24 bit
 *    - \ref 32: 32 bit 
 *
 * @param[in]  stereo   
 *    - \ref 0: stereo
 *	  - \ref 1: mono
 *
 * @param[in]  mode         
 *    - \ref 0: interrupt
 *    - \ref 1: dma
 *
 * @retval            
 *
 * @note 
 * t-i2s=(0,1,44100,16,0,0)  -- M_I2S send(ISR mode) 
 * t-i2s=(0,1,44100,16,0,1)  -- M_I2S send(DMA mode)
 * t-i2s=(0,2,44100,16,0,0)  -- S_I2S recv(ISR mode)
 * t-i2s=(0,2,44100,16,0,1)  -- S_I2S recv(DMA mode)
 */
int tls_i2s_demo(s8  format,
	             s8  tx_rx,
	             s32 freq,  
	             s8  datawidth, 
	             s8  stereo,
	             s8  mode) 
{	
	int i = 0, count = 0;
	I2S_InitDef opts = { I2S_MODE_MASTER, I2S_CTRL_STEREO, I2S_RIGHT_CHANNEL, I2S_Standard, I2S_DataFormat_16, 8000, 5000000 };

	opts.I2S_Mode_MS = (tx_rx-1);
	opts.I2S_Mode_SS = (stereo<<22);
	opts.I2S_Mode_LR = I2S_LEFT_CHANNEL;
	opts.I2S_Trans_STD = (format*0x1000000);
	opts.I2S_DataFormat = (datawidth/8 - 1)*0x10;
	opts.I2S_AudioFreq = freq;
	opts.I2S_MclkFreq = 80000000;
	
    printf("\r\n");
	printf("format:%d, tx_en:%d, freq:%d, ", format, tx_rx, freq);
    printf("datawidth:%d, stereo:%d, mode:%d\r\n", datawidth, stereo, mode);
	wm_i2s_port_init(&opts);
	wm_i2s_register_callback(NULL);

	if (WM_I2S_MODE_INT == mode)
	{
	    if (tx_rx == WM_I2S_TX)
	    {
	        tls_i2s_tx_int_demo();
	    }
	    if (tx_rx == WM_I2S_RX)
	    {
	        tls_i2s_rx_int_demo();
	    }
		if (tx_rx == WM_I2S_TR)
	    {
	    	opts.I2S_Mode_MS = I2S_MODE_MASTER;
	        tls_i2s_tr_int_demo(&opts);
	    }
	}
	else if (WM_I2S_MODE_DMA == mode)
	{
	    if (tx_rx == WM_I2S_TX)
	    {
	        tls_i2s_tx_dma_demo();
	    }
	    else if (tx_rx == WM_I2S_RX)
	    {
	        tls_i2s_rx_dma_demo();
	    }
	    else if (tx_rx == WM_I2S_TR)
	    {
	    	opts.I2S_Mode_MS = I2S_MODE_MASTER;
	        tls_i2s_tr_dma_demo(&opts);
	    }
	    else if (tx_rx == WM_I2S_TR_SLAVE)
	    {
	    	opts.I2S_Mode_MS = I2S_MODE_SLAVE;
	        tls_i2s_tr_dma_demo(&opts);
	    }
	    if(tx_rx > WM_I2S_TX)
	    {
	        while(1)
	        {
	        	if(g_recv_state == 1)
	        	{
		        	g_recv_state = 0;
	        		count = 0;
	        		i = 1;
	        		for(; i < DEMO_DATA_SIZE/2; i++)
	        		{
	        			if(i2s_demo_rx[i] - i2s_demo_rx[i-1] != 1)
	        			{
	        				count++;
	        				printf("%d: %x-%x\n", i, i2s_demo_rx[i-1], i2s_demo_rx[i]);
	        			}
	        		}
	        	//	if(count)
	        		{
	        			printf("%d half %X-%X, err %d\n", g_recv_count, i2s_demo_rx[0], i2s_demo_rx[i - 1], count);
	        		}
	        	}
	        	else if(g_recv_state == 2)
	        	{
		        	g_recv_state = 0;
		        	if(g_recv_count>0 && g_recv_count % 100 == 0)
		        	{
		        		printf("\n[%d]total receive %d*4KB bytes\n\n", tls_os_get_time(), g_recv_count);
		        	}

	        		count = 0;
	        		i = DEMO_DATA_SIZE/2 + 1;
	        		for(; i < DEMO_DATA_SIZE; i++)
	        		{
	        			if(i2s_demo_rx[i] - i2s_demo_rx[i-1] != 1)
	        			{
	        				count++;
	        				printf("%d: %x-%x\n", i, i2s_demo_rx[i-1], i2s_demo_rx[i]);
	        			}
	        		}
	        	//	if(count)
	        		{
	        			printf("%d all %X-%X, err %d\n", g_recv_count, i2s_demo_rx[DEMO_DATA_SIZE/2], i2s_demo_rx[i - 1], count);
	        		}
	        	}
	        }
	    }
	}
	if (WM_I2S_MODE_INT == mode)
	{
		wm_i2s_tx_rx_stop();
		printf("\ntest done.\n");
	}
	
    return WM_SUCCESS;
}
int tls_i2s_io_init(void)
{
	wm_i2s_ck_config(WM_IO_PB_08);
	wm_i2s_ws_config(WM_IO_PB_09);
	wm_i2s_di_config(WM_IO_PB_10);
	wm_i2s_do_config(WM_IO_PB_11);
    printf("\r\n");
	printf("ck--PB08, ws--PB09, di--PB10, do--PB11;\r\n");
    return WM_SUCCESS;
}

#endif

/*** (C) COPYRIGHT 2014 Winner Microelectronics Co., Ltd. ***/

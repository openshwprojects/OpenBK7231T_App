/**
 * @file    wm_master_spi_demo.c
 *
 * @brief   SPI master demo function
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"

#if DEMO_MASTER_SPI
#include "wm_hostspi.h"
#include "wm_gpio_afsel.h"

#define SPI_DATA_LEN    1508
int master_spi_send_data(int clk, int type)
{
    int *p;
    int i;
	char *tx_buf = NULL;

	/*MASTER SPI configuratioin*/
	wm_spi_cs_config(WM_IO_PB_04);
	wm_spi_ck_config(WM_IO_PB_02);
	wm_spi_di_config(WM_IO_PB_03);
	wm_spi_do_config(WM_IO_PB_05);
	printf("\r\n");
	printf("cs--PB04, ck--PB02, di--PB03, do--PB05;\r\n");
	
    if (clk < 0)
    {
        clk = 1000000;          /* default 1M */
    }
    if (-1 == type)
    {
        type = 0;
    }

    if (0 == type)
    {
        tls_spi_trans_type(0);
    }
    else
    {
        tls_spi_trans_type(2);
    }

    tls_spi_setup(TLS_SPI_MODE_0, TLS_SPI_CS_LOW, clk);

    tx_buf = tls_mem_alloc(SPI_DATA_LEN);
    if (NULL == tx_buf)
    {
        printf("\nspi_demo tx mem err\n");
        return WM_FAILED;
    }	

    memset(tx_buf,  0, SPI_DATA_LEN);
    strcpy(tx_buf, "data");
    p = (int *)&tx_buf[4];
    *p = 1500;
    p ++;
    for(i = 0;i < (SPI_DATA_LEN-8)/4;i ++)    
    {
        *p = 0x12345678;
         p ++;
    }
    printf("SPI Master send 1500 byte, modeA, little endian\n");    

	tls_spi_write((u8 *)tx_buf, SPI_DATA_LEN);

    tls_mem_free(tx_buf);
	
    printf("after send\n");
	return WM_SUCCESS;
}

int master_spi_recv_data(int clk, int type)
{
    int *p;
    int i;
    int len;
    int errorflag = 0;
	char *tx_buf = NULL;
	char *rx_buf = NULL;

	/*MASTER SPI configuratioin*/
	wm_spi_cs_config(WM_IO_PB_04);
	wm_spi_ck_config(WM_IO_PB_02);
	wm_spi_di_config(WM_IO_PB_03);
	wm_spi_do_config(WM_IO_PB_05);
	printf("\r\n");
	printf("cs--PB04, ck--PB02, di--PB03, do--PB05;\r\n");
	
    if (clk < 0)
    {
        clk = 1000000;          /* default 1M */
    }
    if (-1 == type)
    {
        type = 0;
    }

    if (0 == type)
    {
        tls_spi_trans_type(0);
    }
    else
    {
        tls_spi_trans_type(2);
    }

    tls_spi_setup(TLS_SPI_MODE_0, TLS_SPI_CS_LOW, clk);

	printf("SPI Master receive 1500 byte, modeA, little endian\n");

    tx_buf = tls_mem_alloc(SPI_DATA_LEN);
    if (NULL == tx_buf)
    {
        printf("\nspi_demo tx mem err\n");
        return WM_FAILED;
    }

    memset(tx_buf,  0, SPI_DATA_LEN);
    strcpy(tx_buf, "up-m");
	p = (int *)&tx_buf[4];
    *p = 1500;
	tls_spi_write((u8 *)tx_buf, SPI_DATA_LEN);

	tls_os_time_delay(100);			

    rx_buf = tls_mem_alloc(SPI_DATA_LEN);
    if (NULL == rx_buf)
    {
    	tls_mem_free(tx_buf);
        printf("\nspi_demo rx mem err\n");
        return WM_FAILED;
    }
    
    memset(rx_buf, 0, SPI_DATA_LEN);
    tls_spi_read((u8 *)rx_buf, SPI_DATA_LEN);
    p = (int *)&rx_buf[0];
    len = *p;
    p ++;
    for(i = 0;i < len/4;i ++)
    {
        if(*(p + i) != 0x12345678)
        {
            errorflag ++;
            printf("[%d]=[%x]\n",i,  *(p + i));
            if(errorflag > 100)
                break;
        }
    }
    if(errorflag > 0)
    {
        printf("rcv spi data error\r\n");
    }
    else
    {
        printf("rcv data len: %d\n", len);
    } 
	
    tls_mem_free(tx_buf);
    tls_mem_free(rx_buf);

	return WM_SUCCESS;
}



#endif




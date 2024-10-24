/**
 * @file    wm_slave_spi_demo.c
 *
 * @brief   SPI slave demo function
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_mem.h"
#include "wm_gpio_afsel.h"


#if DEMO_SLAVE_SPI

#define USER_DEBUG		1
#if USER_DEBUG
#define USER_PRINT printf
#else
#define USER_PRINT(fmt, ...)
#endif
#define HSPI_BUF_SIZE   1024

u32 count = 0;

static u8 GetCrc(u8 *buf, u16 len)
{
    u8 crc = 0;
    int i = 0;

    if(buf != NULL && len > 0)
    {
        for(i=0; i<len; i++)
            crc += buf[i];
    }

    return crc;
}

static s16 HspiRxDataCb(char *buf)
{
    int i = 0, err_num = 0;

    for(i=0; i<HSPI_BUF_SIZE; i++)
    {
        if(buf[i] != ((i + 1)%255))
            err_num++;
    }

    if(err_num != 0)
    {
        USER_PRINT("err_num = %d\n", err_num);
        return -1;
    }
    else
    {
        count++;
        if(count%100 == 0)
            USER_PRINT("RX ok %d\n", count);
    }

    tls_hspi_tx_data(buf, HSPI_BUF_SIZE);
	return 0;
}

static s16 HspiRxCmdCb(char *buf)
{
    u16 len = 0;
    u8 *tx_buf = NULL;
    int i = 0;
	printf("%s\n", __func__);
    if(buf[0] != 0x5A)
        return -1;
    
    len = buf[1] << 8 | buf[2];
    USER_PRINT("rx[%d] :", len);
    for(i=0; i<len; i++)
        USER_PRINT("%02x ", buf[i]);
    USER_PRINT("\n");

    if(buf[len - 1] != GetCrc((u8 *)buf, len - 1))
        return -1;

    if(buf[3] == 0x01)
    {
        tx_buf = tls_mem_alloc(HSPI_BUF_SIZE);
        if(tx_buf == NULL)
            return -1;
        for(i=0; i<HSPI_BUF_SIZE; i++)
            tx_buf[i] = (i + 1)%255;
        
        tls_hspi_tx_data((char *)tx_buf, HSPI_BUF_SIZE);
        tls_mem_free(tx_buf);
		tx_buf = NULL;
    }
	return 0;
}

static void HspiInit(int type)
{

    
    wm_hspi_gpio_config(0);

    tls_slave_spi_init();
    tls_set_high_speed_interface_type(type);
    tls_set_hspi_user_mode(1);
    tls_hspi_rx_data_callback_register(HspiRxDataCb);
    tls_hspi_rx_cmd_callback_register(HspiRxCmdCb);

}

int slave_spi_demo(int type)
{
    if(type == 0)
    {
        type = HSPI_INTERFACE_SPI;
    }
    else
    {
        type = HSPI_INTERFACE_SDIO;
    }
    printf("\r\ntype:%d\r\n", type);

    HspiInit(type);

    return WM_SUCCESS;
}


#endif

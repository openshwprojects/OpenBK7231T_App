/**
 * @file wm_gpio.c
 *
 * @brief GPIO Driver Module
 *
 * @author dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#include "wm_gpio.h"
#include "wm_regs.h"
#include "wm_irq.h"
#include "wm_osal.h"
#include "tls_common.h"
#include "wm_gpio_afsel.h"
#include "wm_debug.h"
#include "wm_pmu.h"

#ifndef WM_SWD_ENABLE
#define WM_SWD_ENABLE   1
#endif
void wm_hspi_gpio_config(uint8_t numsel)
{
    switch(numsel)
    {
		case 0:
			tls_io_cfg_set(WM_IO_PB_06, WM_IO_OPTION3);/*CK*/
			tls_io_cfg_set(WM_IO_PB_07, WM_IO_OPTION3);/*INT*/
			tls_io_cfg_set(WM_IO_PB_09, WM_IO_OPTION3);/*CS*/
			tls_io_cfg_set(WM_IO_PB_10, WM_IO_OPTION3);/*DI*/
			tls_io_cfg_set(WM_IO_PB_11, WM_IO_OPTION3);/*DO*/
			break;

		case 1://W801
			tls_io_cfg_set(WM_IO_PB_12, WM_IO_OPTION1);/*CK*/
			tls_io_cfg_set(WM_IO_PB_13, WM_IO_OPTION1);/*INT*/
			tls_io_cfg_set(WM_IO_PB_14, WM_IO_OPTION1);/*CS*/
			tls_io_cfg_set(WM_IO_PB_15, WM_IO_OPTION1);/*DI*/
			tls_io_cfg_set(WM_IO_PB_16, WM_IO_OPTION1);/*DO*/
			break;			
		
		default:
			TLS_DBGPRT_ERR("highspeed spi gpio config error!");
			break;
    }
}

void wm_spi_ck_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_01:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_02:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_15://w801
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

	case WM_IO_PB_24://w801
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;


    default:
        TLS_DBGPRT_ERR("spi ck afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_LSPI);
}

void wm_spi_cs_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_04:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PB_14://w801
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PB_23://w801
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;		


    default:
        TLS_DBGPRT_ERR("spi cs afsel config error!");
        break;
    }
}


void wm_spi_di_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_03:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

	case WM_IO_PB_16://w801
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;

	case WM_IO_PB_25://w801
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;

    default:
        TLS_DBGPRT_ERR("spi di afsel config error!");
        break;
    }
}

void wm_spi_do_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_07:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_05:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

	case WM_IO_PB_17://w801
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;

	case WM_IO_PB_26://w801
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;

    default:
        TLS_DBGPRT_ERR("spi do afsel config error!");
        break;
    }
}

void wm_sdio_host_config(uint8_t numsel)
{
	switch(numsel)
	{
		case 0://w800 or w801
			tls_io_cfg_set(WM_IO_PB_06, WM_IO_OPTION2);/*CK*/
			tls_io_cfg_set(WM_IO_PB_07, WM_IO_OPTION2);/*CMD*/
			tls_io_cfg_set(WM_IO_PB_08, WM_IO_OPTION2);/*D0*/
			tls_io_cfg_set(WM_IO_PB_09, WM_IO_OPTION2);/*D1*/
			tls_io_cfg_set(WM_IO_PB_10, WM_IO_OPTION2);/*D2*/
			tls_io_cfg_set(WM_IO_PB_11, WM_IO_OPTION2);/*D3*/
			tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_SDIO_MASTER);
			break;

		case 1: 
			tls_io_cfg_set(WM_IO_PA_09, WM_IO_OPTION1);/*CK*/
			tls_io_cfg_set(WM_IO_PA_10, WM_IO_OPTION1);/*CMD*/
			tls_io_cfg_set(WM_IO_PA_11, WM_IO_OPTION1);/*D0*/
			tls_io_cfg_set(WM_IO_PA_12, WM_IO_OPTION1);/*D1*/
			tls_io_cfg_set(WM_IO_PA_13, WM_IO_OPTION1);/*D2*/
			tls_io_cfg_set(WM_IO_PA_14, WM_IO_OPTION1);/*D3*/
			tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_SDIO_MASTER);
			break;

		default:
			TLS_DBGPRT_ERR("sdio host afsel config error!");
			break;
	}
}

void wm_sdio_slave_config(uint8_t numsel)
{
	switch(numsel)
	{
		case 0:
			tls_io_cfg_set(WM_IO_PB_06, WM_IO_OPTION4);/*CK*/
			tls_io_cfg_set(WM_IO_PB_07, WM_IO_OPTION4);/*CMD*/
			tls_io_cfg_set(WM_IO_PB_08, WM_IO_OPTION4);/*D0*/
			tls_io_cfg_set(WM_IO_PB_09, WM_IO_OPTION4);/*D1*/
			tls_io_cfg_set(WM_IO_PB_10, WM_IO_OPTION4);/*D2*/
			tls_io_cfg_set(WM_IO_PB_11, WM_IO_OPTION4);/*D3*/
			break;

		default:
			TLS_DBGPRT_ERR("sdio slave afsel config error!");
			break;
	}
}

void wm_psram_config(uint8_t numsel)
{
	switch(numsel)
	{
		case 0://W800 or w801
			tls_io_cfg_set(WM_IO_PB_00, WM_IO_OPTION4);/*CK*/
			tls_io_cfg_set(WM_IO_PB_01, WM_IO_OPTION4);/*CS*/
			tls_io_cfg_set(WM_IO_PB_02, WM_IO_OPTION4);/*D0*/
			tls_io_cfg_set(WM_IO_PB_03, WM_IO_OPTION4);/*D1*/
			tls_io_cfg_set(WM_IO_PB_04, WM_IO_OPTION4);/*D2*/
			tls_io_cfg_set(WM_IO_PB_05, WM_IO_OPTION4);/*D3*/
			tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PSRAM);			
			break;
			
		case 1://w801
			tls_io_cfg_set(WM_IO_PA_15, WM_IO_OPTION1);/*CK*/
			tls_io_cfg_set(WM_IO_PB_27, WM_IO_OPTION1);/*CS*/
			tls_io_cfg_set(WM_IO_PB_02, WM_IO_OPTION4);/*D0*/
			tls_io_cfg_set(WM_IO_PB_03, WM_IO_OPTION4);/*D1*/
			tls_io_cfg_set(WM_IO_PB_04, WM_IO_OPTION4);/*D2*/
			tls_io_cfg_set(WM_IO_PB_05, WM_IO_OPTION4);/*D3*/
			tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PSRAM);			
			break;

		case 2://w861
			tls_io_cfg_set(WM_IO_PA_15, WM_IO_OPTION1);/*CK*/
			tls_io_cfg_set(WM_IO_PB_27, WM_IO_OPTION1);/*CS*/
			tls_io_cfg_set(WM_IO_PB_28, WM_IO_OPTION1);/*D0*/
			tls_io_cfg_set(WM_IO_PB_29, WM_IO_OPTION1);/*D1*/
			tls_io_cfg_set(WM_IO_PB_30, WM_IO_OPTION1);/*D2*/
			tls_io_cfg_set(WM_IO_PB_31, WM_IO_OPTION1);/*D3*/
			tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PSRAM);			
			break;

		default:
			TLS_DBGPRT_ERR("psram afsel config error!");
			break;
	}
}
void wm_uart0_tx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_19:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

	case WM_IO_PB_27:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("uart0 tx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART0);
}

void wm_uart0_rx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_20:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        tls_bitband_write(HR_GPIOB_DATA_PULLEN, 20, 0);
        break;

    default:
        TLS_DBGPRT_ERR("uart0 rx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART0);
}

void wm_uart1_tx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_06:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    default:
        TLS_DBGPRT_ERR("uart1 tx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART1);
}

void wm_uart1_rx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_07:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        tls_bitband_write(HR_GPIOB_DATA_PULLEN, 7, 0);
        break;

	case WM_IO_PB_16:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		tls_bitband_write(HR_GPIOB_DATA_PULLEN, 16, 0);
		break;

    default:
        TLS_DBGPRT_ERR("uart1 rx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART1);
}

void wm_uart1_rts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_19:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PA_02:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;

    default:
        TLS_DBGPRT_ERR("uart1 rts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART1);
}

void wm_uart1_cts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_20:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PA_03:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;		
    default:
        TLS_DBGPRT_ERR("uart1 cts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART1);
}

void wm_uart2_tx_scio_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_02:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PA_02:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("uart2 tx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART2);
}

void wm_uart2_rx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_03:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        tls_bitband_write(HR_GPIOB_DATA_PULLEN, 3, 0);
        break;
	case WM_IO_PA_03:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
        tls_bitband_write(HR_GPIOA_DATA_PULLEN, 3, 0);		
		break;

    default:
        TLS_DBGPRT_ERR("uart2 rx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART2);
}

void wm_uart2_rts_scclk_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_04:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;
	case WM_IO_PA_05:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("uart2 rts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART2);
}

void wm_uart2_cts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_05:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;
	case WM_IO_PA_06:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("uart2 cts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART2);
}

void wm_uart3_tx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PA_05:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;

    default:
        TLS_DBGPRT_ERR("uart3 tx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART3);
}

void wm_uart3_rx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_01:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        tls_bitband_write(HR_GPIOB_DATA_PULLEN, 1, 0);
        break;
	case WM_IO_PA_06:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
        tls_bitband_write(HR_GPIOA_DATA_PULLEN, 6, 0);		
		break;

    default:
        TLS_DBGPRT_ERR("uart3 rx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART3);
}

void wm_uart3_rts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
	case WM_IO_PA_02:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

    default:
        TLS_DBGPRT_ERR("uart1 rts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART3);
}

void wm_uart3_cts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
	case WM_IO_PA_03:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;		
    default:
        TLS_DBGPRT_ERR("uart1 cts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART3);
}


void wm_uart4_tx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_04:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;    
	case WM_IO_PA_08:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    default:
        TLS_DBGPRT_ERR("uart4 tx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART4);
}

void wm_uart4_rx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_05:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        tls_bitband_write(HR_GPIOB_DATA_PULLEN, 5, 0);
        break;
	case WM_IO_PA_09:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		tls_bitband_write(HR_GPIOA_DATA_PULLEN, 9, 0);
		break;

    default:
        TLS_DBGPRT_ERR("uart4 rx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART4);
}

void wm_uart4_rts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
	case WM_IO_PA_05:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;
	case WM_IO_PA_10:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("uart1 rts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART4);
}

void wm_uart4_cts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
	case WM_IO_PA_06:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;	
	case WM_IO_PA_11:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;		
    default:
        TLS_DBGPRT_ERR("uart1 cts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART4);
}

void wm_uart5_tx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_12:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;    
	case WM_IO_PA_08:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PB_18:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;

    default:
        TLS_DBGPRT_ERR("uart4 tx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART5);
}

void wm_uart5_rx_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_13:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        tls_bitband_write(HR_GPIOA_DATA_PULLEN, 13, 0);
        break;
	case WM_IO_PA_09:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		tls_bitband_write(HR_GPIOA_DATA_PULLEN, 9, 0);
		break;
	case WM_IO_PB_17:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		tls_bitband_write(HR_GPIOB_DATA_PULLEN, 17, 0);
		break;
    default:
        TLS_DBGPRT_ERR("uart4 rx afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART5);
}

void wm_uart5_rts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
	case WM_IO_PB_12:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;
	case WM_IO_PA_14:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("uart1 rts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART5);
}

void wm_uart5_cts_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
	case WM_IO_PB_13:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;	
	case WM_IO_PA_15:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;		
    default:
        TLS_DBGPRT_ERR("uart1 cts afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_UART5);
}


void wm_i2s_ck_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_04:
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;

    case WM_IO_PB_08:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PA_08:
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;	
		
	case WM_IO_PB_12:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

    default:
        TLS_DBGPRT_ERR("i2s master ck afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2S);
}

void wm_i2s_ws_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_01:
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;

    case WM_IO_PB_09:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

	case WM_IO_PA_09:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

	case WM_IO_PB_13:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

    default:
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2S);
}

void wm_i2s_do_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;

    case WM_IO_PB_11:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

	case WM_IO_PA_10:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

	case WM_IO_PB_14:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

    default:
        TLS_DBGPRT_ERR("i2s master do afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2S);
}

void wm_i2s_di_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_07:
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;

    case WM_IO_PB_10:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

	case WM_IO_PA_11:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

	case WM_IO_PB_15:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

    default:
        TLS_DBGPRT_ERR("i2s slave di afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2S);

}

void wm_i2s_mclk_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PA_07:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PB_17:
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;


    default:
        TLS_DBGPRT_ERR("i2s mclk afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2S);

}

void wm_i2s_extclk_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_07:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PB_17:
		tls_io_cfg_set(io_name, WM_IO_OPTION4);
		break;

    default:
        TLS_DBGPRT_ERR("i2s extclk afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2S);

}


void wm_i2c_scl_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_01:
        tls_gpio_cfg(io_name, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_20:
        tls_gpio_cfg(io_name, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;

    default:
        TLS_DBGPRT_ERR("i2c scl afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2C);
}

void wm_i2c_sda_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_04:
        tls_gpio_cfg(io_name, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_19:
        tls_gpio_cfg(io_name, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
        tls_io_cfg_set(io_name, WM_IO_OPTION4);
        break;

    default:
        TLS_DBGPRT_ERR("i2c sda afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_I2C);

}

void wm_pwm0_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PB_19:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PB_12:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

	case WM_IO_PA_02:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;

	case WM_IO_PA_10:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;


    default:
        TLS_DBGPRT_ERR("pwm0 afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PWM);
}


void wm_pwm1_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_01:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PB_20:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

    case WM_IO_PA_03:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;		
	case WM_IO_PA_11:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;
	case WM_IO_PB_13:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("pwm1 afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PWM);
}

void wm_pwm2_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_00:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PB_02:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PA_12:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PB_14:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

	case WM_IO_PB_24:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;


    default:
        TLS_DBGPRT_ERR("pwm2 afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PWM);
}

void wm_pwm3_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_01:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PB_03:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;

    case WM_IO_PA_13:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PB_15:
        tls_io_cfg_set(io_name, WM_IO_OPTION2);
        break;

	case WM_IO_PB_25:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("pwm3 afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PWM);
}

void wm_pwm4_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PA_04:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;

    case WM_IO_PA_07:
        tls_io_cfg_set(io_name, WM_IO_OPTION1);
        break;
	case WM_IO_PA_14:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;
	case WM_IO_PB_16:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;
	case WM_IO_PB_26:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("pwm4 afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PWM);
}

void wm_pwmbrk_config(enum tls_io_name io_name)
{
    switch(io_name)
    {
    case WM_IO_PB_08:
        tls_io_cfg_set(io_name, WM_IO_OPTION3);
        break;
	case WM_IO_PA_05:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;
	case WM_IO_PA_08:
		tls_io_cfg_set(io_name, WM_IO_OPTION1);
		break;
		
	case WM_IO_PA_15:
		tls_io_cfg_set(io_name, WM_IO_OPTION3);
		break;
		
	case WM_IO_PB_17:
		tls_io_cfg_set(io_name, WM_IO_OPTION2);
		break;

    default:
        TLS_DBGPRT_ERR("pwmbrk afsel config error!");
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_PWM);
}

void wm_swd_config(bool enable)
{
    if (enable)
    {
        tls_io_cfg_set(WM_IO_PA_01, WM_IO_OPTION1);
        tls_io_cfg_set(WM_IO_PA_04, WM_IO_OPTION1);
    }
    else
    {
        tls_io_cfg_set(WM_IO_PA_01, WM_IO_OPTION5);
        tls_io_cfg_set(WM_IO_PA_04, WM_IO_OPTION5);
    }
}

void wm_adc_config(u8 Channel)
{
    switch(Channel)
    {
    case 0:
        tls_io_cfg_set(WM_IO_PA_01, WM_IO_OPTION6);
        break;

    case 1:
        tls_io_cfg_set(WM_IO_PA_04, WM_IO_OPTION6);
        break;

    case 2:
        tls_io_cfg_set(WM_IO_PA_03, WM_IO_OPTION6);
        break;

    case 3:
        tls_io_cfg_set(WM_IO_PA_02, WM_IO_OPTION6);
        break;

    default:
        return;
    }
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_SDADC);
}

void wm_touch_sensor_config(enum tls_io_name io_name)
{
	switch(io_name)
	{
		case WM_IO_PA_07: /*touch sensor 1*/
		case WM_IO_PA_09: /*touch sensor 2*/
		case WM_IO_PA_10: /*touch sensor 3*/
		case WM_IO_PB_00: /*touch sensor 4*/			
		case WM_IO_PB_01: /*touch sensor 5*/			
		case WM_IO_PB_02: /*touch sensor 6*/			
		case WM_IO_PB_03: /*touch sensor 7*/			
		case WM_IO_PB_04: /*touch sensor 8*/			
		case WM_IO_PB_05: /*touch sensor 9*/			
		case WM_IO_PB_06: /*touch sensor 10*/			
		case WM_IO_PB_07: /*touch sensor 11*/			
		case WM_IO_PB_08: /*touch sensor 12*/			
		case WM_IO_PB_09: /*touch sensor 13*/
		case WM_IO_PA_12: /*touch sensor 14*/
		case WM_IO_PA_14: /*touch sensor 15*/
			tls_io_cfg_set(io_name, WM_IO_OPTION7);
			break;
		default:
			return;
	}
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_TOUCH_SENSOR);
}

void wm_gpio_af_disable(void)
{
    tls_reg_write32(HR_GPIOA_DATA_DIR, 0x0);
    tls_reg_write32(HR_GPIOB_DATA_DIR, 0x0);

#if	WM_SWD_ENABLE
    tls_reg_write32(HR_GPIOA_AFSEL, 0x12); /*PA1:JTAG_CK,PA4:JTAG_SWO*/
#else
    tls_reg_write32(HR_GPIOA_AFSEL, 0x0);
#endif
    tls_reg_write32(HR_GPIOB_AFSEL, 0x0);

    tls_reg_write32(HR_GPIOA_DATA_PULLEN, 0x0);
    tls_reg_write32(HR_GPIOB_DATA_PULLEN, 0x0);
}


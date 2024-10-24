/**
 * @file    wm_hspi.c
 *
 * @brief   High speed spi slave Module
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm_hspi.h"
#include "wm_regs.h"
#include "wm_config.h"
#include "wm_mem.h"
#include "wm_osal.h"
#include "wm_irq.h"
//#include "lwip/mem.h"
#include "wm_io.h"

#if TLS_CONFIG_HS_SPI

struct tls_slave_hspi g_slave_hspi;
#define SET_BIT(x) (1UL << (x))
void hspi_free_rxdesc(struct tls_hspi_rx_desc *rx_desc);


void hspi_rx_init(struct tls_slave_hspi *hspi)
{
    struct tls_hspi_rx_desc *hspi_rx_desc;
    int i;

/* set current availble rx desc pointer */
    hspi_rx_desc = (struct tls_hspi_rx_desc *) HSPI_RX_DESC_BASE_ADDR;
    hspi->curr_rx_desc = hspi_rx_desc;

/* initialize rx descriptor content */
    for (i = 0; i < HSPI_RX_DESC_NUM; i++)
    {
    /* initialize tx descriptors */
        if (i < HSPI_RXBUF_NUM)
        {
            hspi_rx_desc->valid_ctrl = SET_BIT(31);
            hspi_rx_desc->buf_addr = HSPI_RXBUF_BASE_ADDR + i * HSPI_RXBUF_SIZE;
        }
        else
        {
        /* indicate this descriptor is can't use by hspi */
            hspi_rx_desc->valid_ctrl = 0;
        /* point to null */
            hspi_rx_desc->buf_addr = 0x0;
        }

        if (i == (HSPI_RX_DESC_NUM - 1))
        {
            hspi_rx_desc->next_desc_addr = (u32) HSPI_RX_DESC_BASE_ADDR;
        }
        else
        {
            hspi_rx_desc->next_desc_addr = (u32) (hspi_rx_desc + 1);
        }
        hspi_rx_desc++;
    }
}

void hspi_tx_init(struct tls_slave_hspi *hspi)
{
    struct tls_hspi_tx_desc *hspi_tx_desc;
    int i;

    hspi_tx_desc = (struct tls_hspi_tx_desc *) HSPI_TX_DESC_BASE_ADDR;

    hspi->curr_tx_desc = hspi_tx_desc;

    for (i = 0; i < HSPI_TX_DESC_NUM; i++)
    {
        hspi_tx_desc->valid_ctrl = 0;
        hspi_tx_desc->buf_info = 0;
#if HSPI_TX_MEM_MALLOC
        hspi_tx_desc->txbuf_addr = NULL;
        hspi_tx_desc->buf_addr[0] = 0;
#else
        hspi_tx_desc->buf_addr[0] = HSPI_TXBUF_BASE_ADDR + i * HSPI_TXBUF_SIZE;
#endif
        hspi_tx_desc->buf_addr[1] = 0;
        hspi_tx_desc->buf_addr[2] = 0;
        if (i == (HSPI_TX_DESC_NUM - 1))
        {
            hspi_tx_desc->next_desc_addr = (u32) HSPI_TX_DESC_BASE_ADDR;
        }
        else
        {
            hspi_tx_desc->next_desc_addr = (u32) (hspi_tx_desc + 1);
        }
        hspi_tx_desc++;
    }

}

static int slave_spi_rx_data(struct tls_slave_hspi *hspi)
{
    struct tls_hspi_rx_desc *rx_desc;

/* get rx descriptor */
    rx_desc = hspi->curr_rx_desc;

    while (!(rx_desc->valid_ctrl & SET_BIT(31)))
    {
        if (hspi->rx_data_callback)
            hspi->rx_data_callback((char *) rx_desc->buf_addr);
        hspi_free_rxdesc(rx_desc);

        rx_desc = (struct tls_hspi_rx_desc *) rx_desc->next_desc_addr;
        hspi->curr_rx_desc = rx_desc;
    }

    return 0;

}


void SDIO_RX_IRQHandler(void)
{
    struct tls_slave_hspi *hspi = (struct tls_slave_hspi *) &g_slave_hspi;

#if HSPI_TX_MEM_MALLOC
    hspi->txdoneflag = 1;
#endif
    if (hspi->tx_data_callback)
        hspi->tx_data_callback((char *) hspi->curr_tx_desc->buf_addr);
/* clear interrupt */
    tls_reg_write32(HR_SDIO_INT_SRC, SDIO_WP_INT_SRC_DATA_UP);
}

void SDIO_TX_IRQHandler(void)
{
    struct tls_slave_hspi *hspi = (struct tls_slave_hspi *) &g_slave_hspi;

//�û�ģʽ�£�ֱ�Ӹ������ݣ�����Ĳ��������⿪�ţ�������������������
    if (hspi->ifusermode)
    {
        slave_spi_rx_data(hspi);
    }
    else
    {
        if (hspi->rx_data_callback)
            hspi->rx_data_callback((char *) hspi->curr_rx_desc->buf_addr);
    }

/* clear interrupt */
    tls_reg_write32(HR_SDIO_INT_SRC, SDIO_WP_INT_SRC_DATA_DOWN);
}


void SDIO_RX_CMD_IRQHandler(void)
{
    struct tls_slave_hspi *hspi = (struct tls_slave_hspi *) &g_slave_hspi;

    if (hspi->rx_cmd_callback)
        hspi->rx_cmd_callback((char *) SDIO_CMD_RXBUF_ADDR);

    if (hspi->ifusermode)       // �û�ģʽ�£����ݸ���ȥ֮�󣬼Ĵ����������Լ�����
    {
        tls_reg_write32(HR_SDIO_DOWNCMDVALID, 0x1);
    }

/* clear interrupt */
    tls_reg_write32(HR_SDIO_INT_SRC, SDIO_WP_INT_SRC_CMD_DOWN);
}


ATTRIBUTE_ISR void HSPI_IRQHandler(void)
{
    printf("spi HS irqhandle\n");
}

ATTRIBUTE_ISR void SDIOA_IRQHandler(void)
{
	u32 int_src = tls_reg_read32(HR_SDIO_INT_SRC);
	csi_kernel_intrpt_enter();
	if(int_src & SDIO_WP_INT_SRC_CMD_DOWN)
	{
		SDIO_RX_CMD_IRQHandler();
	}
	else if(int_src & SDIO_WP_INT_SRC_DATA_UP)
	{
		SDIO_RX_IRQHandler();
	}
	else if(int_src & SDIO_WP_INT_SRC_DATA_DOWN)
	{
		SDIO_TX_IRQHandler();
	}
	else if(int_src & SDIO_WP_INT_SRC_CMD_UP)
	{
		tls_reg_write32(HR_SDIO_INT_SRC, SDIO_WP_INT_SRC_CMD_UP);
	}
	csi_kernel_intrpt_exit();
}

void hspi_free_rxdesc(struct tls_hspi_rx_desc *rx_desc)
{
    rx_desc->valid_ctrl = SET_BIT(31);
/* ����hspi/sdio tx enable�Ĵ�������sdioӲ��֪���п��õ�tx descriptor */
    tls_reg_write32(HR_SDIO_TXEN, SET_BIT(0));
}


void hspi_regs_cfg(void)
{
    tls_reg_write32(HR_HSPI_CLEAR_FIFO, 0x1);   /* Clear data up&down interrput
                                                 */
    tls_reg_write32(HR_HSPI_SPI_CFG, 0);    /* CPOL=0, CPHA=0, Small-Endian */
    tls_reg_write32(HR_HSPI_MODE_CFG, 0x0);
    tls_reg_write32(HR_HSPI_INT_MASK, 0x03);
    tls_reg_write32(HR_HSPI_INT_STTS, 0x03);
}

void sdio_init_cis(void)
{
    tls_reg_write32(FN0_TPL_FUNCID, 0x000C0221);
    tls_reg_write32(FN0_TPL_FUNCE, 0x00000422);
    tls_reg_write32(FN0_TPL_FUNCE_MAXBLK, 0x04203208);
    tls_reg_write32(FN0_TPL_MANFID_MID, 0x53470296);
    tls_reg_write32(FN0_TPL_END, 0x000000ff);

    tls_reg_write32(FN1_TPL_FUNCID, 0x000C0221);
    tls_reg_write32(FN1_TPL_FUNCE, 0x01012a22);
    tls_reg_write32(FN1_TPL_FUNCE_VER, 0x00000011);
    tls_reg_write32(FN1_TPL_FUNCE_NSN, 0x02000000);
    tls_reg_write32(FN1_TPL_FUNCE_CSASIZE, 0x08000300);
    tls_reg_write32(FN1_TPL_FUNCE_OCR, 0x00FF8000);
    tls_reg_write32(FN1_TPL_FUNCE_MINPWR, 0x010f0a08);
    tls_reg_write32(FN1_TPL_FUNCE_STANDBY, 0x00000101);
    tls_reg_write32(FN1_TPL_FUNCE_OPTBW, 0x00000000);
    tls_reg_write32(FN1_TPL_FUNCE_NTIMEOUT, 0x00000000);
    tls_reg_write32(FN1_TPL_FUNCE_AVGPWR, 0x00000000);
    tls_reg_write32(FN1_TPL_FUNCE_AVGPWR + 4, 0x00000000);
    tls_reg_write32(FN1_TPL_END, 0x000000ff);
}

void hsdio_regs_cfg(void)
{
    u32 v;

    sdio_init_cis();
    tls_reg_write32(HR_SDIO_CIS0, SDIO_CIS0_ADDR - 0x1000);
    tls_reg_write32(HR_SDIO_CIS1, SDIO_CIS1_ADDR - 0x2000);

    v = tls_reg_read32(HR_SDIO_CIA);
    tls_reg_write32(HR_SDIO_CIA, (v & 0xFFFFF000) | 0x232);

/* set sdio ready */
    tls_reg_write32(HR_SDIO_PROG, 0x02FD);
}


/**
 * @brief          	This function is used to initial HSPI register.
 *
 * @param[in]      	None
 *
 * @retval         	0     success
 * @retval         	other failed
 *
 * @note           	When the system is initialized, the function has been called, so users can not call the function.
 */
int tls_slave_spi_init(void)
{
    struct tls_slave_hspi *hspi;

    hspi = &g_slave_hspi;
    memset(hspi, 0, sizeof(struct tls_slave_hspi));

    hspi_rx_init(hspi);
    hspi_tx_init(hspi);
//    tls_set_high_speed_interface_type(HSPI_INTERFACE_SPI);
/* regiseter hspi tx rx cmd interrupt handler */

/* setting hw interrupt module isr enable regiset */
	tls_irq_enable(SDIO_IRQn);
	tls_irq_enable(SPI_HS_IRQn);

    /********************************************
     * setting hspi wrapper registers
     *********************************************/
/* hspi data down(rx) */
    tls_reg_write32(HR_SDIO_TXBD_ADDR, HSPI_RX_DESC_BASE_ADDR);
    tls_reg_write32(HR_SDIO_TXBD_LINKEN, 1);
    tls_reg_write32(HR_SDIO_TXEN, 1);
/* hspi data up (tx) */
    tls_reg_write32(HR_SDIO_RXBD_ADDR, HSPI_TX_DESC_BASE_ADDR);
    tls_reg_write32(HR_SDIO_RXBD_LINKEN, 1);

/* hspi cmd down */
    tls_reg_write32(HR_SDIO_CMD_ADDR, SDIO_CMD_RXBUF_ADDR);
    tls_reg_write32(HR_SDIO_CMD_SIZE, SDIO_CMD_RXBUF_SIZE);
    tls_reg_write32(HR_SDIO_DOWNCMDVALID, 0x1);

/* enable sdio module register */
    tls_reg_write32(HR_SDIO_INT_MASK, 0x00);

    return 0;
}


/**
 * @brief         	This function is used to set high speed interface type.
 *
 * @param[in]     	type    is the interface type. HSPI_INTERFACE_SPI or HSPI_INTERFACE_SDIO
 *
 * @return        	None
 *
 * @note           	None
 */
void tls_set_high_speed_interface_type(int type)
{


    if (HSPI_INTERFACE_SPI == type)
    {
        hspi_regs_cfg();
    }
    else if (HSPI_INTERFACE_SDIO == type)
    {
        hsdio_regs_cfg();
    }
}

/**
 * @brief          	This function is used to enable or disable user mode.
 *
 * @param[in]      	ifenable     TRUE or FALSE
 *
 * @return         	None
 *
 * @note           	If the user enables the user mode, RICM instruction in the system will not be used by SPI.
 *		        	If the user wants to use the SPI interface as other use, need to enable the user mode.
 *		        	This function must be called before the register function.
 */
void tls_set_hspi_user_mode(u8 ifenable)
{
    struct tls_slave_hspi *hspi = &g_slave_hspi;

    hspi->ifusermode = ifenable;

    if (ifenable)
    {
        hspi->rx_cmd_callback = NULL;
        hspi->rx_data_callback = NULL;
        hspi->tx_data_callback = NULL;
    }
}

/**
 * @brief          	This function is used to register hspi rx command interrupt.
 *
 * @param[in]      	rx_cmd_callback		is the hspi rx interrupt call back function.
 *
 * @return         	None
 *
 * @note           	None
 */
 void tls_hspi_rx_cmd_callback_register(s16(*rx_cmd_callback) (char *buf))
{
    g_slave_hspi.rx_cmd_callback = rx_cmd_callback;
}

/**
 * @brief          	This function is used to register hspi rx data interrupt.
 *
 * @param[in]      	rx_data_callback		is the hspi rx interrupt call back function.
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_hspi_rx_data_callback_register(s16(*rx_data_callback) (char *buf))
{
    g_slave_hspi.rx_data_callback = rx_data_callback;
}

/**
 * @brief          	This function is used to register hspi tx data interrupt.
 *
 * @param[in]     	tx_data_callback		is the hspi tx interrupt call back function.
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_hspi_tx_data_callback_register(s16(*tx_data_callback) (char *buf))
{
    g_slave_hspi.tx_data_callback = tx_data_callback;
}

/**
 * @brief          	This function is used to transfer data.
 *
 * @param[in]      	txbuf			is a buf for saving user data.
 * @param[in]      	len                 	is the data length.
 *
 * @retval         	transfer data len    success
 * @retval         	0                          	failed
 *
 * @note           	None
 */
int tls_hspi_tx_data(char *txbuf, int len)
{
    struct tls_hspi_tx_desc *tx_desc;
    int totallen = len;
    int txlen;

    if (NULL == txbuf || len <= 0 || len > (HSPI_TXBUF_SIZE * HSPI_TX_DESC_NUM))
    {
        printf("\nhspi tx param error\n");
        return 0;
    }
    tx_desc = g_slave_hspi.curr_tx_desc;
    while (1)
    {
        if ((tx_desc->valid_ctrl & SET_BIT(31)) == 0)
            break;
        tls_os_time_delay(1);
    }
    while (!(tx_desc->valid_ctrl & SET_BIT(31)))
    {
        txlen = (totallen > HSPI_TXBUF_SIZE) ? HSPI_TXBUF_SIZE : totallen;
#if HSPI_TX_MEM_MALLOC
        if (tx_desc->txbuf_addr != NULL)
        {
            printf("\nhspi txbuf not null,error %x\n", tx_desc->txbuf_addr);
            if (tx_desc->txbuf_addr == tx_desc->buf_addr[0])
            {
                mem_free((void *) tx_desc->txbuf_addr);
                tx_desc->txbuf_addr = NULL;
            }
            else                // ��Ӧ�ó���
            {
                printf("\nhspi tx mem error\n");
                break;
            }
        }

        tx_desc->txbuf_addr = (u32) mem_malloc(txlen + 1);
        if (NULL == tx_desc->txbuf_addr)
        {
            printf("\nhspi tx data malloc error\n");
            break;
        }
        tx_desc->buf_addr[0] = tx_desc->txbuf_addr;
#endif
        MEMCPY((char *) tx_desc->buf_addr[0], txbuf, txlen);
        tx_desc->buf_info = txlen << 12;
        tx_desc->valid_ctrl = SET_BIT(31);
        tls_reg_write32(HR_SDIO_RXEN, 0x01);
        tx_desc = (struct tls_hspi_tx_desc *) tx_desc->next_desc_addr;
        g_slave_hspi.curr_tx_desc = tx_desc;
        totallen -= txlen;
        if (totallen <= 0)
            break;
    }
    return (len - totallen);
}

#endif



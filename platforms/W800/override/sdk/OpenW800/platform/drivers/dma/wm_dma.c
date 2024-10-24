/**
 * @file    wm_dma.c
 *
 * @brief   DMA Driver Module
 *
 * @author  dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "wm_dma.h"
#include "wm_regs.h"
#include "wm_irq.h"
#include "wm_osal.h"
#include "core_804.h"
#include "wm_pmu.h"


#define  ATTRIBUTE_ISR __attribute__((isr))
static u16 dma_used_bit = 0;
struct tls_dma_channels {
	unsigned char	channels[8];	/* list of channels */
};

typedef void (*dma_irq_callback)(void *p);

struct dma_irq_context {
    u8 flags;
    dma_irq_callback burst_done_pf;
    void *burst_done_priv;
    dma_irq_callback transfer_done_pf;
    void *transfer_done_priv;
};

static struct dma_irq_context dma_context[8];
static struct tls_dma_channels channels;

extern void wm_delay_ticks(uint32_t ticks);
extern void tls_irq_priority(u8 vec_no, u32 prio);

static void dma_irq_proc(void *p)
{
    unsigned char ch;
    unsigned int int_src;
    static uint32_t len[8] = {0,0,0,0,0,0,0,0};

    ch = (unsigned char)(unsigned long)p;
    int_src = tls_reg_read32(HR_DMA_INT_SRC);

    if (ch > 3)
    {
        for (ch = 4; ch < 8; ch++)
        {
            if (int_src & (TLS_DMA_IRQ_BOTH_DONE << ch * 2))
                break;
        }

        if (8 == ch)
            return;
    }

    if (DMA_CTRL_REG(ch) & 0x01)
    {
        uint32_t temp = 0, cur_len = 0;
        
        temp = DMA_CTRL_REG(ch);
        if(len[ch] == 0)
        {
            len[ch] = (temp & 0xFFFF00) >> 8;
        }
        cur_len = (temp & 0xFFFF00) >> 8;
        if((cur_len + len[ch]) > 0xFFFF)
        {
            cur_len = 0;
            DMA_CHNLCTRL_REG(ch) |= (1 << 1);
            while(DMA_CHNLCTRL_REG(ch) & (1 << 0));
            DMA_CHNLCTRL_REG(ch) |= (1 << 0);
        }
        
        temp &= ~(0xFFFF << 8);
        temp |= ((cur_len + len[ch]) << 8);
        DMA_CTRL_REG(ch) = temp;
    }

    if ((int_src & (TLS_DMA_IRQ_BOTH_DONE << ch * 2)) &&
        (TLS_DMA_IRQ_BOTH_DONE == dma_context[ch].flags))
    {
        tls_dma_irq_clr(ch, TLS_DMA_IRQ_BOTH_DONE);
        if (dma_context[ch].burst_done_pf)
            dma_context[ch].burst_done_pf(dma_context[ch].burst_done_priv);
    }
    else if ((int_src & (TLS_DMA_IRQ_BURST_DONE << ch * 2)) &&
             (TLS_DMA_IRQ_BURST_DONE == dma_context[ch].flags))
    {
        tls_dma_irq_clr(ch, TLS_DMA_IRQ_BOTH_DONE);
        if (dma_context[ch].burst_done_pf)
            dma_context[ch].burst_done_pf(dma_context[ch].burst_done_priv);
    }
    else if ((int_src & (TLS_DMA_IRQ_TRANSFER_DONE << ch * 2)) &&
             (TLS_DMA_IRQ_TRANSFER_DONE == dma_context[ch].flags))
    {
        tls_dma_irq_clr(ch, TLS_DMA_IRQ_BOTH_DONE);
        if (dma_context[ch].transfer_done_pf)
            dma_context[ch].transfer_done_pf(dma_context[ch].transfer_done_priv);
    }
    return;
}

ATTRIBUTE_ISR void DMA_Channel0_IRQHandler(void)
{
    csi_kernel_intrpt_enter();
    dma_irq_proc((void *)0);
    csi_kernel_intrpt_exit();
}
ATTRIBUTE_ISR void DMA_Channel1_IRQHandler(void)
{
    csi_kernel_intrpt_enter();
    dma_irq_proc((void *)1);
    csi_kernel_intrpt_exit();
}
ATTRIBUTE_ISR void DMA_Channel2_IRQHandler(void)
{
    csi_kernel_intrpt_enter();
    dma_irq_proc((void *)2);
    csi_kernel_intrpt_exit();
}
ATTRIBUTE_ISR void DMA_Channel3_IRQHandler(void)
{
    csi_kernel_intrpt_enter();
    dma_irq_proc((void *)3);
    csi_kernel_intrpt_exit();
}
ATTRIBUTE_ISR void DMA_Channel4_7_IRQHandler(void)
{
    csi_kernel_intrpt_enter();
    dma_irq_proc((void *)4);
    csi_kernel_intrpt_exit();
}

/**
 * @brief          	This function is used to clear dma interrupt flag.
 *
 * @param[in]     	ch			Channel no.[0~7]
 * @param[in]     	flags		Flags setted to TLS_DMA_IRQ_BURST_DONE, TLS_DMA_IRQ_TRANSFER_DONE, TLS_DMA_IRQ_BOTH_DONE.
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_dma_irq_clr(unsigned char ch, unsigned char flags)
{
    unsigned int int_src = 0;

    int_src |= flags << 2 * ch;

    tls_reg_write32(HR_DMA_INT_SRC, int_src);

    return;
}

/**
 * @brief          	This function is used to register dma interrupt callback function.
 *
 * @param[in]     	ch			Channel no.[0~7]
 * @param[in]     	callback	is the dma interrupt call back function.
 * @param[in]     	arg			the param of the callback function.
 * @param[in]     	flags		Flags setted to TLS_DMA_IRQ_BURST_DONE, TLS_DMA_IRQ_TRANSFER_DONE, TLS_DMA_IRQ_BOTH_DONE.
 *
 * @return         	None
 *
 * @note           	None
 */void tls_dma_irq_register(unsigned char ch, void (*callback)(void *p), void *arg, unsigned char flags)
{
    unsigned int mask;

    mask  = tls_reg_read32(HR_DMA_INT_MASK);
    mask |= TLS_DMA_IRQ_BOTH_DONE << 2 * ch;
    mask &= ~(flags << 2 * ch);
    tls_reg_write32(HR_DMA_INT_MASK, mask);

    dma_context[ch].flags = flags;
    if (flags & TLS_DMA_IRQ_BURST_DONE)
    {
        dma_context[ch].burst_done_pf   = callback;
        dma_context[ch].burst_done_priv = arg;
    }
    if (flags & TLS_DMA_IRQ_TRANSFER_DONE)
    {
        dma_context[ch].transfer_done_pf   = callback;
        dma_context[ch].transfer_done_priv = arg;
    }
    if (ch > 3)
        ch = 4;
    tls_irq_priority(DMA_Channel0_IRQn + ch, ch/2);
    tls_irq_enable(DMA_Channel0_IRQn + ch);
}

/**
 * @brief          This function is used to Wait until DMA operation completes
 *
 * @param[in]      ch    channel no
 *
 * @retval         0     completed
 * @retval        -1     failed
 *
 * @note           None
 */
int tls_dma_wait_complt(unsigned char ch)
{
	unsigned long timeout = 0;

	while(DMA_CHNLCTRL_REG(ch) & DMA_CHNL_CTRL_CHNL_ON) 
	{
		tls_os_time_delay(1);
		timeout ++;
		if(timeout > 500)
			return -1;
	}
	return 0;
}

/**
 * @brief          This function is used to Start the DMA controller by Wrap
 *
 * @param[in]      ch             channel no
 * @param[in]      dma_desc       pointer to DMA channel descriptor structure
 * @param[in]      auto_reload    does restart when current transfer complete
 * @param[in]      src_zize       dource address size
 * @param[in]      dest_zize      destination address size
 *
 * @retval         1     success
 * @retval         0     failed
 *
 * @note
 *                  DMA Descriptor:
 *            		+--------------------------------------------------------------+
 *            		|Vld[31] |                    RSV                              |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                  RSV           |         Dma_Ctrl[16:0]      |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                         Src_Addr[31:0]                       |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                         Dest_Addr[31:0]                      |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                       Next_Desc_Add[31:0]                    |
 *            	 	+--------------------------------------------------------------+
 */
unsigned char tls_dma_start_by_wrap(unsigned char ch, struct tls_dma_descriptor *dma_desc, 
                                    unsigned char auto_reload,
                                    unsigned short src_zize,
                                    unsigned short dest_zize)
{
    if((ch > 7) && !dma_desc) return 1;

    DMA_SRCWRAPADDR_REG(ch) = dma_desc->src_addr;
    DMA_DESTWRAPADDR_REG(ch) = dma_desc->dest_addr;
    DMA_WRAPSIZE_REG(ch) = (dest_zize << 16) | src_zize;
    DMA_CTRL_REG(ch) = ((dma_desc->dma_ctrl & 0x7fffff) << 1) | (auto_reload ? 0x1: 0x0);
    DMA_CHNLCTRL_REG(ch) |= DMA_CHNL_CTRL_CHNL_ON;

    return 0;
}

/**
 * @brief          This function is used to Start the DMA controller
 *
 * @param[in]      ch             channel no
 * @param[in]      dma_desc       pointer to DMA channel descriptor structure
 * @param[in]      auto_reload    does restart when current transfer complete
 *
 * @retval         1     success
 * @retval         0     failed
 *
 * @note
 *                  DMA Descriptor:
 *            		+--------------------------------------------------------------+
 *            		|Vld[31] |                    RSV                              |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                  RSV           |         Dma_Ctrl[16:0]      |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                         Src_Addr[31:0]                       |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                         Dest_Addr[31:0]                      |
 *            	 	+--------------------------------------------------------------+
 *            	 	|                       Next_Desc_Add[31:0]                    |
 *            	 	+--------------------------------------------------------------+
 */
unsigned char tls_dma_start(unsigned char ch, struct tls_dma_descriptor *dma_desc, unsigned char auto_reload)
{
	if((ch > 7) && !dma_desc) return 1;

	if ((dma_used_bit &(1<<ch)) == 0)
	{
		dma_used_bit |= (1<<ch);
	}
	DMA_SRCADDR_REG(ch) = dma_desc->src_addr;
	DMA_DESTADDR_REG(ch) = dma_desc->dest_addr;
	DMA_CTRL_REG(ch) = ((dma_desc->dma_ctrl & 0x7fffff) << 1) | (auto_reload ? 0x1: 0x0);
	DMA_CHNLCTRL_REG(ch) |= DMA_CHNL_CTRL_CHNL_ON;

	return 0;
}

/**
 * @brief          This function is used to To stop current DMA channel transfer
 *
 * @param[in]      ch    channel no. to be stopped
 *
 * @retval         0     success
 * @retval         1     failed
 *
 * @note
 * If channel stop, DMA_CHNL_CTRL_CHNL_ON bit in DMA_CHNLCTRL_REG is cleared.
 */
unsigned char tls_dma_stop(unsigned char ch)
{
	if(ch > 7) return 1;
	if(DMA_CHNLCTRL_REG(ch) & DMA_CHNL_CTRL_CHNL_ON)
	{
		DMA_CHNLCTRL_REG(ch) |= DMA_CHNL_CTRL_CHNL_OFF;

		while(DMA_CHNLCTRL_REG(ch) & DMA_CHNL_CTRL_CHNL_ON);
	}

	return 0;
}

/**
 * @brief          This function is used to Request a free dma channel
 *
 * @param[in]      ch       specified channel when ch is valid and not used.
 * @param[in]      flags    flags setted to selected channel
 *
 * @return         Real DMA Channel No. 
 *
 * @note
 * If ch is invalid or valid but used, the function will select a random free channel.
 * else return the selected channel no.
 */
unsigned char tls_dma_request(unsigned char ch, unsigned char flags)
{
	unsigned char freeCh = 0xFF;
 	int i = 0;

	/*If channel is valid, try to use specified DMA channel!*/
	if ((ch >= 0) && (ch < 8))
	{
		if (!(channels.channels[ch] & TLS_DMA_FLAGS_CHANNEL_VALID))
		{
			freeCh = ch;
		}
	}

	/*If ch is not valid, or ch has been used, try to select another free channel for the caller*/
	if (freeCh == 0xFF)
	{
		for (i = 0; i < 8; i++)
		{
			if (!(channels.channels[i] & TLS_DMA_FLAGS_CHANNEL_VALID))
			{
				freeCh = i;
				break;
			}
		}

		if (8 == i)
		{
			printf("!!!There is no free DMA channel.!!!\n");
		}
	}

	if ((freeCh >= 0) && (freeCh < 8))
	{
		if (dma_used_bit == 0)
		{
			tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_DMA);
		}
		dma_used_bit |= (1<<freeCh);
	    
		channels.channels[freeCh] = flags | TLS_DMA_FLAGS_CHANNEL_VALID;
		DMA_MODE_REG(freeCh) = flags;
	}

	return freeCh;
}

/**
 * @brief          This function is used to Free the DMA channel when not use
 *
 * @param[in]      ch    channel no. that is ready to free
 *
 * @return         None
 *
 * @note           None
 */
void tls_dma_free(unsigned char ch)
{
	if(ch < 8)
	{
		tls_dma_stop(ch);

		DMA_SRCADDR_REG(ch) = 0;
		DMA_DESTADDR_REG(ch) = 0;
		DMA_MODE_REG(ch) = 0;
		DMA_CTRL_REG(ch) = 0;
//		DMA_INTSRC_REG = 0xffff;
		DMA_INTSRC_REG |= 0x03<<(ch*2);

		channels.channels[ch] = 0x00;
		dma_used_bit &= ~(1<<ch);
		if (dma_used_bit == 0)
		{
			tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_DMA);
		}
	}
}

/**
 * @brief          This function is used to Initialize DMA Control
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_dma_init(void)
{
	u32 i = 0;
	u32 value = 0;
	for (i = 0; i < 8; i++)
	{
		if (!(dma_used_bit & (1<<i)))
		{
			value |= 3<<(i*2);
		}
	}

	DMA_INTMASK_REG = value;
	DMA_INTSRC_REG  = value;
}


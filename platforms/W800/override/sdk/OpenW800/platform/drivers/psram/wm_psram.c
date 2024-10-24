
#include <string.h>
#include "wm_regs.h"
#include "wm_psram.h"
#include "wm_dma.h"


/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((uint32_t)X & (sizeof (uint32_t) - 1)) | ((uint32_t)Y & (sizeof (uint32_t) - 1)))
/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (uint32_t) << 2)
/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

volatile static uint32_t dma_rx_tx_done = 0;
static uint32_t psram_channel = 0;


static void wm_psram_dma_go(uint8_t ch)
{
	DMA_CHNLCTRL_REG(ch) = DMA_CHNL_CTRL_CHNL_ON;
	dma_rx_tx_done = 0;
}

static void wm_psram_dma_stop(uint8_t ch)
{
	if(DMA_CHNLCTRL_REG(ch) & DMA_CHNL_CTRL_CHNL_ON)
	{
		DMA_CHNLCTRL_REG(ch) |= DMA_CHNL_CTRL_CHNL_OFF;

		while(DMA_CHNLCTRL_REG(ch) & DMA_CHNL_CTRL_CHNL_ON);
	}
}

static void wm_psram_dma_init(uint8_t ch, uint32_t count, void * src, void *dst)
{	
	DMA_INTMASK_REG &= ~(0x02<<(ch*2));
	DMA_SRCADDR_REG(ch) = (uint32_t)src;
	DMA_DESTADDR_REG(ch) = (uint32_t)dst;
	
	DMA_CTRL_REG(ch) = DMA_CTRL_SRC_ADDR_INC|DMA_CTRL_DEST_ADDR_INC | DMA_CTRL_DATA_SIZE_WORD | DMA_CTRL_BURST_SIZE1;
	DMA_MODE_REG(ch) = 0;
	DMA_CTRL_REG(ch) &= ~0xFFFF00;
	DMA_CTRL_REG(ch) |= (count<<8);
}

void psram_DMA_Channel0_IRQHandler()
{	
	tls_reg_write32(HR_DMA_INT_SRC, 0x02);
	dma_rx_tx_done += 1;
}

void psram_init(psram_mode_t mode)
{
	volatile unsigned int value = 0x600;

	value |= 2<<4;

	if(mode == PSRAM_QPI)
	{
		value |= 0x03;
	}

	/*reset psram*/
	value |= 0x01;
	tls_reg_write32(HR_PSRAM_CTRL_ADDR, value);
	do{
		value = tls_reg_read32(HR_PSRAM_CTRL_ADDR);
	}while(value&0x01);	

	psram_channel = tls_dma_request(0, 0);
    tls_dma_irq_register(psram_channel, psram_DMA_Channel0_IRQHandler, NULL, TLS_DMA_IRQ_TRANSFER_DONE);

}

int memcpy_dma(unsigned char *dst, unsigned char *src, int num)
{
	int offset = 0;
    unsigned char *psram_access_start = src;

    int left_bytes = num&0x03;
    int dw_length = (num&(~0x03))>>2;

	if (!TOO_SMALL(num) && !UNALIGNED (src, dst))
    {
    	if(dw_length)
    	{
    		wm_psram_dma_stop(psram_channel);
			wm_psram_dma_init(psram_channel, dw_length*4, src,dst);
			wm_psram_dma_go(psram_channel);
			while(dma_rx_tx_done == 0);
			offset += dw_length *4;
			psram_access_start += dw_length *4;
    	}
		else
    	{
		    while(dw_length--)
			{
				M32((dst+offset)) = M32(psram_access_start);
				psram_access_start += 4;
				offset+=4;
			}
    	}
	    while(left_bytes--)
	    {
	    	M8((dst+offset)) = M8(psram_access_start);
			psram_access_start += 1;
			offset+=1;
	    }
	}
	else
	{
		while (num--)
    	{
    		M8(dst++) = M8(psram_access_start++);
			offset++;
		}
		
	}
    return offset;
}


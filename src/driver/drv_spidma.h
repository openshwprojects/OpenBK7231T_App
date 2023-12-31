#pragma once

#include "../new_common.h"
#include "spi_pub.h"

#define SPI_TX_DMA_CHANNEL GDMA_CHANNEL_3

#define SPI_PERI_CLK_26M		(26 * 1000 * 1000)
#define SPI_PERI_CLK_DCO		(120 * 1000 * 1000)

#define SPI_BASE (0x00802700)
#define SPI_DAT (SPI_BASE + 3 * 4)
#define SPI_CONFIG (SPI_BASE + 0x1 * 4)
#define SPI_TX_EN (0x01UL << 0)

struct bk_spi_dev {
	uint8_t init_spi : 1;
	uint8_t init_dma_tx : 1;
	uint8_t init_dma_rx : 1;
	uint8_t undef : 5;

	uint8_t *tx_ptr;
	uint8_t *rx_ptr;
	uint32_t tx_len;
	uint32_t rx_len;
	beken_semaphore_t tx_sem;
	beken_semaphore_t rx_sem;
	beken_semaphore_t dma_rx_sem;
	beken_semaphore_t dma_tx_sem;
	beken_mutex_t mutex;
	uint32_t rx_offset;
	uint32_t rx_drop;
	uint32_t total_len;
	volatile uint32_t flag;
};

void SPIDMA_Init(struct spi_message *spi_msg);
void SPIDMA_StartTX(struct spi_message *spi_msg);
void SPIDMA_StopTX(void);

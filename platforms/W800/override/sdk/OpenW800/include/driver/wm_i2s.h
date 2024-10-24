#ifndef __WM_I2S_H
#define __WM_I2S_H

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdbool.h>
//#include "wm_regs_cm3.h"
#include "wm_regs.h"
#include "wm_debug.h"
#include "wm_dma.h"


typedef void (*tls_i2s_callback)(uint32_t *data, uint16_t *len);

typedef struct {
	__IO uint32_t CTRL;
	__IO uint32_t INT_MASK;
	__IO uint32_t INT_SRC;
	__I  uint32_t INT_STATUS;
	__O  uint32_t TX;
	__I	 uint32_t RX;
} I2S_T;


typedef struct {
	uint32_t I2S_Mode_MS; //master or slave mode
	uint32_t I2S_Mode_SS; //stereo or single channel
	uint32_t I2S_Mode_LR; //left or right channel
	uint32_t I2S_Trans_STD;
	uint32_t I2S_DataFormat;
	uint32_t I2S_AudioFreq;
	uint32_t I2S_MclkFreq;
} I2S_InitDef;

typedef struct _wm_dma_desc
{
	unsigned int valid;
	unsigned int dma_ctrl;
	unsigned int src_addr;
	unsigned int dest_addr;
	struct _wm_dma_desc * next;
}wm_dma_desc;

typedef struct _dma_handler_type
{
	uint8_t channel;
	void    (* XferCpltCallback)( struct _dma_handler_type * hdma);         /*!< DMA transfer complete callback         */
	void    (* XferHalfCpltCallback)( struct _dma_handler_type * hdma);     /*!< DMA Half transfer complete callback    */
}wm_dma_handler_type;

#define I2S			            ((I2S_T *)HR_I2S_REG_BASE)

#define I2S_MODE_MASTER		    ((bool)0x0)
#define I2S_MODE_SLAVE		    ((bool)0x1)

#define I2S_RIGHT_CHANNEL		((bool)0x0)
#define I2S_LEFT_CHANNEL		((bool)0x1)

#define I2S_Standard			(0x0UL)
#define I2S_Standard_MSB		(0x1000000UL)
#define I2S_Standard_PCMA		(0x2000000UL)
#define I2S_Standard_PCMB		(0x3000000UL)

#define I2S_DataFormat_8		(8)
#define I2S_DataFormat_16		(16)
#define I2S_DataFormat_24		(24)
#define I2S_DataFormat_32		(32)	

#define I2S_CTRL_CHSEL_MASK		(1UL<<23)
#define I2S_CTRL_CHSEL_LEFT		(1UL<<23)
#define I2S_CTRL_MONO			(1UL<<22)
#define I2S_CTRL_STEREO			(0UL<<22)
#define I2S_CTRL_RXDMA_EN		(1UL<<21)
#define I2S_CTRL_TXDMA_EN		(1UL<<20)
#define I2S_CTRL_RX_CLR			(1UL<<19)
#define I2S_CTRL_TX_CLR			(1UL<<18)
#define I2S_CTRL_LZCEN			(1UL<<17)
#define I2S_CTRL_RZCEN			(1UL<<16)
#define I2S_CTRL_RXTH(n)		((n-1)<<12)
#define I2S_CTRL_TXTH(n)		((n)<<9)
#define I2S_CTRL_SLAVE_SEL		(1UL<<8)
#define I2S_CTRL_MUTE			(1UL<<3)
#define I2S_CTRL_RXE			(1UL<<2)
#define I2S_CTRL_TXE			(1UL<<1)
#define I2S_CTRL_EN				(1UL<<0)

#define I2S_INT_MASK_LZC		((uint16_t)0x200)
#define I2S_INT_MASK_RZC		((uint16_t)0x100)
#define I2S_INT_MASK_TXDONE		((uint16_t)0x080)
#define I2S_INT_MASK_TXTH		((uint16_t)0x040)
#define I2S_INT_MASK_TXOV		((uint16_t)0x020)
#define I2S_INT_MASK_TXUD		((uint16_t)0x010)
#define I2S_INT_MASK_RXDONE		((uint16_t)0x008)
#define I2S_INT_MASK_RXTH		((uint16_t)0x004)
#define I2S_INT_MASK_RXOV		((uint16_t)0x002)
#define I2S_INT_MASK_RXUD		((uint16_t)0x002)

#define I2S_FLAG_TX					((uint16_t)0x1000)
#define I2S_FLAG_RX					((uint16_t)0x0800)
#define I2S_FLAG_I2S				((uint16_t)0x0400)
#define I2S_FLAG_LZC				((uint16_t)0x0200)
#define I2S_FLAG_RZC				((uint16_t)0x0100)
#define I2S_FLAG_TXDONE				((uint16_t)0x0080)
#define I2S_FLAG_TXTH				((uint16_t)0x0040)
#define I2S_FLAG_TXOV				((uint16_t)0x0020)
#define I2S_FLAG_TXUD				((uint16_t)0x0010)
#define I2S_FLAG_RXDONE				((uint16_t)0x0008)
#define I2S_FLAG_RXTH				((uint16_t)0x0004)
#define I2S_FLAG_RXOV				((uint16_t)0x0002)
#define I2S_FLAG_RXUD				((uint16_t)0x0001)

#define WM_I2S_TX_DMA_CHANNEL       (1)
#define WM_I2S_RX_DMA_CHANNEL       (5)


typedef struct wm_i2s_buf_s {
	volatile uint32_t *txbuf;
	volatile uint32_t txlen;
	volatile uint32_t txtail;
	volatile uint32_t *rxbuf;
	volatile uint32_t rxlen;
	volatile uint32_t int_txlen;
	volatile uint32_t rxhead;
	volatile uint8_t rxdata_ready;
	volatile uint8_t txdata_done;

	/** function pointer for data receiver  */
    void (*rx_callback)(void);
    /** function pointer for data transmit  */
    void (*tx_callback)(uint32_t *data, uint16_t *len);
} wm_i2s_buf_t;
/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup I2S_Driver_APIs I2S Driver APIs
 * @brief I2S driver APIs
 */

/**
 * @addtogroup I2S_Driver_APIs
 * @{
 */

/**
  * @brief Register a callback function
  * @param  callback pointer to a callback function in which you can prepare the next buffer
  * @param  callback->data  pointer to data buffer to be prepared
  * @param  callback->len size of the data buffer to be prepared in 32-bit
  * @note The registerred callback function will be called as long as the transmission begins
  * @retval none
  */
void wm_i2s_register_callback(tls_i2s_callback callback);

/**
  * @brief Initializes the I2S according to the specified parameters
  *         in the I2S_InitDef.
  * @param  opts pointer to a I2S_InitDef structure that contains
  *         the configuration information for I2S module
  * @retval status
  */
int wm_i2s_port_init(I2S_InitDef *opts);

/**
  * @brief stop i2s module
  * @retval none
  */
void wm_i2s_tx_rx_stop(void);

/**
  * @brief Transmit an amount of data in blocking mode with Interrupt
  * @param data a 16-bit pointer to data buffer.
  * @param len number of data sample to be sent:
  * @param next_data a 16-bit pointer to the next data buffer, same size with data; set to NULL if it's not needed
  * @note  the len parameter means the number of 16-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.
  * @note This function will call the registerred callback function as long as the transmission begins
  * @retval status
  */
int wm_i2s_tx_int(int16_t *data, uint16_t len, int16_t *next_data);

/**
  * @brief Transmit an amount of data in blocking mode with DMA's normal mode
  * @param data a 16-bit pointer to data buffer.
  * @param len number of data sample to be sent:
  * @param next_data a 16-bit pointer to the next data buffer, same size with data; set to NULL if it's not needed
  * @note  the len parameter means the number of 32-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.
  * @note This function will call the registerred callback function as long as the transmission begins
  * @retval status
  */
int wm_i2s_tx_dma(int16_t *data, uint16_t len, int16_t *next_data);

/**
  * @brief Transmit an amount of data in blocking mode with DMA's link mode
  * @param data a 16-bit pointer to data buffer.
  * @param len number of data sample to be sent:
  * @param next_data a 16-bit pointer to the next data buffer, same size with data:
  * @note  the len parameter means the number of 32-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.Set len to 0xffff will exit this rountine.
  * @note This function will call the registerred callback function as long as the data or next_data
  *       is sent out.So prepare it in the callback.
  * @note See the demo for detail use.
  * @retval status
  */
int wm_i2s_tx_dma_link(int16_t *data, uint16_t len, int16_t *next_data);

/**
  * @brief Receive an amount of data in blocking mode with Interrupt
  * @param data a 16-bit pointer to the Receive data buffer.
  * @param len number of data sample to be received:
  * @note the len parameter means the number of 16-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.
  * @retval status
  */
int wm_i2s_rx_int(int16_t *data, uint16_t len);

/**
  * @brief Receive an amount of data in blocking mode with DMA
  * @param data a 16-bit pointer to the Receive data buffer.
  * @param len number of data sample to be received:
  * @note the len parameter means the number of 16-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.
  * @retval status
  */
int wm_i2s_rx_dma(int16_t *data, uint16_t len);

/**
  * @brief Full-Duplex Transmit/Receive data in blocking mode using Interrupt
  * @param  opts pointer to a I2S_InitDef structure that contains
  *         the configuration information for I2S module
  * @param data_tx a 16-bit pointer to the Transmit data buffer.
  * @param data_rx a 16-bit pointer to the Receive data buffer.
  * @param len number of data sample to be sent:
  * @note  the len parameter means the number of 16-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.
  * @retval status
  */
int wm_i2s_tx_rx_int(I2S_InitDef *opts, int16_t *data_tx, int16_t *data_rx, uint16_t len);

/**
  * @brief Full-Duplex Transmit/Receive data in blocking mode using DMA
  * @param  opts pointer to a I2S_InitDef structure that contains
  *         the configuration information for I2S module
  * @param data_tx a 16-bit pointer to the Transmit data buffer.
  * @param data_rx a 16-bit pointer to the Receive data buffer.
  * @param len number of data sample to be sent:
  * @note  the len parameter means the number of 16-bit data length.
  * @note The I2S is kept enabled at the end of transaction to avoid the clock de-synchronization
  *       between Master and Slave(example: audio streaming).
  * @note This function will block its task until the transmission is over,so perpare the next data
  *       buffer at another task during this interval.
  * @retval status
  */
int wm_i2s_tx_rx_dma(I2S_InitDef *opts, int16_t *data_tx, int16_t *data_rx, uint16_t len);

int wm_i2s_transmit_dma(wm_dma_handler_type *hdma, uint16_t *data, uint16_t len);
int wm_i2s_receive_dma(wm_dma_handler_type *hdma, uint16_t *data, uint16_t len);

/**
 * @}
 */

/**
 * @}
 */


#ifdef __cplusplus 
}
#endif

#endif

#ifndef __WM_SDIO_HOST_H_
#define __WM_SDIO_HOST_H_

#include <core_804.h>
#include "wm_regs.h"

typedef struct {
	__IOM uint32_t MMC_CTL;
	__IOM uint32_t MMC_IO;
	__IOM uint32_t MMC_BYTECNTL;
	__IM  uint32_t MMC_TR_BLOCKCNT;
	__IOM uint32_t MMC_CRCCTL;                       /*!< Offset: 0x010 */
	__IM  uint32_t CMD_CRC;
	__IM  uint32_t DAT_CRCL;
	__IM  uint32_t DAT_CRCH;
	__IOM uint32_t MMC_PORT;                          /*!< Offset: 0x020 */
	__IOM uint32_t MMC_INT_MASK;
	__IOM uint32_t MMC_INT_SRC;
	__IOM uint32_t MMC_CARDSEL;
	__IM  uint32_t MMC_SIG;                          /*!< Offset: 0x030 */
	__IOM uint32_t MMC_IO_MBCTL;
	__IOM uint32_t MMC_BLOCKCNT;
	__IOM uint32_t MMC_TIMEOUTCNT;
	__IOM uint32_t CMD_BUF[16];                      /*!< Offset: 0x040 */
	__IOM uint32_t BUF_CTL;                          /*!< Offset: 0x080 */
	uint32_t RESERVED3[31U];
	__IOM uint32_t  DATA_BUF[128];                    /*!< Offset: 0x100 */
}SDIO_HOST_Type;

#define SDIO_HOST     ((SDIO_HOST_Type *)HR_SDIO_HOST_BASE_ADDR)

typedef struct
{
  long long CardCapacity;
  u32 CardBlockSize;
  u16 RCA;
  u8 CardType;
} SD_CardInfo_t;
extern SD_CardInfo_t SDCardInfo;
/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup SDIOH_Driver_APIs SDIO HOST Driver APIs
 * @brief SDIO HOST driver APIs
 */

/**
 * @addtogroup SDIOH_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used to initial the sd host module .
 *
 * @param[out]     rca_ref   Pointer to the rca reference
 *
 * @retval         status   0 if succeed, otherwise fail
 *
 * @note           None
 */
int sdh_card_init(uint32_t *rca_ref);

/**
 * @brief          This function is used to set the width of bus .
 *
 * @param[in]     rca  the rca reference
 * @param[in]     bus_width: 0:1bit; 2:4bits
 *
 * @retval         status   0 if succeed, otherwise fail
 *
 * @note           None
 */
int wm_sd_card_set_bus_width(uint32_t rca, uint8_t bus_width);

/**
 * @brief          This function is used to read one block data from the sd card with irq mode .
 *
 * @param[in]      sd_addr   address that to be read from
 * @param[in]      buf   Pointer to the buffer that the data shall be read into
 *
 * @retval         status   0 if succeed, otherwise fail
 *
 * @note           None
 */
int wm_sd_card_block_read(uint32_t rca, uint32_t sd_addr, char *buf);

/**
 * @brief          This function is used to write one block data into the sd card with irq mode .
 *
 * @param[in]      sd_addr   address that to be written to
 * @param[in]      buf   Pointer to the buffer that holding the data to be written
 *
 * @retval         status   0 if succeed, otherwise fail
 *
 * @note           None
 */
int wm_sd_card_block_write(uint32_t rca, uint32_t sd_addr, char *buf);

/**
 * @brief          This function is used to read blocks of data from the sd card with dma mode .
 *
 * @param[in]      sd_addr   address that to be read from
 * @param[in]      buf   Pointer to the buffer that the data shall be read into
 * @param[in]      buflen   buffer size, should be integer multiple of 512
 *
 * @retval         status   0 if succeed, otherwise fail
 *
 * @note           None
 */
int wm_sd_card_blocks_read(uint32_t rca, uint32_t sd_addr, char *buf, uint32_t buflen);

/**
 * @brief          This function is used to write blocks of data into the sd card with dma mode .
 *
 * @param[in]      sd_addr   address that to be written to
 * @param[in]      buf   Pointer to the buffer that holding the data to be written
 * @param[in]      buflen   buffer size, should be integer multiple of 512
 *
 * @retval         status   0 if succeed, otherwise fail
 *
 * @note           None
 */
int wm_sd_card_blocks_write(uint32_t rca, uint32_t sd_addr, char *buf, uint32_t buflen);

/**
 * @}
 */

/**
 * @}
 */

#endif //__WM_SDIO_HOST_H_


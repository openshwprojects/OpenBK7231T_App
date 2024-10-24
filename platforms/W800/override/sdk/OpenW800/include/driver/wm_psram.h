#ifndef __WM_PSRAM_H__
#define __WM_PSRAM_H__

#define PSRAM_ADDR_START          0x30000000
#define PSRAM_SIZE_BYTE           0x00800000

typedef enum{
	PSRAM_SPI = 0,
	PSRAM_QPI,
} psram_mode_t;
/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup PSRAM_Driver_APIs PSRAM Driver APIs
 * @brief PSRAM driver APIs
 */

/**
 * @addtogroup PSRAM_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used to init the psram .
 *
 * @param[in]      mode   is work mode, PSRAM_SPI or PSRAM_QPI
 *
 * @retval        none
 *
 * @note           None
 */
void psram_init(psram_mode_t mode);

/**
 * @brief          This function is used to Copy block of memory in dma mode .
 *
 * @param[in]      src   Pointer to the source of data to be copied
 * @param[in]      dst   Pointer to the destination array where the content is to be copied
 * @param[in]      num   Number of bytes to copy
 *
 * @retval         num   Number of bytes that's been copied
 *
 * @note           None
 */
int memcpy_dma(unsigned char *dst, unsigned char *src, int num);
/**
 * @}
 */

/**
 * @}
 */
#endif






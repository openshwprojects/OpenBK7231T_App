#ifndef __WM_TIPC_H_
#define __WM_TIPC_H_

#include <core_804.h>
#include "wm_regs.h"

#define HR_TIPC_BASE         (HR_APB_BASE_ADDR + 0x2400)

typedef union {
    struct {
        uint32_t I2C: 1;                         /*!< bit:      0 */
        uint32_t SAR_ADC: 1;                     /*!< bit:      1 */
        uint32_t SPI_LS: 1;                      /*!< bit:      2 */
        uint32_t UART0: 1;                       /*!< bit:      3 */
        uint32_t UART1: 1;                       /*!< bit:      4 */
        uint32_t UART2: 1;                       /*!< bit:      5 */
        uint32_t UART3: 1;                       /*!< bit:      6 */
        uint32_t UART4: 1;                       /*!< bit:      7 */
        uint32_t UART5: 1;                       /*!< bit:      8 */
        uint32_t PORTA: 1;                       /*!< bit:      9 */
        uint32_t PORTB: 1;                       /*!< bit:      10 */
        uint32_t WD: 1;                          /*!< bit:      11 */
        uint32_t TIMER: 1;                       /*!< bit:      12 */
        uint32_t RFC: 1;                         /*!< bit:      13 */
        uint32_t LCD: 1;                         /*!< bit:      14 */
        uint32_t PWM: 1;                         /*!< bit:      15 */
        uint32_t I2S: 1;                         /*!< bit:      16 */
        uint32_t BT_MODEM: 1;                    /*!< bit:      17 */
        uint32_t _reserved0: 14;
    }b;
    uint32_t w;
} TIPC_VLD0_Type;

typedef union {
    struct {
        uint32_t SDIO_HOST: 1;                   /*!< bit:      0 */
        uint32_t FLASH: 1;                       /*!< bit:      1 */
        uint32_t PSRAM: 1;                       /*!< bit:      2 */
        uint32_t RSA: 1;                         /*!< bit:      3 */
        uint32_t DMA: 1;                         /*!< bit:      4 */
        uint32_t GPSEC: 1;                       /*!< bit:      5 */
        uint32_t BT: 1;                          /*!< bit:      6 */
        uint32_t PMU: 1;                         /*!< bit:      7 */
        uint32_t CLK_RST: 1;                     /*!< bit:      8 */
        uint32_t MMU: 1;                         /*!< bit:      9 */
        uint32_t BBP: 1;                         /*!< bit:      10 */
        uint32_t MAC: 1;                         /*!< bit:      11 */
        uint32_t SEC: 1;                         /*!< bit:      12 */
        uint32_t _reserved0: 1;                  /*!< bit:      13 */
        uint32_t SDIO_SLAVE: 1;                  /*!< bit:      14 */
        uint32_t SPI_HS: 1;                      /*!< bit:      15 */
        uint32_t SDIO_WRAPPER: 1;                /*!< bit:      16 */
        uint32_t RF_BIST: 1;                     /*!< bit:      17 */
        uint32_t _reserved1: 14;
    }b;
    uint32_t w;
} TIPC_VLD1_Type;

typedef struct {
	__IOM uint32_t VLD0;
	__IOM uint32_t VLD1;
} TIPC_Type;

#define TIPC          ((TIPC_Type    *) HR_TIPC_BASE)

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup TIPC_Driver_APIs TIPC Driver APIs
 * @brief TIPC driver APIs
 */

/**
 * @addtogroup TIPC_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used enable i2c.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_i2c(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.I2C = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable i2c.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_i2c(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.I2C = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable uart0.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_uart0(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART0 = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable uart0.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_uart0(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART0 = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable sar adc.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_sar_adc(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.SAR_ADC = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable sar adc.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_sar_adc(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.SAR_ADC = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable low speed spi.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_lspi(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.SPI_LS = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable low speed spi.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_lspi(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.SPI_LS = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable uart1.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_uart1(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART1 = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable uart1.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_uart1(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART1 = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable uart2.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_uart2(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART2 = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable uart2.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_uart2(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART2 = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable uart3.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_uart3(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART3 = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable uart3.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_uart3(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART3 = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable uart4.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_uart4(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART4 = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable uart4.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_uart4(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART4 = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable uart5.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_uart5(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART5 = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable uart5.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_uart5(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.UART5 = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable porta.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_porta(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.PORTA = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable porta.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_porta(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.PORTA = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable portb.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_portb(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.PORTB = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable portb.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_portb(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.PORTB = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable watch dog.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_watch_dog(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.WD = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable watch dog.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_watch_dog(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.WD = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable timer.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_timer(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.TIMER = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable timer.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_timer(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.TIMER = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable rf controler.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_rf_controler(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.RFC = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable rf controler.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_rf_controler(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.RFC = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable lcd.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_lcd(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.LCD = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable lcd.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_lcd(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.LCD = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable pwm.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_pwm(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.PWM = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable pwm.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_pwm(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.PWM = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable i2s.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_i2s(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.I2S = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable i2s.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_i2s(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.I2S = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable bt modem.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_bt_modem(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.BT_MODEM = 1;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used disable bt modem.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_bt_modem(void)
{
	TIPC_VLD0_Type vld0;
	vld0.w = TIPC->VLD0;
	vld0.b.BT_MODEM = 0;
	TIPC->VLD0 = vld0.w;
}

/**
 * @brief          This function is used enable sdio host.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_sdio_host(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SDIO_HOST = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable sdio host.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_sdio_host(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SDIO_HOST = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable flash.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_flash(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.FLASH = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable flash.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_flash(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.FLASH = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable psram.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_psram(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.PSRAM = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable psram.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_psram(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.PSRAM = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable rsa.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_rsa(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.RSA = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable rsa.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_rsa(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.RSA = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable dma.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_dma(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.DMA = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable dma.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_dma(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.DMA = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable gpsec.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_gpsec(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.GPSEC = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable gpsec.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_gpsec(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.GPSEC = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable bt.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_bt(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.BT = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable bt.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_bt(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.BT = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable pmu.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_pmu(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.PMU = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable pmu.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_pmu(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.PMU = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable clock reset.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_clk_rst(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.CLK_RST = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable clock reset.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_clk_rst(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.CLK_RST = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable mmu.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_mmu(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.MMU = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable mmu.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_mmu(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.MMU = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable bbp.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_bbp(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.BBP = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable bbp.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_bbp(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.BBP = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable mac.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_mac(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.MAC = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable mac.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_mac(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.MAC = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable sec.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_sec(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SEC = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable sec.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_sec(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SEC = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable sdio slave.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_sdio_slave(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SDIO_SLAVE = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable sdio slave.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_sdio_slave(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SDIO_SLAVE = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable high speed spi.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_hspi(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SPI_HS = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable high speed spi.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_hspi(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SPI_HS = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable sdio wrapper.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_sdio_wrapper(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SDIO_WRAPPER = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable sdio wrapper.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_sdio_wrapper(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.SDIO_WRAPPER = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used enable rf bist.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_enable_rf_bist(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.RF_BIST = 1;
	TIPC->VLD1 = vld1.w;
}

/**
 * @brief          This function is used disable rf bist.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
__STATIC_INLINE void wm_tipc_disable_rf_bist(void)
{
	TIPC_VLD1_Type vld1;
	vld1.w = TIPC->VLD1;
	vld1.b.RF_BIST = 0;
	TIPC->VLD1 = vld1.w;
}

/**
 * @}
 */

/**
 * @}
 */

#endif

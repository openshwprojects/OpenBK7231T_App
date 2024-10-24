/**
 * @file    wm_pmu.h
 *
 * @brief   pmu driver module
 *
 * @author  dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_PMU_H
#define WM_PMU_H

#include "wm_type_def.h"

/** peripheral type */
typedef enum {
    TLS_PERIPHERAL_TYPE_I2C   = (1 << 0), /**< peripheral type : I2C */
    TLS_PERIPHERAL_TYPE_UART0 = (1 << 1), /**< peripheral type : UART0 */
    TLS_PERIPHERAL_TYPE_UART1 = (1 << 2), /**< peripheral type : UART1 */
	TLS_PERIPHERAL_TYPE_UART2 = (1 << 3), /**< peripheral type : UART2 */
	TLS_PERIPHERAL_TYPE_UART3 = (1 << 4), /**< peripheral type : UART3 */
    TLS_PERIPHERAL_TYPE_UART4 = (1 << 5), /**< peripheral type : UART4 */

    TLS_PERIPHERAL_TYPE_UART5 = (1 << 6), /**< peripheral type : UART4 */

    TLS_PERIPHERAL_TYPE_LSPI  = (1 << 7), /**< peripheral type : LSPI */
    TLS_PERIPHERAL_TYPE_DMA   = (1 << 8), /**< peripheral type : DMA */

    TLS_PERIPHERAL_TYPE_TIMER = (1 << 10), /**< peripheral type : TIMER */
    TLS_PERIPHERAL_TYPE_GPIO  = (1 << 11), /**< peripheral type : GPIO */
    TLS_PERIPHERAL_TYPE_SDADC = (1 << 12), /**< peripheral type : SDADC */
    TLS_PERIPHERAL_TYPE_PWM   = (1 << 13), /**< peripheral type : PWM */
    TLS_PERIPHERAL_TYPE_LCD   = (1 << 14), /**< peripheral type : LCD */
    TLS_PERIPHERAL_TYPE_I2S   = (1 << 15), /**< peripheral type : I2S */
    TLS_PERIPHERAL_TYPE_RSA   = (1 << 16), /**< peripheral type : RSA */
    TLS_PERIPHERAL_TYPE_GPSEC = (1 << 17), /**< peripheral type : GPSEC */

    TLS_PERIPHERAL_TYPE_SDIO_MASTER = (1<<18), /**< peripheral type : SDIO */
    TLS_PERIPHERAL_TYPE_PSRAM = (1<<19), /**< peripheral type : PSRAM */
    TLS_PERIPHERAL_TYPE_BT    = (1<<20), /**< peripheral type : BT */
    TLS_PERIPHERAL_TYPE_TOUCH_SENSOR  = (1 << 21) /**< peripheral type : TOUCH */
}tls_peripheral_type_s;

/** callback function of PMU interrupt */
typedef void (*tls_pmu_irq_callback)(void *arg);

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup PMU_Driver_APIs PMU Driver APIs
 * @brief PMU driver APIs
 */

/**
 * @addtogroup PMU_Driver_APIs
 * @{
 */


/**
 * @brief          	This function is used to register pmu timer1 interrupt
 *
 * @param[in]      	callback    	the pmu timer1 interrupt call back function
 * @param[in]      	arg         		parameter of call back function
 *
 * @return         	None
 *
 * @note
 * user not need clear interrupt flag.
 * pmu timer1 callback function is called in interrupt,
 * so can not operate the critical data in the callback fuuction,
 * recommendation to send messages to other tasks to operate it.
 */
void tls_pmu_timer1_isr_register(tls_pmu_irq_callback callback, void *arg);


/**
 * @brief          	This function is used to register pmu timer0 interrupt
 *
 * @param[in]      	callback    	the pmu timer0 interrupt call back function
 * @param[in]      	arg         		parameter of call back function
 *
 * @return         	None
 *
 * @note
 * user not need clear interrupt flag.
 * pmu timer0 callback function is called in interrupt,
 * so can not operate the critical data in the callback fuuction,
 * recommendation to send messages to other tasks to operate it.
 */
void tls_pmu_timer0_isr_register(tls_pmu_irq_callback callback, void *arg);


/**
 * @brief          	This function is used to register pmu gpio interrupt
 *
 * @param[in]      	callback    	the pmu gpio interrupt call back function
 * @param[in]      	arg         		parameter of call back function
 *
 * @return         	None
 *
 * @note
 * user not need clear interrupt flag.
 * pmu gpio callback function is called in interrupt,
 * so can not operate the critical data in the callback fuuction,
 * recommendation to send messages to other tasks to operate it.
 */
void tls_pmu_gpio_isr_register(tls_pmu_irq_callback callback, void *arg);


/**
 * @brief          	This function is used to register pmu sdio interrupt
 *
 * @param[in]      	callback    	the pmu sdio interrupt call back function
 * @param[in]      	arg         		parameter of call back function
 *
 * @return         	None
 *
 * @note
 * user not need clear interrupt flag.
 * pmu sdio callback function is called in interrupt,
 * so can not operate the critical data in the callback fuuction,
 * recommendation to send messages to other tasks to operate it.
 */
void tls_pmu_sdio_isr_register(tls_pmu_irq_callback callback, void *arg);


/**
 * @brief          	This function is used to select pmu clk
 *
 * @param[in]      	bypass    pmu clk whether or not use bypass mode
 *				1   pmu clk use 32K by 40MHZ
 *				other pmu clk 32K by calibration circuit
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_clk_select(u8 bypass);


/**
 * @brief          	This function is used to start pmu timer0
 *
 * @param[in]      	second  	vlaue of timer0 count[s]
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer0_start(u16 second);


/**
 * @brief          	This function is used to stop pmu timer0
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer0_stop(void);



/**
 * @brief          	This function is used to start pmu timer1
 *
 * @param[in]      	second  	vlaue of timer1 count[ms]
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer1_start(u16 msec);


/**
 * @brief          	This function is used to stop pmu timer1
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer1_stop(void);



/**
 * @brief          	This function is used to start pmu goto standby 
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_standby_start(void);


/**
 * @brief          	This function is used to start pmu goto sleep 
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_sleep_start(void);

/**
 * @brief          	This function is used to close peripheral's clock
 *
 * @param[in]      	devices  	peripherals
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_close_peripheral_clock(tls_peripheral_type_s devices);

/**
 * @brief          	This function is used to open peripheral's clock
 *
 * @param[in]      	devices  	peripherals
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_open_peripheral_clock(tls_peripheral_type_s devices);
/**
 * @}
 */

/**
 * @}
 */

#endif



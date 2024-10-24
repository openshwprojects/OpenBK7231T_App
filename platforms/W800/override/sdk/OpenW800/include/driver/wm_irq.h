/**
 * @file    wm_irq.h
 *
 * @brief   interupt driver module
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#ifndef WM_IRQ_H
#define WM_IRQ_H

#include "wm_type_def.h"

typedef void (*intr_handler_func) (void *);

/**
 * @typedef struct tls_irq_handler
 */
typedef struct tls_irq_handler
{
    void (*handler) (void *);
    void *data;
    u32 counter;
} tls_irq_handler_t;

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup IRQ_Driver_APIs IRQ Driver APIs
 * @brief IRQ driver APIs
 */

/**
 * @addtogroup IRQ_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used to initial system interrupt.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_irq_init(void);

/**
 * @brief          This function is used to register interrupt handler function.
 *
 * @param[in]      vec_no           interrupt NO
 * @param[in]      handler
 * @param[in]      *data
 *
 * @return         None
 *
 * @note           None
 */
void tls_irq_register_handler(u8 vec_no, intr_handler_func handler, void *data);


/**
 * @brief          This function is used to enable interrupt.
 *
 * @param[in]      vec_no       interrupt NO
 *
 * @return         None
 *
 * @note           None
 */
void tls_irq_enable(u8 vec_no);

/**
 * @brief          This function is used to disable interrupt.
 *
 * @param[in]      vec_no       interrupt NO
 *
 * @return         None
 *
 * @note           None
 */
void tls_irq_disable(u8 vec_no);


/**
 * @brief          This function is used to get the isr count.
 *
 * @param[in]      None
 *
 * @retval         count
 *
 * @note           None
 */
u8 tls_get_isr_count(void);

/**
 * @brief          This function is used to enter interrupt.
 *
 * @param[in]      None
 *
 * @return         0:success
 *
 * @note           None
 */
int csi_kernel_intrpt_enter(void);

/**
 * @brief          This function is used to exit from interrupt.
 *
 * @param[in]      None
 *
 * @return         0:success
 *
 * @note           None
 */
int csi_kernel_intrpt_exit(void);

void tls_irq_priority(u8 vec_no, u32 prio);


/**
 * @}
 */

/**
 * @}
 */

#endif /* WM_IRQ_H */

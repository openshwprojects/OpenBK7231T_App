/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     isr.c
 * @brief    source file for the interrupt server route
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <csi_config.h>
#include "wm_regs.h"

extern void systick_handler(void);
extern void socv_SysTick_Handler(void);
extern void xPortSysTickHandler(void);
extern void OSTimeTick(void);
extern void SDIOA_IRQHandler(void);
extern void HSPI_IRQHandler(void);
extern void TIMER0_5_IRQHandler(void);
extern void CRYPTION_IRQHandler(void);
extern void RSA_F_IRQHandler(void);
extern void GPIO_UART0_IRQHandler(void);

extern void tls_wl_mac_isr(void);
extern void tls_wl_rx_isr(void);

extern int csi_kernel_intrpt_enter(void);
extern int csi_kernel_intrpt_exit(void);

extern void PMU_TIMER0_IRQHandler(void);
extern void PMU_TIMER1_IRQHandler(void);
extern void PMU_GPIO_WAKE_IRQHandler(void);
extern void PMU_RTC_IRQHandler(void);

#define  ATTRIBUTE_ISR __attribute__((isr))

#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#ifndef CONFIG_KERNEL_NONE
#define  CSI_INTRPT_ENTER() csi_kernel_intrpt_enter()
#define  CSI_INTRPT_EXIT()  csi_kernel_intrpt_exit()
#else
#define  CSI_INTRPT_ENTER()
#define  CSI_INTRPT_EXIT()
#endif

//static int tick_test = 0;
ATTRIBUTE_ISR void CORET_IRQHandler(void)
{
#if 0
    if (1000==tick_test)
    {
        tick_test = 2000;
        tls_watchdog_start_cal_elapsed_time();
    }
    else if (2000 == tick_test)
    {
        printf("tick=%u\r\n", tls_watchdog_stop_cal_elapsed_time());
        tick_test = 0;
    }
    else
    {
        tick_test++;
    }
#endif

#ifndef CONFIG_KERNEL_FREERTOS
    CSI_INTRPT_ENTER();
#endif

    readl(0xE000E010);

/*
#ifdef TLS_CONFIG_FPGA
    socv_SysTick_Handler();
#endif
*/

#if defined(CONFIG_KERNEL_RHINO)
    systick_handler();
#elif defined(CONFIG_KERNEL_FREERTOS)
    xPortSysTickHandler();
#elif defined(CONFIG_KERNEL_UCOS)
    OSTimeTick();
#endif

#ifndef CONFIG_KERNEL_FREERTOS
    CSI_INTRPT_EXIT();
#endif
}

ATTRIBUTE_ISR void SDIO_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
#ifndef WM_WIFI_SIMULATION_PROJECT
//    SDIOA_IRQHandler();
#endif
    CSI_INTRPT_EXIT();
}

ATTRIBUTE_ISR void GPSEC_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
    CRYPTION_IRQHandler();
    CSI_INTRPT_EXIT();
}

ATTRIBUTE_ISR void RSA_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
    RSA_F_IRQHandler();
    CSI_INTRPT_EXIT();
}


ATTRIBUTE_ISR void TIM0_5_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
    TIMER0_5_IRQHandler();
    CSI_INTRPT_EXIT();
}


ATTRIBUTE_ISR void SPI_HS_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
#ifndef WM_WIFI_SIMULATION_PROJECT
//    HSPI_IRQHandler();
#endif
    CSI_INTRPT_EXIT();
}

ATTRIBUTE_ISR void MAC_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
#ifndef CONFIG_NO_WIFI
    tls_wl_mac_isr();
#endif
    CSI_INTRPT_EXIT();
}

ATTRIBUTE_ISR void SEC_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
#ifndef CONFIG_NO_WIFI
    tls_wl_rx_isr();
#endif
    CSI_INTRPT_EXIT();
}

ATTRIBUTE_ISR void PMU_IRQHandler(void)
{
    CSI_INTRPT_ENTER();
#ifndef CONFIG_NO_WIFI
#ifndef WM_WIFI_SIMULATION_PROJECT
    if (tls_reg_read32(HR_PMU_INTERRUPT_SRC) & BIT(0))
    {
    	PMU_TIMER0_IRQHandler();
    }
    if (tls_reg_read32(HR_PMU_INTERRUPT_SRC) & BIT(1)) /* timer1 interrupt */
    {
    	PMU_TIMER1_IRQHandler();
    }
    if (tls_reg_read32(HR_PMU_INTERRUPT_SRC) & BIT(2)) /* gpio wake interrupt */
    {
    	PMU_GPIO_WAKE_IRQHandler();
    }

    if (tls_reg_read32(HR_PMU_INTERRUPT_SRC) & BIT(4)) /* rtc interrupt */
    {
    	PMU_RTC_IRQHandler();
    }
#endif
#endif
    CSI_INTRPT_EXIT();
}


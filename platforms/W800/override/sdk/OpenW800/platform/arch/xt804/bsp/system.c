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
 * @file     system.c
 * @brief    CSI Device System Source File
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <csi_config.h>
#include "csi_core.h"
#include "wm_regs.h"
#include "wm_cpu.h"

extern int32_t g_top_irqstack;

extern uint32_t csi_coret_get_load(void);
extern uint32_t csi_coret_get_value(void);

static void _mdelay(void)
{
    uint32_t load = csi_coret_get_load();
    uint32_t start = csi_coret_get_value();
    uint32_t cur;
    uint32_t cnt;
    tls_sys_clk sysclk;

    tls_sys_clk_get(&sysclk);
    cnt = sysclk.cpuclk * 1000;

    while (1) {
        cur = csi_coret_get_value();

        if (start > cur) {
            if (start - cur >= cnt) {
                return;
            }
        } else {
            if (load - cur + start > cnt) {
                return;
            }
        }
    }
}

void mdelay(uint32_t ms)
{
    if (ms == 0) {
        return;
    }

    while (ms--) {
        _mdelay();
    }
}

/**
  * @brief  initialize c++ constructor
  * @param  None
  * @return None
  */
extern int __dtor_end__;
extern int __ctor_end__;
extern int __ctor_start__;
typedef void (*func_ptr)(void);
__attribute__((weak)) void cxx_system_init(void)
{
    func_ptr *p;
    for (p = (func_ptr *)&__ctor_end__ -1; p >= (func_ptr *)&__ctor_start__; p--)
    {
        (*p)();
    }
}


/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    __set_VBR((uint32_t) & (irq_vectors));

#if defined(CONFIG_SEPARATE_IRQ_SP) && !defined(CONFIG_KERNEL_NONE)
    /* 801 not supported */
    __set_Int_SP((uint32_t)&g_top_irqstack);
    __set_CHR(__get_CHR() | CHR_ISE_Msk);
    VIC->TSPR = 0xFF;
#endif

    __set_CHR(__get_CHR() | CHR_IAE_Msk);

    /* Clear active and pending IRQ */
    VIC->IABR[0] = 0x0;
    VIC->ICPR[0] = 0xFFFFFFFF;

#ifdef CONFIG_KERNEL_NONE
    __enable_excp_irq();
#endif

	/*c++ variable init*/
	cxx_system_init();

    //csi_coret_config(g_system_clock / CONFIG_SYSTICK_HZ, SYS_TICK_IRQn);    //10ms
//#ifndef CONFIG_KERNEL_NONE
    csi_vic_enable_irq(SYS_TICK_IRQn);
//#endif
}

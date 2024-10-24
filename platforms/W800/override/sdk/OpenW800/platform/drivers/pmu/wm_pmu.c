#include <string.h>
#include "wm_debug.h"
#include "wm_regs.h"
#include "wm_irq.h"
#include "wm_pwm.h"
#include "wm_gpio.h"
#include "wm_timer.h"
#include "wm_cpu.h"
#include "tls_common.h"
#include "wm_pmu.h"
#include "wm_wifi.h"

struct pmu_irq_context {
    tls_pmu_irq_callback callback;
    void *arg;
};

static struct pmu_irq_context pmu_timer1_context = {0};
static struct pmu_irq_context pmu_timer0_context = {0};
static struct pmu_irq_context pmu_gpio_wake_context = {0};
static struct pmu_irq_context pmu_sdio_wake_context = {0};


void PMU_TIMER1_IRQHandler(void)
{
    tls_reg_write32(HR_PMU_INTERRUPT_SRC, BIT(1)|0x180); /* clear timer1 interrupt */
	tls_reg_write32(HR_PMU_TIMER1, tls_reg_read32(HR_PMU_TIMER1) & (~BIT(16)));
    if (NULL != pmu_timer1_context.callback)
    {
        pmu_timer1_context.callback(pmu_timer1_context.arg);
    }
    return;	
}

void PMU_TIMER0_IRQHandler(void)
{
    tls_reg_write32(HR_PMU_INTERRUPT_SRC, BIT(0)|0x180); /* clear timer0 interrupt */
	tls_reg_write32(HR_PMU_TIMER0, tls_reg_read32(HR_PMU_TIMER0) & (~BIT(16)));

    if (NULL != pmu_timer0_context.callback)
        pmu_timer0_context.callback(pmu_timer0_context.arg);
    return;
}

void PMU_GPIO_WAKE_IRQHandler(void)
{
    tls_reg_write32(HR_PMU_INTERRUPT_SRC, BIT(2)|0x180); /* clear gpio wake interrupt */

    if (NULL != pmu_gpio_wake_context.callback)
        pmu_gpio_wake_context.callback(pmu_gpio_wake_context.arg);
    return;
}

void PMU_SDIO_WAKE_IRQHandler(void)
{
    tls_reg_write32(HR_PMU_INTERRUPT_SRC, BIT(3)|0x180); /* clear sdio wake interrupt */

    if (NULL != pmu_sdio_wake_context.callback)
        pmu_sdio_wake_context.callback(pmu_sdio_wake_context.arg);
    return;
}


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
void tls_pmu_timer1_isr_register(tls_pmu_irq_callback callback, void *arg)
{
    pmu_timer1_context.callback = callback;
    pmu_timer1_context.arg      = arg;

    tls_irq_enable(PMU_IRQn);

    return;
}


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
void tls_pmu_timer0_isr_register(tls_pmu_irq_callback callback, void *arg)
{
    pmu_timer0_context.callback = callback;
    pmu_timer0_context.arg      = arg;

    tls_irq_enable(PMU_IRQn);

    return;
}


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
void tls_pmu_gpio_isr_register(tls_pmu_irq_callback callback, void *arg)
{
    pmu_gpio_wake_context.callback = callback;
    pmu_gpio_wake_context.arg      = arg;

    tls_irq_enable(PMU_IRQn);

    return;
}


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
void tls_pmu_sdio_isr_register(tls_pmu_irq_callback callback, void *arg)
{
    pmu_sdio_wake_context.callback = callback;
    pmu_sdio_wake_context.arg      = arg;

    tls_irq_enable(PMU_IRQn);

    return;
}

/**
 * @brief          	This function is used to select pmu clk
 *
 * @param[in]      	bypass    	pmu clk whether or not use bypass mode
 *				ohter   		pmu clk use 32K by 40MHZ
 *				0 			pmu clk 32K by calibration circuit
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_clk_select(u8 bypass)
{
	u32 val;

	val = tls_reg_read32(HR_PMU_PS_CR);
	if(bypass)
	{
		val |= BIT(4);
	}
	else
	{
		val &= ~BIT(4);
	}
	val |= BIT(3);	
	tls_reg_write32(HR_PMU_PS_CR, val);	
}


/**
 * @brief          	This function is used to start pmu timer0
 *
 * @param[in]      	second  	vlaue of timer0 count[s]
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer0_start(u16 second)
{
	u32 val;
	val = tls_reg_read32(HR_PMU_INTERRUPT_SRC);
	if (val&0x180)
	{
		tls_reg_write32(HR_PMU_INTERRUPT_SRC,val);
	}
	
	val = tls_reg_read32(HR_PMU_PS_CR);
	/*cal 32K osc*/
	val |= BIT(3);
	tls_reg_write32(HR_PMU_PS_CR, val);		

	val = second;
	val |= BIT(16);
	tls_reg_write32(HR_PMU_TIMER0, val); 
}


/**
 * @brief          	This function is used to stop pmu timer0
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer0_stop(void)
{
	u32 val;

	val = tls_reg_read32(HR_PMU_TIMER0);
	val &= ~BIT(16);
	tls_reg_write32(HR_PMU_TIMER0, val); 
}



/**
 * @brief          	This function is used to start pmu timer1
 *
 * @param[in]      	second  	vlaue of timer1 count[ms]
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer1_start(u16 msec)
{
	u32 val;

	val = tls_reg_read32(HR_PMU_INTERRUPT_SRC);
	if (val&0x180)
	{
		tls_reg_write32(HR_PMU_INTERRUPT_SRC,val);
	}
	
	val = tls_reg_read32(HR_PMU_PS_CR);
	/*cal 32K osc*/
	val |= BIT(3);	
	if (!(val & BIT(4)))
	{
		tls_reg_write32(HR_PMU_PS_CR, val);		
		if (msec < 5)
		{
			val = 5;
		}
		else
		{
			val = msec;
		}
		//Ĭ�ϲ�����С��λ1ms
		val = (val - 5) | (1<<16) | (0<<17) | (0<<20) | (0<<24);
	}
	else
	{
		//Ĭ�ϲ�����С��λ1ms
		val = (msec-1)|(1<<16) | (0<<17) | (0<<20) | (0<<24);
	}
	tls_reg_write32(HR_PMU_TIMER1, val);
}


/**
 * @brief          	This function is used to stop pmu timer1
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_timer1_stop(void)
{
	u32 val;
	val = tls_reg_read32(HR_PMU_TIMER1);
	val &= ~BIT(16);
    val &= ~BIT(17);
	tls_reg_write32(HR_PMU_TIMER1, val);
}



/**
 * @brief          	This function is used to start pmu goto standby 
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_standby_start(void)
{
	u32 val;

	tls_irq_enable(PMU_IRQn);		//Ĭ�ϴ��ж�Ϊ�����IO���ѵ��жϱ��

	/*Clear Sleep status after exit sleep mode and enter standby mode*/
	val = tls_reg_read32(HR_PMU_INTERRUPT_SRC);
	if (val&0x180)
	{
		tls_reg_write32(HR_PMU_INTERRUPT_SRC,val);
	}
		
	val = tls_reg_read32(HR_PMU_PS_CR);
	TLS_DBGPRT_INFO("goto standby here\n");
	val |= BIT(0);
	tls_reg_write32(HR_PMU_PS_CR, val);
}

/**
 * @brief          	This function is used to start pmu goto sleep 
 *
 * @param		None
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_pmu_sleep_start(void)
{
	u32 val;
	u32 use40M;

	tls_irq_enable(PMU_IRQn);		//Ĭ�ϴ��ж�Ϊ�����IO���ѵ��жϱ��


	/*Clear Standby status after exit standby mode and enter sleep mode*/
	val = tls_reg_read32(HR_PMU_INTERRUPT_SRC);
	if (val&0x180)
	{
		tls_reg_write32(HR_PMU_INTERRUPT_SRC,val);
	}
	
	val = tls_reg_read32(HR_PMU_PS_CR);
	if (val&BIT(4))
	{
		use40M	= tls_reg_read32(HR_PMU_WLAN_STTS);
		use40M |= BIT(8);
		tls_reg_write32(HR_PMU_WLAN_STTS, use40M);
	}
	TLS_DBGPRT_INFO("goto sleep here\n");
	val |= BIT(1);
	tls_reg_write32(HR_PMU_PS_CR, val);
}


/**
 * @brief          	This function is used to close peripheral's clock
 *
 * @param[in]      	devices  	peripherals
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_close_peripheral_clock(tls_peripheral_type_s devices)
{
    tls_reg_write32(HR_CLK_BASE_ADDR, tls_reg_read32(HR_CLK_BASE_ADDR) & ~(devices));

    return;
}

/**
 * @brief          	This function is used to open peripheral's clock
 *
 * @param[in]      	devices  	peripherals
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_open_peripheral_clock(tls_peripheral_type_s devices)
{
    tls_reg_write32(HR_CLK_BASE_ADDR, tls_reg_read32(HR_CLK_BASE_ADDR) | devices);

    return;
}



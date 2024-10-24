/**
 * @file    wm_touchsensor.c
 *
 * @brief   touchsensor Driver Module
 *
 * @author  
 *
 * Copyright (c) 2021 Winner Microelectronics Co., Ltd.
 */
#include "wm_debug.h"
#include "wm_regs.h"
#include "wm_irq.h"
#include "wm_cpu.h"
#include "wm_gpio.h"

#define  ATTRIBUTE_ISR __attribute__((isr))

typedef void (*touchsensor_cb)(u32 status);
touchsensor_cb tc_callback = NULL;
/**
 * @brief          This function is used to initialize touch sensor.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 * @param[in]      scan_period is scan period for per touch sensor ,unit:16ms, >0
 * @param[in]      window      is count window, window must be greater than 2.Real count window is window - 2.
 * @param[in]      enable      is touch sensor enable bit.
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_init_config(u32 sensorno, u8 scan_period, u8 window, u32 enable)
{
	u32 regval = 0;
#if 0
	/*cfg touch bias*/
	regval = tls_reg_read32(HR_PMU_WLAN_STTS);
	regval &=~(0x70);
	regval |=(0x20);
	tls_reg_write32(HR_PMU_WLAN_STTS, regval);
#endif

	regval = tls_reg_read32(HR_TC_CONFIG);	
	/*firstly, disable scan function */
	tls_reg_write32(HR_TC_CONFIG,regval&(~(1<<TOUCH_SENSOR_EN_BIT)));

	if (scan_period <= 0x3F)
	{
		regval &= ~(0x3F<<SCAN_PERID_SHIFT_BIT);
		regval |= (scan_period<<SCAN_PERID_SHIFT_BIT);
	}

	if (window)
	{
		regval &= ~(0x3F<<CAPDET_CNT_SHIFT_BIT);
		regval |= (window<<CAPDET_CNT_SHIFT_BIT);
	}

	if (sensorno && (sensorno <= 15))
	{
		regval |= (1<<(sensorno-1+TOUCH_SENSOR_SEL_SHIFT_BIT));
	}

	if (enable)
	{
		regval |= (1<<TOUCH_SENSOR_EN_BIT);
	}
	
	tls_reg_write32(HR_TC_CONFIG,regval);

	return 0;
}

/**
 * @brief          This function is used to initialize touch scan channel.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_chan_config(u32 sensorno)
{
	u32 regval = 0;

	regval = tls_reg_read32(HR_TC_CONFIG);	
	if (sensorno && (sensorno <= 15))
	{
		regval |= (1<<(sensorno-1+TOUCH_SENSOR_SEL_SHIFT_BIT));
	}
	
	tls_reg_write32(HR_TC_CONFIG,regval);

	return 0;
}

/**
 * @brief          This function is used to initialize touch general configuration.
 *
 * @param[in]      scanperiod  is scan period for per touch sensor ,unit:16ms, >0
 * @param[in]      window      is count window, window must be greater than 2.Real count window is window - 2.
 * @param[in]      bias        is touch sensor bias current
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_scan_config(u8 scanperiod, u8 window, u8 bias)
{
	u32 regval = 0;

	regval = tls_reg_read32(HR_PMU_WLAN_STTS);
	if (bias <= 7)
	{
		regval &=~(0x70);
		regval |=(bias<<4);
	}
	else
	{
		regval &=~(0x70);
		regval |=(0x4<<4);
	}
	tls_reg_write32(HR_PMU_WLAN_STTS, regval);
	
	regval = tls_reg_read32(HR_TC_CONFIG);	
	if (scanperiod <= 0x3F)
	{
		regval &= ~(0x3F<<SCAN_PERID_SHIFT_BIT);
		regval |= (scanperiod<<SCAN_PERID_SHIFT_BIT);
	}

	if (window)
	{
		regval &= ~(0x3F<<CAPDET_CNT_SHIFT_BIT);
		regval |= (window<<CAPDET_CNT_SHIFT_BIT);
	}
	tls_reg_write32(HR_TC_CONFIG,regval);

	return 0;
}

/**
 * @brief          This function is used to start touch scan
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_scan_start(void)
{
	u32 regval = 0;

	regval = tls_reg_read32(HR_TC_CONFIG);	
	regval |= (1<<TOUCH_SENSOR_EN_BIT);
	tls_reg_write32(HR_TC_CONFIG,regval);

	return 0;
}

/**
 * @brief          This function is used to stop touch scan
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_scan_stop(void)
{
	u32 regval = 0;

	regval = tls_reg_read32(HR_TC_CONFIG);	
	regval &= ~(1<<TOUCH_SENSOR_EN_BIT);
	tls_reg_write32(HR_TC_CONFIG,regval);

	return 0;
}

/**
 * @brief          This function is used to deinit touch sensor's selection and disable touch.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 *
 * @retval         0:success
 *
 * @note           if do not use touch sensor, user can deinit by this interface and configure this touch sensor as GPIO.
 */
int tls_touchsensor_deinit(u32 sensorno)
{
	u32 regval = 0;

	regval = tls_reg_read32(HR_TC_CONFIG);
	if (sensorno && (sensorno <= 15))
	{
		regval &= ~(1<<(sensorno-1+TOUCH_SENSOR_SEL_SHIFT_BIT));
	}
	regval &= ~(1<<TOUCH_SENSOR_EN_BIT);
	tls_reg_write32(HR_TC_CONFIG,regval);

	return 0;
}


/**
 * @brief          This function is used to set threshold per touch sensor.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 * @param[in]      threshold   is the sensorno's touch sensor threshold,max value is 127.
 *
 * @retval         0:success. minus value: parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_threshold_config(u32 sensorno, u8 threshold)
{
	u32 regvalue = 0;
	if((sensorno == 0) || (sensorno > 15))
	{
		return -1;
	}

	if (threshold > 0x7F)
	{
		return -2;
	}

	regvalue = tls_reg_read32(HR_TC_CONFIG+sensorno*4);
	regvalue &= ~(0x7F);
	regvalue |= threshold;
	tls_reg_write32(HR_TC_CONFIG + sensorno*4, regvalue);
	return 0;
}

/**
 * @brief          This function is used to get touch sensor's count number.
 *
 * @param[in]      sensorno    is the touch sensor number from 1 to 15.
 *
 * @retval         sensorno's count number  .
 *
 * @note           None
 */
int tls_touchsensor_countnum_get(u32 sensorno)
{
	if((sensorno == 0) || (sensorno > 15))
	{
		return -1;
	}

	return ((tls_reg_read32(HR_TC_CONFIG+sensorno*4)>>8)&0x3FFF);	 
}

/**
 * @brief          This function is used to enable touch sensor's irq.
 *
 * @param[in]      sensorno    is the touch sensor number  from 1 to 15.
 *
 * @retval         0:successfully enable irq, -1:parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_irq_enable(u32 sensorno)
{
	u32 value = 0;
	if (sensorno && (sensorno <= 15))
	{
		value = tls_reg_read32(HR_TC_INT_EN);
		value |= (1<<(sensorno+15));
		tls_reg_write32(HR_TC_INT_EN, value);
		tls_irq_enable(TOUCH_IRQn);
		return 0;
	}

	return -1;
}

/**
 * @brief          This function is used to disable touch sensor's irq.
 *
 * @param[in]      sensorno    is the touch sensor number  from 1 to 15.
 *
 * @retval         0:successfully disable irq, -1:parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_irq_disable(u32 sensorno)
{
	u32 value = 0;
	if (sensorno && (sensorno <= 15))
	{	
		value = tls_reg_read32(HR_TC_INT_EN);
		value &= ~(1<<(sensorno+15));	
		tls_reg_write32(HR_TC_INT_EN, value);
		if ((value & 0xFFFF0000) == 0)
		{
			tls_irq_disable(TOUCH_IRQn);
		}
		return 0;
	}

	return -1;
}

/**
 * @brief          This function is used to register touch sensor's irq callback.
 *
 * @param[in]      callback    is call back for user's application.
 *
 * @retval         None.
 *
 * @note           None
 */
void tls_touchsensor_irq_register(void (*callback)(u32 status))
{
	tc_callback = callback;
}

/**
 * @brief          This function is touch sensor's irq handler.
 *
 * @param[in]      None
 *
 * @retval         None
 *
 * @note           None
 */
//static u32 tc1cnt[16] = {0};
ATTRIBUTE_ISR void tls_touchsensor_irq_handler(void)
{
	u32 value = 0;
//	int i = 0;
	value = tls_reg_read32(HR_TC_INT_EN);
#if 0	
	for (i = 0; i < 15; i++)
	{
		if (value&BIT(i))
		{
			tc1cnt[i]++;
			printf("tcnum[%02d]:%04d,%04d\r\n", i+1, tc1cnt[i], tls_touchsensor_countnum_get(i+1));
		}	
	}
	totalvalue |= (value&0xFFFF);
	printf("val:%04x,%04x\r\n", value&0xFFFF, totalvalue);
#endif
	if (tc_callback)
	{
		tc_callback(value&0xFFFF);
	}
	tls_reg_write32(HR_TC_INT_EN, value);
}

/**
 * @brief          This function is used to get touch sensor's irq status.
 *
 * @param[in]      sensorno    is the touch sensor number  from 1 to 15.
 *
 * @retval         >=0:irq status, -1:parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_irq_status_get(u32 sensorno)
{
	u32 value = 0;

	if (sensorno && (sensorno <= 15))
	{
		value = tls_reg_read32(HR_TC_INT_EN);
		return (value&(1<<(sensorno-1)))?1:0;
	}

	return -1;
}





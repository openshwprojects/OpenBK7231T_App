/***************************************************************************** 
* 
* File Name : wm_adc.h 
* 
* Description: adc Driver Module 
* 
* Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-8-15
*****************************************************************************/ 

#ifndef WM_ADC_H
#define WM_ADC_H

#include "wm_type_def.h"

#define ADC_DEST_BUFFER_SIZE			16383//以字为单位	


/*ADC Result*/
#define ADC_RESULT_MASK					(0x3FFFF)
#define ADC_RESULT_VAL(n)				((n)&ADC_RESULT_MASK)

/*ADC_ANALOG_CTRL*/
#define CONFIG_ADC_CHL_SEL_MASK 		(0xF<<8)
#define CONFIG_ADC_CHL_SEL(n)	 		((n)<<8)

#define CONFIG_PD_ADC_MASK             	(0x1<<2)
#define CONFIG_PD_ADC_VAL(n)           	((n)<<2)	/*1:pd adc, 0: normal work*/

#define CONFIG_RSTN_ADC_MASK           	(0x1<<1)
#define CONFIG_RSTN_ADC_VAL(n)          ((n)<<1)   /*1:normal work, 0:adc reset*/

#define CONFIG_EN_LDO_ADC_MASK         	(0x1<<0)
#define CONFIG_EN_LDO_ADC_VAL(n)       	((n)<<0)	/*1:ldo work, 0: ldo shutdown*/

/*PGA_CTRL*/
#define CLK_CHOP_SEL_PGA_MASK			(0x7<<4)
#define CLK_CHOP_SEL_PGA_VAL(n)			((n)<<4)


#define GAIN_CTRL_PGA_MASK				(0x3<<7)
#define GAIN_CTRL_PGA_VAL(n)			((n)<<7)


#define PGA_BYPASS_MASK					(0x1<<3)
#define PGA_BYPASS_VAL(n)				((n)<<3)   /*1:bypass pga, 0:use pga*/

#define BYPASS_INNER_REF_SEL			(0x1<<2)   /*Internal or external reference select*/

#define PGA_CHOP_ENP_MASK				(0x1<<1)
#define PGA_CHOP_ENP_VAL(n)				((n)<<1)   /*1: enable chop, 0: disable chop*/

#define PGA_EN_MASK						(0x1<<0)
#define PGA_EN_VAL(n)					((n)<<0)   /*1: enable pga, 0: disable pga*/


/*Temperature Control*/
#define TEMP_GAIN_MASK					(0x3<<4)
#define TEMP_GAIN_VAL(n)				((n)<<4)

#define TEMP_CAL_OFFSET_MASK			(0x1<<1)

#define TEMP_EN_MASK					(0x1<<0)
#define TEMP_EN_VAL(n)					((n)<<0)  /*1: enable temperature, 0: disable temperature*/


/*ADC CTRL*/
#define ANALOG_SWITCH_TIME_MASK			(0x3FF<<20)
#define ANALOG_SWITCH_TIME_VAL(n)		(((n)&0x3FF)<<20)

#define ANALOG_INIT_TIME_MASK			(0x3FF<<8)
#define ANALOG_INIT_TIME_VAL(n)			(((n)&0x3FF)<<8)

#define CMP_POLAR_MASK                  (0x1<<6)

#define CMP_IRQ_EN_MASK                 (0x1<<5)
#define CMP_IRQ_EN_VAL(n)				((n)<<5)  /*1: enable cmp irq, 0: disable cmp irq*/


#define CMP_EN_MASK                 	(0x1<<4)
#define CMP_EN_VAL(n)					((n)<<4) /*1: enable cmp function, 0: disable cmp function*/

#define ADC_IRQ_EN_MASK                 (0x1<<1)
#define ADC_IRQ_EN_VAL(n)				((n)<<1)   /*1:enable adc transfer irq, 0: disable*/

#define ADC_DMA_EN_MASK                 (0x1<<0)
#define ADC_DMA_EN_VAL(n)				((n)<<0)   /*1:enable adc dma, 0: disable*/


/*ADC IRQ Status*/
#define CMP_INT_MASK					(0x1<<1)

#define ADC_INT_MASK					(0x1<<0)

/*CMP Value*/
#define CONFIG_ADC_INPUT_CMP_VAL(n)		((n)&0x3FFFF)


/*ADC Channel*/
#define CONFIG_ADC_CHL_OFFSET			(0x0E)
#define CONFIG_ADC_CHL_VOLT				(0x0D)
#define CONFIG_ADC_CHL_TEMP				(0x0C)



#define ADC_INT_TYPE_ADC				0
#define ADC_INT_TYPE_DMA				1
#define ADC_INT_TYPE_ADC_COMP 			2

#define ADC_REFERENCE_EXTERNAL  		0       //外部参考
#define ADC_REFERENCE_INTERNAL  		1       //内部参考

typedef struct adc_st{
	u8 dmachannel;
	void (*adc_cb)(int *buf, u16 len);
	void (*adc_bigger_cb)(int *buf, u16 len);
	void (*adc_dma_cb)(int *buf,u16 len);
	u16 valuelen;		/*dma 采样数据长度*/
	u16 offset;
}ST_ADC;

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup ADC_Driver_APIs ADC Driver APIs
 * @brief ADC driver APIs
 */

/**
 * @addtogroup ADC_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used to init adc.
 *
 * @param[in]      ifusedma    if use dma
 * @param[in]      dmachannel  dma channel
 *
 * @return         None
 *
 * @note           None
 */
void tls_adc_init(u8 ifusedma,u8 dmachannel);

/**
 * @brief          This function is used to register interrupt callback function.
 *
 * @param[in]      inttype    interrupt type:
 *                 ADC_INT_TYPE_ADC		adc interrupt,user get adc result from the callback function.
 *				   ADC_INT_TYPE_DMA		dma interrupt,dma transfer the adc result to the user's buffer.
 * @param[in]      callback   interrupt callback function.
 *
 * @return         None
 *
 * @note           None
 */
void tls_adc_irq_register(int inttype, void (*callback)(int *buf, u16 len));

/**
 * @brief          This function is used to clear the interrupt source.
 *
 * @param[in]      inttype    interrupt type:
 *                 ADC_INT_TYPE_ADC		adc interrupt,user get adc result from the callback function.
 *				   ADC_INT_TYPE_DMA		dma interrupt,dma transfer the adc result to the user's buffer.
 *
 * @return         None
 *
 * @note           None
 */
void tls_adc_clear_irq(int inttype);

/**
 * @brief          This function is used to register interrupt callback function.
 *
 * @param[in]      Channel    adc channel,from 0 to 3 is single input;4 and 5 is differential input.
 * @param[in]      Length     byte data length,is an integer multiple of half word,need <= 0x500
 *
 * @return         None
 *
 * @note           None
 */
void tls_adc_start_with_dma(int Channel, int Length);

/**
 * @brief          This function is used to start adc.
 *
 * @param[in]      Channel    adc channel,from 0 to 3 is single input;4 and 5 is differential input.
 *
 * @return         None
 *
 * @note           None
 */
void tls_adc_start_with_cpu(int Channel);

/**
 * @brief           This function is used to read adc result.
 *
 * @param[in]      	None
 *
 * @retval          adc result
 *
 * @note            None
 */
u32 tls_read_adc_result(void);

/**
 * @brief           This function is used to stop the adc.
 *
 * @param[in]      	ifusedma    if use dma
 *
 * @return          None
 *
 * @note            None
 */
void tls_adc_stop(int ifusedma);

/**
 * @brief           This function is used to config adc bigger register.
 *
 * @param[in]      	cmp_data    compare data
 * @param[in]      	cmp_pol     compare pol
 *
 * @return          None
 *
 * @note            None
 */
void tls_adc_config_cmp_reg(int cmp_data, int cmp_pol);

/**
 * @brief           This function is used to set adc reference source.
 *
 * @param[in]      	ref     ADC_REFERENCE_EXTERNAL,ADC_REFERENCE_INTERNAL
 *
 * @return          None
 *
 * @note            None
 */
void tls_adc_reference_sel(int ref);

/**
 * @brief           This function is used to read internal temperature.
 *
 * @param[in]      	None
 *
 * @retval          temperature
 *
 * @note            None
 */
int adc_get_interTemp(void);

/**
 * @brief           This function is used to read input voltage.
 *
 * @param[in]      	channel    adc channel,from 0 to 3 is single input;8 and 9 is differential input.
 *
 * @retval          voltage    unit:mV
 *
 * @note            None
 */
int adc_get_inputVolt(u8 channel);

/**
 * @brief           This function is used to read internal voltage.
 *
 * @param[in]      	None
 *
 * @retval          voltage (mV)
 *
 * @note            None
 */
u32 adc_get_interVolt(void);

/**
 * @brief           This function is used to read temperature.
 *
 * @param[in]      	None
 *
 * @retval          temperature
 *
 * @note            None
 */
int adc_temp(void);

/**
 * @brief          This function is used to calibrate adc single-end voltage. offset after FT or multipoint calibration.
 *
 * @param[in]      chan: adc calibration channel to be used, ADC channel 0,1,2,3
 * @param[in]      refvoltage: calibration reference voltage to be used, unit:mV
 *                             input range[100,2300)mV, suggest reference voltage[500,2000]mV
 *
 * @return         0: success, < 0: failure
 *
 * @note           After FT calibration or mulitpoint calibration, adc curve is y=ax+b,
 *                 y is real voltage(unit:mV), x is adc sample data
 *                 a and b is the coefficient. This fuction only used to revise b value.
 */ 
int adc_offset_calibration(int chan, int refvoltage);

/**
 * @brief          This function can used to calibrate adc coefficient if not calibrated,
 *                 or adc offset after FT or multipoint calibration.
 *
 * @param[in]      chanused: bitmap, specified calibration channel,only bit0-3 can be used
 * @param[in]      refvoltage[]: array, calibration reference voltage to be used, unit:mV
 *                               refvoltage keep the same position with chan bitmap
 *                               input range[100,2300)mV, suggest reference voltage[500,2000]mV
 *
 * @return         0: success, < 0: failure
 *
 * @note           1)Adc curve is y=ax+b,y is real voltage(unit:mV), x is adc sample data.
 *                 After calibration, we can get a and b, or only update b.
 *                 2) Only used single-end adc
 *                 3) For example, use chan 0,1,3, and refvoltage 500,1000,2000, 
 * 		             then chanused is 0xB, refvoltage[] value is  {500,1000,0, 2000};
 */
int adc_multipoint_calibration(int chanused, int refvoltage[]);


/**
 * @}
 */

/**
 * @}
 */

void tls_adc_enable_calibration_buffer_offset(void);
void tls_adc_voltage_start_with_cpu(void);
void tls_adc_temp_offset_with_cpu(u8 calTemp12);
void tls_adc_voltage_start_with_dma(int Length);
void tls_adc_set_clk(int div);
void signedToUnsignedData(int *adcValue);
void tls_adc_buffer_bypass_set(u8 isset);
void tls_adc_cmp_start(int Channel, int cmp_data, int cmp_pol);
u32  adc_get_offset(void);
void tls_adc_set_pga(int gain1, int gain2);
int  cal_voltage(double vol);


#endif


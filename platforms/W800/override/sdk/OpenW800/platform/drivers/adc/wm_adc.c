
/***************************************************************************** 
* 
* File Name : wm_adc.c 
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wm_regs.h"
#include "wm_adc.h"
#include "wm_dma.h"
#include "wm_io.h"
#include "wm_irq.h"
#include "wm_mem.h"
#include "wm_efuse.h"

#define  ATTRIBUTE_ISR __attribute__((isr))

typedef struct 
{
	int poly_n;
	double a[3];
}ST_ADC_POLYFIT_PARAM;

/*f(x) = kx + b*/
//#define ADC_CAL_K_POS   (6)
//#define ADC_CAL_B_POS   (7)


ST_ADC_POLYFIT_PARAM _polyfit_param = {0};
extern void polyfit(int n,double x[],double y[],int poly_n,double a[]);

static int adc_offset = 0;
static int *adc_dma_buffer = NULL;
volatile ST_ADC gst_adc;

ATTRIBUTE_ISR void ADC_IRQHandler(void)
{
	u32 adcvalue;
	int reg;
	csi_kernel_intrpt_enter();

	reg = tls_reg_read32(HR_SD_ADC_INT_STATUS);
	if(reg & ADC_INT_MASK)      //ADC中断
	{
		tls_adc_clear_irq(ADC_INT_TYPE_ADC);


	    if(gst_adc.adc_cb)
		{
		    adcvalue = tls_read_adc_result();
			gst_adc.adc_cb((int *)&adcvalue,1);
	    }
	}
	if(reg & CMP_INT_MASK)
	{

	    tls_adc_clear_irq(ADC_INT_TYPE_ADC_COMP);
	    if(gst_adc.adc_bigger_cb)
			gst_adc.adc_bigger_cb(NULL, 0);
	}
	csi_kernel_intrpt_exit();
}

static void adc_dma_isr_callbk(void)
{
	if(gst_adc.adc_dma_cb)
	{
		if (adc_dma_buffer)
		{
			gst_adc.adc_dma_cb((int *)(adc_dma_buffer), gst_adc.valuelen);	
		}
	}
}
int adc_polyfit_init(ST_ADC_POLYFIT_PARAM *polyfit_param)
{
	FT_ADC_CAL_ST adc_cal;
	/*function f(x) = ax + b*/
	float a = 0.0;
	float b = 0.0;
	int i;
	double x[8] = {0.0};
	double y[8] = {0.0};

	polyfit_param->poly_n = 0;
	memset(&adc_cal, 0, sizeof(adc_cal));
	tls_get_adc_cal_param(&adc_cal);
	if ((adc_cal.valid_cnt == 4)
		||(adc_cal.valid_cnt == 2) 
		|| (adc_cal.valid_cnt == 3))
	{
		//memcpy((char *)&a, (char *)&adc_cal.units[ADC_CAL_K_POS], 4);
		//memcpy((char *)&b, (char *)&adc_cal.units[ADC_CAL_B_POS], 4);	
		a = adc_cal.a;
		b = adc_cal.b;
		if ((a > 1.0) && (a < 1.3) && (b < -1000.0)) /*new calibration*/
		{
			polyfit_param->poly_n = 1;
			polyfit_param->a[1] = a;
			polyfit_param->a[0] = b;
		}
		else /*old calibration*/
		{
			for(i = 0; i < adc_cal.valid_cnt; i++)
			{
				x[i] = (double)adc_cal.units[i].real_val;
				y[i] = (double)adc_cal.units[i].ref_val;
			}
			polyfit_param->poly_n = 1;
			polyfit(adc_cal.valid_cnt,x,y,1, polyfit_param->a);
			if (b < -1000.0)
			{
				polyfit_param->a[0] = b;
			}
		}
	}

	return 0;
}

void tls_adc_init(u8 ifusedma,u8 dmachannel)
{
	/*ADC LDO ON here, after 90ns, ADC can work to avoid ADC affect those ADC IOs that are multiplexed GPIO input to capture irq*/
	u32 value = 0;
	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value |= CONFIG_EN_LDO_ADC_VAL(1);
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);

	memset(&_polyfit_param, 0, sizeof(_polyfit_param));
	adc_polyfit_init(&_polyfit_param);
	tls_reg_write32(HR_SD_ADC_CTRL, ANALOG_SWITCH_TIME_VAL(0x50)|ANALOG_INIT_TIME_VAL(0x50)|ADC_IRQ_EN_VAL(0x1));
	tls_irq_enable(ADC_IRQn);

	//注册中断和channel有关，所以需要先请求
	if(ifusedma)
	{
		gst_adc.dmachannel = tls_dma_request(dmachannel, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_SDADC_CH0 + dmachannel) |
                            TLS_DMA_FLAGS_HARD_MODE);	//请求dma，不要直接指定，因为请求的dma可能会被别的任务使用
        if (gst_adc.dmachannel != 0xFF)
       	{
			tls_dma_irq_register(gst_adc.dmachannel, (void(*)(void*))adc_dma_isr_callbk, NULL, TLS_DMA_IRQ_TRANSFER_DONE);
        }
	}

	//printf("\ndma channel = %d\n",gst_adc.dmachannel);
}

void tls_adc_clear_irq(int inttype)
{
    int reg;
    reg = tls_reg_read32(HR_SD_ADC_INT_STATUS);
	if(ADC_INT_TYPE_ADC == inttype)
	{
	    reg |= ADC_INT_MASK;
	    tls_reg_write32(HR_SD_ADC_INT_STATUS, reg);
	}
	else if(ADC_INT_TYPE_ADC_COMP== inttype)
	{
	    reg |= CMP_INT_MASK;
	    tls_reg_write32(HR_SD_ADC_INT_STATUS, reg);
	}
	else if(ADC_INT_TYPE_DMA == inttype)
	{
	    tls_dma_irq_clr(gst_adc.dmachannel, TLS_DMA_IRQ_TRANSFER_DONE);
	}
}

void tls_adc_irq_register(int inttype, void (*callback)(int *buf, u16 len))
{
	if(ADC_INT_TYPE_ADC == inttype)
	{
		gst_adc.adc_cb = callback;
	}
	else if(ADC_INT_TYPE_DMA == inttype)
	{
		gst_adc.adc_dma_cb = callback;
	}
	else if(ADC_INT_TYPE_ADC_COMP == inttype)
	{
	    gst_adc.adc_bigger_cb = callback;
	}
}

u32 tls_read_adc_result(void)
{
	u32 value;
	u32 ret;
	
	value = tls_reg_read32(HR_SD_ADC_RESULT_REG);
	ret = ADC_RESULT_VAL(value);
	
	return ret;
}

void tls_adc_start_with_cpu(int Channel)
{
	u32 value;

	/* Stop adc first */
	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value |= CONFIG_PD_ADC_VAL(1);
	value &= ~(CONFIG_RSTN_ADC_VAL(1));
	value &= ~(CONFIG_ADC_CHL_SEL_MASK);
	value |= CONFIG_ADC_CHL_SEL(Channel);

	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);
	
	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value &= ~(CONFIG_PD_ADC_VAL(1));
	value |= (CONFIG_RSTN_ADC_VAL(1));
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);
}


void tls_adc_start_with_dma(int Channel, int Length)
{
	u32 value;
	int len;

	if(Channel < 0 || Channel > 11)
		return;
        
	if(Length > ADC_DEST_BUFFER_SIZE)
		len = ADC_DEST_BUFFER_SIZE;
	else
		len = Length;

	gst_adc.valuelen = len;

	if (adc_dma_buffer)
	{
		tls_mem_free(adc_dma_buffer);
		adc_dma_buffer = NULL;
	}

	adc_dma_buffer = tls_mem_alloc(len*4);
	if (adc_dma_buffer == NULL)
	{
		//wm_printf("adc dma buffer alloc failed\r\n");
		return;
	}

	Channel &= 0xF;

	/*disable adc:set adc pd, rstn and ldo disable*/
	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value |= CONFIG_PD_ADC_VAL(1);
	value &= ~(CONFIG_RSTN_ADC_VAL(1));	
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);	
	
	/* Stop dma if necessary */
	while(DMA_CHNLCTRL_REG(gst_adc.dmachannel) & 1)
	{
		DMA_CHNLCTRL_REG(gst_adc.dmachannel) = 2;
	}

    DMA_SRCADDR_REG(gst_adc.dmachannel) = HR_SD_ADC_RESULT_REG;
	DMA_DESTADDR_REG(gst_adc.dmachannel) = (u32)adc_dma_buffer;
	DMA_SRCWRAPADDR_REG(gst_adc.dmachannel) = HR_SD_ADC_RESULT_REG;
	DMA_DESTWRAPADDR_REG(gst_adc.dmachannel) = (u32)adc_dma_buffer;
    DMA_WRAPSIZE_REG(gst_adc.dmachannel) = (len*4) << 16;

	/* Dest_add_inc, halfword,  */
	DMA_CTRL_REG(gst_adc.dmachannel) = (3<<3)|(2<<5)|((len*4)<<8)|(1<<0);
	DMA_INTMASK_REG &= ~(0x01 << (gst_adc.dmachannel *2 + 1));
	DMA_CHNLCTRL_REG(gst_adc.dmachannel) = 1;		/* Enable dma */

	/*Enable dma*/
	value = tls_reg_read32(HR_SD_ADC_CTRL);
	value |= (1<<0);
	tls_reg_write32(HR_SD_ADC_CTRL, value); 

	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
    value &= ~(CONFIG_ADC_CHL_SEL_MASK);
	value |= CONFIG_ADC_CHL_SEL(Channel);
	value &= ~(CONFIG_PD_ADC_VAL(1));
	value |= (CONFIG_RSTN_ADC_VAL(1));	
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);		/*start adc*/

}

void tls_adc_stop(int ifusedma)
{
	u32 value;

	tls_reg_write32(HR_SD_ADC_PGA_CTRL, 0);

	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value |= CONFIG_PD_ADC_VAL(1);
	value &= ~(CONFIG_RSTN_ADC_VAL(1)|CONFIG_EN_LDO_ADC_VAL(1));	
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);

	/*Disable dma*/
	value = tls_reg_read32(HR_SD_ADC_CTRL);
	value &= ~(1<<0);
	tls_reg_write32(HR_SD_ADC_CTRL, value); 

	/*Disable compare function and compare irq*/
	value = tls_reg_read32(HR_SD_ADC_CTRL);
	value &= ~(3<<4);
	tls_reg_write32(HR_SD_ADC_CTRL, value);	

	if(ifusedma)
		tls_dma_free(gst_adc.dmachannel);

	if (adc_dma_buffer)
	{
		tls_mem_free(adc_dma_buffer);
		adc_dma_buffer = NULL;
	}
}

void tls_adc_config_cmp_reg(int cmp_data, int cmp_pol)
{
    u32 value;

    tls_reg_write32(HR_SD_ADC_CMP_VALUE, CONFIG_ADC_INPUT_CMP_VAL(cmp_data));

    value = tls_reg_read32(HR_SD_ADC_CTRL);
	if(cmp_pol)
	{
		value |= CMP_POLAR_MASK;
	}
	else
	{
		value &= ~CMP_POLAR_MASK;
	}
    tls_reg_write32(HR_SD_ADC_CTRL, value);
}

void tls_adc_cmp_start(int Channel, int cmp_data, int cmp_pol)
{
	u32 value;
	
	/* Stop adc first */
	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value |= CONFIG_PD_ADC_VAL(1);
	value &= ~(CONFIG_RSTN_ADC_VAL(1));		
	value |= CONFIG_ADC_CHL_SEL(Channel);
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);

	tls_adc_config_cmp_reg(cmp_data, cmp_pol);
	
	value = tls_reg_read32(HR_SD_ADC_ANA_CTRL);
	value &= ~(CONFIG_PD_ADC_VAL(1));
	value |= (CONFIG_RSTN_ADC_VAL(1));	
	tls_reg_write32(HR_SD_ADC_ANA_CTRL, value);		/*start adc*/

	/*Enable compare function and compare irq*/
	value = tls_reg_read32(HR_SD_ADC_CTRL);
	value |= (3<<4);
	tls_reg_write32(HR_SD_ADC_CTRL, value);	
}


void tls_adc_reference_sel(int ref)
{
    u32 value;
    
    value = tls_reg_read32(HR_SD_ADC_PGA_CTRL);
    if(ADC_REFERENCE_EXTERNAL == ref)
    {
		value |= BYPASS_INNER_REF_SEL;
    }
    else if(ADC_REFERENCE_INTERNAL == ref)
    {
		value &= ~BYPASS_INNER_REF_SEL;
    }
    tls_reg_write32(HR_SD_ADC_PGA_CTRL, value);    
}

void tls_adc_set_clk(int div)
{
    u32 value;

    value = tls_reg_read32(HR_CLK_SEL_CTL);
    value &= ~(0xFF<<8);
    value |=  (div&0xFF)<<8;
    tls_reg_write32(HR_CLK_SEL_CTL, value);
    value = tls_reg_read32(HR_CLK_DIV_CTL);
    value |= (1 << 31);
    tls_reg_write32(HR_CLK_DIV_CTL, value);
}

void tls_adc_set_pga(int gain1, int gain2)
{
	u32 val = 0;
	u8 gain1times = 0;
	u8 gain2times = 0;	
	switch(gain1)
	{
		case 1:
			gain1times = 0;
		break;
		case 16:
			gain1times = 1;
		break;
		case 32:
			gain1times = 2;
		break;
		case 64:
			gain1times = 3;
		break;
		case 128:
			gain1times = 4;
		break;
		case 256:
			gain1times = 5;
		break;
		default:
			gain1times = 0;
			break;
	}
	
	switch(gain2)
	{
		case 1:
			gain2times = 0;
			break;
		case 2:
			gain2times = 1;
			break;
		case 3:
			gain2times = 2;
			break;
		case 4:
			gain2times = 3;
			break;
		default:
			gain2times = 0;
			break;
	}

	val = tls_reg_read32(HR_SD_ADC_PGA_CTRL);
	val = GAIN_CTRL_PGA_VAL(gain2times)|CLK_CHOP_SEL_PGA_VAL(gain1times)|PGA_BYPASS_VAL(0)|PGA_CHOP_ENP_VAL(1)|PGA_EN_VAL(1);
	tls_reg_write32(HR_SD_ADC_PGA_CTRL, val);
}

void signedToUnsignedData(int *adcValue)
{
	if (*adcValue &0x20000)
	{
		*adcValue = *adcValue &0x1FFFF;
	}
	else
	{
		*adcValue = *adcValue |0x20000;
	}
}

static void waitForAdcDone(void)
{
    u32 counter = 0;
	u32 timeout = 10000;
	u32 reg = 0;

	/*wait for transfer success*/
	tls_irq_disable(ADC_IRQn);
	while(timeout--)
	{
		reg = tls_reg_read32(HR_SD_ADC_INT_STATUS);
		if (reg & ADC_INT_MASK)
		{
			counter++;
		    tls_reg_write32(HR_SD_ADC_INT_STATUS, reg|ADC_INT_MASK);			
			if (counter == 4)
			{
				break;
			}
		}
		else if(reg & CMP_INT_MASK)
        {
        	counter++;
			tls_reg_write32(HR_SD_ADC_INT_STATUS, reg|CMP_INT_MASK);
			if (counter == 4)
			{
				break;
			}
        }
	}
	tls_irq_enable(ADC_IRQn);
}

int cal_voltage(double vol)
{
	double y1, voltage;
	int average = ((int)vol >> 2) & 0xFFFF;

	if(_polyfit_param.poly_n == 1)
	{
		y1 = _polyfit_param.a[1]*average + _polyfit_param.a[0];
	}
	else
	{
		voltage = ((double)vol - (double)adc_offset)/4.0;
		voltage = 1.196 + voltage*(126363/1000.0)/1000000;
		
	    y1 = voltage*10000;
	}

	return (int)(y1/10);
}

u32 adc_get_offset(void)
{ 
	adc_offset = 0;
    tls_adc_init(0, 0); 
	tls_adc_reference_sel(ADC_REFERENCE_INTERNAL);
	tls_adc_start_with_cpu(CONFIG_ADC_CHL_OFFSET);	
	tls_adc_set_pga(1,1);
	tls_adc_set_clk(0x28);		

    waitForAdcDone();
	adc_offset = tls_read_adc_result(); //获取adc转换结果
	signedToUnsignedData(&adc_offset);
	tls_adc_stop(0);
//printf("adc_offset: 0x%x\r\n", adc_offset);
    return adc_offset;
}

int adc_get_interTemp(void)
{
	return adc_temp();
}

int adc_get_inputVolt(u8 channel)
{
	int average = 0;
	double voltage = 0.0;

	if(_polyfit_param.poly_n == 0 || (channel == 8) || (channel == 9))
	{
		adc_get_offset();
	}
	
	tls_adc_init(0, 0); 
	tls_adc_reference_sel(ADC_REFERENCE_INTERNAL);
	tls_adc_set_pga(1,1);
	tls_adc_set_clk(0x28);	
	tls_adc_start_with_cpu(channel);

	waitForAdcDone();
	average = tls_read_adc_result();
	signedToUnsignedData(&average); 	
	tls_adc_stop(0);

	if ((channel == 8) || (channel == 9))
	{
		voltage = ((double)average - (double)adc_offset)/4.0;
		voltage = voltage*(126363/1000)/1000000;
	}
	else
	{
		return cal_voltage((double)average);
	}

	average = (int)(voltage*1000);
    return average;
}

u32 adc_get_interVolt(void)
{
	u32 voltValue;
	u32 value = 0;
	//u32 code = 0;	
	int i = 0;
	adc_get_offset();

    tls_adc_init(0, 0); 
	tls_adc_reference_sel(ADC_REFERENCE_INTERNAL);
	tls_adc_set_pga(1,1);
	tls_adc_set_clk(0x28);	
	tls_adc_start_with_cpu(CONFIG_ADC_CHL_VOLT);
	voltValue = 0;
	for (i = 0;i < 10; i++)
	{
		waitForAdcDone();
		value = tls_read_adc_result();
		signedToUnsignedData((int *)&value);
		voltValue += value;
	}
	voltValue = voltValue/10;
	//code = voltValue;
	voltValue = voltValue;
	adc_offset = adc_offset;
	tls_adc_stop(0);
	voltValue = ((voltValue - adc_offset)*685/20+1200000)*2;
	value = voltValue - voltValue*10/100;
	//printf("Power voltage code:0x%x, interVolt:%d(uV)---%d.%d(V)\r\n", code, value, value/1000000, (value%1000000)/1000);

    return value/1000;
}

/**
 * @brief          This function is used to get chip's internal work temperature
 *
 * @return         chip temperature, unit: 1/1000 degree
 *
 * @note           Only use to get chip's internal work temperature.
 */
int adc_temp(void)
{
	u32 code1 = 0, code2 = 0;
	u32 val = 0;
	int temperature = 0;

    tls_adc_init(0, 0); 
	tls_adc_reference_sel(ADC_REFERENCE_INTERNAL);
	tls_adc_set_pga(1,4);
	tls_adc_start_with_cpu(CONFIG_ADC_CHL_TEMP);
	tls_adc_set_clk(0x28);
	val = tls_reg_read32(HR_SD_ADC_TEMP_CTRL);
	val &= ~TEMP_GAIN_MASK;
	val |= TEMP_GAIN_VAL(0);
	val |= TEMP_EN_VAL(1);

	val &= (~(TEMP_CAL_OFFSET_MASK));
	tls_reg_write32(HR_SD_ADC_TEMP_CTRL, val);		
	waitForAdcDone();
    code1 = tls_read_adc_result(); 
	signedToUnsignedData((int *)&code1);

	val |= TEMP_CAL_OFFSET_MASK;
	tls_reg_write32(HR_SD_ADC_TEMP_CTRL, val);
	waitForAdcDone();
    code2 = tls_read_adc_result(); 
	signedToUnsignedData((int *)&code2);

	val &= ~(TEMP_EN_VAL(1));
	tls_reg_write32(HR_SD_ADC_TEMP_CTRL, val);

	tls_adc_stop(0);

	temperature = ((int)code1 - (int)code2);

	temperature = ((temperature*1000/(int)(2*2*4)-4120702)*1000/15548);


	return temperature;
}



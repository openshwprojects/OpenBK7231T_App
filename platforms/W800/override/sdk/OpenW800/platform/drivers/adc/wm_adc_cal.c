
/***************************************************************************** 
* 
* File Name : wm_adc_cal.c 
* 
* Description: adc calibration 
* 
* Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
* All rights reserved. 
* 
* Author : 
* 
* Date : 2022-10-10
*****************************************************************************/ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wm_include.h"
#include "wm_regs.h"
#include "wm_adc.h"
#include "wm_io.h"
#include "wm_gpio_afsel.h"
#include "wm_efuse.h"

#define ZERO_CAL_MAX_NUM   (10)
/*f(x) = kx + b*/
//#define ADC_CAL_B_POS   (7)
//#define ADC_CAL_K_POS   (6)
#define ADC_CAL_REF_VOLTAGE_POS (5)
extern void polyfit(int n,double x[],double y[],int poly_n,double a[]);

/**
 * @brief          This function can used to calibrate adc coefficient if not calibrated,
 *                 or adc offset after FT or multipoint calibration.
 *
 * @param[in]      chanused: bitmap, specified calibration channel,only bit0-3 can be used
 * @param[in]      refvoltage[]: array, calibration reference voltage to be used, unit:mV
 *                               refvoltage keep the same position with chan bitmap
 *                               real range[100,2300)mV, suggest reference voltage[500,2000]mV
 *
 * @return         0: success, < 0: failure
 *
 * @note           1)Adc curve is y=ax+b,y is real voltage(unit:mV), x is adc sample data.
 *                 After calibration, we can get a and b, or only update b.
 *                 2) Only used single-end adc
 *                 3) For example, use chan 0,1,3, and refvoltage 500,1000,2000, 
 * 		             then chanused is 0xB, refvoltage[] value is  {500,1000,0, 2000};
 */
int adc_multipoint_calibration(int chanused, int refvoltage[])
{
	#define MAX_ADC_CAL_CHAN   (4)
	#define MAX_ADC_REF_VOLTAGE   (2300)
	#define MIN_ADC_REF_VOLTAGE   (100)
	if (((chanused &((1<<MAX_ADC_CAL_CHAN)-1)) == 0)  || (refvoltage == NULL))
	{
		return -1;
	}

	FT_ADC_CAL_ST adc_cal;
	int average = 0;
	int chancnt = 0;
	u32 sum_value = 0;
	float b = 0.0;

	int *prefvoltage = refvoltage;
	
	/*update calibration parameter*/
	memset(&adc_cal, 0xFF, sizeof(FT_ADC_CAL_ST));

	for (int j = 0; j < MAX_ADC_CAL_CHAN; j++)
	{
		if ((chanused&(1<<j)) == 0)
		{
			continue;
		}
        
		if ((prefvoltage[j] < MIN_ADC_REF_VOLTAGE) || (prefvoltage[j] >= MAX_ADC_REF_VOLTAGE))
		{
			return -1;
		}
		/*config adc chan for calibration*/
		wm_adc_config(j);

		/*config adc and start capture data*/
		tls_adc_init(0, 0); 
		tls_adc_reference_sel(ADC_REFERENCE_INTERNAL);
		tls_adc_set_pga(1,1);
		tls_adc_set_clk(40);	
		tls_adc_start_with_cpu(j);

		tls_os_time_delay(1);
		sum_value = 0;
		for (int i = 1; i <= ZERO_CAL_MAX_NUM; i++)
		{
			tls_os_time_delay(1);
			average = *(TLS_REG *)(HR_SD_ADC_RESULT_REG);
			average = ADC_RESULT_VAL(average);		
			signedToUnsignedData(&average); 
			sum_value += average;
		}

		adc_cal.units[chancnt].ref_val  = prefvoltage[j]*10;
		adc_cal.units[chancnt].real_val = sum_value/ZERO_CAL_MAX_NUM/4;
		chancnt++;
		tls_adc_stop(0);
	}

	if (chancnt >= 2)
	{
		float k = 0.0;
		double a[4] = {0.0};	
		double x[8] = {0.0};
		double y[8] = {0.0};

		for(int i = 0; i < chancnt; i++)
		{
			x[i] = (double)adc_cal.units[i].real_val;
			y[i] = (double)adc_cal.units[i].ref_val;
		}

		polyfit(chancnt, x, y, 1, a);

		k = (float)a[1];
		b = (float)a[0];
		//memcpy((char *)&adc_cal.units[ADC_CAL_K_POS], (char *)&k, 4);
		//memcpy((char *)&adc_cal.units[ADC_CAL_B_POS], (char *)&b, 4);
		adc_cal.a = k;
		adc_cal.b = b;
		adc_cal.valid_cnt = chancnt;
		return tls_set_adc_cal_param(&adc_cal);
	}
	else /*only one reference voltage*/
	{
		/*update calibration parameter*/
		int currentvalue = cal_voltage((double)adc_cal.units[0].real_val*4);
		int ret = tls_get_adc_cal_param(&adc_cal);
		if ((ret == 0) && (adc_cal.valid_cnt >= 2) && (adc_cal.valid_cnt <= 4))
		{
			/*reference voltage by user settings*/
			for (int i = 0; i < MAX_ADC_CAL_CHAN; i++)
			{
				if (chanused&(1<<i))
				{
					adc_cal.units[ADC_CAL_REF_VOLTAGE_POS].ref_val = prefvoltage[i];
					average = prefvoltage[i];
					break;
				}
			}
			/*update parameter b that used by the function f(x)=ax+b */
			//memcpy((char *)&b, (char *)&adc_cal.units[ADC_CAL_B_POS], 4);
			b = adc_cal.b;
			b = b + (average - currentvalue)*10; /*0.1mV*/
			//memcpy((char *)&adc_cal.units[ADC_CAL_B_POS], (char *)&b, 4);
			adc_cal.b = b;
			return tls_set_adc_cal_param(&adc_cal);
		}
		else
		{
			return -1;
		}
	}
}

/**
 * @brief          This function is used to calibrate adc offset after FT or multipoint calibration.
 *
 * @param[in]      chan: adc calibration channel to be used
 * @param[in]      refvoltage: calibration reference voltage to be used, unit:mV
 *
 * @return         0: success, < 0: failure
 *
 * @note           After FT calibration or mulitpoint calibration, adc curve is y=ax+b,
 *                 y is real voltage(unit:mV), x is adc sample data
 *                 a and b is the coefficient. This fuction only used to revise b value.
 */
int adc_offset_calibration(int chan, int refvoltage)
{
	if (chan >= 0 && chan < 4)
	{
		int refvol[4] = {0, 0, 0, 0};
		refvol[chan] = refvoltage;
		return adc_multipoint_calibration((1<<chan), refvol);
	}
	return -1;
}



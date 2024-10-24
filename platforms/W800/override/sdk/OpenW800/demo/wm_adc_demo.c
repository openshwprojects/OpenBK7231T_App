/***************************************************************************** 
* 
* File Name : wm_adc_demo.c 
* 
* Description: adc demo function 
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-8-18
*****************************************************************************/ 
#include "wm_include.h"
#include "wm_adc.h"
#include "wm_gpio_afsel.h"


#if DEMO_ADC

int adc_input_voltage_demo(u8 chan)
{
	int voltage =0;

	if (chan <= 1)
	{
    	wm_adc_config(chan);
	}
	else if (chan == 8 )
	{
    	wm_adc_config(0);		
    	wm_adc_config(1);				
	}
    voltage = adc_get_inputVolt(chan);
	if (voltage < 0)
	{
		voltage = 0 - voltage;
		printf("chan:%d, -%d(mV) or -%d.%03d(V)\r\n", chan, voltage, voltage/1000, voltage%1000);
	}
	else
	{
		printf("chan:%d, %d(mV) or %d.%03d(V)\r\n", chan, voltage, voltage/1000, voltage%1000);	
	}
    
    return 0;
}


int adc_chip_temperature_demo(void)
{
    char temperature[8] = {0};
    int temp;
    
    temp = adc_temp();
	if (temp < 0)
	{
		temp = 0 - temp;
    	sprintf(temperature, "-%d.%03d", temp/1000, temp%1000);
	}
	else
	{
    	sprintf(temperature, "%d.%03d", temp/1000, temp%1000);	
	}
    printf("tem: %s\r\n", temperature);
    
    return 0;
}


int adc_power_voltage_demo(void)
{
	int voltage =0;

    voltage = adc_get_interVolt();
	printf("Power voltage:%d(mV) or %d.%03d(V)\r\n",voltage, voltage/1000, voltage%1000);	
    
    return 0;
}

#endif






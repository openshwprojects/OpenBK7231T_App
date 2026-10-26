#if PLATFORM_ARMINO

#include "../hal_adc.h"
#include "gpio_driver.h"
#include "sys_driver.h"
#include "driver/adc.h"

#define ADC_TIMEOUT_MS 1000
#define BUF_SIZE 5+5
static uint16_t adc_buf[BUF_SIZE];

int HAL_ADC_Read(int pinNumber)
{
	adc_config_t config = { 0 };
	int value = 0;
	switch(pinNumber)
	{
#if PLATFORM_BK7236
		case 0:  config.chan = 12; break;
		case 1:  config.chan = 13; break;
		case 8:  config.chan = 10; break;
		case 12: config.chan = 14; break;
		case 13: config.chan = 15; break;
		case 21: config.chan = 6;  break;
		case 22: config.chan = 5;  break;
		case 23: config.chan = 3;  break;
		case 24: config.chan = 2;  break;
		case 25: config.chan = 1;  break;
		case 28: config.chan = 4;  break;
#elif PLATFORM_BK7239N || PLATFORM_BK7236N
		case 2:  config.chan = 1;  break;
		case 3:  config.chan = 2;  break;
		case 4:  config.chan = 3;  break;
		case 5:  config.chan = 4;  break;
		case 6:  config.chan = 5;  break;
		case 7:  config.chan = 6;  break;
		case 8:  config.chan = 10; break;
		case 12: config.chan = 14; break;
		case 13: config.chan = 15; break;
#endif
		default: return 0;
	}
	config.clk = SOC_ADC_MAX_CLK; // 3203125;
	config.sample_rate = 32;
	config.adc_filter = 0;
	config.steady_ctrl = 7;
	config.adc_mode = ADC_CONTINUOUS_MODE;
	config.saturate_mode = ADC_SATURATE_MODE_2;
#if CONFIG_SARADC_V1P1
	config.src_clk = ADC_SCLK_XTAL_26M;
#else
	config.src_clk = ADC_SCLK_XTAL;
#endif

	sys_drv_set_ana_pwd_gadc_buf(1);
	bk_adc_acquire();
	gpio_dev_unmap(pinNumber);
	bk_adc_init(config.chan);
	bk_adc_set_config(&config);
	bk_adc_enable_bypass_clalibration();
	bk_adc_start();
	bk_adc_read_raw((uint16_t*)&adc_buf, 10, ADC_TIMEOUT_MS);
	bk_adc_stop();
	bk_adc_deinit(config.chan);
	bk_adc_release();
	sys_drv_set_ana_pwd_gadc_buf(0);
	uint32_t cnt = 0;
	for(int i = 5; i < BUF_SIZE; ++i)
	{
		if(adc_buf[i] == 0) continue;
		value += adc_buf[i];
		++cnt;
	}
	if(cnt == 0)
		value = adc_buf[0];
	else
		value /= cnt;
	return value;
}

#endif

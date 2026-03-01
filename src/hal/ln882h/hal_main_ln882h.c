#if PLATFORM_LN882H || PLATFORM_LN8825

#include "osal/osal.h"
#include "utils/debug/log.h"
#include "wifi.h"
#include "netif/ethernetif.h"
#include "wifi_manager.h"
#include "lwip/tcpip.h"
#include "utils/system_parameter.h"
#include "utils/sysparam_factory_setting.h"
#include "hal/hal_adc.h"
#include "ln_nvds.h"

#define PM_DEFAULT_SLEEP_MODE             (ACTIVE)

#define USR_APP_TASK_STACK_SIZE           (10*256) //Byte

#if PLATFORM_LN882H
#include "utils/debug/ln_assert.h"
#include "utils/power_mgmt/ln_pm.h"
#include "ln_wifi_err.h"
#include "ln_misc.h"
#include "ln882h.h"
#include "../hal_adc.h"
#define HAL_TEMP_ADC 0xFF
#else
#include "utils/debug/art_assert.h"
#include "hal/hal_sleep.h"
#define LN_ASSERT ART_ASSERT
#define HAL_ADC_Init(x) drv_adc_init()
#define HAL_ADC_Read(x) drv_adc_read(x)
#define HAL_TEMP_ADC INTL_ADC_CHAN_0
#define ln_pm_sleep_mode_set hal_sleep_set_mode
#endif

static OS_Thread_t g_usr_app_thread;
float g_wifi_temperature = 0.0f;

extern void Main_Init();
extern void Main_OnEverySecond();

void usr_app_task_entry(void *params)
{
	// from tuyaopen
#if PLATFORM_LN882H
	*(volatile int*)(0x40012034) = 0x540;
#endif
	wifi_manager_init();

	int8_t cap_comp = 0;
	uint16_t adc_val = 0;

	if(NVDS_ERR_OK == ln_nvds_get_xtal_comp_val((uint8_t*)&cap_comp))
	{
		if((uint8_t)cap_comp == 0xFF)
		{
			cap_comp = 0;
		}
	}

	HAL_ADC_Init(HAL_TEMP_ADC);
	OS_MsDelay(1);
	wifi_temp_cal_init(HAL_ADC_Read(HAL_TEMP_ADC), cap_comp);

	Main_Init();
  
	while(1)
	{
		adc_val = HAL_ADC_Read(HAL_TEMP_ADC);
		wifi_do_temp_cal_period(adc_val);
		g_wifi_temperature = (25 + ((adc_val & 0xFFF) - 770) / 2.54f);
		OS_MsDelay(1000);
		Main_OnEverySecond();
	}
}

void creat_usr_app_task(void)
{
	ln_pm_sleep_mode_set(PM_DEFAULT_SLEEP_MODE);

#if PLATFORM_LN882H
	/**
	 * CLK_G_EFUSE: For wifi temp calibration
	 * CLK_G_BLE  CLK_G_I2S  CLK_G_WS2811  CLK_G_DBGH  CLK_G_SDIO  CLK_G_EFUSE  CLK_G_AES
	*/
	ln_pm_always_clk_disable_select(CLK_G_BLE | CLK_G_I2S | CLK_G_WS2811 | CLK_G_SDIO | CLK_G_AES);

	/**
	 * ADC0: For wifi temp calibration
	 * TIM3: For wifi pvtcmd evm test
	 * CLK_G_ADC  CLK_G_GPIOA  CLK_G_GPIOB  CLK_G_SPI0  CLK_G_SPI1  CLK_G_I2C0  CLK_G_UART1  CLK_G_UART2
	 * CLK_G_WDT  CLK_G_TIM_REG  CLK_G_TIM1  CLK_G_TIM2  CLK_G_TIM3  CLK_G_TIM4  CLK_G_MAC  CLK_G_DMA
	 * CLK_G_RF  CLK_G_ADV_TIMER  CLK_G_TRNG
	*/
	ln_pm_lightsleep_clk_disable_select(CLK_G_GPIOA | CLK_G_GPIOB | CLK_G_SPI0 | CLK_G_SPI1 | CLK_G_I2C0 |
		CLK_G_UART1 | CLK_G_UART2 | CLK_G_WDT | CLK_G_TIM1 | CLK_G_TIM2 | CLK_G_MAC | CLK_G_DMA | CLK_G_RF | CLK_G_ADV_TIMER | CLK_G_TRNG);


	/* print sdk version */
	LOG(LOG_LVL_INFO, "LN882H SDK Ver: %s [build time:%s][0x%08x]\r\n",
		LN882H_SDK_VERSION_STRING, LN882H_SDK_BUILD_DATE_TIME, LN882H_SDK_VERSION);
#else
	LOG(LOG_LVL_INFO, "SDK version string: %s\r\n",     LN_SDK_VERSION_STRING);
	LOG(LOG_LVL_INFO, "SDK version number: 0x%08x\r\n", LN_SDK_VERSION);
	LOG(LOG_LVL_INFO, "SDK build time    : %s\r\n",     LN_SDK_BUILD_DATE_TIME);
#endif

	if(OS_OK != OS_ThreadCreate(&g_usr_app_thread, "OBK", usr_app_task_entry, NULL, OS_PRIORITY_BELOW_NORMAL, USR_APP_TASK_STACK_SIZE))
	{
		LN_ASSERT(1);
	}
}

__attribute__((weak))
void* os_malloc(size_t size)
{
	return OS_Malloc(size);
}

__attribute__((weak))
void os_free(void* ptr)
{
	OS_Free(ptr);
}

#endif

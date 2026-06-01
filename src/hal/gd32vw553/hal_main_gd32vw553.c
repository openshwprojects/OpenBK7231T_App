#if PLATFORM_GD32VW553

#include <stdint.h>
#include "wifi_export.h"
#include "gd32vw55x_platform.h"
#include "uart.h"
#include "ble_init.h"
#include "gd32vw55x.h"
#include "wrapper_os.h"
#include "cmd_shell.h"
#include "atcmd.h"
#include "util.h"
#include "wlan_config.h"
#include "wifi_init.h"
#include "user_setting.h"
#include "version.h"
#include "_build_date.h"
#include "config_gdm32.h"
#include "gd32vw55x_adc.h"
#include "gd32vw55x_rcu.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rom_export.h"

char reset_str[64] = "";
uint8_t running_idx = IMAGE_0;
uint8_t* g_mac = NULL;
extern uint8_t* wifi_vif_mac_addr_get(int vif_idx);
extern float g_wifi_temperature;
extern void Main_Init();
extern void Main_OnEverySecond();
static void obk_task(void* pvParameters)
{
	uint8_t temp_sec = 10;
	vTaskDelay(50 / portTICK_PERIOD_MS);
	Main_Init();
	for(;;)
	{
		if(temp_sec >= 10)
		{
			adc_flag_clear(ADC_FLAG_EOC);
			adc_tempsensor_vrefint_enable();
			//adc_clock_config(ADC_ADCCK_PCLK2_DIV6);
			adc_external_trigger_config(ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
			adc_channel_length_config(ADC_ROUTINE_CHANNEL, 1);
			adc_routine_channel_config(0, ADC_CHANNEL_9, ADC_SAMPLETIME_55POINT5);
			adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);
			adc_special_function_config(ADC_SCAN_MODE, DISABLE);
			adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
			adc_channel_length_config(ADC_ROUTINE_CHANNEL, 1);
			adc_resolution_config(ADC_RESOLUTION_12B);

			adc_enable();
			adc_software_trigger_enable(ADC_ROUTINE_CHANNEL);
			while(SET != adc_flag_get(ADC_FLAG_EOC));

			uint16_t raw = adc_routine_data_read();
			adc_disable();

			g_wifi_temperature = (1.42f - raw * 3.3f / 4096) * 1000 / 4.3f + 25;
			temp_sec = 0;
		}
		temp_sec++;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		Main_OnEverySecond();
	}
	vTaskDelete(NULL);
}

int main(void)
{
	sys_os_init();
	platform_init();

	dbg_print(NOTICE, "SDK Version: %s\n", WIFI_GIT_REVISION);
	dbg_print(NOTICE, "Build date: %s\n", SDK_BUILD_DATE);
	dbg_print(NOTICE, "Image Version: %s%x.%x.%x.%03x\n",
		RE_CUSTOMER_NAME,
		(RE_IMG_VERSION >> 28),
		(RE_IMG_VERSION >> 20) & 0xFF,
		(RE_IMG_VERSION >> 12) & 0xFF,
		RE_IMG_VERSION & 0xFFF);

	util_init();

	user_setting_init();

	if(wifi_init())
	{
		dbg_print(ERR, "wifi init failed\r\n");
	}

	g_mac = wifi_vif_mac_addr_get(WIFI_VIF_INDEX_DEFAULT);

	rcu_periph_clock_enable(RCU_SYSCFG);

	rcu_periph_clock_enable(RCU_ADC);
	adc_clock_config(ADC_ADCCK_PCLK2_DIV6);

	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);

	uint32_t flags = (RCU_RSTSCK >> 26U) & 0x3F;
	rcu_all_reset_flag_clear();
	if(flags & (1U << 0)) strcat(reset_str, "EPRST & ");
	if(flags & (1U << 1)) strcat(reset_str, "PORRST & ");
	if(flags & (1U << 2)) strcat(reset_str, "SWRST & ");
	if(flags & (1U << 3)) strcat(reset_str, "FWDGT & ");
	if(flags & (1U << 4)) strcat(reset_str, "WWDGT & ");
	if(flags & (1U << 5)) strcat(reset_str, "LPRST & ");
	uint32_t len = strlen(reset_str);
	if(len >= 3)
	{
		reset_str[len - 3] = '\0';
	}
	rom_sys_status_get(SYS_RUNNING_IMG, LEN_SYS_RUNNING_IMG, &running_idx);

	HAL_BTProxy_Init();

	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		6,
		NULL);

	sys_os_start();
}

#endif

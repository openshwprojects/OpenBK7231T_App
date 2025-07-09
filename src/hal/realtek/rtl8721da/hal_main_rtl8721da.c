#if PLATFORM_RTL8721DA || PLATFORM_RTL8720E

#include "../../../new_common.h"
#include "wifi_api_types.h"
#include "wifi_api_ext.h"
#include "flash_api.h"
#include "ameba_ota.h"

//rtw_mode_t wifi_mode = RTW_MODE_STA;
TaskHandle_t g_sys_task_handle1;
uint32_t current_fw_idx = 0;
uint8_t wmac[6] = { 0 };
unsigned char ap_ip[4] = { 192, 168, 4, 1 }, ap_netmask[4] = { 255, 255, 255, 0 }, ap_gw[4] = { 192, 168, 4, 1 };
extern void wifi_fast_connect_enable(unsigned char enable);
extern int (*p_wifi_do_fast_connect)(void);
__attribute__((weak)) flash_t flash;

void sys_task1(void* pvParameters)
{
	if(p_wifi_do_fast_connect) p_wifi_do_fast_connect = NULL;
	wifi_get_mac_address(0, (struct rtw_mac*)&wmac, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	Main_Init();
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		Main_OnEverySecond();
	}
}

void app_pre_example(void)
{
	wifi_fast_connect_enable(0);
	current_fw_idx = ota_get_cur_index(1);
	//if(p_wifi_do_fast_connect) p_wifi_do_fast_connect = NULL;
	xTaskCreate(
		sys_task1,
		"OpenBeken",
		8 * 256,
		NULL,
		3,
		&g_sys_task_handle1);
}

#endif // PLATFORM_RTL8721DA

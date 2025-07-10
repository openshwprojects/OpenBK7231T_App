#if PLATFORM_RTL8721DA || PLATFORM_RTL8720E

#include "../../../new_common.h"
#include "wifi_api_types.h"
#include "wifi_api_ext.h"
#include "flash_api.h"
#include "ameba_ota.h"
#include "ameba_soc.h"
#include "os_wrapper.h"

TaskHandle_t g_sys_task_handle1;
uint32_t current_fw_idx = 0;
uint8_t wmac[6] = { 0 };
uint8_t flash_size_8720;
unsigned char ap_ip[4] = { 192, 168, 4, 1 }, ap_netmask[4] = { 255, 255, 255, 0 }, ap_gw[4] = { 192, 168, 4, 1 };
extern void wifi_fast_connect_enable(unsigned char enable);
extern int (*p_wifi_do_fast_connect)(void);
__attribute__((weak)) flash_t flash;

#if PLATFORM_RTL8720E

extern float g_wifi_temperature;
void temp_func(void* pvParameters)
{
	RCC_PeriphClockCmd(APBPeriph_ATIM, APBPeriph_ATIM_CLOCK, ENABLE);
	TM_Cmd(ENABLE);
	TM_InitTypeDef TM_InitStruct;
	TM_StructInit(&TM_InitStruct);
	TM_Init(&TM_InitStruct);
	for(;;)
	{
		g_wifi_temperature = TM_GetCdegree(TM_GetTempResult());
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}

#endif

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
	u8 flash_ID[4];
	FLASH_RxCmd(flash_init_para.FLASH_cmd_rd_id, 3, flash_ID);
	flash_size_8720 = (1 << (flash_ID[2] - 0x11)) / 8;
	current_fw_idx = ota_get_cur_index(1);
	xTaskCreate(
		sys_task1,
		"OpenBeken",
		8 * 256,
		NULL,
		3,
		&g_sys_task_handle1);

#if PLATFORM_RTL8720E

	xTaskCreate(
		temp_func,
		"TempFunc",
		1024,
		NULL,
		13,
		NULL);

#endif
}

#endif // PLATFORM_RTL8721DA

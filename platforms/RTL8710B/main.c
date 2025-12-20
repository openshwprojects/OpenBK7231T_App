#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"
#include "wifi_constants.h"
#include "rtl8711b_efuse.h"
#include "platform_stdlib.h"

extern void Main_Init();
extern void Main_OnEverySecond();
extern int g_sleepfactor;

rtw_mode_t wifi_mode = RTW_MODE_NONE;
TaskHandle_t g_sys_task_handle1;
uint8_t wmac[6] = { 0 };
char set_key[40];
uint32_t current_fw_idx;

void LOGUART_SetBaud_FromFlash(void) {}

static void obk_task(void* pvParameters)
{
	vTaskDelay(50 / portTICK_PERIOD_MS);
	Main_Init();
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS / g_sleepfactor);
		Main_OnEverySecond();
	}
	vTaskDelete(NULL);
}

int main(void)
{
	wlan_network();
	sys_jtag_off();

	uint8_t* efuse = (uint8_t*)malloc(512);
	EFUSE_LogicalMap_Read(efuse);
	memcpy(wmac, efuse + 0x11A, 6);
	free(efuse);

	current_fw_idx = ota_get_cur_index();

	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		6,
		NULL);

	/* Enable Schedule, Start Kernel */
	vTaskStartScheduler();

	/* Should NEVER reach here */
	return 0;
}

#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"
#include "wifi_constants.h"
#include "platform_stdlib.h"
#include <lwip/netif.h>
#include "rtl8195a_sys_on.h"
#include "device_lock.h"

extern void Main_Init();
extern void Main_OnEverySecond();
extern struct netif xnetif[NET_IF_NUM];

rtw_mode_t wifi_mode = RTW_MODE_NONE;
TaskHandle_t g_sys_task_handle1;
uint8_t wmac[6] = { 0 };

static void obk_task(void* pvParameters)
{
	vTaskDelay(50 / portTICK_PERIOD_MS);
	Main_Init();
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		Main_OnEverySecond();
	}
	vTaskDelete(NULL);
}

int main(void)
{
	wlan_network();
	//sys_jtag_off();

	// this is not mac, but at least it should be unique
	device_mutex_lock(RT_DEV_LOCK_EFUSE);
	for(int i = 0xF4; i < 0xFA; i++)
	{
		HALEFUSEOneByteReadROM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), i, &wmac[i - 0xF4], 7);
	}
	device_mutex_unlock(RT_DEV_LOCK_EFUSE);

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

#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"
#include "wifi_constants.h"
#include "hal_misc.h"
#include "hal_sys_ctrl.h"
#include "efuse_logical_api.h"

extern uint32_t get_cur_fw_idx(void);
void Main_Init();
void Main_OnEverySecond();

hal_reset_reason_t reset_reason;
rtw_mode_t wifi_mode = RTW_MODE_STA;
TaskHandle_t g_sys_task_handle1;
uint32_t current_fw_idx = 0;
uint8_t wmac[6] = { 0 };

void print_wlan_help(void* arg) {}
void hci_tp_close(void) {}
void sys_task1(void* pvParameters)
{
	Main_Init();
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		Main_OnEverySecond();
	}
}

int main(void)
{
	rtl8710c_reset_reason_get(&reset_reason);
	current_fw_idx = get_cur_fw_idx();

	wlan_network();

	hal_misc_swd_pin_ctrl(0);

	efuse_logical_read(0x11A, 6, (uint8_t*)wmac);

	xTaskCreate(
		sys_task1,
		"OpenBeken",
		8 * 256,
		NULL,
		3,
		&g_sys_task_handle1);

	/* Enable Schedule, Start Kernel */
	vTaskStartScheduler();

	/* Should NEVER reach here */
	return 0;
}
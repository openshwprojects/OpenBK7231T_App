#include "system.h"
#include "standalone.h"
#include "wdt.h"
#include "drv_adc.h"

extern void hal_rtc_init(void);
extern void wifi_drv_init(void);
extern sys_err_t wifi_load_nv_cfg();

void Main_Init();
void Main_OnEverySecond();

TaskHandle_t g_sys_task_handle1;

void sys_task1(void* pvParameters)
{
	Main_Init();
	while(true)
	{
		sys_delay_ms(1000);
		Main_OnEverySecond();
	}
}

int main()
{
	hal_rtc_init();

	TrPsmInit();

	partion_init();
	easyflash_init();

	Hal_WtdSoftTask_Init();

	wifi_drv_init();
	standalone_main();
	wifi_load_nv_cfg();

	xTaskCreate(
		sys_task1,
		"OpenBeken",
		8 * 256,
		NULL,
		SYSTEM_CLI_TASK_PRIORITY,
		&g_sys_task_handle1);

	vTaskStartScheduler();
	return 0;
}

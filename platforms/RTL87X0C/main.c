#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"

extern void console_init(void);
void Main_Init();
void Main_OnEverySecond();

TaskHandle_t g_sys_task_handle1;

#if defined(ENABLE_AMAZON_COMMON) || (defined(CONFIG_MATTER) && CONFIG_MATTER)
static void *app_mbedtls_calloc_func(size_t nelements, size_t elementSize)
{
	size_t size;
	void *ptr = NULL;

	size = nelements * elementSize;
	ptr = pvPortMalloc(size);

	if (ptr) {
		memset(ptr, 0, size);
	}

	return ptr;
}

void app_mbedtls_init(void)
{
	mbedtls_platform_set_calloc_free(app_mbedtls_calloc_func, vPortFree);
}
#endif

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
	console_init();

	wlan_network();

	hal_misc_swd_pin_ctrl(0);

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
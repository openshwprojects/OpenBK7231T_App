#if PLATFORM_XRADIO

#include "../../new_common.h"

extern void Main_Init();
extern void Main_OnEverySecond();
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

void user_main(void)
{
	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		6,
		NULL);
}

#endif // PLATFORM_XR809

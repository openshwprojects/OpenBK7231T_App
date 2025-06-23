#if PLATFORM_XRADIO

#include "../../new_common.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_prcm.h"

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
#if !PLATFORM_XR809
	HAL_PRCM_SetWakeupDebClk0(0);
#endif
	for(int i = 0; i < WAKEUP_IO_MAX; i++)
	{
#if !PLATFORM_XR809
		HAL_PRCM_SetWakeupIOxDebSrc(i, 0);
		HAL_PRCM_SetWakeupIOxDebounce(i, 0);
#endif
		HAL_GPIO_DeInit(GPIO_PORT_A, WakeIo_To_Gpio(i));
		HAL_GPIO_DisableIRQ(GPIO_PORT_A, WakeIo_To_Gpio(i));
		HAL_Wakeup_ClrIO(i);
	}

	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		6,
		NULL);
}

#endif // PLATFORM_XR809

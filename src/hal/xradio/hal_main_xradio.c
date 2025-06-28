#if PLATFORM_XRADIO

#include "../../new_common.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_prcm.h"

#if PLATFORM_XR806

#include "debug/heap_trace.h"
#include "sys/sys_heap.h"

size_t sram_free_heap_size(void)
{
	uint8_t* start, * end, * current;
	heap_info_t sram_info;
	heap_get_space(&start, &end, &current);
	sram_heap_trace_info_get(&sram_info);
	return (end - start - sram_info.sum - sram_info.entry_cnt * 16);
}

#endif

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
	printf("%s\r\n", __func__);
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

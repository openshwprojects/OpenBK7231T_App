#if PLATFORM_ARMINO

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/flash_partition.h>

extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void user_main(void);

uint32_t g_ota_end_addr = 0;
uint32_t g_ota_start_addr = 0;

void user_app_main(void)
{
	//Main_Init();
	//for(;;)
	//{
	//	rtos_delay_milliseconds(1000);
	//	Main_OnEverySecond();
	//}
	bk_logic_partition_t* partition_info = bk_flash_partition_get_info(BK_PARTITION_OTA);
	BK_ASSERT(NULL != partition_info);
	g_ota_start_addr = partition_info->partition_start_addr;
	g_ota_end_addr = g_ota_start_addr + partition_info->partition_length;

	if(g_ota_start_addr == 0 || g_ota_end_addr == 0)
	{
		BK_ASSERT(0);
	}

	bk_gpio_driver_init();
	user_main();
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#endif
	bk_init();

	return 0;
}

#endif

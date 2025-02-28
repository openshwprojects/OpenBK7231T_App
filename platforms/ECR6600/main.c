#include "oshal.h"
#include "cli.h"
#include "flash.h"
#include "sdk_version.h"
#include "psm_system.h"

#define DFE_VAR
#include "tx_power_config.h"

extern void vTaskStartScheduler(void);
extern void wifi_main();
extern void rf_platform_int();
extern void ke_init(void);
extern void amt_cal_info_obtain();
extern void wifi_conn_main();
extern int version_get(void);

extern void Main_Init();
extern void Main_OnEverySecond();

TaskHandle_t g_sys_task_handle1;
extern uint8_t wmac[6];

int _close_r()
{
	return 0;
}

void sys_task1(void* pvParameters)
{
	while(!wifi_is_ready())
	{
		sys_delay_ms(20);
		printf("wifi not ready!\n");
	}

	Main_Init();
	while(true)
	{
		sys_delay_ms(1000);
		Main_OnEverySecond();
	}
}

#undef os_malloc
void* os_malloc(size_t size)
{
	return os_malloc_debug(size, (char*)0, 0);
}

int main(void)
{
	component_cli_init(E_UART_SELECT_BY_KCONFIG);
	printf("SDK version %s, Release version %s\n",
			  sdk_version, RELEASE_VERSION);

	rf_platform_int();

	int pinit = partion_init();
	if (pinit != 0)
	{
		printf("partition error %i\n", pinit);
	}
	if (easyflash_init() != 0)
	{
		printf("easyflash error\n");
	}

	ke_init();

	if (version_get() == 1)
	{
		drv_adc_init();
		get_volt_calibrate_data();
		time_check_temp();
	}

	amt_cal_info_obtain();
	wifi_main();
	psm_wifi_ble_init();
	psm_boot_flag_dbg_op(true, 1);

	// efuse mac is not burnt on wg236p
	//drv_efuse_read(0x18, (unsigned int*)wmac, 6);
	xTaskCreate(
		sys_task1,
		"OpenBeken",
		8 * 256,
		NULL,
		6,
		&g_sys_task_handle1);

	vTaskStartScheduler();
	return 0;
}

#if PLATFORM_ECR6600

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

E_DRV_UART_NUM uart_num = E_UART_NUM_2;
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
	return pvPortMalloc(size);
}

int main(void)
{
	component_cli_init(E_UART_NUM_2);
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
	//rf_pta_config();

	drv_adc_init();
	get_volt_calibrate_data();
	time_check_temp();

	// disabling this allowed connecting to ap with -91 rssi
	amt_cal_info_obtain();

	wifi_main();

	time_check_temp();
	os_task_create("time_check_temp", SYSTEM_EVENT_LOOP_PRIORITY, 4096, time_check_temp, NULL);

	// disabling this will crash when getting ip
	psm_wifi_ble_init();

	psm_boot_flag_dbg_op(true, 1);
	extern volatile int rtc_task_handle;
	extern void calculate_rtc_task();
	rtc_task_handle = os_task_create("calculate_rtc_task", SYSTEM_EVENT_LOOP_PRIORITY, 4096, calculate_rtc_task, NULL);
	if(rtc_task_handle)
	{
		os_printf(LM_OS, LL_INFO, "rtc calculate start!\r\n");
	}
	extern int health_mon_create_by_nv();
	health_mon_create_by_nv();
	//psm_wifi_ps_to_active();
	//psm_sleep_mode_ena_op(true, 0);
	//psm_set_psm_enable(0);
	//psm_pwr_mgt_ctrl(0);
	//psm_sleep_mode_ena_op(true, 0);
	//PSM_SLEEP_CLEAR(MODEM_SLEEP);
	//PSM_SLEEP_CLEAR(WFI_SLEEP);
	//psm_set_device_status(PSM_DEVICE_WIFI_STA, PSM_DEVICE_STATUS_ACTIVE);
	//psm_set_normal();
	//psm_set_sleep_mode(0, 0);
	//psm_boot_flag_dbg_op(true, 1);
	//AmtRfStart(1, "1");
	//int txgf = AmtRfGetTxGainFlag();
	//int txg = AmtRfGetTxGain();
	//printf("\r\nAmtRfGetTxGainFlag:%i AmtRfGetTxGain: %i\r\n", txgf, txg);
	// max txpower reported in console is 137 - 13.7dbm.
	//AmtRfSetApcIndex(1, "137");
	//uint8_t str[240];
	//amt_get_env_blob("txPower", &str, 240, NULL);
	//printf("txPower: %.*s\r\n", 240, str);
	//for(int i = 0; i < 240; i += 2)
	//{
	//	str[i] = 0xFF;
	//	str[i + 1] = 0x89;
	//}
	//amt_set_env_blob("txPower", str, 240);
	// efuse mac is not burnt on wg236p (or probably on anything, since writing it is unsupported in console)
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

#endif

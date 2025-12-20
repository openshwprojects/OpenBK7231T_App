#include "ameba_soc.h"
#include "main.h"
#include "wifi_constants.h"
#include "rom_map.h"
#include "rtl8721d_flash.h"
#include "device_lock.h"

extern void wlan_network(void);
void app_captouch_init(void);
void app_keyscan_init(u8 reset_status);

static void* app_mbedtls_calloc_func(size_t nelements, size_t elementSize)
{
	size_t size;
	void* ptr = NULL;

	size = nelements * elementSize;
	ptr = pvPortMalloc(size);

	if(ptr)
		_memset(ptr, 0, size);

	return ptr;
}

void app_mbedtls_rom_init(void)
{
	mbedtls_platform_set_calloc_free(app_mbedtls_calloc_func, vPortFree);
	//rom_ssl_ram_map.use_hw_crypto_func = 1;
	rtl_cryptoEngine_init();
}

VOID app_start_autoicg(VOID)
{
	u32 temp = 0;

	temp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA);
	temp |= BIT_HSYS_PLFM_AUTO_ICG_EN;
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA, temp);
}

VOID app_pmu_init(VOID)
{
	if(BKUP_Read(BKUP_REG0) & BIT_SW_SIM_RSVD)
	{
		return;
	}

	pmu_set_sleep_type(SLEEP_PG);

	/* if wake from deepsleep, that means we have released wakelock last time */
	//if (SOCPS_DsleepWakeStatusGet() == TRUE) {
	//	pmu_set_sysactive_time(2);
	//	pmu_release_wakelock(PMU_OS);
	//	pmu_tickless_debug(ENABLE);
	//}	
}

/* enable or disable BT shared memory */
/* if enable, KM4 can access it as SRAM */
/* if disable, just BT can access it */
/* 0x100E_0000	0x100E_FFFF	64K */
VOID app_shared_btmem(u32 NewStatus)
{
	u32 temp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA);

	if(NewStatus == ENABLE)
	{
		temp |= BIT_HSYS_SHARE_BT_MEM;
	}
	else
	{
		temp &= ~BIT_HSYS_SHARE_BT_MEM;
	}

	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA, temp);
}

static void app_dslp_wake(void)
{
	u32 aon_wake_event = SOCPS_AONWakeReason();

	DBG_8195A("hs app_dslp_wake %x \n", aon_wake_event);

	if(BIT_GPIO_WAKE_STS & aon_wake_event)
	{
		DBG_8195A("DSLP AonWakepin wakeup, wakepin %x\n", SOCPS_WakePinCheck());
	}

	if(BIT_AON_WAKE_TIM0_STS & aon_wake_event)
	{
		SOCPS_AONTimerCmd(DISABLE);
		DBG_8195A("DSLP Aontimer wakeup \n");
	}

	if(BIT_RTC_WAKE_STS & aon_wake_event)
	{
		DBG_8195A("DSLP RTC wakeup \n");
	}

	if(BIT_DLPS_TSF_WAKE_STS & aon_wake_event)
	{
		DBG_8195A("DSLP TSF wakeup \n");
	}

	if(BIT_KEYSCAN_WAKE_STS & aon_wake_event)
	{
		DBG_8195A("DSLP KS wakeup\n");
	}

	if(BIT_CAPTOUCH_WAKE_STS & aon_wake_event)
	{
		DBG_8195A("DSLP Touch wakeup\n");
	}

	SOCPS_AONWakeClear(BIT_ALL_WAKE_STS);
}

extern void Main_Init();
extern void Main_OnEverySecond();
extern uint32_t ota_get_cur_index(void);
extern void WiFI_GetMacAddress(char* mac);

TaskHandle_t g_sys_task_handle1;
uint8_t wmac[6] = { 0 };
rtw_mode_t wifi_mode = RTW_MODE_NONE;
uint32_t current_fw_idx;
uint8_t flash_size_8720 = 32;
extern int g_sleepfactor;

__attribute__((weak)) void _fini(void) {}

static void obk_task(void* pvParameters)
{
	WiFI_GetMacAddress(&wmac);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	Main_Init();
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS / g_sleepfactor);
		Main_OnEverySecond();
	}
	vTaskDelete(NULL);
}

//default main
int main(void)
{
	if(rtk_wifi_config.wifi_ultra_low_power &&
		rtk_wifi_config.wifi_app_ctrl_tdma == FALSE)
	{
		SystemSetCpuClk(CLK_KM4_100M);
	}
	InterruptRegister(IPC_INTHandler, IPC_IRQ, (u32)IPCM0_DEV, 5);
	InterruptEn(IPC_IRQ, 5);

#ifdef CONFIG_MBED_TLS_ENABLED
	app_mbedtls_rom_init();
#endif

	/* init console */
	shell_recv_all_data_onetime = 1;
	shell_init_rom(0, 0);
	shell_init_ram();
	ipc_table_init();

	/* Register Log Uart Callback function */
	//InterruptRegister((IRQ_FUN)shell_uart_irq_rom, UART_LOG_IRQ, (u32)NULL, 5);
	//InterruptEn(UART_LOG_IRQ, 5);

	if(TRUE == SOCPS_DsleepWakeStatusGet())
	{
		app_dslp_wake();
	}

	wlan_network();

	uint8_t* efuse = (uint8_t*)malloc(1024);
	device_mutex_lock(RT_DEV_LOCK_EFUSE);
	EFUSE_LMAP_READ(efuse);
	device_mutex_unlock(RT_DEV_LOCK_EFUSE);
	memcpy(wmac, efuse + 0x11A, 6);
	free(efuse);
	current_fw_idx = ota_get_cur_index();

	uint8_t flash_ID[4];
	FLASH_RxCmd(flash_init_para.FLASH_cmd_rd_id, 3, flash_ID);
	flash_size_8720 = (1 << (flash_ID[2] - 0x11)) / 8;
	app_start_autoicg();
	//app_shared_btmem(ENABLE);

	app_pmu_init();

	if((BKUP_Read(0) & BIT_KEY_ENABLE))
		app_keyscan_init(FALSE); /* 5uA */
	if((BKUP_Read(0) & BIT_CAPTOUCH_ENABLE))
		app_captouch_init(); /* 1uA */
	//if ((BKUP_Read(0) & BIT_GPIO_ENABLE))
	//	app_hp_jack_init(); 

	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		tskIDLE_PRIORITY + 6,
		NULL);

	//DBG_8195A("M4U:%d \n", RTIM_GetCount(TIMM05));
	/* Enable Schedule, Start Kernel */
	vTaskStartScheduler();
}

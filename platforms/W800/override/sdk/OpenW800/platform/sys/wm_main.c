/*****************************************************************************
*
* File Name : wm_main.c
*
* Description: wm main
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author :
*
* Date : 2014-6-14
*****************************************************************************/
#include <string.h>
#include "wm_irq.h"
#include "tls_sys.h"

#include "wm_regs.h"
#include "wm_type_def.h"
#include "wm_timer.h"
#include "wm_irq.h"
#include "wm_params.h"
#include "wm_hostspi.h"
#include "wm_flash.h"
#include "wm_fls_gd25qxx.h"
#include "wm_internal_flash.h"
#include "wm_efuse.h"
#include "wm_debug.h"
#include "wm_netif.h"
#include "wm_at_ri_init.h"
#include "wm_config.h"
#include "wm_osal.h"
#include "wm_http_client.h"
#include "wm_cpu.h"
#include "wm_webserver.h"
#include "wm_io.h"
#include "wm_mem.h"
#include "wm_wl_task.h"
#include "wm_wl_timers.h"
#ifdef TLS_CONFIG_HARD_CRYPTO
#include "wm_crypto_hard.h"
#endif
#include "wm_gpio_afsel.h"
#include "wm_pmu.h"
#include "wm_ram_config.h"
#include "wm_uart.h"
#include "wm_watchdog.h"
#include "wm_wifi.h"
#if TLS_CONFIG_ONLY_FACTORY_ATCMD
#include "../../src/app/factorycmd/factory_atcmd.h"
#endif

/* c librayr mutex */
tls_os_sem_t    *libc_sem;
/*----------------------------------------------------------------------------
 *      Standard Library multithreading interface
 *---------------------------------------------------------------------------*/

#ifndef __MICROLIB
/*--------------------------- _mutex_initialize -----------------------------*/

int _mutex_initialize (u32 *mutex)
{
    /* Allocate and initialize a system mutex. */
    //tls_os_sem_create(&libc_sem, 1);
    //mutex = (u32 *)libc_sem;
    return(1);
}


/*--------------------------- _mutex_acquire --------------------------------*/

void _mutex_acquire (u32 *mutex)
{
    //u8 err;
    /* Acquire a system mutex, lock stdlib resources. */
    tls_os_sem_acquire(libc_sem, 0);
}


/*--------------------------- _mutex_release --------------------------------*/

void _mutex_release (u32 *mutex)
{
    /* Release a system mutex, unlock stdlib resources. */
    tls_os_sem_release(libc_sem);
}

#endif

#define     TASK_START_STK_SIZE         768     /* Size of each task's stacks (# of WORDs)  */
/*If you want to delete main task after it works, you can open this MACRO below*/
#define MAIN_TASK_DELETE_AFTER_START_FTR  1

u8 *TaskStartStk = NULL;
tls_os_task_t tststarthdl = NULL;

#define FW_MAJOR_VER           0x1
#define FW_MINOR_VER           0x0
#define FW_PATCH_VER           0x10

const char FirmWareVer[4] =
{
    'v',
    FW_MAJOR_VER,  /* Main version */
    FW_MINOR_VER, /* Subversion */
    FW_PATCH_VER  /* Internal version */
};
const char HwVer[6] =
{
    'H',
    0x1,
    0x0,
    0x0,
    0x0,
    0x0
};
extern const char WiFiVer[];
extern u8 tx_gain_group[];
extern void *tls_wl_init(u8 *tx_gain, u8 *mac_addr, u8 *hwver);
extern int wpa_supplicant_init(u8 *mac_addr);
extern void tls_sys_auto_mode_run(void);
extern void UserMain(void);
extern void tls_bt_entry();
#if (TLS_CONFIG_HOSTIF&&TLS_CONFIG_UART)
extern int tls_uart_get_at_cmd_port(void);
extern void tls_uart_set_at_cmd_port(int at_cmd_port);
#endif
void tls_mem_get_init_available_size(void);
void task_start (void *data);

/****************/
/* main program */
/****************/

void vApplicationIdleHook( void )
{
    __WAIT();
    return;
}

void wm_gpio_config()
{
#if (TLS_CONFIG_HOSTIF&&TLS_CONFIG_UART)
	int at_port = tls_uart_get_at_cmd_port();
#endif	
    /* must call first */
    wm_gpio_af_disable();

    wm_uart0_tx_config(WM_IO_PB_19);
    wm_uart0_rx_config(WM_IO_PB_20);

	/*Please Attention, only one IO's multiplex can be used at one times' configuration. */

	/*AT command's port multiplex*/
#if (TLS_CONFIG_HOSTIF&&TLS_CONFIG_UART)
	switch(at_port)
	{
		case TLS_UART_1:
		    wm_uart1_rx_config(WM_IO_PB_07);
		    wm_uart1_tx_config(WM_IO_PB_06);
		break;
		case TLS_UART_2:
			wm_uart2_rx_config(WM_IO_PA_03);
			wm_uart2_tx_scio_config(WM_IO_PA_02);
		break;
		case TLS_UART_3:
		    wm_uart3_rx_config(WM_IO_PB_01);
		    wm_uart3_tx_config(WM_IO_PB_00);
		break;
		case TLS_UART_4:
			wm_uart4_rx_config(WM_IO_PA_09);
			wm_uart4_tx_config(WM_IO_PA_08);
		break;
		case TLS_UART_5:
		    wm_uart5_rx_config(WM_IO_PA_13);
		    wm_uart5_tx_config(WM_IO_PA_12);
		break;
		default:
		    wm_uart1_rx_config(WM_IO_PB_07);
		    wm_uart1_tx_config(WM_IO_PB_06);
		break;
	}
#endif

#if (TLS_CONFIG_LS_SPI)	
	wm_spi_cs_config(WM_IO_PB_04);
	wm_spi_ck_config(WM_IO_PB_02);
	wm_spi_di_config(WM_IO_PB_03);
	wm_spi_do_config(WM_IO_PB_05);
#endif
}
#if MAIN_TASK_DELETE_AFTER_START_FTR
void task_start_free()
{
	if (TaskStartStk)
	{
		tls_mem_free(TaskStartStk);
		TaskStartStk = NULL;
	}
}
#endif
int main(void)
{
    u32 value = 0;

	/*standby reason setting in here,because pmu irq will clear it.*/
	if ((tls_reg_read32(HR_PMU_INTERRUPT_SRC)>>7)&0x1)
	{
		tls_sys_set_reboot_reason(REBOOT_REASON_STANDBY);
	}
	
    /*32K switch to use RC circuit & calibration*/
    tls_pmu_clk_select(0);
#if (TLS_CONFIG_HOSTIF&&TLS_CONFIG_UART)
	/*Configure uart port for user's AT Command*/
	tls_uart_set_at_cmd_port(TLS_UART_1);
#endif
    /*Switch to DBG*/
    value = tls_reg_read32(HR_PMU_BK_REG);
    value &= ~(BIT(19));
    tls_reg_write32(HR_PMU_BK_REG, value);
    value = tls_reg_read32(HR_PMU_PS_CR);
    value &= ~(BIT(5));
    tls_reg_write32(HR_PMU_PS_CR, value);
	
    /*Close those not initialized clk except touchsensor/trng, uart0,sdadc,gpio,rfcfg*/
    value = tls_reg_read32(HR_CLK_BASE_ADDR);
    value &= ~0x3fffff;
    value |= 0x201a02;
    tls_reg_write32(HR_CLK_BASE_ADDR, value);


    tls_sys_clk_set(CPU_CLK_80M);
    tls_os_init(NULL);

    /* before use malloc() function, must create mutex used by c_lib */
    tls_os_sem_create(&libc_sem, 1);

    /*configure wake up source begin*/
	csi_vic_set_wakeup_irq(SDIO_IRQn);
    csi_vic_set_wakeup_irq(MAC_IRQn);
    csi_vic_set_wakeup_irq(SEC_IRQn);
    csi_vic_set_wakeup_irq(DMA_Channel0_IRQn);
    csi_vic_set_wakeup_irq(DMA_Channel1_IRQn);
    csi_vic_set_wakeup_irq(DMA_Channel2_IRQn);
    csi_vic_set_wakeup_irq(DMA_Channel3_IRQn);
    csi_vic_set_wakeup_irq(DMA_Channel4_7_IRQn);
    csi_vic_set_wakeup_irq(DMA_BRUST_IRQn);
    csi_vic_set_wakeup_irq(I2C_IRQn);
    csi_vic_set_wakeup_irq(ADC_IRQn);
    csi_vic_set_wakeup_irq(SPI_LS_IRQn);
	csi_vic_set_wakeup_irq(SPI_HS_IRQn);
    csi_vic_set_wakeup_irq(GPIOA_IRQn);
    csi_vic_set_wakeup_irq(GPIOB_IRQn);
    csi_vic_set_wakeup_irq(UART0_IRQn);
    csi_vic_set_wakeup_irq(UART1_IRQn);
    csi_vic_set_wakeup_irq(UART24_IRQn);
    csi_vic_set_wakeup_irq(BLE_IRQn);
    csi_vic_set_wakeup_irq(BT_IRQn);
    csi_vic_set_wakeup_irq(PWM_IRQn);
    csi_vic_set_wakeup_irq(I2S_IRQn);
	csi_vic_set_wakeup_irq(SIDO_HOST_IRQn);
    csi_vic_set_wakeup_irq(SYS_TICK_IRQn);
    csi_vic_set_wakeup_irq(RSA_IRQn);
    csi_vic_set_wakeup_irq(CRYPTION_IRQn);
    csi_vic_set_wakeup_irq(PMU_IRQn);
    csi_vic_set_wakeup_irq(TIMER_IRQn);
    csi_vic_set_wakeup_irq(WDG_IRQn);
	/*should be here because main stack will be allocated and deallocated after task delete*/
	tls_mem_get_init_available_size();
	
    /*configure wake up source end*/
	TaskStartStk = tls_mem_alloc(sizeof(u32)*TASK_START_STK_SIZE);
	if (TaskStartStk)
    {
        tls_os_task_create(&tststarthdl, NULL,
                           task_start,
                           (void *)0,
                           (void *)TaskStartStk,          /* 任务栈的起始地址 */
                           TASK_START_STK_SIZE * sizeof(u32), /* 任务栈的大小     */
                           1,
                           0);
	   tls_os_start_scheduler();
    }
	else
	{
		while(1);
	}	

    return 0;
}

unsigned int tls_get_wifi_ver(void)
{
	return (WiFiVer[0]<<16)|(WiFiVer[1]<<8)|WiFiVer[2];
}

void disp_version_info(void)
{
    TLS_DBGPRT_INFO("\n\n");
    TLS_DBGPRT_INFO("****************************************************************\n");
    TLS_DBGPRT_INFO("*                                                              *\n");
    TLS_DBGPRT_INFO("* Copyright (C) 2014 WinnerMicro Co. Ltd.                      *\n");
    TLS_DBGPRT_INFO("* All rights reserved.                                         *\n");
    TLS_DBGPRT_INFO("* WinnerMicro Firmware Version: %x.%x.%X                         *\n",
                    FirmWareVer[1], FirmWareVer[2], FirmWareVer[3]);
    TLS_DBGPRT_INFO("* WinnerMicro Hardware Version: %x.%x.%x.%x.%x                      *\n",
                    HwVer[1], HwVer[2], HwVer[3], HwVer[4], HwVer[5]);
    TLS_DBGPRT_INFO("*                                                              *\n");
    TLS_DBGPRT_INFO("* WinnerMicro Wi-Fi Lib Version: %x.%x.%x                         *\n",
                    WiFiVer[0], WiFiVer[1], WiFiVer[2]);
    TLS_DBGPRT_INFO("****************************************************************\n");
}

unsigned int total_mem_size;
void tls_mem_get_init_available_size(void)
{
	u8 *p = NULL;
	total_mem_size = (unsigned int)&__heap_end - (unsigned int)&__heap_start;
	while(total_mem_size > 512)
	{
		p = malloc(total_mem_size);
		if (p)
		{
			free(p);
			p = NULL;
			break;
		}
		total_mem_size = total_mem_size - 512;
	}
}

void tls_pmu_chipsleep_callback(int sleeptime)
{
	//wm_printf("c:%d\r\n", sleeptime);
	/*set wakeup time*/
	tls_pmu_timer1_start(sleeptime);
	/*enter chip sleep*/
	tls_pmu_sleep_start();
}


/*****************************************************************************
 * Function Name        // task_start
 * Descriptor             // before create multi_task, we create a task_start task
 *                      	   // in this example, this task display the cpu usage
 * Input
 * Output
 * Return
 ****************************************************************************/
void task_start (void *data)
{
	u8 enable = 0;
    u8 mac_addr[6] = {0x00, 0x25, 0x08, 0x09, 0x01, 0x0F};

#if TLS_CONFIG_CRYSTAL_24M
    tls_wl_hw_using_24m_crystal();
#endif

    /* must call first to configure gpio Alternate functions according the hardware design */
    wm_gpio_config();

    tls_irq_init();

#if TLS_CONFIG_HARD_CRYPTO
    tls_crypto_init();
#endif

#if (TLS_CONFIG_LS_SPI)
    tls_spi_init();
    tls_spifls_init();
#endif

    tls_fls_init();
    tls_fls_sys_param_postion_init();

    /*PARAM GAIN,MAC default*/
    tls_ft_param_init();
    tls_param_load_factory_default();
    tls_param_init(); /*add param to init sysparam_lock sem*/

    tls_get_tx_gain(&tx_gain_group[0]);
    TLS_DBGPRT_INFO("tx gain ");
    TLS_DBGPRT_DUMP((char *)(&tx_gain_group[0]), 27);
    if (tls_wifi_mem_cfg(WIFI_MEM_START_ADDR, 7, 3)) /*wifi tx&rx mem customized interface*/
    {
        TLS_DBGPRT_INFO("wl mem initial failured\n");
    }

    tls_get_mac_addr(&mac_addr[0]);
    TLS_DBGPRT_INFO("mac addr ");
    TLS_DBGPRT_DUMP((char *)(&mac_addr[0]), 6);
    if(tls_wl_init(NULL, &mac_addr[0], NULL) == NULL)
    {
        TLS_DBGPRT_INFO("wl driver initial failured\n");
    }
    if (wpa_supplicant_init(mac_addr))
    {
        TLS_DBGPRT_INFO("supplicant initial failured\n");
    }
	/*wifi-temperature compensation,default:open*/
	tls_wifi_set_tempcomp_flag(0);
	tls_wifi_set_psm_chipsleep_flag(0);
	tls_wifi_psm_chipsleep_cb_register((tls_wifi_psm_chipsleep_callback)tls_pmu_chipsleep_callback, NULL, NULL);
    tls_ethernet_init();

#if TLS_CONFIG_BT
    tls_bt_entry();
#endif

    tls_sys_init();
#if TLS_CONFIG_ONLY_FACTORY_ATCMD
	factory_atcmd_init();
#else
    /*HOSTIF&UART*/
#if TLS_CONFIG_HOSTIF
    tls_hostif_init();

#if (TLS_CONFIG_HS_SPI)
    tls_hspi_init();
#endif

#if TLS_CONFIG_UART
    tls_uart_init();
#endif
#endif

	tls_param_get(TLS_PARAM_ID_PSM, &enable, TRUE);	
	if (enable != TRUE)
	{
	    enable = TRUE;
	    tls_param_set(TLS_PARAM_ID_PSM, &enable, TRUE);	  
	}

    UserMain();

    tls_sys_auto_mode_run();
#endif

    for (;;)
    {
#if MAIN_TASK_DELETE_AFTER_START_FTR
		if (tststarthdl)
		{
    		tls_os_task_del_by_task_handle(tststarthdl,task_start_free);
		}
        tls_os_time_delay(0x10000000);
#else
        //printf("start up\n");
        extern void tls_os_disp_task_stat_info(void);
        tls_os_disp_task_stat_info();
        tls_os_time_delay(1000);
#endif
    }
}


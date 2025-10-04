#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"

#if PLATFORM_TXW81X

#include "sys_config.h"
#include "typesdef.h"
#include "osal/work.h"
#include "lib/skb/skbpool.h"
#include "lib/syscfg/syscfg.h"
#include "syscfg.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "project_config.h"

extern struct vpp_device* vpp_test;
bool isStarted = false;
bool showTimestamp = false;
extern time_t g_ntpTime;
extern void set_time_watermark(uint16 year, uint16 month, uint16 day, uint16 hour, uint16 min, uint16 sec);
int dvp_scl = PC_2;
int dvp_sda = PC_3;

uint8 vcam_en()
{
	uint8 ret = TRUE;
#if VCAM_EN
	pmu_vcam_dis();
	os_sleep_ms(1);
	pmu_set_vcam_vol(VCAM_VOL_2V80);
	pmu_vcam_lc_en();
	pmu_vcam_oc_detect_dis();
	pmu_vcam_oc_int_dis();
	pmu_vcam_discharge_dis();
	pmu_vcam_pg_dis();
#ifdef VCAM_33
	pmu_set_vcam_vol(VCAM_VOL_3V25);
	pmu_vcam_en();
	os_sleep_ms(1);
	pmu_vcam_pg_en();
#else
	pmu_vcam_en();
	os_sleep_ms(1);
#endif    
	//    sys_reset_pending_clr();
	pmu_vcam_lc_dis();
#ifndef VCAM_33
	pmu_vcam_oc_detect_en();
#endif
	if(PMU_VCAM_OC_PENDING)
	{
		return FALSE;
	}
	pmu_vcam_oc_detect_dis();
	pmu_vcam_oc_int_dis();
	pmu_lvd_oe_en();
#endif
	return ret;
}

void CMD_ShowTimestamp(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	showTimestamp = Tokenizer_GetArgInteger(0) > 0 ? 1 : 0;
	vpp_set_watermark0_enable(vpp_test, showTimestamp);
}

void TXW_Cam_Init(void)
{
	if(!isStarted)
	{
		dvp_scl = PIN_FindPinIndexForRole(IOR_SOFT_SCL, dvp_scl);
		dvp_sda = PIN_FindPinIndexForRole(IOR_SOFT_SDA, dvp_sda);
		uint32_t buf_size = Tokenizer_GetArgIntegerDefault(1, 60 * 1000);
		uint8 vcam;
		vcam = vcam_en();
		void* custom_buf = (void*)os_malloc(buf_size);
		custom_mem_init(custom_buf, buf_size);
		print_custom_sram();
		stream_work_queue_start();
		jpg_cfg(HG_JPG0_DEVID, VPP_DATA0);
		bool csi_ret;
		bool csi_cfg();
		csi_ret = csi_cfg();
		bool csi_open();
		if(csi_ret)
			csi_open();
		audio_adc_init();
		spook_init();
	}
	isStarted = true;

	//cmddetail:{"name":"CAM_Show_Timestamp","args":"CMD_ShowTimestamp",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_ShowTimestamp","file":"driver/drv_txw81x_camera.c","requires":"NTP",
	//cmddetail:"examples":"CAM_Show_Timestamp 1"}
	CMD_RegisterCommand("CAM_Show_Timestamp", CMD_ShowTimestamp, NULL);
}

void TXW_Cam_RunEverySecond(void)
{
	if(showTimestamp)
	{
		struct tm* ltm = gmtime(&g_ntpTime);
		set_time_watermark(ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
	}
}

#endif
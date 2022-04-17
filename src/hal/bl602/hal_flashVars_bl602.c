#ifdef PLATFORM_BL602

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"

#include <easyflash.h>


void BL602_InitEasyFlashIfNeeded();

#define EASYFLASH_MY_BOOTCOUNTS     "myBtCnts"

typedef struct bl602_bootCounts_s {
    unsigned short boot_count; // number of times the device has booted
    unsigned short boot_success_count; // if a device boots completely (>30s), will equal boot_success_count
} bl602_bootCounts_t;

bl602_bootCounts_t g_bootCounts;


static int BL602_ReadFlashVars(void *target, int dataLen){
	int readLen;

	BL602_InitEasyFlashIfNeeded();

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_ReadFlashVars: will read %d bytes", dataLen);
	readLen = ef_get_env_blob(EASYFLASH_MY_BOOTCOUNTS, target, dataLen , NULL);
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_ReadFlashVars: really loaded %d bytes", readLen);

    return dataLen;
}



static int BL602_SaveFlashVars(void *src, int dataLen){

	EfErrCode  res;

	BL602_InitEasyFlashIfNeeded();

	res = ef_set_env_blob(EASYFLASH_MY_BOOTCOUNTS, src, dataLen);
	if(res == EF_ENV_INIT_FAILED) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_SaveFlashVars: EF_ENV_INIT_FAILED for %d bytes", dataLen);
		return 0;
	}
	if(res == EF_ENV_ARG_ERR) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_SaveFlashVars: EF_ENV_ARG_ERR for %d bytes", dataLen);
		return 0;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_SaveFlashVars: saved %d bytes", dataLen);
    return dataLen;
}


void HAL_FlashVars_SaveBootComplete(){
	g_bootCounts.boot_success_count = g_bootCounts.boot_count;
	// save after set
	BL602_SaveFlashVars(&g_bootCounts,sizeof(g_bootCounts));
}

int HAL_FlashVars_GetBootCount(){
	return g_bootCounts.boot_count;
}
int HAL_FlashVars_GetBootFailures(){
    int diff = 0;
    diff = g_bootCounts.boot_count - g_bootCounts.boot_success_count;
    return diff;
}
void HAL_FlashVars_IncreaseBootCount(){
	// defaults - in case read fails
	g_bootCounts.boot_count = 0;
	g_bootCounts.boot_success_count = 0;
	// read saved
	BL602_ReadFlashVars(&g_bootCounts,sizeof(g_bootCounts));
	g_bootCounts.boot_count++;
	// save after increase
	BL602_SaveFlashVars(&g_bootCounts,sizeof(g_bootCounts));
}



#endif // PLATFORM_XR809



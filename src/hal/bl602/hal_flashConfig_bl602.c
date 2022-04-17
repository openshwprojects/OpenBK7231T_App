#ifdef PLATFORM_BL602

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"

#include <easyflash.h>

static int g_easyFlash_Ready = 0;

void BL602_InitEasyFlashIfNeeded(){
	if(g_easyFlash_Ready==0){
		easyflash_init();
		g_easyFlash_Ready = 1;
	}
}

int WiFI_SetMacAddress(char *mac) {
	return 0;

}
#define EASYFLASH_MY_OBK_CONF     "mY0bcFg"

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){
	int readLen;

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_ReadConfigMemory: will read %d bytes", dataLen);
	readLen = ef_get_env_blob(EASYFLASH_MY_OBK_CONF, target, dataLen , NULL);
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_ReadConfigMemory: really loaded %d bytes", readLen);

    return dataLen;
}


void HAL_Configuration_GenerateMACForThisModule(unsigned char *out) {
}



int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){

	EfErrCode  res;

	res = ef_set_env_blob(EASYFLASH_MY_OBK_CONF, src, dataLen);
	if(res == EF_ENV_INIT_FAILED) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory: EF_ENV_INIT_FAILED for %d bytes", dataLen);
		return 0;
	}
	if(res == EF_ENV_ARG_ERR) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory: EF_ENV_ARG_ERR for %d bytes", dataLen);
		return 0;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory: saved %d bytes", dataLen);
    return dataLen;
}




#endif // PLATFORM_XR809



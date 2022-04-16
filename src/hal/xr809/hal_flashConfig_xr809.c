#ifdef PLATFORM_XR809

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"

#include "image/fdcm.h"
#include "image/image.h"
#include "driver/chip/hal_crypto.h"
#include "efpg/efpg.h"


#define OUR_CUSTOM_SYSINFO_ADR PRJCONF_SYSINFO_ADDR
#define OUR_CUSTOM_SYSINFO_SIZE 4096

static fdcm_handle_t *g_fdcm_hdl = 0;

static int XR809_InitSysInfo() {
	uint32_t image_size = image_get_size();
	if (image_size == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "XR809_InitSysInfo: get image size failed\n");
		return -1;
	}
	if (image_size > OUR_CUSTOM_SYSINFO_ADR) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "XR809_InitSysInfo: image is too big: %#x, please make it smaller than %#x\n",
		            image_size, OUR_CUSTOM_SYSINFO_ADR);
		return -1;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "XR809_InitSysInfo: image size %i, our adr %i\n",image_size,OUR_CUSTOM_SYSINFO_ADR);
	g_fdcm_hdl = fdcm_open(PRJCONF_SYSINFO_FLASH, OUR_CUSTOM_SYSINFO_ADR, OUR_CUSTOM_SYSINFO_SIZE);
	if (g_fdcm_hdl == NULL) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "fdcm open failed, hdl %p\n", g_fdcm_hdl);
		return -1;
	}
	return 0;
}

int WiFI_SetMacAddress(char *mac) {
	return 0;

}
int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){

	if(g_fdcm_hdl == 0) {
		XR809_InitSysInfo();
	}

	if (fdcm_read(g_fdcm_hdl, target, dataLen) != dataLen) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_ReadConfigMemory: fdcm read failed\n");
		return 0;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_ReadConfigMemory: read %d bytes", dataLen);

    return dataLen;
}


static void sysinfo_gen_mac_by_chipid(uint8_t mac_addr[6])
{
	int i;
	uint8_t chipid[16];

	efpg_read(EFPG_FIELD_CHIPID, chipid);

	for (i = 0; i < 2; ++i) {
		mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 12];
	}
	for (i = 2; i < 6; ++i) {
		mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 10];
	}
	mac_addr[0] &= 0xFC;
}
static void sysinfo_gen_mac_random(uint8_t mac_addr[6])
{
	int i;
	for (i = 0; i < 6; ++i) {
		mac_addr[i] = (uint8_t)OS_Rand32();
	}
	mac_addr[0] &= 0xFC;
}
void HAL_Configuration_GenerateMACForThisModule(unsigned char *out) {
	int i;
	if (efpg_read(EFPG_FIELD_MAC, out) == 0) {
		return;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "read mac addr from eFuse fail\n");
	sysinfo_gen_mac_by_chipid(out);
	for (i = 0; i < 6; ++i) {
		if (out[i] != 0)
			return;
	}
	sysinfo_gen_mac_random(out);
}



int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){
	if(g_fdcm_hdl == 0) {
		XR809_InitSysInfo();
	}

	if (fdcm_write(g_fdcm_hdl, src, dataLen) != dataLen) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory: fdcm write failed\n");
		return 0;
	}
	
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory: saved %d bytes", dataLen);
    return dataLen;
}




#endif // PLATFORM_XR809



#if PLATFORM_XRADIO

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"

#include "image/fdcm.h"
#include "image/image.h"
#include "driver/chip/hal_crypto.h"
#include "efpg/efpg.h"
#include <easyflash.h>

static int g_easyFlash_Ready = 0;
void InitEasyFlashIfNeeded()
{
	if(g_easyFlash_Ready == 0)
	{
		easyflash_init();
		g_easyFlash_Ready = 1;
	}
}

int HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	InitEasyFlashIfNeeded();
	return ef_get_env_blob("ObkCfg", target, dataLen, NULL);
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	InitEasyFlashIfNeeded();
	ef_set_env_blob("ObkCfg", src, dataLen);
	return 1;
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
#if PLATFORM_XR806
#define EFPG_FIELD_MAC EFPG_FIELD_MAC_WLAN
#endif
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

#endif // PLATFORM_XR809



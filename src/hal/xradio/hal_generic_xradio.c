#if PLATFORM_XRADIO

#include "arch/cc.h"
#include "image/fdcm.h"
#include "image/image.h"
#include "efpg/efpg.h"
#include "driver/chip/hal_crypto.h"
#include "driver/chip/hal_util.h"
#include "driver/chip/hal_wdg.h"
#include "driver/chip/hal_clock.h"
#include "../../logging/logging.h"

void HAL_RebootModule() 
{
	HAL_WDG_Reboot();
}

void HAL_Delay_us(int delay)
{
	uint32_t i;
	uint32_t loop = HAL_GetCPUClock() / 1000000 * delay / 6;

	for(i = 0; i < loop; ++i) __asm("nop");
}

void HAL_Configure_WDT()
{
	WDG_InitParam param;
#if PLATFORM_XR809
	param.event = WDG_EVT_RESET;
	param.timeout = WDG_TIMEOUT_16SEC;
	param.resetCycle = WDG_DEFAULT_RESET_CYCLE;
#else
	param.hw.event = WDG_EVT_RESET;
	param.hw.resetCpuMode = WDG_RESET_CPU_CORE;
	param.hw.timeout = WDG_TIMEOUT_16SEC;
	param.hw.resetCycle = WDG_DEFAULT_RESET_CYCLE;
#endif
	HAL_WDG_Init(&param);
	HAL_WDG_Start();
}

void HAL_Run_WDT()
{
	HAL_WDG_Feed();
}

static void sysinfo_gen_mac_by_chipid(uint8_t mac_addr[6])
{
	int i;
	uint8_t chipid[16];

	efpg_read(EFPG_FIELD_CHIPID, chipid);

	for(i = 0; i < 2; ++i)
	{
		mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 12];
	}
	for(i = 2; i < 6; ++i)
	{
		mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 10];
	}
	mac_addr[0] &= 0xFC;
}
static void sysinfo_gen_mac_random(uint8_t mac_addr[6])
{
	int i;
	for(i = 0; i < 6; ++i)
	{
		mac_addr[i] = (uint8_t)OS_Rand32();
	}
	mac_addr[0] &= 0xFC;
}
void HAL_Configuration_GenerateMACForThisModule(unsigned char* out)
{
#if PLATFORM_XR806
#define EFPG_FIELD_MAC EFPG_FIELD_MAC_WLAN
#endif
	int i;
	if(efpg_read(EFPG_FIELD_MAC, out) == 0)
	{
		return;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "read mac addr from eFuse fail\n");
	sysinfo_gen_mac_by_chipid(out);
	for(i = 0; i < 6; ++i)
	{
		if(out[i] != 0)
			return;
	}
	sysinfo_gen_mac_random(out);
}

#endif // PLATFORM_XR809
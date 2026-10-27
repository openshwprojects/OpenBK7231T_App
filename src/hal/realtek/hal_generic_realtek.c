#ifdef PLATFORM_REALTEK

#include "../hal_generic.h"
#include "sys_api.h"
#include "wdt_api.h"

extern void ota_platform_reset(void);

#if PLATFORM_RTL8710B || PLATFORM_RTL8720D
extern int g_sleepfactor;
#define haldelay delay * g_sleepfactor
#else
#define haldelay delay
#endif

void HAL_RebootModule()
{
#if !PLATFORM_REALTEK_NEW
	ota_platform_reset();
#else
	sys_reset();
#endif
}

void HAL_Delay_us(int delay)
{
#if PLATFORM_RTL87X0C
	hal_delay_us(delay);
#elif PLATFORM_RTL8710A
	HalDelayUs(delay);
#else
	DelayUs(haldelay);
#endif
}

void HAL_Configure_WDT()
{
	watchdog_init(10 * 1000);
	watchdog_start();
}

void HAL_Run_WDT()
{
	watchdog_refresh();
}

#if PLATFORM_REALTEK_NEW

#include "../../cmnds/cmd_public.h"

extern int g_urx_pin;
extern int g_utx_pin;
extern int g_rts_pin;
extern int g_cts_pin;

static commandResult_t CMD_SetUARTPins(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);

	int rxpin = Tokenizer_GetPin(0, -1);
	int txpin = Tokenizer_GetPin(1, -1);
	int rtspin = Tokenizer_GetPin(2, -1);
	int ctspin = Tokenizer_GetPin(3, -1);
	if(rxpin == -1 || txpin == -1)
		return CMD_RES_BAD_ARGUMENT;
	g_urx_pin = rxpin;
	g_utx_pin = txpin;
	g_rts_pin = rtspin;
	g_cts_pin = ctspin;
	return CMD_RES_OK;
}

void HAL_RegisterPlatformSpecificCommands()
{
	//cmddetail:{"name":"SetUARTPins","args":"[rx pin] [tx pin] [rts pin] [cts pin]",
	//cmddetail:"descr":"Set UART pins",
	//cmddetail:"fn":"CMD_SetUARTPins","file":"hal/realtek/hal_generic_realtek.c","requires":"PLATFORM_REALTEK_NEW",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SetUARTPins", CMD_SetUARTPins, NULL);
}

#endif

#endif // PLATFORM_REALTEK

#if PLATFORM_RDA5981

#include "rda_sys_wrapper.h"
#include "console.h"
#include "rda59xx_daemon.h"
#include "wland_flash.h"
#include "WiFiStackInterface.h"

extern "C" {
	extern void Main_Init();
	extern void Main_OnEverySecond();
	extern uint32_t OTA_Offset;
	uint8_t macaddr[6];
}

extern WiFiStackInterface wifi;

__attribute__((used)) int main()
{
	rda5981_set_user_data_addr(0x180FF000, 0x180FE000, 0x1000);
	wifi.init();
	rda5981_flash_read_mac_addr((unsigned char*)&macaddr);
	Main_Init();
	while(true)
	{
		osDelay(1000);
		Main_OnEverySecond();
	}
}

#endif

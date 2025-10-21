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
	char* temp_buf = (char*)malloc(256);
	rda5981_read_flash(OTA_Offset, temp_buf, 256);
	if(temp_buf[0] == 0xAE && temp_buf[1] == 0xAE) rda5981_erase_flash(OTA_Offset, 1024);
	free(temp_buf);
	while(true)
	{
		osDelay(1000);
		Main_OnEverySecond();
	}
}

#endif

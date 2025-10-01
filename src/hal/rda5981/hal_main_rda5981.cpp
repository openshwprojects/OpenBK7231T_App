#if PLATFORM_RDA5981

#include "rda_sys_wrapper.h"
#include "console.h"
#include "rda59xx_daemon.h"
#include "wland_flash.h"
#include "WiFiStackInterface.h"

extern "C" {
	extern void Main_Init();
	extern void Main_OnEverySecond();
}

extern WiFiStackInterface wifi;

__attribute__((used)) int main()
{
	//rda5981_set_user_data_addr(0x180FB000, 0x180FC000, 3584);
	wifi.init();
	Main_Init();
	while(true)
	{
		osDelay(1000);
		Main_OnEverySecond();
	}
}

#endif

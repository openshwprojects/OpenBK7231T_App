#ifdef PLATFORM_MT7687

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../quicktick.h"
#include <stdio.h>
#include "hal.h"
#include "top.h"
#include "mt7687.h"
#include "system_mt7687.h"
#include "wifi_api.h"
#include "lwip/netif.h"
#include "connsys_profile.h"
#include "ethernetif.h"
#include "dhcpd.h"
#include "wifi_api.h"
#include "wifi_scan.h"
extern const char _ctype_[];
const char* __ctype_ptr__ = _ctype_;
uint8_t g_mac[6];
extern void Main_Init();
extern void Main_OnEverySecond();
static void obk_task(void* pvParameters)
{
	//vTaskDelay(50 / portTICK_PERIOD_MS);
	Main_Init();
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		Main_OnEverySecond();
	}
	vTaskDelete(NULL);
}

int main(void)
{
	system_init();
	//log_init(NULL, NULL, NULL);

	wifi_config_t config = { 0 };
	wifi_config_ext_t ext = { 0 };
	ext.sta_auto_connect_present = 1;
	ext.sta_auto_connect = 0;
	config.opmode = WIFI_MODE_STA_ONLY;
	wifi_init(&config, &ext);
	lwip_tcpip_config_t tcpip_config =
	{
		.ip_mode = STA_IP_MODE_DHCP,
		.sta_addr = {0},
		.sta_mask = {0},
		.sta_gateway = {0},
		.ap_addr = {0},
		.ap_mask = {0},
		.ap_gateway = {0},
	};
	lwip_tcpip_init(&tcpip_config, WIFI_MODE_STA_ONLY);

	wifi_config_get_mac_address(WIFI_PORT_STA, g_mac);

	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		14,
		NULL);

	SysInitStatus_Set();

	vTaskStartScheduler();
	while(1);
}

#endif // PLATFORM_MT7687
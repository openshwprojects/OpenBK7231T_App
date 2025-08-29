#if PLATFORM_TXW81X

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../quicktick.h"
#include "sys_config.h"
#include "typesdef.h"
#include "osal/work.h"
#include "lib/skb/skbpool.h"
#include "lib/syscfg/syscfg.h"
#include "syscfg.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "project_config.h"

static struct os_work main_wk;
extern void TXW_TakeEvents();
uint8_t qc_mode = 0;
struct spi_nor_flash* obk_flash = NULL;

static int32 main_loop(struct os_work* work)
{
	os_run_work_delay(&main_wk, 1000);
	Main_OnEverySecond();
	return 0;
}

int main(void)
{
	uint32 sysheap_freesize(struct sys_heap* heap);
	os_printf("freemem:%d\r\n", sysheap_freesize(&sram_heap));
	void* skb_pool_addr = (void*)os_malloc(10 * 1024);
	if(!skb_pool_addr)
	{
		while(1)
		{
			os_printf("skb malloc fail,malloc size:%d\tremain size:%d\n", 10 * 1024, sysheap_freesize(&sram_heap));
			os_sleep_ms(1000);
		}
	}
	skbpool_init((uint32_t)skb_pool_addr, 10 * 1024, 80, 0);
	syscfg_set_default_val();

	ip_addr_t ipaddr, netmask, gw;
	struct netdev* ndev;
	int offset = sys_cfgs.wifi_mode == WIFI_MODE_STA ? WIFI_MODE_STA : WIFI_MODE_AP;
	tcpip_init(NULL, NULL);

	struct syscfg_info info;
	os_memset(&info, 0, sizeof(info));
	syscfg_info_get(&info);
	obk_flash = info.flash1;
	spi_nor_open(obk_flash);

	TXW_TakeEvents();
	eloop_init();
	os_task_create("eloop_run", user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
	Main_Init();
	OS_WORK_INIT(&main_wk, main_loop, 0);
	os_run_work_delay(&main_wk, 1000);
}

#endif

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bl_fw_api.h"
#include "fhost_api.h"
#include "wifi_mgmr_ext.h"
#include "wifi_mgmr.h"
#include "bflb_adc.h"
#include "bflb_efuse.h"
#include "bflb_mtd.h"
#include "rfparam_adapter.h"
#include "mm.h"

extern void Main_Init();
extern void Main_OnEverySecond();
struct bflb_device_s* adc;
float g_wifi_temperature = 0.0f;
void* __dso_handle = NULL;

void stats_display(void) {}

#include <lwip/tcpip.h>
void* _os_malloc(size_t size)
{
	return kmalloc(size, MM_FLAG_ALIGN_32BYTE);
}

void _os_free(void* ptr)
{
	kfree(ptr);
}

void __wrap_bflb_gpio_pad_check(uint8_t pin) {}

static void obk_task(void* pvParameters)
{
	Main_Init();

	while(1)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		Main_OnEverySecond();
		g_wifi_temperature = bflb_adc_tsen_get_temp(adc);
	}
	vTaskDelete(NULL);
}

int main(void)
{
	board_init();
	bflb_mtd_init();

	if(0 != rfparam_init(0, NULL, 0))
	{
		printf("PHY RF init failed!\r\n");
		return 0;
	}
	tcpip_init(NULL, NULL);

	board_adc_gpio_init();

	adc = bflb_device_get_by_name("adc");

	/* adc clock = XCLK / 2 / 32 */
	struct bflb_adc_config_s cfg;
	cfg.clk_div = ADC_CLK_DIV_32;
	cfg.scan_conv_mode = false;
	cfg.continuous_conv_mode = true;
	cfg.differential_mode = false;
	cfg.resolution = ADC_RESOLUTION_16B;
	cfg.vref = ADC_VREF_2P0V;

	struct bflb_adc_channel_s chan;

	chan.pos_chan = ADC_CHANNEL_TSEN_P;
	chan.neg_chan = ADC_CHANNEL_GND;

	bflb_adc_init(adc, &cfg);
	bflb_adc_channel_config(adc, &chan, 1);
	bflb_adc_tsen_init(adc, ADC_TSEN_MOD_INTERNAL_DIODE);

	xTaskCreate(
		obk_task,
		"OpenBeken",
		8 * 256,
		NULL,
		6,
		NULL);

	vTaskStartScheduler();
	while(1) { }
}
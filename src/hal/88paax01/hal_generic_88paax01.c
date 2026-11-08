#include "../../new_common.h"
#include "../hal_generic.h"
#include "hw.h"
#include "layout.h"
#include "image.h"
#include "spi_flash.h"
#include "cache.h"
#include "crc.h"
#include "timer.h"
#include "uart.h"
#include "usbcon.h"
#include "common_cmds.h"
#include "net.h"

extern void jump_to(uint32_t entry, uint32_t arg) __attribute__((noreturn));

void HAL_RebootModule(void)
{
	printf("[88PAAX01] Rebooting module...\n");
	uart_flush();
	usbcon_stop();
	timer_delay_ms(50);

	/* Reload the bootloader from flash and let it boot the highest-sequence slot */
	const image_header_t *hdr = (const image_header_t *)flash_mmap(FLASH_BL_OFFSET);
	dcache_invalidate_range(FLASH_BL_OFFSET, FLASH_BL_OFFSET + FLASH_BL_SIZE);
	if (image_validate(hdr, IMAGE_TYPE_BL, BL_LOAD_ADDR, BL_CODE_MAX, 1) == IMG_OK) {
		uint32_t dst = RW_ALIAS(hdr->load_address);
		memcpy((void *)dst, flash_mmap(FLASH_BL_OFFSET + IMAGE_HEADER_SIZE), hdr->payload_size);
		cache_sync_code(dst, dst + hdr->payload_size,
		                hdr->load_address, hdr->load_address + hdr->payload_size);
		if (crc32_compute((const void *)(uintptr_t)hdr->load_address, hdr->payload_size) == hdr->payload_crc32)
			jump_to(hdr->entry_address, 0);
	}
	system_reboot();
}

void HAL_Delay_us(int delay)
{
	if (delay > 0)
		timer_delay_us((uint32_t)delay);
}

void HAL_Configure_WDT(void)
{
}

void HAL_Run_WDT(void)
{
	watchdog_kick();
}

int HAL_GetResetReason(void)
{
	return 0;
}

void HAL_GetMACWiFi(unsigned char *mac)
{
	const uint8_t *w = net_wifi_mac();
	if (w && (w[0] | w[1] | w[2] | w[3] | w[4] | w[5])) {
		memcpy(mac, w, 6);
		return;
	}
	const uint8_t *m = net_mac();
	if (m)
		memcpy(mac, m, 6);
}

#ifndef LINUX
void usleep(int r)
{
	if (r > 0)
		timer_delay_us((uint32_t)r);
}
#endif

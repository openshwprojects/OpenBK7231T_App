#include "../../new_common.h"
#include "../hal_flashConfig.h"
#include "hal_88paax01.h"
#include "spi_flash.h"

static int g_vflash_ready = 0;
static int g_spi_flash_ok = 0;
static uint32_t g_sector_loaded_mask = 0;

static int spi_write_bytes(uint32_t addr, const uint8_t *src, uint32_t len)
{
	while (len > 0) {
		uint32_t page_rem = 256u - (addr & 0xFFu);
		uint32_t chunk = len < page_rem ? len : page_rem;
		if (flash_program_page(addr, src, chunk) != FLASH_OK)
			return -1;
		addr += chunk;
		src  += chunk;
		len  -= chunk;
	}
	return 0;
}

static void ensure_vflash_range(uint32_t offset, uint32_t len)
{
	pax_flash_init();
	if (len == 0 || offset >= VFLASH_TOTAL_SIZE)
		return;
	if (offset + len > VFLASH_TOTAL_SIZE)
		len = VFLASH_TOTAL_SIZE - offset;

	uint32_t first_sec = offset >> 12;
	uint32_t last_sec  = (offset + len - 1u) >> 12;
	uint8_t *base = (uint8_t *)VFLASH_MEM_BASE;

	for (uint32_t s = first_sec; s <= last_sec; s++) {
		if (g_sector_loaded_mask & (1u << s))
			continue;
		uint32_t sec_off = s << 12;
		if (g_spi_flash_ok) {
			flash_read(SPI_VFLASH_OFFSET + sec_off, base + sec_off, 0x1000u);
		} else {
			memset(base + sec_off, 0xFF, 0x1000u);
		}
		g_sector_loaded_mask |= (1u << s);
	}
}

void pax_flash_init(void)
{
	if (g_vflash_ready)
		return;

	if (flash_mode_was_detected() || flash_autodetect_mode() >= 0) {
		g_spi_flash_ok = 1;
	}
	g_vflash_ready = 1;
}

int HAL_FlashRead(char *buffer, int readlen, int startaddr)
{
	if (readlen <= 0 || startaddr < 0 ||
	    (uint32_t)(startaddr + readlen) > VFLASH_TOTAL_SIZE)
		return -1;
	ensure_vflash_range((uint32_t)startaddr, (uint32_t)readlen);
	memcpy(buffer, (const void *)(VFLASH_MEM_BASE + (uint32_t)startaddr),
	       (size_t)readlen);
	return readlen;
}

int HAL_FlashWrite(char *buf, unsigned int len, unsigned int addr)
{
	if (addr + len > VFLASH_TOTAL_SIZE)
		return -1;
	ensure_vflash_range(addr, len);
	uint8_t *dst = (uint8_t *)(VFLASH_MEM_BASE + addr);
	for (unsigned int i = 0; i < len; i++)
		dst[i] &= (uint8_t)buf[i];
	if (g_spi_flash_ok)
		spi_write_bytes(SPI_VFLASH_OFFSET + addr, (const uint8_t *)buf, len);
	return (int)len;
}

int HAL_FlashEraseSector(int startaddr)
{
	pax_flash_init();
	if (startaddr < 0 || (uint32_t)startaddr >= VFLASH_TOTAL_SIZE)
		return -1;
	uint32_t aligned = (uint32_t)startaddr & ~0xFFFu;
	uint32_t sec = aligned >> 12;
	memset((void *)(VFLASH_MEM_BASE + aligned), 0xFF, 0x1000u);
	g_sector_loaded_mask |= (1u << sec);
	if (g_spi_flash_ok)
		flash_erase_sector(SPI_VFLASH_OFFSET + aligned);
	return 0;
}

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen)
{
	if (dataLen <= 0 || (uint32_t)dataLen > VFLASH_VARS_OFF)
		return 0;
	ensure_vflash_range(VFLASH_CONFIG_OFF, (uint32_t)dataLen);
	memcpy(target, (const void *)(VFLASH_MEM_BASE + VFLASH_CONFIG_OFF),
	       (size_t)dataLen);
	return dataLen;
}

int HAL_Configuration_SaveConfigMemory(void *src, int dataLen)
{
	if (dataLen <= 0 || (uint32_t)dataLen > VFLASH_VARS_OFF)
		return 0;
	ensure_vflash_range(VFLASH_CONFIG_OFF, (uint32_t)dataLen);
	uint8_t *dst = (uint8_t *)(VFLASH_MEM_BASE + VFLASH_CONFIG_OFF);
	if (memcmp(dst, src, (size_t)dataLen) == 0)
		return dataLen;
	memcpy(dst, src, (size_t)dataLen);
	if (g_spi_flash_ok) {
		uint32_t off = SPI_VFLASH_OFFSET + VFLASH_CONFIG_OFF;
		uint32_t rem = (uint32_t)dataLen;
		const uint8_t *p = (const uint8_t *)src;
		while (rem > 0) {
			uint32_t chunk = rem > 0x1000u ? 0x1000u : rem;
			flash_erase_sector(off);
			spi_write_bytes(off, p, chunk);
			off += 0x1000u;
			p   += chunk;
			rem -= chunk;
		}
	}
	return dataLen;
}

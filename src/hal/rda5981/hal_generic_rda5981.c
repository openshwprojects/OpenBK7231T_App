#if PLATFORM_RDA5981

#include "../hal_generic.h"
#include "wland_flash.h"
#include "rda_wdt_api.h"
#include "wait_api.h"

static wdt_t obj;

void HAL_RebootModule()
{
	//rda_wdt_softreset();
	rda_sys_softreset();
}

void HAL_Delay_us(int delay)
{
	wait_us(delay);
}

void HAL_Configure_WDT()
{
	rda_wdt_init(&obj, 10);
	rda_wdt_start(&obj);
}

void HAL_Run_WDT()
{
	rda_wdt_feed(&obj);
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	return rda5981_read_flash(0x18000000 + startaddr, (uint8_t*)buffer, readlen);
}

int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr)
{
	if(len == 0) return 0;
	addr += 0x18000000;
	int ret;
	int left;
	unsigned int addr_t, len_t;
	char* temp_buf = NULL;
	addr_t = addr & 0xffffff00;
	len_t = addr - addr_t + len;
	if(len_t % 256)
		len_t += 256 - len_t % 256;
	//printf("addr %x addr_t %x\r\n", addr, addr_t);
	//printf("len %d len_t %d\r\n", len, len_t);
	temp_buf = (char*)malloc(256);
	if(temp_buf == NULL)
		return -1;

	ret = rda5981_read_flash(addr_t, temp_buf, 256);
	if(ret)
	{
		free(temp_buf);
		return -1;
	}
	left = 256 - (addr - addr_t);
	if(len < left)
		memcpy(temp_buf + addr - addr_t, buf, len);//256-(addr-addr_t));
	else
		memcpy(temp_buf + addr - addr_t, buf, left);
	ret = rda5981_write_flash(addr_t, temp_buf, 256);
	if(ret)
	{
		free(temp_buf);
		return -1;
	}

	len_t -= 256;
	buf += 256 - (addr - addr_t);
	len -= 256 - (addr - addr_t);
	addr_t += 256;

	while(len_t != 0)
	{
		//printf("len_t %d buf %x len %d addr_t %x\r\n", len_t, buf, len, addr_t);
		if(len >= 256)
		{
			memcpy(temp_buf, buf, 256);
		}
		else
		{
			ret = rda5981_read_flash(addr_t, temp_buf, 256);
			if(ret)
			{
				free(temp_buf);
				return -1;
			}
			memcpy(temp_buf, buf, len);
		}
		ret = rda5981_write_flash(addr_t, temp_buf, 256);
		if(ret)
		{
			free(temp_buf);
			return -1;
		}
		len_t -= 256;
		buf += 256;
		len -= 256;
		addr_t += 256;
	}
	free(temp_buf);
	return 0;
}

int HAL_FlashEraseSector(int startaddr)
{
	return rda5981_erase_flash(0x18000000 + startaddr, 0x1000);
}

#endif // PLATFORM_RDA5981

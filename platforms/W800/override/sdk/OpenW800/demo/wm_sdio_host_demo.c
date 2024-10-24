
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wm_include.h"
#include "wm_sdio_host.h"
#include "wm_debug.h"
#include "wm_dma.h"
#include "wm_cpu.h"
#include "random.h"
#include "wm_gpio_afsel.h"

#if DEMO_SDIO_HOST
extern int wm_sd_card_set_blocklen(uint32_t blocklen);
extern int wm_sd_card_query_status(uint32_t rca, uint32_t *respCmd0);
#define TEST_DEBUG_EN           0
#if TEST_DEBUG_EN
#define TEST_DEBUG(fmt, ...)    printf("%s: "fmt, __func__, ##__VA_ARGS__)
#else
#define TEST_DEBUG(fmt, ...)
#endif

/* single block write & read
 * bus_width: 0:1bit; 2:4bits
 * */
static int sdh_card_wr_sb(uint32_t rca, uint8_t bus_width, const uint32_t tsize)
{
	int ret = -1;
	int i = 0;
	char* buf = NULL;
	char* bufR = NULL;

	buf = tls_mem_alloc(512);
	if(buf == NULL)
		goto end;
	bufR = tls_mem_alloc(512);
	if(bufR == NULL)
		goto end;
	random_get_bytes(buf, 512);
	TEST_DEBUG("bus width %s\n", bus_width == 0 ? "1bit" : "4bits");
	ret = wm_sd_card_set_bus_width(rca, bus_width);
	if(ret)
		goto end;
	ret = wm_sd_card_set_blocklen(0x200); //512
	if(ret)
		goto end;

	for(i=0; i<(tsize/512); i++)
	{
		ret = wm_sd_card_block_write(rca, i, buf);
		if(ret)
			goto end;
	}
	ret = wm_sd_card_query_status(rca, NULL);
	if(ret)
		goto end;
	for(i=0; i<(tsize/512); i++)
	{
		ret = wm_sd_card_block_read(rca, i, bufR);
		if(ret)
			goto end;
		if(memcmp(buf, bufR, 512))
		{
			ret = -2;
			goto end;
		}
	}

	ret = 0;
end:
	if(buf)
	{
		tls_mem_free(buf);
	}
	if(bufR)
	{
		tls_mem_free(bufR);
	}
	TEST_DEBUG("ret %d\n", ret);
	return ret;
}

/* multi blocks write & read by dma
 * bus_width: 0:1bit; 2:4bits
 * */
static int sdh_card_wr_mbs_dma_4M_test(uint32_t rca, uint8_t bus_width, uint32_t block_cnt)
{
	int ret = -1;
	int i = 0;
	char* buf = NULL;
	char* bufR = NULL;
	const uint32_t tsize = 4*1024;//*1024;
	int buflen = 512*block_cnt;

	buf = tls_mem_alloc(buflen);
	if(buf == NULL)
		goto end;
	bufR = tls_mem_alloc(buflen);
	if(bufR == NULL)
		goto end;
	random_get_bytes(buf, buflen);
	TEST_DEBUG("bus width %s\n", bus_width == 0 ? "1bit" : "4bits");
	ret = wm_sd_card_set_bus_width(rca, bus_width);
	if(ret)
		goto end;
	ret = wm_sd_card_set_blocklen(0x200); //512
	if(ret)
		goto end;

	//(tsize/512)---->total block number; 
	//(block_cnt)---->write/read x blocks every time;
	//(sd_addr)---->start from address 0(block 0 too)
	for(i=0; i<(tsize/512); i+=block_cnt)
	{
		ret = wm_sd_card_blocks_write(rca, i, buf, buflen);
		if(ret)
			goto end;
	}
	ret = wm_sd_card_query_status(rca, NULL);
	if(ret)
		goto end;

	for(i=0; i<(tsize/512); i+=block_cnt)
	{
		memset(bufR, 0, buflen);
		ret = wm_sd_card_blocks_read(rca, i, bufR, buflen);
		if(ret)
			goto end;
		if(memcmp(buf, bufR, buflen))
		{
			ret = -2;
			goto end;
		}
	}
	ret = 0;
end:
	if(buf)
	{
		tls_mem_free(buf);
	}
	if(bufR)
	{
		tls_mem_free(bufR);
	}
	TEST_DEBUG("ret %d\n", ret);
	return ret;
}

static void wr_delay(int cir)
{
#ifdef TLS_CONFIG_CPU_XT804
		volatile int delay = cir;
#else
		int delay = count;
#endif
	
		while(delay--)
			;

}

int sd_card_test(void)
{	
	uint32_t rca;
	int ret=0;
	
	wm_sdio_host_config(0);
	ret += sdh_card_init(&rca);
	printf("\nsdh_card_init, ret = %d\n", ret);
	ret += sdh_card_wr_sb(rca, 0, 1024);
	printf("\nW & R 1, ret = %d\n", ret);
	wr_delay(10000);
	ret += sdh_card_wr_sb(rca, 2, 1024);
	printf("W & R 2, ret = %d\n", ret);
	wr_delay(10000);
	ret += sdh_card_wr_mbs_dma_4M_test(rca, 0, 4);
	printf("W & R 3, ret = %d\n", ret);
	wr_delay(10000);
	ret += sdh_card_wr_mbs_dma_4M_test(rca, 2, 6);
	printf("W & R 4, ret = %d\n", ret);
	wr_delay(10000);

	if( ret ==0 ) {
		printf("\nsd card write read OK, ret = %d\n", ret);
	}
	else {
		printf("\nsd card write read FAIL, ret = %d\n", ret);
	}
	
	return 0;
}

#endif


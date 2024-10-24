#include "wm_sdio_host.h"
#include "wm_debug.h"
#include "wm_mem.h"
#include "wm_dma.h"
#include "wm_cpu.h"

#define TEST_DEBUG_EN           0
#if TEST_DEBUG_EN
#define TEST_DEBUG(fmt, ...)    printf("%s: "fmt, __func__, ##__VA_ARGS__)
#else
#define TEST_DEBUG(fmt, ...)
#endif

SD_CardInfo_t SDCardInfo;
extern void delay_cnt(int count);
static void sh_dumpBuffer(char *name, char* buffer, int len)
{
#if TEST_DEBUG_EN
	int i = 0;
	printf("%s:\n", name);
	for(; i < len; i++)
	{
		printf("%02X, ", buffer[i]);
		if((i + 1) % 16 == 0)
		{
			printf("\n");
		}
	}
	printf("\n");
#endif
}

void wm_sdh_send_cmd(uint8_t cmdnum, uint32_t cmdarg, uint8_t mmcio)
{
	SDIO_HOST->CMD_BUF[4] = cmdnum | 0x40;
	SDIO_HOST->CMD_BUF[3] = (cmdarg >> 24) & 0xFF;
	SDIO_HOST->CMD_BUF[2] = (cmdarg >> 16) & 0xFF;
	SDIO_HOST->CMD_BUF[1] = (cmdarg >> 8) & 0xFF;
	SDIO_HOST->CMD_BUF[0] = (cmdarg & 0xFF);
	SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
	SDIO_HOST->MMC_IO = mmcio;
}

void wm_sdh_get_response(uint32_t * respbuf, uint32_t buflen)
{
	int i = 0;
	for(i = 0; i < buflen; i++)
	{
		respbuf[i] = (SDIO_HOST->CMD_BUF[i*4 + 3] << 24) | (SDIO_HOST->CMD_BUF[i*4 + 2] << 16) | (SDIO_HOST->CMD_BUF[i*4 + 1] << 8) | (SDIO_HOST->CMD_BUF[i*4]);
	}
}

static int sm_sdh_wait_interrupt(uint8_t srcbit, int timeout)
{
	int ret = 0;
	unsigned int tmp = (1 << srcbit);
	volatile int vtimeout= timeout;

	if(vtimeout == -1) {
		vtimeout = 0x7FFFF;
	}

	while(1)
	{
		if(SDIO_HOST->MMC_INT_SRC & tmp)
		{
			SDIO_HOST->MMC_INT_SRC |= tmp;
			if(SDIO_HOST->MMC_INT_SRC)
			{
				TEST_DEBUG("Err Int 0x%x\n", SDIO_HOST->MMC_INT_SRC);
			}
			break;
		}
		vtimeout--;
		if((vtimeout == 0) || (vtimeout < 0))
		{
			ret = 1; //timeout, err
			break;
		}
		delay_cnt(1);
	}
	return ret;
}

int wm_sdh_config(void)
{
    tls_sys_clk sysclk;	

	tls_sys_clk_get(&sysclk);
	SDIO_HOST->MMC_CARDSEL = 0xC0 | (sysclk.cpuclk / 2 - 1);//0xd3; //enable module, enable mmcclk
	SDIO_HOST->MMC_CTL = 0xD3;//0xC3; //4bits, low speed, 1/4 divider, auto transfer, mmc mode.
	SDIO_HOST->MMC_INT_MASK = 0x100; //unmask sdio data interrupt.
	SDIO_HOST->MMC_CRCCTL = 0xC0; //
	SDIO_HOST->MMC_TIMEOUTCNT = 0xff;
	return 0;
}

int wm_sd_card_initialize(uint32_t *rca)
{
	int ret = -1;
	uint32_t respCmd[4];
	int recnt = 5;
	
	wm_sdh_config();
	//======================================================
	// set up
	// Test:  Init sequence, With response check
	// CMD 0  Reset Card
	// CMD 8  Get voltage (Only 2.0 Card response to this)
	// CMD55  Indicate Next Command are Application specific
	// ACMD41 Get Voltage windows
	// CMD 2  CID reg
	// CMD 3  Get RCA.
	//======================================================
begin:
	wm_sdh_send_cmd(0, 0, 0x04); //Send CMD0
	sm_sdh_wait_interrupt(0, -1);
	delay_cnt(1000);
	wm_sdh_send_cmd(8, 0x1AA, 0x44); //Send CMD8
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD8 respCmd", (char *)respCmd, 5);
	if(respCmd[0] != 0x1AA || (respCmd[1] & 0xFF) != 8)
	{
		TEST_DEBUG("CMD8 Error\n");
		if(recnt--)
			goto begin;
		goto end;
	}
	while(1)
	{
		wm_sdh_send_cmd(55, 0, 0x44); //Send CMD55
		sm_sdh_wait_interrupt(0, -1);
		wm_sdh_get_response(respCmd, 2);
		sh_dumpBuffer("CMD55 respCmd", (char *)respCmd, 5);
		if((respCmd[1] & 0xFF) != 55)
			goto end;

		wm_sdh_send_cmd(41, 0xC0100000, 0x44); //Send ACMD41
		sm_sdh_wait_interrupt(0, -1);
		sm_sdh_wait_interrupt(3, 1000); //由于sd规范中，Acmd41返回的crc永远是11111，也就是应该忽略crc;这里的crc错误应该忽略。
		wm_sdh_get_response(respCmd, 2);
		sh_dumpBuffer("ACMD41 respCmd", (char *)respCmd, 5);
		if((respCmd[1] & 0xFF) != 0x3F) //sd规范定义固定为0x3F,所以导致crc错误
			goto end;
		if(respCmd[0] >> 31 & 0x1)
		{
			TEST_DEBUG("card is ready\n");
			break;
		}
	}

	wm_sdh_send_cmd(2, 0, 0x54); //Send CMD2
	sm_sdh_wait_interrupt(0, -1);
	sm_sdh_wait_interrupt(3, 1000);
	wm_sdh_get_response(respCmd, 4);
	sh_dumpBuffer("CMD2 respCmd", (char *)respCmd, 16);
	if((respCmd[3] >> 24 & 0xFF) != 0x3F) //sd规范定义固定为0x3F,所以导致crc错误
		goto end;
	wm_sdh_send_cmd(3, 0, 0x44); //Send CMD3
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD3 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 3)
		goto end;
	*rca = respCmd[0] >> 16;
	TEST_DEBUG("RCA = %x\n", *rca);

	ret = 0;
end:
	return ret;
}

static uint32_t SD_GetCapacity(uint8_t *csd, SD_CardInfo_t *SDCardInfo)
{
  uint32_t Capacity;
  uint16_t n;
  uint32_t csize; 

  if((csd[0]&0xC0)==0x40)//判断bit126是否为1
  { 
    csize = csd[9] + ((uint32_t)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1;
    Capacity = csize << 9;
	SDCardInfo->CardCapacity = (long long)Capacity*1024;
	SDCardInfo->CardBlockSize = 512;
  }
  else
  { 
    n = (csd[5] & 0x0F) + ((csd[10] & 0x80) >> 7) + ((csd[9] & 0x03) << 1) + 2;
    csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 0x03) << 10) + 1;
    Capacity = (uint32_t)csize << (n - 10);
	SDCardInfo->CardCapacity = (long long)Capacity*1024;
	SDCardInfo->CardBlockSize = 1<<(csd[5] & 0x0F);
  }
  return Capacity;
}

int wm_sd_card_query_csd(uint32_t rca)
{
	int ret = -1, i;
	uint32_t respCmd[4];
	char adjustResp[16];
	
	wm_sdh_send_cmd(9, rca<<16, 0x54); //Send CMD9
	sm_sdh_wait_interrupt(0, -1);
	sm_sdh_wait_interrupt(3, 1000);
	wm_sdh_get_response(respCmd, 4);
	for(i=0; i<16; i++) adjustResp[15-i] = SDIO_HOST->CMD_BUF[i];
	SD_GetCapacity((uint8_t*)&adjustResp[1], &SDCardInfo);
	sh_dumpBuffer("CMD9 respCmd", adjustResp, 16);
	if((respCmd[3] >> 24 & 0xFF) != 0x3F) //sd规范定义固定为0x3F,所以导致crc错误
		goto end;
	ret = 0;
end:
	return ret;
}

int wm_sd_card_set_blocklen(uint32_t blocklen)
{
	int ret = -1;
	uint32_t respCmd[2];
	wm_sdh_send_cmd(16, blocklen, 0x44); //Send CMD16
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD16 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 16)
		goto end;
	ret = 0;
end:
	return ret;
}

int wm_sd_card_select(uint32_t rca)
{
	int ret = -1;
	uint32_t respCmd[2];
	wm_sdh_send_cmd(7, rca<<16, 0x44); //Send CMD7
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD7 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 7)
		goto end;
	ret = 0;
end:
	return ret;
}

void wm_sd_card_deselect()
{
	wm_sdh_send_cmd(7, 0, 0x04); //Send CMD7
	sm_sdh_wait_interrupt(0, -1);
}

int wm_sd_card_query_status(uint32_t rca, uint32_t *respCmd0)
{
	int ret = -1;
#if TEST_DEBUG_EN	
	uint8_t current_state = 0;
	uint8_t error_state = 0;
#endif	
	uint32_t respCmd[2];
	wm_sdh_send_cmd(13, rca<<16, 0x44); //Send CMD13
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD13 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 13)
		goto end;
	if(respCmd0)
	{
		*respCmd0 = respCmd[0];
	}
#if TEST_DEBUG_EN	
	current_state = respCmd[0] >> 9 & 0xF;
	error_state = respCmd[0] >> 19 & 0x1;
	TEST_DEBUG("current_state %d, error_state %d\n",current_state,error_state);
#endif	
	ret = 0;
end:
	return ret;
}

/*
 * speed_mode: 0: low speed; 1: high speed;
 * */
int wm_sd_card_switch_func(uint8_t speed_mode)
{
	int ret = -1;
	int i;
	uint32_t respCmd[2];

	wm_sdh_send_cmd(6, 0x00fffff1, 0x44); //Send CMD6
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD6 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 6)
		goto end;
	SDIO_HOST->BUF_CTL = 0x4020; //disable dma, read sd card
	SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
	SDIO_HOST->MMC_IO  = 0x3;   //!!!read data, auto transfer
	sm_sdh_wait_interrupt(1, -1);
	TEST_DEBUG("read complete\n");
	for(i = 0; i < 128; i++)
	{
		respCmd[0] = SDIO_HOST->DATA_BUF[0];
		if(i == 4)
		{
			respCmd[1] = respCmd[0];
		}
		printf("0x%x, ", respCmd[0]);
		if(i % 4 == 3)
		{
			printf("\n");
		}
	}
	TEST_DEBUG("the value of byte 17~20 is 0x%x\n", respCmd[1]);
	if(respCmd[1] & 0xF) //support high speed
	{
		wm_sdh_send_cmd(6, 0x80fffff0 | speed_mode, 0x44); //Send CMD6
		sm_sdh_wait_interrupt(0, -1);
		wm_sdh_get_response(respCmd, 2);
		sh_dumpBuffer("CMD6 respCmd", (char *)respCmd, 5);
		if((respCmd[1] & 0xFF) != 6)
			goto end;
		SDIO_HOST->BUF_CTL = 0x4020; //disable dma, read sd card
		SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
		SDIO_HOST->MMC_IO  = 0x3;   //!!!read data, auto transfer
		sm_sdh_wait_interrupt(1, -1);
		TEST_DEBUG("read complete\n");
		for(i = 0; i < 128; i++)
		{
			respCmd[0] = SDIO_HOST->DATA_BUF[0];
			if(i == 4)
			{
				respCmd[1] = respCmd[0];
			}
			printf("0x%x, ", respCmd[0]);
			if(i % 4 == 3)
			{
				printf("\n");
			}
		}
		TEST_DEBUG("the value of byte 17~20 is 0x%x\n", respCmd[1]);
		if((respCmd[1] & 0xF) == speed_mode)
		{
			if(speed_mode == 1)
			{
				SDIO_HOST->MMC_CTL |= (1 << 6);
			}
			else
			{
				SDIO_HOST->MMC_CTL &= ~(1 << 6);
			}
			TEST_DEBUG("switch speed_mode %d success\n", speed_mode);
		}
	}
	ret = 0;
end:
	return ret;
}

/*
 * bus_width: 0:1bit; 2:4bits
 * */
int wm_sd_card_set_bus_width(uint32_t rca, uint8_t bus_width)
{
	int ret = -1;
	uint32_t respCmd[2];
	if(bus_width != 0 && bus_width != 2)
	{
		TEST_DEBUG("bus width parameter error\n");
		goto end;
	}
	if(bus_width == 2)
	{
		SDIO_HOST->MMC_CTL |= (1 << 7);
	}
	else
	{
		SDIO_HOST->MMC_CTL &= ~(1 << 7);
	}
	wm_sdh_send_cmd(55, rca<<16, 0x44); //Send CMD55
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD55 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 55)
		goto end;
	wm_sdh_send_cmd(6, bus_width, 0x44); //Send ACMD6
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("ACMD6 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 6)
		goto end;
	ret = 0;
end:
	return ret;
}

int wm_sd_card_stop_trans(void)
{
	int ret = -1;
	uint32_t respCmd[2];
	wm_sdh_send_cmd(12, 0, 0x44); //Send CMD12
	ret = sm_sdh_wait_interrupt(0, -1);
	if(ret)
		goto end;
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD12 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 12)
		goto end;
	ret = 0;
end:
	return ret;
}

static void sdio_host_reset(void)
{
	tls_bitband_write(HR_CLK_RST_CTL, 27, 0);

	tls_bitband_write(HR_CLK_RST_CTL, 27, 1);
	while(tls_bitband_read(HR_CLK_RST_CTL, 27) == 0);
}


int sdh_card_init(uint32_t *rca_ref)
{
	int ret = -1;
	uint32_t rca = 0;

	sdio_host_reset();
	
	ret = wm_sd_card_initialize(&rca);
	if(ret)
		goto end;
	ret = wm_sd_card_query_csd(rca);
	if(ret)
		goto end;
	ret = wm_sd_card_query_status(rca, NULL);
	if(ret)
		goto end;
	ret = wm_sd_card_select(rca);
	if(ret)
		goto end;
	ret = wm_sd_card_query_status(rca, NULL);
	if(ret)
		goto end;
	*rca_ref = rca;
	ret = 0;
end:
	TEST_DEBUG("ret %d\n", ret);
	return ret;
}

/*buf's len must >= 512*/
int wm_sd_card_block_read(uint32_t rca, uint32_t sd_addr, char *buf)
{
	int ret = -1;
	int i;
	uint32_t respCmd[2];
	wm_sdh_send_cmd(17, sd_addr, 0x44); //Send CMD17
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD17 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 17)
		goto end;
	SDIO_HOST->BUF_CTL = 0x4020; //disable dma, read sd card
	SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
	SDIO_HOST->MMC_IO  = 0x3;   //!!!read data, auto transfer
	sm_sdh_wait_interrupt(1, -1);
	TEST_DEBUG("read complete\n");
	for(i = 0; i < 128; i++)
	{
		*((uint32_t*)(buf + 4*i)) = SDIO_HOST->DATA_BUF[0];
	}
	ret = 0;
end:
	return ret;
}

/*buf's len must be 512*/
int wm_sd_card_block_write(uint32_t rca, uint32_t sd_addr, char *buf)
{
	int i;
	int ret = -1;
	uint32_t respCmd[2];
	uint8_t current_state = 0;
	uint8_t error_state = 0;

	wm_sdh_send_cmd(24, sd_addr, 0x44); //Send CMD24
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD24 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 24)
		goto end;

	SDIO_HOST->BUF_CTL = 0x4820; //disable dma, write sd card
	for(i = 0; i < 128; i++)
	{
		SDIO_HOST->DATA_BUF[i] = *((uint32*)(buf + 4*i));
	}
	SDIO_HOST->MMC_BYTECNTL = 512;
	SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
	SDIO_HOST->MMC_IO  = 0x1;   //!!!write data, auto transfer
	sm_sdh_wait_interrupt(1, -1);

	while(true)
	{
		ret = wm_sd_card_query_status(rca, &respCmd[0]);
		if(ret)
			goto end;
		current_state = respCmd[0] >> 9 & 0xF;
		error_state = respCmd[0] >> 19 & 0x1;
		if(current_state == 4) //tran
		{
			break;
		}
		if(error_state)
		{
			ret = -1;
			goto end;
		}
	}

	ret = 0;
end:
	TEST_DEBUG("write complete err %d\n", ret);
	return ret;
}

/* dir: 1, write; 0, read*/
int wm_sd_card_dma_config(u32*mbuf,u32 bufsize,u8 dir)
{
	int ch;
	u32 addr_inc = 0;

	ch = tls_dma_request(0, NULL);
	DMA_CHNLCTRL_REG(ch) = DMA_CHNL_CTRL_CHNL_OFF;

	if(dir)
	{
		DMA_SRCADDR_REG(ch) = (unsigned int)mbuf;
		DMA_DESTADDR_REG(ch) = (unsigned int)SDIO_HOST->DATA_BUF;
		addr_inc = (1 << 1);
	}
	else
	{
		DMA_SRCADDR_REG(ch) = (unsigned int)SDIO_HOST->DATA_BUF;
		DMA_DESTADDR_REG(ch) = (unsigned int)mbuf;
		addr_inc = (1 << 3);
	}
	DMA_CTRL_REG(ch) = addr_inc | (2 << 5) | (bufsize << 8);   
	DMA_MODE_REG(ch) = DMA_MODE_SEL_SDIOHOST | DMA_MODE_HARD_MODE;
	DMA_CHNLCTRL_REG(ch) = DMA_CHNL_CTRL_CHNL_ON;
	
	return ch;
}

static int wm_sdh_wait_blocks_done(void)
{
	int ret = 0;
    uint32_t timeout = 0x7FFFF * 8;

	while(1)
	{
        if((SDIO_HOST->MMC_IO_MBCTL & 0x01) == 0x00)
		{
            break;
		}
        
        if(timeout == 0)
        {
            ret = -1;
            tls_os_time_delay(HZ);

        }
        else
        {
            delay_cnt(1);
            timeout--;
        }
	}

	return ret;
}

/*read blocks by dma
 * buflen must be integer multiple of 512
 * */
int wm_sd_card_blocks_read(uint32_t rca, uint32_t sd_addr, char *buf, uint32_t buflen)
{
	int ret = -1, dma_channel = 0xFF, retresp = -100;
	uint32_t respCmd[2];
	int block_cnt = buflen/512;
	uint8_t current_state = 0;
	uint8_t error_state = 0;	

	wm_sdh_send_cmd(18, sd_addr, 0x44); //Send CMD18
	sm_sdh_wait_interrupt(0, -1);
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD18 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 18)
		goto end;

	SDIO_HOST->BUF_CTL = 0x4000; //disable dma,
	dma_channel = wm_sd_card_dma_config((u32*)buf, 512*block_cnt, 0);
	SDIO_HOST->BUF_CTL = 0x404; //enable dma, read sd card
	SDIO_HOST->MMC_BLOCKCNT = block_cnt;
	SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
	SDIO_HOST->MMC_IO_MBCTL = 0xa3; //read data, enable multi blocks data transfer
//	sm_sdh_wait_interrupt(1, -1);
	ret = wm_sdh_wait_blocks_done();
	if(ret)
	{
		TEST_DEBUG("wm_sd_card_blocks_read: timeout error\n");
        goto end;
	}
	TEST_DEBUG("read complete\n");
	ret = wm_sd_card_stop_trans();
	if(ret)
		goto end;
	
	/*waiting for card to trans state*/
	do
	{
		ret = wm_sd_card_query_status(rca, &respCmd[0]);
		if(ret)
		   break;
		current_state = respCmd[0] >> 9 & 0xF;
		error_state = respCmd[0] >> 19 & 0x1;
		if(error_state)
		{
			ret = -1;
			break;
		}
	}while(current_state != 4);
	if (ret)
	{
		TEST_DEBUG("mr blocks:%x\r\n", error_state);
		goto end;
	}
	ret = 0;
end:
	if (ret)
	{
		wm_sd_card_stop_trans();
		do
		{
			retresp = wm_sd_card_query_status(rca, &respCmd[0]);
			if(retresp)
			   break;
			current_state = respCmd[0] >> 9 & 0xF;
			error_state = respCmd[0] >> 19 & 0x1;
			if(error_state)
			{
				ret = -1;
				break;
			}
		}while(current_state != 4);
	}
	tls_dma_free(dma_channel);

	return ret;
}

/*write blocks by dma
 * buflen must be integer multiple of 512
 * */
int wm_sd_card_blocks_write(uint32_t rca, uint32_t sd_addr, char *buf, uint32_t buflen)
{
	int dma_channel = 0xFF;
	int ret = -1, retresp = -100;
	uint32_t respCmd[2];
	int block_cnt = buflen/512;
	uint8_t current_state = 0;
	uint8_t error_state = 0;

	wm_sdh_send_cmd(25, sd_addr, 0x44); //Send CMD25
	ret = sm_sdh_wait_interrupt(0, -1);
	if(ret)
		goto end;
	wm_sdh_get_response(respCmd, 2);
	sh_dumpBuffer("CMD25 respCmd", (char *)respCmd, 5);
	if((respCmd[1] & 0xFF) != 25) {
		ret = -1;
		goto end;
	}

	SDIO_HOST->BUF_CTL = 0x4000; //disable dma,
	dma_channel = wm_sd_card_dma_config((u32*)buf, 512*block_cnt, 1);
	SDIO_HOST->BUF_CTL = 0xC20; //enable dma, write sd card
	SDIO_HOST->MMC_BLOCKCNT = block_cnt;
	SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
	SDIO_HOST->MMC_IO_MBCTL = 0xa1;////write data, enable multi blocks data transfer
#if 0
	ret = sm_sdh_wait_interrupt(1, -1);
	if(ret)
		goto end;
#endif
	ret = wm_sdh_wait_blocks_done();
	if(ret)
        goto end;
	ret = wm_sd_card_stop_trans();
	if(ret)
		goto end;
	/*waiting for card to trans state*/
	do
	{
		ret = wm_sd_card_query_status(rca, &respCmd[0]);
		if(ret)
		   break;
		current_state = respCmd[0] >> 9 & 0xF;
		error_state = respCmd[0] >> 19 & 0x1;
		if(error_state)
		{
			ret = -1;
			break;
		}
	}while(current_state != 4);
	if (ret)
	{
		TEST_DEBUG("mw blocks:%x\r\n", error_state);
		goto end;
	}	
	TEST_DEBUG("write complete\n");
	ret = 0;
end:
	if (ret)
	{
		wm_sd_card_stop_trans();
		do
		{
			retresp = wm_sd_card_query_status(rca, &respCmd[0]);
			if(retresp)
			   break;
			current_state = respCmd[0] >> 9 & 0xF;
			error_state = respCmd[0] >> 19 & 0x1;
			if(error_state)
			{
				ret = -1;
				break;
			}
		}while(current_state != 4);
	}

	tls_dma_free(dma_channel);

	return ret;
}


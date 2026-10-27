#include "ameba.h"
#include "FreeRTOS.h"
#include "build_info.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "ssl_rom_to_ram_map.h"
#include "rtk_compiler.h"
#include "ameba_cache.h"
#include <vfs.h>
#include <kv.h>

#define MAGIC 						0xA5
#define ACK_MAGIC 					0x5A 

#define STATE_ERR					0xFF
#define STATE_SYN					0x00
#define STATE_RAM_DOWNLOAD			0x01
#define STATE_FLASH_DOWNLOAD		0x02
#define STATE_FLASH_UPLOAD			0x03
#define STATE_FLASH_ERASE			0x04
#define STATE_FLASH_CHIPERASE		0x05
#define STATE_RUN					0x06
#define STATE_BOUND					0x07
#define STATE_MAX					0x08

#define STATUS_SUCCESS				0x00
#define STATUS_ERROR				0x01
#define STATUS_ADDR_ERROR			0x02
#define STATUS_TYPE_ERROR			0x03
#define STATUS_LEN_ERROR			0x04
#define STATUS_CRC_ERROR			0x05

#define RESPONSE_FAIL				0xFF
#define RESPONSE_OK					0x00
#define RESPONSE_SYNC_BOOTROM		0x01
#define RESPONSE_SYNC_SBL			0x02

#define ACK_OK  0x00
#define ACK_ERR 0x01

#define MSG_OK 0x00
#define MSG_ERR -1

#define SYNC_REQUEST_VALUE			0x73796E63
#define SYNC_REQUEST_SIZE			0x04
#define SYNC_REQUEST_TIMEOUT		120

#define HEAD_SIZE					4
#define CFG_SIZE					8
#define ACK_SIZE					6
#define CMD_DATA_MAX_LEN 			(4 + 1 + 1024*64 + 2)
#define RESPONSE_SIZE				0x01
#define LOAD_MAX_SIZE_BIG			0x10000

struct sburner_cmd
{
	unsigned int msg_type;
	unsigned int arg0;
	unsigned int arg1;
};

struct message_rec_head
{
	unsigned char magic;
	unsigned char type;
	unsigned short data_len;
	unsigned int run_addr;
	unsigned char CRC8;
};

#define MESSAGE_REC_SIZE sizeof(struct message_rec_head)

struct message_ack_head
{
	unsigned char magic;
	unsigned char type;
	unsigned short data_len;
	unsigned char status;
	unsigned char CRC8;
};
#define MESSAGE_ACK_SIZE sizeof(struct message_ack_head)

struct load_cfg_msg
{
	unsigned int	addr;
	unsigned int	len;
} load_cfg_msg_t;
#define MESSAGE_LOAD_SIZE sizeof(struct load_cfg_msg)

#pragma pack(1)
typedef struct message_head
{
	uint8_t sof;
	uint8_t type;
	uint32_t data_len;
	uint8_t sub_type;
	uint32_t check_sum;
} message_head_t;
#pragma pack()

struct download_cfg_msg
{
	int msg_type;
	int addr;
	int len;
};

struct download_t
{
	int download_state;
	int download_addr;
	int download_len;
	int download_timeout;
	char* downloader_buf;
};

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CRC_MODE 'C'
#define XMODEM_BLOCK_SIZE_1K 1024

uint32_t g_flash_id = 0;
unsigned int g_flash_size = 0;
struct message_ack_head ACK_msg =
{
	.magic = ACK_MAGIC,
	.type = 0,
	.data_len = 0,
	.status = STATUS_SUCCESS,
	.CRC8 = 0
};

unsigned char cmd_data_buf[CMD_DATA_MAX_LEN] = { 0 };

int uart_cmd_parser(void);

#ifdef CONFIG_AMEBADPLUS
#define SPICCLKSL BIT_LSYS_CKSL_SPIC_XTAL
#elif CONFIG_AMEBALITE
#define SPICCLKSL BIT_LSYS_CKSL_SPIC_LBUS
u32 IPC_SEMTake(IPC_SEM_IDX SEM_Idx, u32 timeout)
{
	(void)SEM_Idx;
	(void)timeout;
	return true;
}
u32 IPC_SEMFree(IPC_SEM_IDX SEM_Idx)
{
	(void)SEM_Idx;
	return true;
}
#endif

void flasher_stub(void)
{
	DCache_CleanInvalidate(0xFFFFFFFF, 0xFFFFFFFF);
	SCB_DisableDCache();
	_memset((void*)__image1_bss_start__, 0, (__image1_bss_end__ - __image1_bss_start__));

	void sburner_flash_init(void);
	sburner_flash_init();
	WDG_Refresh(IWDG_DEV);
	InterruptDis(UART_LOG_IRQ);

	RCC_PeriphClockSource_SPIC(SPICCLKSL);
	FLASH_Read_HandShake_Cmd(0, DISABLE);
	FLASH_DeepPowerDown(DISABLE);
#ifdef CONFIG_AMEBADPLUS
	if (flash_init_para.FLASH_addr_phase_len == ADDR_4_BYTE) {
		FLASH_Addr4ByteEn();
	}
#endif
	RCC_PeriphClockCmd(APBPeriph_SHA, APBPeriph_SHA_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_LX, APBPeriph_LX_CLOCK, ENABLE);
	extern HeapRegion_t xHeapRegions[];
	bool os_heap_add(u8 * start_addr, size_t heap_size);
	os_heap_add((uint8_t*)0x20000000, (size_t)0xA000);
	os_heap_add((uint8_t*)0x20050000, (size_t)0x30000);
	vPortDefineHeapRegions(xHeapRegions);
	vfs_init();
	vfs_user_register(VFS_PREFIX, VFS_LITTLEFS, VFS_INF_FLASH, VFS_REGION_1, VFS_RW);
	rt_kv_init();
	CRYPTO_SHA_Init(NULL);
	LOGUART_SetBaud(LOGUART_DEV, 115200);
	while(1) uart_cmd_parser();
}

IMAGE1_ENTRY_SECTION
RAM_FUNCTION_START_TABLE RamStartTable = {
	.RamStartFun = flasher_stub,
	.RamWakeupFun = flasher_stub,
	.RamPatchFun0 = flasher_stub,
	.RamPatchFun1 = flasher_stub,
	.RamPatchFun2 = flasher_stub
};

//void rtk_log_write_nano(rtk_log_level_t level, const char* tag, const char letter, const char* fmt, ...) 
//{
//	(void)level;
//	(void)tag;
//	(void)letter;
//	(void)fmt;
//}

void FLASH_EraseXIP(u32 EraseType, u32 Address)
{
	FLASH_Erase(EraseType, Address);
}

int FLASH_ReadStream(u32 address, u32 len, u8* pbuf)
{
	assert_param(pbuf != NULL);

	_memcpy(pbuf, (const void*)(SPI_FLASH_BASE + address), len);

	return 1;
}

int FLASH_WriteStream(u32 address, u32 len, u8* pbuf)
{
	/* Check address: 4byte aligned & page(256bytes) aligned */
	u32 page_begin = address & (~0xff);
	u32 page_end = (address + len - 1) & (~0xff);
	u32 page_cnt = ((page_end - page_begin) >> 8) + 1;

	u32 addr_begin = address;
	u32 addr_end = (page_cnt == 1) ? (address + len) : (page_begin + 0x100);
	u32 size = addr_end - addr_begin;

	if(len == 0)
	{
		//RTK_LOGW(NOTAG, "function %s, data length is invalid (0) \r\n", __func__);
		goto exit;
	}

	if(IS_FLASH_ADDR((u32)pbuf))
	{
		//RTK_LOGE(NOTAG, "function %s, source address(%08x) can not be flash address\r\n", __func__, pbuf);
		//assert_param(0);
	}

	while(page_cnt)
	{
		FLASH_TxData(addr_begin, size, pbuf);
		pbuf += size;

		page_cnt--;
		addr_begin = addr_end;
		addr_end = (page_cnt == 1) ? (address + len) : (addr_begin + 0x100);
		size = addr_end - addr_begin;
	}

	//DCache_Invalidate(SPI_FLASH_BASE + address, len);
	//RSIP_MMU_Cache_Clean();

exit:
	return 1;
}

void uart_fifo_reset(void)
{
	LOGUART_ClearRxFifo(LOGUART_DEV);
}
void uart_putc(char ch)
{
#ifdef CONFIG_AMEBADPLUS
	extern LOG_UART_PORT LOG_UART_IDX_FLAG[];
	while(!LOGUART_Writable());
	LOGUART_WaitTx();
	LOGUART_DEV->LOGUART_UART_THRx[LOG_UART_IDX_FLAG[SYS_CPUID()].idx] = ch;
	//LOGUART_WaitTxComplete();
#else
	LOGUART_WaitTxComplete();
	LOGUART_DEV->LOGUART_UART_THRx[0] = ch;
#endif
}

void uart_write(unsigned char* data, uint32_t len)
{
	if(!data || !len)
	{
		return;
	}

	for(size_t i = 0; i < len; ++i)
	{
		uart_putc(data[i]);
	}
}

signed char uart_getc(uint8_t* out_byte, uint32_t timeout)
{
	uint64_t timeout_ticks = 0;
	
	if(out_byte == NULL)
		return -1;
	
	while(timeout_ticks++ < timeout * 10)
	{
		if(timeout_ticks % 100 == 0) WDG_Refresh(IWDG_DEV);
		u32 loguart_lsr = LOGUART_GetStatus(LOGUART_DEV);
		if((loguart_lsr & LOGUART_BIT_DRDY))
		//if(LOGUART_Readable())
		//if(LOGUART_GetRxCount())
		{
			*out_byte = (uint8_t)LOGUART_GetChar(false);
			return 0;
		}
		DelayUs(10);
	}

	return -1;    // timeout
}

void FLASH_EraseByLength(uint32_t Address, uint32_t Length)
{
	if((Address & 0x3) != 0)
		return;
	if(Length == 0)
		return;

	uint32_t cur = Address;
	uint32_t end = Address + Length;

	while(cur < end)
	{
		if((cur % 0x10000U == 0) && (end - cur >= 0x10000U))
		{
			FLASH_Erase(EraseBlock, cur);
			cur += 0x10000U;
		}
		else
		{
			FLASH_Erase(EraseSector, cur);
			cur += 0x1000U;
		}
	}

	return ;
}

unsigned char uboot_mesage_check(unsigned char* buf, unsigned short length)
{
	unsigned int crc = 0;
	unsigned char ret = 0;
	unsigned int i = 0;

	for(i = 0; i < length; i++)
	{
		crc += buf[i];
	}
	ret = crc % 256;
	return ret;
}

void sburner_flash_init(void)
{
	FLASH_RxCmd(flash_init_para.FLASH_cmd_rd_id, 3, (uint8_t*)&g_flash_id);

	unsigned char id = (g_flash_id >> 16) & 0xff;
	switch(id)
	{
		case 0x15:
			g_flash_size = 0x200000; break;
		default:
		case 0x16:
			g_flash_size = 0x400000; break;
		case 0x17:
			g_flash_size = 0x800000; break;
		case 0x18:
			g_flash_size = 0x1000000; break;
		case 0x19:
			g_flash_size = 0x2000000; break;
		case 0x20:
			g_flash_size = 0x4000000; break;
	}
}

static uint16_t crc16_ccitt(const uint8_t* data, uint16_t len)
{
	uint16_t crc = 0;
	while(len--)
	{
		crc ^= (uint16_t)(*data++) << 8;
		for(uint8_t i = 0; i < 8; i++)
			crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
	}
	return crc;
}

void uboot_sync(void)
{
	unsigned int i = 0;
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;

	//SYNC
	for(i = 0; i < SYNC_REQUEST_SIZE; i++)
	{
		if(cmd_data_buf[HEAD_SIZE + i] != (char)((SYNC_REQUEST_VALUE >> (i * 8)) & 0xff))
		{
			ACK_msg.status = STATUS_ERROR;
			ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
			uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
			return;	//eroor, again
		}
	}
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_flash_erase_handle(void* buf)
{
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;

	_memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	FLASH_EraseByLength(cfg_msg.addr, cfg_msg.len);
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_flash_chiperase_handle(void* buf)
{
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;

	FLASH_Erase(EraseChip, 0);
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_buad(void)
{
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	LOGUART_SetBaud(LOGUART_DEV, msg->run_addr);
	DelayUs(100000);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_flashid(void)
{
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	ACK_msg.data_len = 4;
	_memcpy(cmd_data_buf, &ACK_msg, HEAD_SIZE);
	_memcpy(&cmd_data_buf[HEAD_SIZE], &g_flash_id, 4);
	cmd_data_buf[HEAD_SIZE + 4] = STATUS_SUCCESS;
	cmd_data_buf[HEAD_SIZE + 4 + 1] = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + 4 + 1);
	uart_write((unsigned char*)cmd_data_buf, HEAD_SIZE + 4 + 2);
}

void uboot_flash_xmodem_dl(void* buf)
{
	uint8_t header[3] = { 0x00 };
	uint8_t data[XMODEM_BLOCK_SIZE_1K] = { 0x00 };
	uint8_t crc_bytes[2] = { 0x00 };
	uint16_t crc_calc, crc_recv;
	uint32_t offset = 0;
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	_memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	FLASH_EraseByLength(cfg_msg.addr, cfg_msg.len);

	uart_putc(CRC_MODE);

	for(;;)
	{
		// header
		if(uart_getc(&header[0], 1000) != 0)
		{
			uart_putc(CRC_MODE);
			continue;
		}

		if(header[0] == EOT)
		{
			uart_putc(ACK);
			break;
		}

#define NAKCONTINUE { uart_putc(NAK); continue; }

		if(header[0] != STX) NAKCONTINUE

		if(uart_getc(&header[1], 2000) == -1) NAKCONTINUE
		if(uart_getc(&header[2], 2000) == -1) NAKCONTINUE

		if((header[1] + header[2]) != 0xFF) NAKCONTINUE

		// recv data + crc
		for(int i = 0; i < XMODEM_BLOCK_SIZE_1K; i++)
			if(uart_getc(&data[i], 2000) == -1) NAKCONTINUE
		if(uart_getc(&crc_bytes[0], 2000) == -1) NAKCONTINUE
		if(uart_getc(&crc_bytes[1], 2000) == -1) NAKCONTINUE

		crc_recv = ((uint16_t)crc_bytes[0] << 8) | crc_bytes[1];
		crc_calc = crc16_ccitt(data, XMODEM_BLOCK_SIZE_1K);

		if(crc_recv != crc_calc) NAKCONTINUE

		FLASH_WriteStream(cfg_msg.addr + offset, XMODEM_BLOCK_SIZE_1K, data);
		offset += XMODEM_BLOCK_SIZE_1K;

		uart_putc(ACK);
	}
	//LOGUART_SetBaud(LOGUART_DEV, 115200);
}

void uboot_flash_xmodem_ul(void* buf)
{
	uint8_t block_num = 1;
	uint8_t data[XMODEM_BLOCK_SIZE_1K];
	uint8_t resp = 0;
	uint32_t offset = 0;
	int retry, ret;
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	_memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);
	uint32_t data_len = cfg_msg.len;

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	// wait for 'C'
	do
	{
		if(uart_getc(&resp, 1000) != 0)
			continue;
	} while(resp != CRC_MODE);

	while(data_len > 0)
	{
		uint32_t chunk = (data_len > XMODEM_BLOCK_SIZE_1K) ? XMODEM_BLOCK_SIZE_1K : data_len;
		memset(data, 0xFF, XMODEM_BLOCK_SIZE_1K); // pad with 0xFF

		FLASH_ReadStream(cfg_msg.addr + offset, chunk, data);

		uint16_t crc = crc16_ccitt(data, XMODEM_BLOCK_SIZE_1K);

		retry = 0;
		do
		{
			uart_putc(STX);
			uart_putc(block_num);
			uart_putc(~block_num);

			for(uint16_t i = 0; i < XMODEM_BLOCK_SIZE_1K; i++)
				uart_putc(data[i]);

			uart_putc((crc >> 8) & 0xFF);
			uart_putc(crc & 0xFF);

			ret = uart_getc(&resp, 5000);
			if(ret == 0 && resp == ACK)
				break;

			retry++;
		} while(retry < 50);

		if(retry >= 50)
		{
			uart_putc(CAN);
			uart_putc(CAN);
			return;
		}

		offset += chunk;
		data_len -= chunk;
		block_num++;
	}

	retry = 0;
	do
	{
		uart_putc(EOT);

		ret = uart_getc(&resp, 5000);

		if(ret == 0 && resp == ACK)
			break;

		retry++;
	} while(retry < 10);

	if(retry >= 10)
	{
		uart_putc(CAN);
		uart_putc(CAN);
	}

	return;
}

void uboot_flash_sha256(void* buf)
{
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 32;

	_memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.data_len = 0;
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}
	ALIGNMTO(CACHE_LINE_SIZE) u8 hash[32] = { 0 };
	hw_sha_context  ctx = { 0 };
	rtl_crypto_sha2_init(SHA2_256 , &ctx);
	uint32_t addr = cfg_msg.addr;
	uint32_t remaining = cfg_msg.len;
	while(remaining > 0)
	{
		uint32_t chunk = remaining > 0x40000 ? 0x40000 : remaining;
		WDG_Refresh(IWDG_DEV);
		rtl_crypto_sha2_update((uint8_t*)(SPI_FLASH_BASE + addr), chunk, &ctx);
		addr += chunk;
		remaining -= chunk;
	}
	
	rtl_crypto_sha2_final(hash, &ctx);

	_memcpy(cmd_data_buf, &ACK_msg, HEAD_SIZE);
	_memcpy(&cmd_data_buf[HEAD_SIZE], &hash, 32);
	cmd_data_buf[HEAD_SIZE + 32] = STATUS_SUCCESS;
	cmd_data_buf[HEAD_SIZE + 32 + 1] = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + 32 + 1);
	uart_write((unsigned char*)cmd_data_buf, HEAD_SIZE + 32 + 2);
}

void uboot_kv_get(void* buf)
{
	struct message_rec_head* msg = (struct message_rec_head*)buf;
	char kvname[MAX_KEY_LENGTH] = { 0 };
	strncpy((char*)&kvname, (const char*)&(cmd_data_buf[HEAD_SIZE]), msg->data_len);
	int kvsize = rt_kv_size(kvname);

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = kvsize;

	if(kvsize <= 0)
	{
		ACK_msg.data_len = 0;
		ACK_msg.status = STATUS_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	_memcpy(cmd_data_buf, &ACK_msg, HEAD_SIZE);
	rt_kv_get(kvname, &cmd_data_buf[HEAD_SIZE], kvsize);
	cmd_data_buf[HEAD_SIZE + kvsize] = STATUS_SUCCESS;
	cmd_data_buf[HEAD_SIZE + kvsize + 1] = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + kvsize + 1);
	uart_write((unsigned char*)cmd_data_buf, HEAD_SIZE + kvsize + 2);
}

void uboot_kv_set(void* buf)
{
	uint8_t header[3] = { 0x00 };
	uint8_t data[XMODEM_BLOCK_SIZE_1K] = { 0x00 };
	uint8_t crc_bytes[2] = { 0x00 };
	uint16_t crc_calc, crc_recv;
	uint32_t offset = 0;
	struct message_rec_head* msg = (struct message_rec_head*)buf;
	char kvname[16] = { 0 };
	unsigned char namelen = cmd_data_buf[HEAD_SIZE];
	strncpy((char*)&kvname, (const char*)&(cmd_data_buf[HEAD_SIZE + 3]), namelen);
	uint16_t datasize = cmd_data_buf[HEAD_SIZE + 1] | cmd_data_buf[HEAD_SIZE + 2] << 8;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	for(;;)
	{
		// header
		if(uart_getc(&header[0], 1000) != 0)
		{
			uart_putc(CRC_MODE);
			continue;
		}

		if(header[0] == EOT)
		{
			uart_putc(ACK);
			break;
		}

#define NAKCONTINUE { uart_putc(NAK); continue; }

		if(header[0] != STX) NAKCONTINUE

		if(uart_getc(&header[1], 2000) == -1) NAKCONTINUE
		if(uart_getc(&header[2], 2000) == -1) NAKCONTINUE

		if((header[1] + header[2]) != 0xFF) NAKCONTINUE

		// recv data + crc
		for(int i = 0; i < XMODEM_BLOCK_SIZE_1K; i++)
			if(uart_getc(&data[i], 2000) == -1) NAKCONTINUE
		if(uart_getc(&crc_bytes[0], 2000) == -1) NAKCONTINUE
		if(uart_getc(&crc_bytes[1], 2000) == -1) NAKCONTINUE

		crc_recv = ((uint16_t)crc_bytes[0] << 8) | crc_bytes[1];
		crc_calc = crc16_ccitt(data, XMODEM_BLOCK_SIZE_1K);

		if(crc_recv != crc_calc) NAKCONTINUE

		_memcpy(cmd_data_buf + offset, data, XMODEM_BLOCK_SIZE_1K);
		offset += XMODEM_BLOCK_SIZE_1K;

		uart_putc(ACK);
	}
	rt_kv_set(kvname, &cmd_data_buf, datasize);
}

void uboot_get_mac(void)
{
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	ACK_msg.data_len = 6;
	_memcpy(cmd_data_buf, &ACK_msg, HEAD_SIZE);
	OTP_LogicalMap_Read(&cmd_data_buf[HEAD_SIZE], 0x11A, 6);
	cmd_data_buf[HEAD_SIZE + 6] = STATUS_SUCCESS;
	cmd_data_buf[HEAD_SIZE + 6 + 1] = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + 6 + 1);
	uart_write((unsigned char*)cmd_data_buf, HEAD_SIZE + 6 + 2);
}

int uart_cmd_parser(void)
{
	unsigned int i = 0;
	signed char resp = 0;
	unsigned char CRC8 = 0;
	unsigned char buf[HEAD_SIZE] = { 0 };
	struct message_rec_head rec_head;

	unsigned char cfgbuf[CFG_SIZE] = { 0 };

	uart_fifo_reset();

	do
	{
		memset(buf, 0, HEAD_SIZE);
		for(i = 0; i < HEAD_SIZE; ++i)
		{
			resp = uart_getc((unsigned char*)&buf[i], 0xFFFFFFFF);
			if(resp < 0)
			{
				resp = MSG_ERR;
				return resp;
			}
			if((buf[0]) != MAGIC)
			{
				i = 0xFFFFFFFF;
			}
		}

		_memcpy(cmd_data_buf, buf, HEAD_SIZE);
		_memcpy(&rec_head, buf, HEAD_SIZE);

		if(buf[1] <= 0x9F)
		{
			for(i = 0; i < rec_head.data_len; i++)
			{
				resp = uart_getc((unsigned char*)&cfgbuf[i], 0xFFFFFFFF);
				if(resp < 0)
				{
					resp = MSG_ERR;
					return resp;
				}
			}
			_memcpy(&(cmd_data_buf[HEAD_SIZE]), cfgbuf, rec_head.data_len);

			resp = uart_getc((unsigned char*)&cmd_data_buf[HEAD_SIZE + rec_head.data_len], 0xFFFFFFFF);
			if(resp < 0)
			{
				resp = MSG_ERR;
				return resp;
			}
			CRC8 = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + rec_head.data_len);
			if(cmd_data_buf[HEAD_SIZE + rec_head.data_len] != CRC8)
			{
				rec_head.type = STATE_ERR;
			}
		}
		else
		{
			rec_head.type = STATE_ERR;
		}


		switch(rec_head.type)
		{
			case STATE_SYN:
				uboot_sync();
				break;
			case STATE_FLASH_ERASE: // 0x04
				uboot_flash_erase_handle(&cmd_data_buf);
				break;
			case STATE_FLASH_CHIPERASE: // 0x05
				uboot_flash_chiperase_handle(&cmd_data_buf);
				break;
			case STATE_BOUND: // 0x07
				uboot_buad();
				break;
			case 0x09:
				uboot_flash_sha256(&cmd_data_buf);
				break;
			case 0x90:
				uboot_flashid();
				break;
			case 0x91:
				uboot_flash_xmodem_dl(&cmd_data_buf);
				break;
			case 0x92:
				uboot_flash_xmodem_ul(&cmd_data_buf);
				break;
			case 0x93:
				uboot_kv_get(&cmd_data_buf);
				break;
			case 0x94:
				uboot_kv_set(&cmd_data_buf);
				break;
			case 0x95:
				uboot_get_mac();
				break;

			default:
				ACK_msg.type = STATE_ERR;
				ACK_msg.status = STATUS_TYPE_ERROR;
				ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
				uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
				break;
		}
	} while(1);
}

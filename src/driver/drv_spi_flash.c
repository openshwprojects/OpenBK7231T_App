#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../quicktick.h"
#include "drv_local.h"
#include "drv_soft_spi.h"
#include "../driver/drv_soft_spi.h"

#define SCK_PIN 26   // SCK pin - D5
#define MISO_PIN 24  // MISO pin - D6
#define MOSI_PIN 6  // MOSI pin - D7
#define SS_PIN 7    // chip select pin for the SPI flash - D8

#define READ_ID_FLASH_CMD 0x9F
#define READ_FLASH_CMD 0x03 // Read Flash command opcode
#define WRITEENABLE_FLASH_CMD 0x06
#define ERASE_WHOLE_FLASH_CMD 0xC7
#define WRITE_FLASH_CMD 0x02
#define ERASE_SECTOR_CMD 0x20 // 4KB erase
#define READ_STATUS_REG_CMD 0x05
#define STATUS_BUSY_MASK 0x01
#define STATUS_WEL_MASK 0x02

#define OBK_ENABLE_INTERRUPTS 
#define OBK_DISABLE_INTERRUPTS

// LFS api is very simple.
// To post file, use POST:
// http://192.168.0.166/api/lfs/
// To get file, use GET:
// http://192.168.0.166/api/lfs/
#if (PLATFORM_BK7231T || PLATFORM_BK7231N) && !PLATFORM_BEKEN_NEW

#include "include.h"
#include "arm_arch.h"
#include "gpio_pub.h"
#include "gpio.h"
#include "drv_model_pub.h"
#include "sys_ctrl_pub.h"
#include "uart_pub.h"
#include "intc_pub.h"
#include "icu_pub.h"

#define reg_val_HIGH 0x02
#define reg_val_LOW 0x00

#define FAST_SET_HIGH(gpio_cfg_addr) REG_WRITE(gpio_cfg_addr, reg_val_HIGH);
#define FAST_SET_LOW(gpio_cfg_addr) REG_WRITE(gpio_cfg_addr, reg_val_LOW);
#define FAST_READ_PIN(gpio_cfg_addr) ((REG_READ(gpio_cfg_addr) & GCFG_INPUT_BIT))

void FastSPI_Setup(softSPI_t *spi) {
	HAL_PIN_Setup_Output(spi->sck);
	HAL_PIN_Setup_Input(spi->miso);
	HAL_PIN_Setup_Output(spi->mosi);
	HAL_PIN_Setup_Output(spi->ss);
	HAL_PIN_SetOutputValue(spi->ss, 1); // set SS_PIN to inactive

	ADDLOG_INFO(LOG_FEATURE_CMD, "Fast SPI Path\n");

	{
		int id = spi->sck;
#if (CFG_SOC_NAME != SOC_BK7231)
		if (id >= GPIO32)
			id += 16;
#endif // (CFG_SOC_NAME != SOC_BK7231)
		spi->sck_reg = (unsigned int *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	}
	{
		int id = spi->mosi;
#if (CFG_SOC_NAME != SOC_BK7231)
		if (id >= GPIO32)
			id += 16;
#endif // (CFG_SOC_NAME != SOC_BK7231)
		spi->mosi_reg = (unsigned int *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	}
	{
		int id = spi->ss;
#if (CFG_SOC_NAME != SOC_BK7231)
		if (id >= GPIO32)
			id += 16;
#endif // (CFG_SOC_NAME != SOC_BK7231)
		spi->ss_reg = (unsigned int *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	}
	{
		int id = spi->miso;
#if (CFG_SOC_NAME != SOC_BK7231)
		if (id >= GPIO32)
			id += 16;
#endif // (CFG_SOC_NAME != SOC_BK7231)
		spi->miso_reg = (unsigned int *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	}
}

#define FastSPI_Begin(spi) FAST_SET_LOW(spi->ss_reg);
#define FastSPI_End(spi) FAST_SET_HIGH(spi->ss_reg);

#define SEND_BIT(spi, bit) \
	if (dataToSend & (1 << bit)) { \
		FAST_SET_HIGH(spi->mosi_reg); \
	} else { \
		FAST_SET_LOW(spi->mosi_reg); \
	} \
	FAST_SET_HIGH(spi->sck_reg); \
	FAST_SET_LOW(spi->sck_reg);

void FastSPI_Send(softSPI_t *spi, byte dataToSend) {
	SEND_BIT(spi, 7)
	SEND_BIT(spi, 6)
	SEND_BIT(spi, 5)
	SEND_BIT(spi, 4)
	SEND_BIT(spi, 3)
	SEND_BIT(spi, 2)
	SEND_BIT(spi, 1)
	SEND_BIT(spi, 0)
}
#define READ_BIT(spi, bit) \
	FAST_SET_HIGH(spi->sck_reg); \
	receivedData |= (FAST_READ_PIN(spi->miso_reg) << bit); \
	FAST_SET_LOW(spi->sck_reg);

byte FastSPI_Read(softSPI_t *spi) {
	byte receivedData = 0;
	READ_BIT(spi, 7)
	READ_BIT(spi, 6)
	READ_BIT(spi, 5)
	READ_BIT(spi, 4)
	READ_BIT(spi, 3)
	READ_BIT(spi, 2)
	READ_BIT(spi, 1)
	READ_BIT(spi, 0)
	return receivedData;
}

#define SPI_Setup FastSPI_Setup
#define SPI_Send FastSPI_Send
#define SPI_Read FastSPI_Read
#define SPI_Begin FastSPI_Begin
#define SPI_End FastSPI_End
#else


#endif

void spi_flash_read_id(softSPI_t* spi, byte* jedec_id) {
	SPI_Begin(spi);
	SPI_Send(spi, READ_ID_FLASH_CMD);
	jedec_id[0] = SPI_Read(spi);
	jedec_id[1] = SPI_Read(spi);
	jedec_id[2] = SPI_Read(spi);
	SPI_End(spi);
}



static void flash_wait_for(softSPI_t* spi, byte mask) {
	byte status;
	int loops = 0;
	while (1) {
		SPI_Begin(spi);
		SPI_Send(spi, READ_STATUS_REG_CMD);
		status = SPI_Read(spi);
		SPI_End(spi);
		if (!(status & mask)) {
			break;
		}
		loops++;
		rtos_delay_milliseconds(loops);
	}

	//ADDLOG_INFO(LOG_FEATURE_CMD, "flash_wait_for: done %i loops\n", loops);

}

static void flash_wait_while_busy(softSPI_t* spi) {
	flash_wait_for(spi, STATUS_BUSY_MASK);
}
void softspi_flash_read(softSPI_t* spi, int adr, byte* out, int cnt) {
	int i;

	OBK_DISABLE_INTERRUPTS;

	SPI_Begin(spi);
	SPI_Send(spi, READ_FLASH_CMD);
	SPI_Send(spi, (adr >> 16) & 0xFF);
	SPI_Send(spi, (adr >> 8) & 0xFF);
	SPI_Send(spi, adr & 0xFF);
	for (i = 0; i < cnt; i++) {
		out[i] = SPI_Read(spi);
	}
	SPI_End(spi);

	OBK_ENABLE_INTERRUPTS;
}

//static void flash_wait_for_wel(softSPI_t* spi) {
//	flash_wait_for(spi, STATUS_WEL_MASK);
//}

void spi_flash_erase(softSPI_t* spi) {
	OBK_DISABLE_INTERRUPTS;

	SPI_Begin(spi);
	SPI_Send(spi, WRITEENABLE_FLASH_CMD);
	SPI_End(spi);

	SPI_Begin(spi);
	SPI_Send(spi, ERASE_WHOLE_FLASH_CMD);
	SPI_End(spi);

	flash_wait_while_busy(spi);

	OBK_ENABLE_INTERRUPTS;
}

void spi_flash_write2(softSPI_t* spi, int adr, const byte* data, int cnt) {
	int remaining = cnt;
	int offset = 0;

	OBK_DISABLE_INTERRUPTS;

	//ADDLOG_INFO(LOG_FEATURE_CMD, "spi_flash_write2 %i at %i\n", cnt, adr);

	while (remaining > 0) {
		int page_offset = adr % 256;
		int write_len = 256 - page_offset;
		if (write_len > remaining)
			write_len = remaining;


		SPI_Begin(spi);
		SPI_Send(spi, WRITEENABLE_FLASH_CMD);
		SPI_End(spi);

		//ADDLOG_INFO(LOG_FEATURE_CMD, "Write fragmnent %i at %i\n", write_len, adr);
		SPI_Begin(spi);
		SPI_Send(spi, WRITE_FLASH_CMD);
		SPI_Send(spi, (adr >> 16) & 0xFF);
		SPI_Send(spi, (adr >> 8) & 0xFF);
		SPI_Send(spi, adr & 0xFF);
		for (int i = 0; i < write_len; i++) {
			SPI_Send(spi, data[offset + i]);
		}
		SPI_End(spi);

		flash_wait_while_busy(spi);

		adr += write_len;
		offset += write_len;
		remaining -= write_len;
	}

	OBK_ENABLE_INTERRUPTS;
}

void softspi_flash_erase_sector(softSPI_t* spi, int addr) {
	OBK_DISABLE_INTERRUPTS;

	SPI_Begin(spi);
	SPI_Send(spi, WRITEENABLE_FLASH_CMD);
	SPI_End(spi);

	SPI_Begin(spi);
	SPI_Send(spi, ERASE_SECTOR_CMD);
	SPI_Send(spi, (addr >> 16) & 0xFF);
	SPI_Send(spi, (addr >> 8) & 0xFF);
	SPI_Send(spi, addr & 0xFF);
	SPI_End(spi);

	flash_wait_while_busy(spi);

	OBK_ENABLE_INTERRUPTS;
}

void flash_test_pages(softSPI_t* spi, int baseAddr, int length, byte pattern) {
	int i, err = 0;
	byte *writeBuf = malloc(length);
	byte *readBuf = malloc(length);

	for (i = 0; i < length; i++)
		writeBuf[i] = pattern;

	if (0) {
		spi_flash_erase(spi);
	}
	else {
		int startAddr = baseAddr & ~(0xFFF); // align down to 4KB
		int endAddr = (baseAddr + length - 1) & ~(0xFFF); // align down last used address

		for (int addr = startAddr; addr <= endAddr; addr += 4096) {
			softspi_flash_erase_sector(spi, addr);
		}
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "Erased flash.");

	softspi_flash_read(spi, baseAddr, readBuf, length);
	for (i = 0; i < length; i++) {
		if (readBuf[i] != 0xFF)
			err++;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "Flash erased test: errors = %d/%d", err, length);

	err = 0;
	spi_flash_write2(spi, baseAddr, writeBuf, length);
	ADDLOG_INFO(LOG_FEATURE_CMD, "Wrote pattern %02X to flash.", pattern);

	softspi_flash_read(spi, baseAddr, readBuf, length);
	for (i = 0; i < length; i++) {
		if (readBuf[i] != pattern)
			err++;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Flash write-read test: errors = %d/%d", err, length);

	free(writeBuf);
	free(readBuf);
}

softSPI_t g_lfs_spi;
int g_lfs_spi_ready = 0;
void LFS_SPI_Init() {
	g_lfs_spi.miso = MISO_PIN;
	g_lfs_spi.mosi = MOSI_PIN;
	g_lfs_spi.ss = SS_PIN;
	g_lfs_spi.sck = SCK_PIN;

	SPI_Setup(&g_lfs_spi);
	g_lfs_spi_ready = 1;
}
void LFS_SPI_Flash_EraseSector(int addr) {
	if (g_lfs_spi_ready == 0) {
		LFS_SPI_Init();
	}
	softspi_flash_erase_sector(&g_lfs_spi, addr);
}
void spi_test() {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;
	byte jedec_id[3];

	SPI_Setup(&spi);

	spi_flash_read_id(&spi, jedec_id);

	ADDLOG_INFO(LOG_FEATURE_CMD, "ID %02X %02X %02X", jedec_id[0], jedec_id[1], jedec_id[2]);
}
void LFS_SPI_Flash_Read(int adr, int cnt, byte *data) {
	if (g_lfs_spi_ready == 0) {
		LFS_SPI_Init();
	}
	softspi_flash_read(&g_lfs_spi, adr, data, cnt);
}
void LFS_SPI_Flash_Write(int adr, const byte *data, int cnt) {
	if (g_lfs_spi_ready == 0) {
		LFS_SPI_Init();
	}
	spi_flash_write2(&g_lfs_spi, adr, data, cnt);
}
void spi_test_read_and_print(int adr, int cnt) {
	byte *data;
	int i;

	data = malloc(cnt);

	LFS_SPI_Flash_Read(adr, cnt, data);

	int chunkLen = 8;
	int j;
	for (i = 0; i < cnt; i += chunkLen) {
		char tmp[32];
		char tmp2[8];
		tmp[0] = 0;
		for (j = i; j < i + chunkLen && j < cnt; j++) {
			sprintf(tmp2, "%02X", data[j]);
			strcat(tmp, tmp2);
		}
		ADDLOG_INFO(LOG_FEATURE_CMD, "Ofs %i - %s", i, tmp);
	}
	free(data);
}

void spi_test_erase() {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	SPI_Setup(&spi);

	spi_flash_erase(&spi);
}


static commandResult_t CMD_SPITestFlash_ReadData(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int addr = 0;
	int len = 16;

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_ReadData");

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		addr = Tokenizer_GetArgInteger(0);
		if (Tokenizer_GetArgsCount() > 1) {
			len = Tokenizer_GetArgInteger(1);
		}
	}

	spi_test_read_and_print(addr, len);

	return CMD_RES_OK;
}

static commandResult_t CMD_SPITestFlash_Erase(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_Erase");

	spi_test_erase();

	return CMD_RES_OK;
}

static commandResult_t CMD_SPITestFlash_WriteStr(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int addr = 0;
	const char *str = "This is a test";

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_WriteStr");

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		addr = Tokenizer_GetArgInteger(0);
		if (Tokenizer_GetArgsCount() > 1) {
			str = Tokenizer_GetArg(1);
		}
	}

	LFS_SPI_Flash_Write(addr, (const byte*)str, strlen(str));

	return CMD_RES_OK;
}

static commandResult_t CMD_SPITestFlash_ReadID(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_ReadID");

	spi_test();

	return CMD_RES_OK;
}

static commandResult_t CMD_SPITestFlash_TestPages(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int addr = 0;
	int len = 64;
	int pattern = 0xA5;

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_TestPages");

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		addr = Tokenizer_GetArgInteger(0);
		if (Tokenizer_GetArgsCount() > 1) {
			len = Tokenizer_GetArgInteger(1);
			if (Tokenizer_GetArgsCount() > 2) {
				pattern = Tokenizer_GetArgInteger(2);
			}
		}
	}

	softSPI_t spi;
	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	SPI_Setup(&spi);
	unsigned int st = g_timeMs;
	flash_test_pages(&spi, addr, len, (byte)pattern);
	int ela = g_timeMs - st;
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_TestPages %i ms",ela);

	return CMD_RES_OK;
}


void DRV_InitFlashMemoryTestFunctions() {

	//cmddetail:{"name":"SPITestFlash_ReadID","args":"CMD_SPITestFlash_ReadID",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_SPITestFlash_ReadID","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_ReadID", CMD_SPITestFlash_ReadID, NULL);

	//cmddetail:{"name":"SPITestFlash_WriteStr","args":"CMD_SPITestFlash_WriteStr",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_SPITestFlash_WriteStr","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_WriteStr", CMD_SPITestFlash_WriteStr, NULL);

	//cmddetail:{"name":"SPITestFlash_Erase","args":"CMD_SPITestFlash_Erase",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_SPITestFlash_Erase","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_Erase", CMD_SPITestFlash_Erase, NULL);

	// backlog startDriver TESTSPIFLASH; SPITestFlash_ReadID
	// backlog startDriver TESTSPIFLASH; SPITestFlash_Test 0 1024
	// backlog startDriver TESTSPIFLASH; SPITestFlash_Test 0 16000
	// backlog startDriver TESTSPIFLASH; SPITestFlash_Test 0 65536
	// backlog startDriver TESTSPIFLASH; SPITestFlash_WriteStr 254 BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

	//cmddetail:{"name":"SPITestFlash_ReadData","args":"CMD_SPITestFlash_ReadData",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_SPITestFlash_ReadData","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_ReadData", CMD_SPITestFlash_ReadData, NULL);

	//cmddetail:{"name":"SPITestFlash_Test","args":"CMD_SPITestFlash_TestPages",
	//cmddetail:"descr":"TODO",
	//cmddetail:"fn":"CMD_SPITestFlash_TestPages","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_Test", CMD_SPITestFlash_TestPages, NULL);

	/*
	SPITestFlash_Erase
	SPITestFlash_ReadData 0 512
	SPITestFlash_WriteStr 256 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	SPITestFlash_WriteStr 254 BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
	
	*/
}


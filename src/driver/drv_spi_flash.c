#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_soft_spi.h"
#include "../driver/drv_soft_spi.h"

#define SCK_PIN 26   // SCK pin - D5
#define MISO_PIN 24  // MISO pin - D6
#define MOSI_PIN 6  // MOSI pin - D7
#define SS_PIN 7    // chip select pin for the SPI flash - D8

#define READ_FLASH_CMD 0x03 // Read Flash command opcode
#define WRITEENABLE_FLASH_CMD 0x06
#define ERASE_FLASH_CMD 0xC7
#define WRITE_FLASH_CMD 0x02

#define SPI_USLEEP(x)


#define OBK_ENABLE_INTERRUPTS 
#define OBK_DISABLE_INTERRUPTS

void spi_flash_read_id(softSPI_t* spi, byte* jedec_id) {
	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(spi);
	SPI_USLEEP(500);
	SPI_USLEEP(500);
	SPI_USLEEP(500);

	SPI_Begin(spi);
	SPI_Send(spi, 0x9F);
	jedec_id[0] = SPI_Read(spi);
	jedec_id[1] = SPI_Read(spi);
	jedec_id[2] = SPI_Read(spi);
	SPI_End(spi);

	OBK_ENABLE_INTERRUPTS;
}

void spi_flash_read(softSPI_t* spi, int adr, byte* out, int cnt) {
	int i;

	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(spi);
	SPI_USLEEP(500);
	SPI_USLEEP(500);
	SPI_USLEEP(500);

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

#define READ_STATUS_REG_CMD 0x05
#define STATUS_BUSY_MASK 0x01
#define STATUS_WEL_MASK 0x02


static void flash_wait_while_busy(softSPI_t* spi) {
	byte status;
	do {
		SPI_Begin(spi);
		SPI_Send(spi, READ_STATUS_REG_CMD);
		status = SPI_Read(spi);
		SPI_End(spi);
		//SPI_USLEEP(1000);
		rtos_delay_milliseconds(5);
	} while (status & STATUS_BUSY_MASK);
}
static void flash_wait_for_wel(softSPI_t* spi) {
	byte status;
	do {
		SPI_Begin(spi);
		SPI_Send(spi, READ_STATUS_REG_CMD);
		status = SPI_Read(spi);
		SPI_End(spi);
		//SPI_USLEEP(1000);
		rtos_delay_milliseconds(5);
	} while (status & STATUS_WEL_MASK);
}

void spi_flash_erase(softSPI_t* spi) {
	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(spi);
	SPI_USLEEP(500);
	SPI_USLEEP(500);

	SPI_Begin(spi);
	SPI_Send(spi, WRITEENABLE_FLASH_CMD);
	SPI_End(spi);
	SPI_USLEEP(500);

	SPI_Begin(spi);
	SPI_Send(spi, ERASE_FLASH_CMD);
	SPI_End(spi);

	flash_wait_while_busy(spi);

	OBK_ENABLE_INTERRUPTS;
}

void spi_flash_write2(softSPI_t* spi, int adr, const byte* data, int cnt) {
	int remaining = cnt;
	int offset = 0;

	OBK_DISABLE_INTERRUPTS;

	ADDLOG_INFO(LOG_FEATURE_CMD, "spi_flash_write2 %i at %i\n", cnt, adr);

	while (remaining > 0) {
		int page_offset = adr % 256;
		int write_len = 256 - page_offset;
		if (write_len > remaining)
			write_len = remaining;

		SPI_Setup(spi);
		SPI_USLEEP(500);

		SPI_Begin(spi);
		SPI_Send(spi, WRITEENABLE_FLASH_CMD);
		SPI_End(spi);
		SPI_USLEEP(500);

		ADDLOG_INFO(LOG_FEATURE_CMD, "Write fragmnent %i at %i\n", write_len, adr);
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
#define ERASE_SECTOR_CMD 0x20 // 4KB erase

void spi_flash_erase_sector(softSPI_t* spi, int addr) {
	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(spi);
	SPI_USLEEP(500);

	SPI_Begin(spi);
	SPI_Send(spi, WRITEENABLE_FLASH_CMD);
	SPI_End(spi);
	SPI_USLEEP(500);

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
			spi_flash_erase_sector(spi, addr);
		}
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "Erased flash.");

	spi_flash_read(spi, baseAddr, readBuf, length);
	for (i = 0; i < length; i++) {
		if (readBuf[i] != 0xFF)
			err++;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "Flash erased test: errors = %d/%d", err, length);

	err = 0;
	spi_flash_write2(spi, baseAddr, writeBuf, length);
	ADDLOG_INFO(LOG_FEATURE_CMD, "Wrote pattern %02X to flash.", pattern);

	spi_flash_read(spi, baseAddr, readBuf, length);
	for (i = 0; i < length; i++) {
		if (readBuf[i] != pattern)
			err++;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Flash write-read test: errors = %d/%d", err, length);

	free(writeBuf);
	free(readBuf);
}

void LFS_SPI_Flash_EraseSector(int addr) {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	spi_flash_erase_sector(&spi, addr);
}
void spi_test() {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;
	byte jedec_id[3];

	spi_flash_read_id(&spi, jedec_id);

	ADDLOG_INFO(LOG_FEATURE_CMD, "ID %02X %02X %02X", jedec_id[0], jedec_id[1], jedec_id[2]);
}
void LFS_SPI_Flash_Read(int adr, int cnt, byte *data) {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	spi_flash_read(&spi, adr, data, cnt);
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

	spi_flash_erase(&spi);
}

void LFS_SPI_Flash_Write(int adr, const byte *data, int cnt) {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	spi_flash_write2(&spi, adr, data, cnt);
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

	flash_test_pages(&spi, addr, len, (byte)pattern);

	return CMD_RES_OK;
}


void DRV_InitFlashMemoryTestFunctions() {

	//cmddetail:{"name":"SPITestFlash_ReadID","args":"CMD_SPITestFlash_ReadID",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_ReadID", CMD_SPITestFlash_ReadID, NULL);

	//cmddetail:{"name":"SPITestFlash_WriteStr","args":"CMD_SPITestFlash_WriteStr",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_WriteStr", CMD_SPITestFlash_WriteStr, NULL);

	//cmddetail:{"name":"SPITestFlash_Erase","args":"CMD_SPITestFlash_Erase",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_spi_flash.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_Erase", CMD_SPITestFlash_Erase, NULL);

	// backlog startDriver TESTSPIFLASH; SPITestFlash_ReadID
	// backlog startDriver TESTSPIFLASH; SPITestFlash_Test
	// backlog startDriver TESTSPIFLASH; SPITestFlash_WriteStr 254 BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

	//cmddetail:{"name":"SPITestFlash_ReadData","args":"CMD_SPITestFlash_ReadData",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_ReadData", CMD_SPITestFlash_ReadData, NULL);

	CMD_RegisterCommand("SPITestFlash_Test", CMD_SPITestFlash_TestPages, NULL);

	/*
	SPITestFlash_Erase
	SPITestFlash_ReadData 0 512
	SPITestFlash_WriteStr 256 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	SPITestFlash_WriteStr 254 BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
	
	*/
}


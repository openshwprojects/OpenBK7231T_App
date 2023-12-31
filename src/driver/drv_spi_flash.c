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

#ifdef PLATFORM_BEKEN
#define OBK_DISABLE_INTERRUPTS \
	GLOBAL_INT_DECLARATION(); \
	GLOBAL_INT_DISABLE();
#define OBK_ENABLE_INTERRUPTS \
	GLOBAL_INT_RESTORE();
#else
#define OBK_ENABLE_INTERRUPTS 
#define OBK_DISABLE_INTERRUPTS
#endif
void spi_test() {
	softSPI_t spi;


	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;
	byte jedec_id[3];

	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(&spi);
	usleep(500);
	usleep(500);
	usleep(500);

	SPI_Begin(&spi); // enable SPI communication with the flash
	SPI_Send(&spi, 0x9F); // send the JEDEC ID command
	jedec_id[0] = SPI_Read(&spi); // read manufacturer ID
	jedec_id[1] = SPI_Read(&spi); // read memory type
	jedec_id[2] = SPI_Read(&spi); // read capacity
	SPI_End(&spi); // disable SPI communication with the flash

	OBK_ENABLE_INTERRUPTS;

	ADDLOG_INFO(LOG_FEATURE_CMD, "ID %02X %02X %02X", jedec_id[0], jedec_id[1], jedec_id[2]);

}
void spi_test_read(int adr, int cnt) {
	byte *data;
	int i;
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	OBK_DISABLE_INTERRUPTS;

	data = malloc(cnt);
	SPI_Setup(&spi);
	usleep(500);
	usleep(500);
	usleep(500);

	SPI_Begin(&spi); // enable SPI communication with the flash
	SPI_Send(&spi, READ_FLASH_CMD); // send the Read Flash command
	SPI_Send(&spi, (adr >> 16) & 0xFF); // send the address MSB
	SPI_Send(&spi, (adr >> 8) & 0xFF); // send the address middle byte
	SPI_Send(&spi, adr & 0xFF); // send the address LSB
	for (i = 0; i < cnt; i++) {
		data[i] = SPI_Read(&spi);
	}
	SPI_End(&spi); // disable SPI communication with the flash

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
	OBK_ENABLE_INTERRUPTS;
	free(data);

}
void spi_test_erase() {
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(&spi);
	usleep(500);
	usleep(500);

	SPI_Begin(&spi); // enable SPI communication with the flash
	SPI_Send(&spi, WRITEENABLE_FLASH_CMD);
	SPI_End(&spi); // disable SPI communication with the flash
	usleep(500);

	SPI_Begin(&spi); // enable SPI communication with the flash
	SPI_Send(&spi, ERASE_FLASH_CMD);
	SPI_End(&spi); // disable SPI communication with the flash

	// THIS WILL TAKE TIME! YOU will have to wait (and check status register to see if it's ready)

}
void spi_test_write(int adr, const byte *data, int cnt) {
	int i;
	softSPI_t spi;

	spi.miso = MISO_PIN;
	spi.mosi = MOSI_PIN;
	spi.ss = SS_PIN;
	spi.sck = SCK_PIN;

	OBK_DISABLE_INTERRUPTS;

	SPI_Setup(&spi);
	usleep(500);
	usleep(500);
	usleep(500);

	SPI_Begin(&spi); // enable SPI communication with the flash
	SPI_Send(&spi, WRITEENABLE_FLASH_CMD);
	SPI_End(&spi); // disable SPI communication with the flash
	usleep(500);
	SPI_Begin(&spi); // enable SPI communication with the flash
	SPI_Send(&spi, WRITE_FLASH_CMD); // send the Read Flash command
	SPI_Send(&spi, (adr >> 16) & 0xFF); // send the address MSB
	SPI_Send(&spi, (adr >> 8) & 0xFF); // send the address middle byte
	SPI_Send(&spi, adr & 0xFF); // send the address LSB
	for (i = 0; i < cnt; i++) {
		SPI_Send(&spi, data[i]);
	}
	SPI_End(&spi); // disable SPI communication with the flash

	OBK_ENABLE_INTERRUPTS;
}
// SPITestFlash_ReadData 0 16
static commandResult_t CMD_SPITestFlash_ReadData(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int addr = 0;
	int len = 16;

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_ReadData");

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		addr = Tokenizer_GetArgInteger(0);
		if (Tokenizer_GetArgsCount() > 0) {
			len = Tokenizer_GetArgInteger(1);
		}
	}

	spi_test_read(addr, len);

	return CMD_RES_OK;
}
// SPITestFlash_Erase
static commandResult_t CMD_SPITestFlash_Erase(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_Erase");

	spi_test_erase();

	return CMD_RES_OK;
}
// SPITestFlash_WriteStr 0 
static commandResult_t CMD_SPITestFlash_WriteStr(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int addr = 0;
	const char *str = "This is a test";

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_WriteStr");

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		addr = Tokenizer_GetArgInteger(0);
		if (Tokenizer_GetArgsCount() > 0) {
			str = Tokenizer_GetArg(1);
		}
	}

	spi_test_write(addr, (const byte*)str, strlen(str));

	return CMD_RES_OK;
}

// SPITestFlash_ReadID
static commandResult_t CMD_SPITestFlash_ReadID(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SPITestFlash_ReadID");

	spi_test();

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

	//cmddetail:{"name":"SPITestFlash_ReadData","args":"CMD_SPITestFlash_ReadData",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPITestFlash_ReadData", CMD_SPITestFlash_ReadData, NULL);

}


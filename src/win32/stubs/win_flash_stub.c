#ifdef WINDOWS

#include "flash_pub.h"
#include "../../new_common.h"
void doNothing() {
}

#define FLASH_SIZE 2 * 1024 * 1024

char fname[512] = { 0 };
byte *g_flash = 0;
bool g_flashLoaded = false;
bool g_bFlashModified = false;

bool SIM_IsFlashModified() {
	return g_bFlashModified;
}
void allocFlashIfNeeded() {
	if (g_flash != 0) {
		return;
	}
	g_flash = (byte*)malloc(FLASH_SIZE);
	memset(g_flash, 0, FLASH_SIZE);
}
void SIM_SetupEmptyFlashModeNoFile() {
	if (g_flash != 0) {
		free(g_flash);
		g_flash = 0;
	}
	fname[0] = 0;
	allocFlashIfNeeded();
}

void SIM_SaveFlashData(const char *targetPath) {
	FILE *f;
	allocFlashIfNeeded();
	strcpy(fname, targetPath);
	if (fname[0]) {
		f = fopen(fname, "wb");
		fwrite(g_flash, FLASH_SIZE, 1, f);
		fclose(f);
		g_bFlashModified = false;
	}
}
void SIM_SetupFlashFileReading(const char *flashPath) {
	int loaded = 0;
	allocFlashIfNeeded();
	strcpy(fname, flashPath);
	FILE *f = fopen(fname, "rb");
	if (f != 0) {
		loaded = fread(g_flash, 1, FLASH_SIZE, f);
		if (loaded != FLASH_SIZE) {
			printf("SIM_SetupFlashFileReading: failed to read whole memory\n");
		}
		fclose(f);
		g_bFlashModified = false;
	}
}
void SIM_SetupNewFlashFile(const char *flashPath) {
	allocFlashIfNeeded();
	strcpy(fname, flashPath);
	SIM_SaveFlashData(fname);
	g_bFlashModified = false;
}


UINT32 flash_read(char *user_buf, UINT32 count, UINT32 address) {
	//FILE *f;

	allocFlashIfNeeded();

	memcpy(user_buf, g_flash + address, count);
	//f = fopen(fname,"rb");
	//fseek(f, address,SEEK_SET);
	//fread(user_buf, count, 1, f);
	//fclose(f);
	return 0;
}
int bekken_hal_flash_read(const uint32_t addr, uint8_t *dst, const uint32_t size) {
	return flash_read(dst, size, addr);;
}
UINT32 flash_write(char *user_buf, UINT32 count, UINT32 address) {

	allocFlashIfNeeded();
	if (memcmp(g_flash + address, user_buf, count)) {
		g_bFlashModified = true;
		memcpy(g_flash + address, user_buf, count);
	}
	return 0;
}
UINT32 flash_ctrl(UINT32 cmd, void *parm) {
	return 0;
}

#endif
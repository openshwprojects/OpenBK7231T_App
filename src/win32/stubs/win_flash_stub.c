#ifdef WINDOWS

#include "flash_pub.h"
#include "../../new_common.h"
void doNothing() {
}

#define FLASH_SIZE 2 * 1024 * 1024

const char *fname = "flash_image.bin";
byte *g_flash = 0;
bool g_flashLoaded = false;

void allocFlashIfNeeded() {
	if (g_flash != 0) {
		return;
	}
	g_flash = (byte*)malloc(FLASH_SIZE);
	memset(g_flash, 0, FLASH_SIZE);

	FILE *f = fopen(fname,"rb");
	if (f != 0) {
		fread(g_flash, 1, FLASH_SIZE, f);
		fclose(f);
	}
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
	FILE *f;

	allocFlashIfNeeded();

	memcpy(g_flash + address, user_buf, count);
	//createDefaultFlashIfNeeded();
	f = fopen(fname,"wb");
	fwrite(g_flash, FLASH_SIZE, 1, f);
	fclose(f);
	return 0;
}
UINT32 flash_ctrl(UINT32 cmd, void *parm) {
	return 0;
}

#endif
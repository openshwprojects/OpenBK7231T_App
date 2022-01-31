// stubs so I can compile HTTP server on Windows for testing and faster development
#include "new_common.h"

#if WINDOWS


int tuya_hal_flash_read(const uint32_t addr, uint8_t *dst, const uint32_t size) {
	memset(dst,0,size);
	return 0;
}

#endif
// stubs so I can compile HTTP server on Windows for testing and faster development
#include "new_common.h"

#if WINDOWS


int bekken_hal_flash_read(const uint32_t addr, uint8_t *dst, const uint32_t size) {
	int i;
	memset(dst,0,size);
	for(i = 0; i < size; i++){
		dst[i] = rand()%128;
	}
	return 0;
}

#endif
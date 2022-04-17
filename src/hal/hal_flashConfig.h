

#include "../new_common.h"

// debug
int config_get_tableOffsets(int tableID, int *outStart, int *outLen);
// copy of tuya_hal_flash_read (in BK7231T it was in SDK source, but in BK7231N it was in tuya lib)
int bekken_hal_flash_read(const unsigned int addr, void *dst, const unsigned int size);


int HAL_Configuration_ReadConfigMemory(void *target, int dataLen);
int HAL_Configuration_SaveConfigMemory(void *src, int dataLen);
void HAL_Configuration_GenerateMACForThisModule(unsigned char *out);



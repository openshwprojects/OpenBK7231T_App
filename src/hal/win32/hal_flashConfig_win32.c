#ifdef WINDOWS

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"

// TODO
#define MY_ADDR_OF_BK_PARTITION_NET_PARAM 0x1e1000

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){
	//FILE *f;

	//printf("[WIN32] HAL_Configuration_ReadConfigMemory will try to read memory!\n");

	//f = fopen("configMemory.bin","rb");
	//if(f == 0){
	//	memset(target,dataLen,0);
	//	return dataLen;
	//}
	//fread(target,dataLen,1,f);
	//fclose(f);

	flash_read(target, dataLen, MY_ADDR_OF_BK_PARTITION_NET_PARAM);
    return dataLen;
}



int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){

	//FILE *f;

	//printf("[WIN32] HAL_Configuration_SaveConfigMemory will save memory!\n");

	//f = fopen("configMemory.bin","wb");
	//if(f != 0){
	//	fwrite(src,dataLen,1,f);
	//	fclose(f);
	//}

	flash_write(src, dataLen, MY_ADDR_OF_BK_PARTITION_NET_PARAM);

    return dataLen;
}




#endif // WINDOWS



#include "../hal_flashConfig.h"


#include "drv_model_pub.h"
#include "net_param_pub.h"
#include "flash_pub.h"
#include "BkDriverFlash.h"
#include "BkDriverUart.h"

#include "../../logging/logging.h"

static SemaphoreHandle_t config_mutex = 0;

int config_get_tableOffsets(int tableID, int *outStart, int *outLen) {
	bk_logic_partition_t *pt;
	pt = bk_flash_get_info(tableID);
	*outStart = 0;
	*outLen = 0;
	if(pt == 0)
		return 1;
    *outStart = pt->partition_start_addr;
    *outLen = pt->partition_length;
	return 0;
}

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){
	UINT32 flashaddr, flashlen;
    UINT32 status;
    DD_HANDLE flash_handle;
	bk_logic_partition_t *pt = bk_flash_get_info(BK_PARTITION_NET_PARAM);

    flashaddr = pt->partition_start_addr;
    flashlen = pt->partition_length;

    if (dataLen > flashlen){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "HAL_Configuration_ReadConfigMemory - table too big - can't save");
        return 0;
    }

	hal_flash_lock();
    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    ddev_read(flash_handle, (char *)target, dataLen, flashaddr);
    ddev_close(flash_handle);
	hal_flash_unlock();

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_ReadConfigMemory: read %d bytes to %d", dataLen, flashaddr);

    return dataLen;
}




int bekken_hal_flash_read(const unsigned int addr, void *dst, const unsigned int size)
{
    UINT32 status;
    if(NULL == dst) {
        return 1;
    }
	hal_flash_lock();

    DD_HANDLE flash_handle;
    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    ddev_read(flash_handle, dst, size, addr);
    ddev_close(flash_handle);
    
	hal_flash_unlock();

    return 0;
}




int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){
	UINT32 flashaddr, flashlen;
	bk_logic_partition_t *pt = bk_flash_get_info(BK_PARTITION_NET_PARAM);

    flashaddr = pt->partition_start_addr;
    flashlen = pt->partition_length;

    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );

    if (dataLen > flashlen){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory - table too big - can't save");
        if (taken == pdTRUE) 
			xSemaphoreGive( config_mutex );
        return 0;
    }


	hal_flash_lock();
	bk_flash_enable_security(FLASH_PROTECT_NONE);
	bk_flash_erase(BK_PARTITION_NET_PARAM,0,dataLen);
	bk_flash_write(BK_PARTITION_NET_PARAM,0,(uint8_t *)src,dataLen);
	bk_flash_enable_security(FLASH_PROTECT_ALL);
	hal_flash_unlock();

    if (taken == pdTRUE)
		xSemaphoreGive( config_mutex );
    
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "HAL_Configuration_SaveConfigMemory: saved %d bytes to %d", dataLen, flashaddr);
    return dataLen;
}




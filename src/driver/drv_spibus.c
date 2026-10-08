#include "../obkdef.h"
#include "../obkhelper.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"

#include "../hal/hal_pins.h"
#include "../hal/hal_spi.h"

#include "drv_spibus.h"

#include "drv_local.h"

static uint8_t    SPIBus_init = 0;
static obk_list_t SPIBus_list;

// startDriver HWSPI
void HWSPI_Init() {
	
	obk_list_init(&SPIBus_list);
	SPIBus_init = 1;
	
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HW SPI Init Start");
	//
	/* Init HAL SPI */
	HAL_SPI_INIT();
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HW SPI Init Done");	
	/* Register all spi bus */	
	obk_spibus_t *hal_spi_bus;
	uint8_t hal_spi_bus_i = 0;	
	while (NULL != (hal_spi_bus = HAL_SPI_GETBUS(hal_spi_bus_i))) {			
	    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HW SPI [%s] get", hal_spi_bus->parent.name);
		
		if (obk_spi_bus_register(hal_spi_bus) == 0) addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "SPI bus [%s] register succesfull", hal_spi_bus->parent.name);
		else                                        addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "SPI bus [%s] allready registred!", hal_spi_bus->parent.name);
		
		hal_spi_bus_i++;
	}
}

void HWSPI_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	/*
	if(bPreState)
		return;
	hprintf255(request, "<h2>SHTC3 Temperature=%.1fC, Humidity=%.0f%%</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
	*/
}

// stopDriver HWSPI
void HWSPI_Stop() {
	SPIBus_init = 0;
	// TODO: clear list
	
}

/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */
obk_err_t obk_spi_bus_register(obk_spibus_t *bus)
{	
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}
	/* search? if bus name allready exists */
	obk_list_t *node = NULL;
    obk_list_for_each(node, &(SPIBus_list)) {
		obk_device_t *_dev = obk_list_entry(node, obk_device_t, list);
		if (strcmp(_dev->name, bus->parent.name) == 0) {			
			return SPI_RES_ALREADY_REGISTRED;
		}
	}
	// mutex ?
	
	obk_init_mutex(&bus->lock);
		 
	obk_list_insert_after(&(SPIBus_list), &(bus->parent.list));		
	return SPI_RES_OK;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */							   
obk_err_t obk_spi_bus_attach_device(obk_spidevice_t *device,
					            	const char      *bus_name,
						            void            *user_data)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}
	/* try to find object */
	obk_list_t *node = NULL;
    obk_list_for_each(node, &(SPIBus_list)) {
		obk_device_t *_dev = obk_list_entry(node, obk_device_t, list);
		if (strcmp(_dev->name, bus_name) == 0) {
			/* bus finded */
			device->bus = (obk_spibus_t *)_dev;
			/* init bus */
						
			return SPI_RES_OK;
		}
	}	
	return SPI_RES_BUS_NOT_FOUND;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_take_bus(obk_spidevice_t *device)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}	
	//return SPI_RES_OK;
	obk_err_t result = SPI_RES_OK;
	if (device == NULL) return SPI_RES_ERROR;
	if (device->bus == NULL) return SPI_RES_ERROR;
		
	result = obk_lock_mutex(&(device->bus->lock), OBK_WAITING_FOREVER);
	if (result != OBK_EOK) return SPI_RES_BUS_BUSY;
		
	/* configure SPI bus */
	if (device->bus->owner != device) {
		/* not the same owner as current, re-configure SPI bus */
        result = device->bus->ops->configure(device, &device->config);
		if (result == OBK_EOK) {
			/* set SPI bus owner */
            device->bus->owner = device;
		} else {
            /* configure SPI bus failed */
            return obk_unlock_mutex(&(device->bus->lock));
        }
	}	
	return result;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_release_bus(obk_spidevice_t *device)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}	
	//return SPI_RES_OK;
	if (device == NULL) return SPI_RES_ERROR;
    if (device->bus == NULL) return SPI_RES_ERROR;
    if (device->bus->owner != device) return SPI_RES_ERROR;

    /* release lock */
    return obk_unlock_mutex(&(device->bus->lock));	
	
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_take_device(obk_spidevice_t *device)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}	
    struct obk_spi_message message;

    if (device == NULL) return SPI_RES_ERROR;
    if (device->bus == NULL) return SPI_RES_ERROR;

    memset(&message, 0, sizeof(message));
    message.cs_take = 1;

    size_t result = device->bus->ops->xfer(device, &message);
    if (result < 0)
    {
        return (obk_err_t)result;
    }    
	return SPI_RES_OK;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_release_device(obk_spidevice_t *device)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}	
	struct obk_spi_message message;

    if (device == NULL) return SPI_RES_ERROR;
    if (device->bus == NULL) return SPI_RES_ERROR;

    memset(&message, 0, sizeof(message));
    message.cs_release = 1;

    size_t result = device->bus->ops->xfer(device, &message);
    if (result < 0)
    {
        return (obk_err_t)result;
    }    
	return SPI_RES_OK;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_bus_configure(obk_spidevice_t  *device)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}	
	obk_err_t result = OBK_ERROR;

    if (device->bus != NULL) {
        result = obk_lock_mutex(&(device->bus->lock), OBK_WAITING_FOREVER);
        if (result == OBK_EOK) {
            if (device->bus->owner == device) {
                /* current device is using, re-configure SPI bus */
                result = device->bus->ops->configure(device, &device->config);
                if (result != OBK_EOK) {
                    /* configure SPI bus failed */
                    //LOG_E("SPI device %s configuration failed", device->parent.parent.name);
                }
            } else {
                /* OBK_EBUSY is not an error condition and
                 * the configuration will take effect once the device has the bus
                 */
                result = OBK_EBUSY;
            }
            /* release lock */
            obk_unlock_mutex(&(device->bus->lock));
        }
    } else {
        result = OBK_EOK;
    }

    return result;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_configure(obk_spidevice_t  *device, 
                            obk_spi_config_t *cfg)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return SPI_RES_ERROR;
	}	
	if (device == NULL) return SPI_RES_ERROR;
	if (cfg    == NULL) return SPI_RES_ERROR;	
	
	/* If the configurations are the same, we don't need to set again. */
	if (device->config.data_width == cfg->data_width &&
        device->config.mode       == (cfg->mode & OBK_SPI_MODE_MASK) &&
        device->config.max_hz     == cfg->max_hz)
    {
        return SPI_RES_OK;
    }	
	/* set configuration */
    device->config.data_width = cfg->data_width;
    device->config.mode       = cfg->mode & OBK_SPI_MODE_MASK;
    device->config.max_hz     = cfg->max_hz;
	device->config.nss_pin    = cfg->nss_pin;
	device->config.nss_level  = cfg->nss_level;

    return obk_spi_bus_configure(device);
}								
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_spi_message_t *obk_spi_transfer_message(obk_spidevice_t  *device,
                                           obk_spi_message_t *message)
{
	if (SPIBus_init != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "HWSPI not started");	
		return (obk_spi_message_t *)SPI_RES_ERROR;
	}	
	obk_err_t result;
    struct obk_spi_message *index;

    if (device == NULL) return message;

    /* get first message */
    index = message;
    if (index == NULL)
        return index;

    /* transmit each SPI message */
    while (index != NULL) {
        /* transmit SPI message */
        result = device->bus->ops->xfer(device, index);
        if (result < 0){
            break;
        }

        index = index->next;
    }
    return index;
}
#if PLATFORM_ESP8266

#include "hal_spi_espidf.h"
#include "../hal_spi.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"
#include "../../driver/drv_spibus.h"
#include "esp8266/spi_register.h"
#include "esp8266/spi_struct.h"
#include "driver/spi.h"
#include "driver/gpio.h"

#ifdef OBK_USING_SPI1
static obk_spibus_t spi_bus1; // CSPI_HOST
#endif
static obk_spibus_t spi_bus2; // HSPI_HOST
#ifdef OBK_USING_SPI2
static struct esp8266_spi spi_bus_obj[] = {
#endif

#ifdef OBK_USING_SPI1
    {
		.spi_host = CSPI_HOST,
        .bus_name = "cspi",
        .spi_bus = &spi_bus1,
        .sck_pin = -1,
		.miso_pin = -1,
		.mosi_pin = -1,
		.nss_pin = -1,
    },
#endif /* BSP_USING_SPI1 */
#ifdef OBK_USING_SPI2
    {
		.spi_host = HSPI_HOST,
        .bus_name = "hspi",
        .spi_bus = &spi_bus2,
        .sck_pin = GPIO_Pin_14,
		.miso_pin = GPIO_Pin_12,
		.mosi_pin = GPIO_Pin_13,
		.nss_pin = -1,
    },
#endif /* BSP_USING_SPI2 */
};

/* private spi ops function */
obk_err_t HAL_SPI_CONFIGURE(obk_spidevice_t *device, obk_spi_config_t *configuration);
size_t HAL_SPI_XFER(obk_spidevice_t *device, obk_spi_message_t *message);
	
static struct obk_spi_ops esp8266_spi_ops =
{
    .configure = HAL_SPI_CONFIGURE,
    .xfer      = HAL_SPI_XFER,
};
/**
 * @brief Init SPIBus data structure
 */ 
void HAL_SPI_INIT(void) {
	
	for(int i = 0; i < (sizeof(spi_bus_obj)/sizeof(spi_bus_obj[0])); i++) {
        /* link to hal settings */
		memset((uint8_t*) spi_bus_obj[i].spi_bus, 0, sizeof(obk_spibus_t));
		memset((uint8_t*)&spi_bus_obj[i].config,  0, sizeof(spi_config_t));
		
		spi_bus_obj[i].spi_bus->parent.user_data = (void *)&spi_bus_obj[i];
		/* ops functions */
		spi_bus_obj[i].spi_bus->ops = &esp8266_spi_ops;
			
		/* copy bus name */
		memset(spi_bus_obj[i].spi_bus->parent.name, 0, OBK_NAME_MAX);
		memmove(spi_bus_obj[i].spi_bus->parent.name, spi_bus_obj[i].bus_name, strlen(spi_bus_obj[i].bus_name));
		spi_bus_obj[i].spi_bus->parent.list.next = NULL;
		spi_bus_obj[i].spi_bus->parent.list.prev = NULL;        
    }	
}
/**
 * @brief Return spibus
 */ 
obk_spibus_t *HAL_SPI_GETBUS(uint8_t bus_index) {
	if (bus_index > ((sizeof(spi_bus_obj)/sizeof(spi_bus_obj[0]))-1)) 
		return NULL;
	return  spi_bus_obj[bus_index].spi_bus;	
}
/**
 * @brief Configure HAL SPI
 */ 
obk_err_t HAL_SPI_CONFIGURE(obk_spidevice_t *device, obk_spi_config_t *configuration) {
    		
	if (device == NULL) return OBK_ERROR;
	if (configuration == NULL) return OBK_ERROR;
	
	obk_spibus_t * spi_bus = (obk_spibus_t *)device->bus;
    struct esp8266_spi *spi_device = (struct esp8266_spi *)spi_bus->parent.user_data;
    
//	spi_interface_t interface;      /*!< SPI bus interface */  
//  spi_intr_enable_t intr_enable;  /*!< check if enable SPI interrupt */  
//  spi_event_callback_t event_cb;  /*!< SPI interrupt event callback */  
//  spi_mode_t mode;                /*!< SPI mode */  
//  spi_clk_div_t clk_div;          /*!< SPI clock divider */    	

	/* master or slave */
    if(configuration->mode & OBK_SPI_SLAVE)
		spi_device->config.mode = SPI_SLAVE_MODE;
	else 
		spi_device->config.mode = SPI_MASTER_MODE;

    /* data_width */
	spi_device->data_width = configuration->data_width;
    
    /* baudrate */
    spi_device->config.clk_div = (80000000L / configuration->max_hz) - 1;
	if (spi_device->config.clk_div == 0) spi_device->config.clk_div = 1;
	if (spi_device->config.clk_div >= 40) spi_device->config.clk_div = 39; // check max div
    /* cpol and chpa*/
    if (configuration->mode & OBK_SPI_CPOL)
        spi_device->config.interface.cpol = SPI_CPOL_HIGH;
	else 
		spi_device->config.interface.cpol = SPI_CPOL_LOW;
	
    if (configuration->mode & OBK_SPI_CPHA)
        spi_device->config.interface.cpha = SPI_CPHA_HIGH;
	else 
		spi_device->config.interface.cpha = SPI_CPHA_LOW;

    /* MSB or LSB */
    if(configuration->mode & OBK_SPI_MSB)
	{
        spi_device->config.interface.bit_tx_order = 0; //1: LSB first; 0: MSB first
		spi_device->config.interface.bit_rx_order = 0;
		spi_device->config.interface.byte_tx_order = 0; //1: little-endian; 0: big_endian
		spi_device->config.interface.byte_rx_order = 0; //1: little-endian; 0: big_endian
 	} else {
        spi_device->config.interface.bit_tx_order = 1;
		spi_device->config.interface.bit_rx_order = 1;
		spi_device->config.interface.byte_tx_order = 0;
		spi_device->config.interface.byte_rx_order = 0;
	}	
	if (spi_device->mosi_pin != -1)
		spi_device->config.interface.mosi_en = 1;   /*!< MOSI line enable */
	else 
		spi_device->config.interface.mosi_en = 0;   /*!< MOSI line disable */
	
	/* TODO: check if spi-3 wire (shared MOSI/MISO) not supported ESP8266*/
	if ((spi_device->miso_pin == -1) || (configuration->mode & OBK_SPI_3WIRE))
		spi_device->config.interface.miso_en = 0;   /*!< MISO line disable */
	else
		spi_device->config.interface.miso_en = 1;   /*!< MISO line enable */
	
	spi_device->nss_pin   = configuration->nss_pin; 
	if (configuration->mode & OBK_SPI_CS_HIGH) 
		spi_device->nss_level = 1;
	else 
		spi_device->nss_level = 0;
	
	spi_device->config.interface.cs_en   = 0;       /*!< CS hal line always disable */
	if (spi_device->nss_pin != -1) {
		// Config for CS output
		gpio_config_t conf = {};
		conf.pin_bit_mask = 1ULL << (uint32_t)spi_device->nss_pin;
		conf.mode = GPIO_MODE_OUTPUT;
		conf.pull_up_en = GPIO_PULLUP_DISABLE;
		conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
		conf.intr_type = GPIO_INTR_DISABLE;
		gpio_config(&conf);
		if (!spi_device->nss_level)
			gpio_set_level(spi_device->nss_pin, 0x00L);
		else
			gpio_set_level(spi_device->nss_pin, 0x01L);
	}
	spi_device->config.event_cb = NULL;
    /* init SPI */
    spi_init(spi_device->spi_host, &spi_device->config);
	spi_set_dummy(spi_device->spi_host, 0);

    return OBK_EOK;
}
/**
 * @brief Transfer message HAL SPI
 */ 
size_t HAL_SPI_XFER(obk_spidevice_t *device, obk_spi_message_t *message) {

	obk_spibus_t * spi_bus = (obk_spibus_t *)device->bus;
    struct esp8266_spi *spi_device = (struct esp8266_spi *)spi_bus->parent.user_data;
	
	/* take CS */	
    if(message->cs_take) {
		if (!spi_device->nss_level)
			gpio_set_level(spi_device->nss_pin, 0x00L);
		else
			gpio_set_level(spi_device->nss_pin, 0x01L);
    }	
	
	uint32_t mosi_buf[16];
	uint32_t miso_buf[16];
	//memset((uint8_t*)mosi_buf, 0xFF, 64);
	//memset((uint8_t*)miso_buf, 0xFF, 64);
	
	const uint8_t * send_ptr = message->send_buf;
    uint8_t * recv_ptr = message->recv_buf;
    size_t size = message->length * message->repeat;
			
	spi_trans_t spi_trans_msg;
	spi_trans_msg.cmd = NULL;       /*!< SPI transmission command */  
    spi_trans_msg.addr = NULL;      /*!< SPI transmission address */  
    spi_trans_msg.mosi = mosi_buf;  /*!< SPI transmission MOSI buffer */  
    spi_trans_msg.miso = miso_buf;  /*!< SPI transmission MISO buffer */  
    spi_trans_msg.bits.cmd  = 0;    /*!< SPI transmission command bits */  
    spi_trans_msg.bits.addr = 0;    /*!< SPI transmission address bits */ 
    //spi_trans_msg.bits.mosi = 0;    /*!< SPI transmission MOSI buffer bits */  
    //spi_trans_msg.bits.miso = 0;    /*!< SPI transmission MISO buffer bits */ 

	size_t _m_length = message->length;	
	
    while(size > 0) {		
		
		int _size;
		if (size >= 64) _size = 64;
		else            _size = size;
		size -= _size;
		spi_trans_msg.bits.mosi = _size << 3;
		spi_trans_msg.bits.miso = 0; // ?
		
		if (send_ptr!= NULL) {
			uint8_t* _mosi_buf = (uint8_t*)mosi_buf;			
			while (message->repeat > 0) {
				if (_size <= _m_length) {
					memmove(_mosi_buf, send_ptr, _size);
					send_ptr = send_ptr + _size;
					_mosi_buf = _mosi_buf + _size;
					_m_length -= _size;
					if (_m_length == 0) {
						_m_length = message->length;
						send_ptr = message->send_buf;
						message->repeat--;
					}
					_size = 0;
				} else {
					memmove(_mosi_buf, send_ptr, _m_length);
					_mosi_buf = _mosi_buf + _m_length;
					_m_length = message->length;
					send_ptr = message->send_buf;
					_size -= _m_length;
				}				
				if (_size == 0) break;
			}		
		} else {
			memset((uint8_t*)mosi_buf, 0xFF, _size);
		}
		
		
		spi_trans(spi_device->spi_host, &spi_trans_msg);
		
		if (recv_ptr!= NULL) {
			
			memmove(recv_ptr, (uint8_t*)miso_buf, _size);			
			recv_ptr = recv_ptr + _size;
			
		}
		
    }

    /* release CS */
    if(message->cs_release) {
		if (!spi_device->nss_level)
			gpio_set_level(spi_device->nss_pin, 0x01L);
		else
			gpio_set_level(spi_device->nss_pin, 0x00L);
    }

    return message->length;
}

#endif //PLATFORM_ESP8266
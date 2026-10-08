#if PLATFORM_ESP8266

#include "../hal_spi.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"
#include "driver/spi.h"
#include "driver/gpio.h"


/* esp8266 spi dirver class */
struct esp8266_spi {
    spi_host_t    		spi_host;
    char               *bus_name;
    
    obk_spibus_t       *spi_bus;
    spi_config_t        config;
	
	uint8_t 	        data_width;
	
    int16_t 			sck_pin;
    int16_t 			miso_pin;
    int16_t 			mosi_pin;
	int16_t 			nss_pin;
	
	uint8_t  nss_level;  //high (1) or low (0) active level for slave select line
	
};

#endif
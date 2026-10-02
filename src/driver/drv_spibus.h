#pragma once

#include "../obkdef.h"
#include "../new_common.h"
#include "../hal/hal_os_wrapper.h"
#include <stdint.h>

/**
 * @brief SPI message structure
 */
typedef struct obk_spi_message {
    const void             *send_buf;
    void                   *recv_buf;
    size_t                  length;
	size_t                  repeat;
	
    struct obk_spi_message *next;

    unsigned 				cs_take    : 1;
    unsigned 				cs_release : 1;
} obk_spi_message_t;

/**
 * @brief SPI result
 */
typedef enum spi_Result_e {
	SPI_RES_OK = 0,               /**< SPI no error */
	SPI_RES_ERROR,                /**< SPI error */
	SPI_RES_ALREADY_REGISTRED,    /**< SPI bus name already registred */
	SPI_RES_BUS_NOT_FOUND,        /**< SPI bus with name not found/registred */
	SPI_RES_BUS_BUSY,             /**< SPI bus busy */
} spi_Result_t;

/**
 * @brief SPI configuration structure
 */
/**
 * At CPOL=0 the base value of the clock is zero
 *  - For CPHA=0, data are captured on the clock's rising edge (low->high transition)
 *    and data are propagated on a falling edge (high->low clock transition).
 *  - For CPHA=1, data are captured on the clock's falling edge and data are
 *    propagated on a rising edge.
 * At CPOL=1 the base value of the clock is one (inversion of CPOL=0)
 *  - For CPHA=0, data are captured on clock's falling edge and data are propagated
 *    on a rising edge.
 *  - For CPHA=1, data are captured on clock's rising edge and data are propagated
 *    on a falling edge.
 */
#define OBK_SPI_CPHA     (1<<0)                             /*!< bit[0]:CPHA, clock phase */
#define OBK_SPI_CPOL     (1<<1)                             /*!< bit[1]:CPOL, clock polarity */

#define OBK_SPI_LSB      (0<<2)                             /*!< bit[2]: 0-LSB */
#define OBK_SPI_MSB      (1<<2)                             /*!< bit[2]: 1-MSB */

#define OBK_SPI_MASTER   (0<<3)                             /*!< SPI master device */
#define OBK_SPI_SLAVE    (1<<3)                             /*!< SPI slave device */

#define OBK_SPI_CS_HIGH  (1<<4)                             /*!< Chipselect active high */
#define OBK_SPI_NO_CS    (1<<5)                             /*!< No chipselect */
#define OBK_SPI_3WIRE    (1<<6)                             /*!< SI/SO pin shared */
#define OBK_SPI_READY    (1<<7)                             /*!< Slave pulls low to pause */

#define OBK_SPI_MODE_MASK    (OBK_SPI_CPHA | OBK_SPI_CPOL | OBK_SPI_MSB | OBK_SPI_SLAVE | OBK_SPI_CS_HIGH | OBK_SPI_NO_CS | OBK_SPI_3WIRE | OBK_SPI_READY)

#define OBK_SPI_MODE_0       (0 | 0)                        /*!< CPOL = 0, CPHA = 0 */
#define OBK_SPI_MODE_1       (0 | OBK_SPI_CPHA)             /*!< CPOL = 0, CPHA = 1 */
#define OBK_SPI_MODE_2       (OBK_SPI_CPOL | 0)             /*!< CPOL = 1, CPHA = 0 */
#define OBK_SPI_MODE_3       (OBK_SPI_CPOL | OBK_SPI_CPHA)  /*!< CPOL = 1, CPHA = 1 */

typedef struct obk_spi_configuration {
   uint8_t 	mode;
   uint8_t 	data_width;
   uint16_t reserved;
   uint32_t max_hz;
   
   int16_t 	nss_pin;    //pin address or -1, if disabled
   uint8_t  nss_level;  //high (1) or low (0) active level for slave select line
   
} obk_spi_config_t;

/**
 * @brief SPI bus structure
 */
struct obk_spi_ops;
struct obk_spidevice;
 
typedef struct obk_spibus {
	struct obk_device         parent;	
	const struct obk_spi_ops *ops;
	obk_mutex_t               lock;
	struct obk_spidevice     *owner;
	void                     *user_data;
} obk_spibus_t;

/**
 * @brief SPI operators
 */
struct obk_spi_ops {
    obk_err_t (*configure)(struct obk_spidevice *device, obk_spi_config_t *configuration);
    size_t (*xfer)(struct obk_spidevice *device, obk_spi_message_t *message);
};

/**
 * @brief SPI device structure
 */
typedef struct obk_spidevice {
	obk_device_t            *parent;
	obk_spibus_t            *bus;
	obk_spi_config_t         config;
    void                    *user_data;
} obk_spidevice_t;

/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */
obk_err_t obk_spi_bus_register(obk_spibus_t *bus);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */							   
obk_err_t obk_spi_bus_attach_device(obk_spidevice_t *device,
					            	const char      *bus_name,
						            void            *user_data);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_take_bus(obk_spidevice_t *device);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_release_bus(obk_spidevice_t *device);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_take_device(obk_spidevice_t *device);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_release_device(obk_spidevice_t *device);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_err_t obk_spi_configure(obk_spidevice_t  *device, 
                            obk_spi_config_t *cfg);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */								
obk_spi_message_t *obk_spi_transfer_message(obk_spidevice_t   *device,
                                            obk_spi_message_t *message);

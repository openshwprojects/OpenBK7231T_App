#pragma once

#include <stdint.h>

// spi_config_t and its member types are copied from BK7321N SPI implementation.
// Don't modify. If modified a mapping routine is required for BK7321N.

typedef enum {
	SPI_ROLE_SLAVE = 0, /**< SPI as slave */
	SPI_ROLE_MASTER,    /**< SPI as master */
} spi_role_t;

typedef enum {
	SPI_BIT_WIDTH_8BITS = 0, /**< SPI bit width 8bits */
	SPI_BIT_WIDTH_16BITS,    /**< SPI bit width 16bits */
} spi_bit_width_t;

typedef enum {
	SPI_POLARITY_LOW = 0, /**< SPI clock polarity low */
	SPI_POLARITY_HIGH,    /**< SPI clock polarity high */
} spi_polarity_t;

typedef enum {
	SPI_PHASE_1ST_EDGE = 0, /**< SPI clock phase the first edge */
	SPI_PHASE_2ND_EDGE,     /**< SPI clock phase the second edge */
} spi_phase_t;

// I think the SPI_3WIRE_MODE here means, that no CS is used. In contrast
// wikipedia says "Three-wire serial buses" use only one data line.
typedef enum {
	SPI_4WIRE_MODE = 0, /**< SPI four wire mode */
	SPI_3WIRE_MODE,     /**< SPI three wire mode */
} spi_wire_mode_t;

typedef enum {
	SPI_MSB_FIRST = 0, /**< SPI MSB first */
	SPI_LSB_FIRST,     /**< SPI LSB first */
} spi_bit_order_t;

typedef struct {
	spi_role_t role;           /**< SPI as master or slave */
	spi_bit_width_t bit_width; /**< SPI data bit witdth */
	spi_polarity_t polarity;   /**< SPI clock polarity */
	spi_phase_t phase;         /**< SPI clock phase */
	spi_wire_mode_t wire_mode; /**< SPI wire mode */
	uint32_t baud_rate;        /**< SPI transmit and receive SCK clock */
	spi_bit_order_t bit_order; /**< SPI bit order, MSB/LSB */
} spi_config_t;

int SPI_DriverInit(void);
int SPI_DriverDeinit(void);
int SPI_Init(const spi_config_t *config);
int SPI_Deinit(void);
int SPI_WriteBytes(const void *data, uint32_t size);
int SPI_ReadBytes(void *data, uint32_t size);
int SPI_Transmit(const void *txData, uint32_t txSize, void *rxData,
		uint32_t rxSize);

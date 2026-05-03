/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP IO expander
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IO_COUNT_MAX        (sizeof(uint32_t) * 8)

/**
 * @brief IO Expander Device Type
 */
typedef struct esp_io_expander_s esp_io_expander_t;
typedef esp_io_expander_t *esp_io_expander_handle_t;

/**
 * @brief IO Expander Pin Num
 */
typedef enum {
    IO_EXPANDER_PIN_NUM_0  = (1ULL << 0),
    IO_EXPANDER_PIN_NUM_1  = (1ULL << 1),
    IO_EXPANDER_PIN_NUM_2  = (1ULL << 2),
    IO_EXPANDER_PIN_NUM_3  = (1ULL << 3),
    IO_EXPANDER_PIN_NUM_4  = (1ULL << 4),
    IO_EXPANDER_PIN_NUM_5  = (1ULL << 5),
    IO_EXPANDER_PIN_NUM_6  = (1ULL << 6),
    IO_EXPANDER_PIN_NUM_7  = (1ULL << 7),
    IO_EXPANDER_PIN_NUM_8  = (1ULL << 8),
    IO_EXPANDER_PIN_NUM_9  = (1ULL << 9),
    IO_EXPANDER_PIN_NUM_10 = (1ULL << 10),
    IO_EXPANDER_PIN_NUM_11 = (1ULL << 11),
    IO_EXPANDER_PIN_NUM_12 = (1ULL << 12),
    IO_EXPANDER_PIN_NUM_13 = (1ULL << 13),
    IO_EXPANDER_PIN_NUM_14 = (1ULL << 14),
    IO_EXPANDER_PIN_NUM_15 = (1ULL << 15),
    IO_EXPANDER_PIN_NUM_16 = (1ULL << 16),
    IO_EXPANDER_PIN_NUM_17 = (1ULL << 17),
    IO_EXPANDER_PIN_NUM_18 = (1ULL << 18),
    IO_EXPANDER_PIN_NUM_19 = (1ULL << 19),
    IO_EXPANDER_PIN_NUM_20 = (1ULL << 20),
    IO_EXPANDER_PIN_NUM_21 = (1ULL << 21),
    IO_EXPANDER_PIN_NUM_22 = (1ULL << 22),
    IO_EXPANDER_PIN_NUM_23 = (1ULL << 23),
    IO_EXPANDER_PIN_NUM_24 = (1ULL << 24),
    IO_EXPANDER_PIN_NUM_25 = (1ULL << 25),
    IO_EXPANDER_PIN_NUM_26 = (1ULL << 26),
    IO_EXPANDER_PIN_NUM_27 = (1ULL << 27),
    IO_EXPANDER_PIN_NUM_28 = (1ULL << 28),
    IO_EXPANDER_PIN_NUM_29 = (1ULL << 29),
    IO_EXPANDER_PIN_NUM_30 = (1ULL << 30),
    IO_EXPANDER_PIN_NUM_31 = (1ULL << 31),
} esp_io_expander_pin_num_t;

/**
 * @brief IO Expander Pin direction
 */
typedef enum {
    IO_EXPANDER_INPUT,          /*!< Input direction */
    IO_EXPANDER_OUTPUT,         /*!< Output direction */
} esp_io_expander_dir_t;

/**
 * @brief IO Expander Configuration Type
 */
typedef struct {
    uint8_t io_count;                       /*!< Count of device's IO, must be less or equal than `IO_COUNT_MAX` */
    struct {
        uint8_t dir_out_bit_zero : 1;       /*!< If the direction of IO is output, the corresponding bit of the direction register is 0 */
        uint8_t input_high_bit_zero : 1;    /*!< If the input level of IO is high, the corresponding bit of the input register is 0 */
        uint8_t output_high_bit_zero : 1;   /*!< If the output level of IO is high, the corresponding bit of the output register is 0 */
    } flags;
    /* Don't support with interrupt mode yet, will be added soon */
} esp_io_expander_config_t;

struct esp_io_expander_s {

    /**
     * @brief Read value from input register (mandatory)
     *
     * @note The value represents the input level from IO
     * @note If there are multiple input registers in the device, their values should be spliced together in order to form the `value`.
     *
     * @param handle: IO Expander handle
     * @param value: Register's value
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*read_input_reg)(esp_io_expander_handle_t handle, uint32_t *value);

    /**
     * @brief Write value to output register (mandatory)
     *
     * @note The value represents the output level to IO
     * @note If there are multiple input registers in the device, their values should be spliced together in order to form the `value`.
     *
     * @param handle: IO Expander handle
     * @param value: Register's value
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*write_output_reg)(esp_io_expander_handle_t handle, uint32_t value);

    /**
     * @brief Read value from output register (mandatory)
     *
     * @note The value represents the expected output level to IO
     * @note This function can be implemented by reading the physical output register, or simply by reading a variable that record the output value (more faster)
     * @note If there are multiple input registers in the device, their values should be spliced together in order to form the `value`.
     *
     * @param handle: IO Expander handle
     * @param value: Register's value
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*read_output_reg)(esp_io_expander_handle_t handle, uint32_t *value);

    /**
     * @brief Write value to direction register (mandatory)
     *
     * @note The value represents the direction of IO
     * @note If there are multiple input registers in the device, their values should be spliced together in order to form the `value`.
     *
     * @param handle: IO Expander handle
     * @param value: Register's value
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*write_direction_reg)(esp_io_expander_handle_t handle, uint32_t value);

    /**
     * @brief Read value from directioin register (mandatory)
     *
     * @note The value represents the expected direction of IO
     * @note This function can be implemented by reading the physical direction register, or simply by reading a variable that record the direction value (more faster)
     * @note If there are multiple input registers in the device, their values should be spliced together in order to form the `value`.
     *
     * @param handle: IO Expander handle
     * @param value: Register's value
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*read_direction_reg)(esp_io_expander_handle_t handle, uint32_t *value);

    /**
     * @brief Reset the device to its initial state (mandatory)
     *
     * @note This function will reset all device's registers
     *
     * @param handle: IO Expander handle
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*reset)(esp_io_expander_handle_t handle);

    /**
     * @brief Delete device (mandatory)
     *
     * @param handle: IO Expander handle
     *
     * @return
     *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
     */
    esp_err_t (*del)(esp_io_expander_handle_t handle);

    /**
     * @brief Configuration structure
     */
    esp_io_expander_config_t config;
};

/**
 * @brief Set the direction of a set of target IOs
 *
 * @param handle: IO Exapnder handle
 * @param pin_num_mask: Bitwise OR of allowed pin num with type of `esp_io_expander_pin_num_t`
 * @param direction: IO direction (only support input or output now)
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_set_dir(esp_io_expander_handle_t handle, uint32_t pin_num_mask, esp_io_expander_dir_t direction);

/**
 * @brief Set the output level of a set of target IOs
 *
 * @note All target IOs must be in output mode first, otherwise this function will return the error `ESP_ERR_INVALID_STATE`
 *
 * @param handle: IO Exapnder handle
 * @param pin_num_mask: Bitwise OR of allowed pin num with type of `esp_io_expander_pin_num_t`
 * @param level: 0 - Low level, 1 - High level
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_set_level(esp_io_expander_handle_t handle, uint32_t pin_num_mask, uint8_t level);

/**
 * @brief Get the input level of a set of target IOs
 *
 * @note This function can be called whenever target IOs are in input mode or output mode
 *
 * @param handle: IO Exapnder handle
 * @param pin_num_mask: Bitwise OR of allowed pin num with type of `esp_io_expander_pin_num_t`
 * @param level_mask: Bitwise OR of levels. For each bit, 0 - Low level, 1 - High level
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_get_level(esp_io_expander_handle_t handle, uint32_t pin_num_mask, uint32_t *level_mask);

/**
 * @brief Print the current status of each IO of the device, including direction, input level and output level
 *
 * @param handle: IO Exapnder handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_print_state(esp_io_expander_handle_t handle);

/**
 * @brief Reset the device to its initial status
 *
 * @note This function will reset all device's registers
 *
 * @param handle: IO Expander handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_reset(esp_io_expander_handle_t handle);

/**
 * @brief Delete device
 *
 * @param handle: IO Expander handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_del(esp_io_expander_handle_t handle);

#ifdef __cplusplus
}
#endif

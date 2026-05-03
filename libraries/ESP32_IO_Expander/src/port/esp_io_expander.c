/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdlib.h>

#include "esp_bit_defs.h"
#include "esp_check.h"
#include "esp_log.h"

#include "esp_io_expander.h"

#include "esp_expander_utils.h"

#define VALID_IO_COUNT(handle)      ((handle)->config.io_count <= IO_COUNT_MAX ? (handle)->config.io_count : IO_COUNT_MAX)

/**
 * @brief Register type
 */
typedef enum {
    REG_INPUT = 0,
    REG_OUTPUT,
    REG_DIRECTION,
} reg_type_t;

static char *TAG = "io_expander";

static esp_err_t write_reg(esp_io_expander_handle_t handle, reg_type_t reg, uint32_t value);
static esp_err_t read_reg(esp_io_expander_handle_t handle, reg_type_t reg, uint32_t *value);

esp_err_t esp_io_expander_set_dir(esp_io_expander_handle_t handle, uint32_t pin_num_mask, esp_io_expander_dir_t direction)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    if (pin_num_mask >= BIT64(VALID_IO_COUNT(handle))) {
        ESP_LOGW(TAG, "Pin num mask out of range, bit higher than %d won't work", VALID_IO_COUNT(handle) - 1);
    }

    bool is_output = (direction == IO_EXPANDER_OUTPUT) ? true : false;
    uint32_t dir_reg, temp;
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_DIRECTION, &dir_reg), TAG, "Read direction reg failed");
    temp = dir_reg;
    if ((is_output && !handle->config.flags.dir_out_bit_zero) || (!is_output && handle->config.flags.dir_out_bit_zero)) {
        /* 1. Output && Set 1 to output */
        /* 2. Input && Set 1 to input */
        dir_reg |= pin_num_mask;
    } else {
        /* 3. Output && Set 0 to output */
        /* 4. Input && Set 0 to input */
        dir_reg &= ~pin_num_mask;
    }
    /* Write to reg only when different */
    if (dir_reg != temp) {
        ESP_RETURN_ON_ERROR(write_reg(handle, REG_DIRECTION, dir_reg), TAG, "Write direction reg failed");
    }

    return ESP_OK;
}

esp_err_t esp_io_expander_set_level(esp_io_expander_handle_t handle, uint32_t pin_num_mask, uint8_t level)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    if (pin_num_mask >= BIT64(VALID_IO_COUNT(handle))) {
        ESP_LOGW(TAG, "Pin num mask out of range, bit higher than %d won't work", VALID_IO_COUNT(handle) - 1);
    }

    uint32_t dir_reg, dir_bit;
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_DIRECTION, &dir_reg), TAG, "Read direction reg failed");

    uint8_t io_count = VALID_IO_COUNT(handle);
    /* Check every target pin's direction, must be in output mode */
    for (int i = 0; i < io_count; i++) {
        if (pin_num_mask & BIT(i)) {
            dir_bit = dir_reg & BIT(i);
            /* Check whether it is in input mode */
            if ((dir_bit && handle->config.flags.dir_out_bit_zero) || (!dir_bit && !handle->config.flags.dir_out_bit_zero)) {
                /* 1. 1 && Set 1 to input */
                /* 2. 0 && Set 0 to input */
                ESP_LOGE(TAG, "Pin[%d] can't set level in input mode", i);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    uint32_t output_reg, temp;
    /* Read the current output level */
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_OUTPUT, &output_reg), TAG, "Read Output reg failed");
    temp = output_reg;
    /* Set expected output level */
    if ((level && !handle->config.flags.output_high_bit_zero) || (!level && handle->config.flags.output_high_bit_zero)) {
        /* 1. High level && Set 1 to output high */
        /* 2. Low level && Set 1 to output low */
        output_reg |= pin_num_mask;
    } else {
        /* 3. High level && Set 0 to output high */
        /* 4. Low level && Set 0 to output low */
        output_reg &= ~pin_num_mask;
    }
    /* Write to reg only when different */
    if (output_reg != temp) {
        ESP_RETURN_ON_ERROR(write_reg(handle, REG_OUTPUT, output_reg), TAG, "Write Output reg failed");
    }

    return ESP_OK;
}

esp_err_t esp_io_expander_get_level(esp_io_expander_handle_t handle, uint32_t pin_num_mask, uint32_t *level_mask)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(level_mask, ESP_ERR_INVALID_ARG, TAG, "Invalid level");
    if (pin_num_mask >= BIT64(VALID_IO_COUNT(handle))) {
        ESP_LOGW(TAG, "Pin num mask out of range, bit higher than %d won't work", VALID_IO_COUNT(handle) - 1);
    }

    uint32_t input_reg;
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_INPUT, &input_reg), TAG, "Read input reg failed");
    if (!handle->config.flags.input_high_bit_zero) {
        /* Get 1 when input high level */
        *level_mask = input_reg & pin_num_mask;
    } else {
        /* Get 0 when input high level */
        *level_mask = ~input_reg & pin_num_mask;
    }

    return ESP_OK;
}

esp_err_t esp_io_expander_print_state(esp_io_expander_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    uint8_t io_count = VALID_IO_COUNT(handle);
    uint32_t input_reg, output_reg, dir_reg;
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_INPUT, &input_reg), TAG, "Read input reg failed");
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_OUTPUT, &output_reg), TAG, "Read output reg failed");
    ESP_RETURN_ON_ERROR(read_reg(handle, REG_DIRECTION, &dir_reg), TAG, "Read direction reg failed");
    /* Get 1 if high level */
    if (handle->config.flags.input_high_bit_zero) {
        input_reg ^= 0xffffffff;
    }
    /* Get 1 if high level */
    if (handle->config.flags.output_high_bit_zero) {
        output_reg ^= 0xffffffff;
    }
    /* Get 1 if output */
    if (handle->config.flags.dir_out_bit_zero) {
        dir_reg ^= 0xffffffff;
    }

    for (int i = 0; i < io_count; i++) {
        ESP_LOGI(TAG, "Index[%d] | Dir[%s] | In[%d] | Out[%d]", i, (dir_reg & BIT(i)) ? "Out" : "In",
                 (input_reg & BIT(i)) ? 1 : 0, (output_reg & BIT(i)) ? 1 : 0);
    }

    return ESP_OK;
}

esp_err_t esp_io_expander_reset(esp_io_expander_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(handle->reset, ESP_ERR_NOT_SUPPORTED, TAG, "reset isn't implemented");

    return handle->reset(handle);
}

esp_err_t esp_io_expander_del(esp_io_expander_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(handle->del, ESP_ERR_NOT_SUPPORTED, TAG, "del isn't implemented");

    return handle->del(handle);
}

/**
 * @brief Write the value to a specific register
 *
 * @param handle: IO Expander handle
 * @param reg: Specific type of register
 * @param value: Expected register's value
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
static esp_err_t write_reg(esp_io_expander_handle_t handle, reg_type_t reg, uint32_t value)
{
    switch (reg) {
    case REG_OUTPUT:
        ESP_RETURN_ON_FALSE(handle->write_output_reg, ESP_ERR_NOT_SUPPORTED, TAG, "write_output_reg isn't implemented");
        return handle->write_output_reg(handle, value);
    case REG_DIRECTION:
        ESP_RETURN_ON_FALSE(handle->write_direction_reg, ESP_ERR_NOT_SUPPORTED, TAG, "write_direction_reg isn't implemented");
        return handle->write_direction_reg(handle, value);
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

/**
 * @brief Read the value from a specific register
 *
 * @param handle: IO Expander handle
 * @param reg: Specific type of register
 * @param value: Actual register's value
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
static esp_err_t read_reg(esp_io_expander_handle_t handle, reg_type_t reg, uint32_t *value)
{
    ESP_RETURN_ON_FALSE(value, ESP_ERR_INVALID_ARG, TAG, "Invalid value");

    switch (reg) {
    case REG_INPUT:
        ESP_RETURN_ON_FALSE(handle->read_input_reg, ESP_ERR_NOT_SUPPORTED, TAG, "read_input_reg isn't implemented");
        return handle->read_input_reg(handle, value);
    case REG_OUTPUT:
        ESP_RETURN_ON_FALSE(handle->read_output_reg, ESP_ERR_NOT_SUPPORTED, TAG, "read_output_reg isn't implemented");
        return handle->read_output_reg(handle, value);
    case REG_DIRECTION:
        ESP_RETURN_ON_FALSE(handle->read_direction_reg, ESP_ERR_NOT_SUPPORTED, TAG, "read_direction_reg isn't implemented");
        return handle->read_direction_reg(handle, value);
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

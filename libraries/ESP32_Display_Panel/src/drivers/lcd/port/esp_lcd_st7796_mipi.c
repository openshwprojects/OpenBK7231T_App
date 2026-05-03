/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../esp_panel_lcd_conf_internal.h"
#if ESP_PANEL_DRIVERS_LCD_ENABLE_ST7796

#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_lcd_st7796.h"
#include "esp_lcd_st7796_interface.h"

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_panel_lcd_vendor_types.h"

typedef struct {
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_val; // save surrent value of LCD_CMD_COLMOD register
    const esp_panel_lcd_vendor_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        unsigned int reset_level: 1;
    } flags;
    // To save the original functions of MIPI DPI panel
    esp_err_t (*del)(esp_lcd_panel_t *panel);
    esp_err_t (*init)(esp_lcd_panel_t *panel);
} st7796_panel_t;

static const char *TAG = "st7796_mipi";

static esp_err_t panel_st7796_del(esp_lcd_panel_t *panel);
static esp_err_t panel_st7796_init(esp_lcd_panel_t *panel);
static esp_err_t panel_st7796_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_st7796_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_st7796_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_st7796_disp_on_off(esp_lcd_panel_t *panel, bool on_off);

esp_err_t esp_lcd_new_panel_st7796_mipi(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                        esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "invalid arguments");
    esp_panel_lcd_vendor_config_t *vendor_config = (esp_panel_lcd_vendor_config_t *)panel_dev_config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor_config && vendor_config->mipi_config.dpi_config && vendor_config->mipi_config.dsi_bus, ESP_ERR_INVALID_ARG, TAG,
                        "invalid vendor config");

    esp_err_t ret = ESP_OK;
    st7796_panel_t *st7796 = (st7796_panel_t *)calloc(1, sizeof(st7796_panel_t));
    ESP_RETURN_ON_FALSE(st7796, ESP_ERR_NO_MEM, TAG, "no mem for st7796 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->color_space) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        st7796->madctl_val = 0;
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        st7796->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        st7796->colmod_val = 0x55;
        break;
    case 18: // RGB666
        st7796->colmod_val = 0x66;
        break;
    case 24: // RGB888
        st7796->colmod_val = 0x77;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    st7796->io = io;
    st7796->init_cmds = vendor_config->init_cmds;
    st7796->init_cmds_size = vendor_config->init_cmds_size;
    st7796->reset_gpio_num = panel_dev_config->reset_gpio_num;
    st7796->flags.reset_level = panel_dev_config->flags.reset_active_high;

    // Create MIPI DPI panel
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_dpi(vendor_config->mipi_config.dsi_bus, vendor_config->mipi_config.dpi_config, &panel_handle), err, TAG,
                      "create MIPI DPI panel failed");
    ESP_LOGD(TAG, "new MIPI DPI panel @%p", panel_handle);

    // Save the original functions of MIPI DPI panel
    st7796->del = panel_handle->del;
    st7796->init = panel_handle->init;
    // Overwrite the functions of MIPI DPI panel
    panel_handle->del = panel_st7796_del;
    panel_handle->init = panel_st7796_init;
    panel_handle->reset = panel_st7796_reset;
    panel_handle->mirror = panel_st7796_mirror;
    panel_handle->invert_color = panel_st7796_invert_color;
    panel_handle->disp_on_off = panel_st7796_disp_on_off;
    panel_handle->user_data = st7796;
    *ret_panel = panel_handle;
    ESP_LOGD(TAG, "new st7796 panel @%p", st7796);

    return ESP_OK;

err:
    if (st7796) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(st7796);
    }
    return ret;
}

static const esp_panel_lcd_vendor_init_cmd_t vendor_specific_init_default[] = {
//  {cmd, { data }, data_size, delay_ms}
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x36, (uint8_t []){0x48}, 1, 0},
    {0x3A, (uint8_t []){0x77}, 1, 0},
    {0xF0, (uint8_t []){0xC3}, 1, 0},
    {0xF0, (uint8_t []){0x96}, 1, 0},
    {0xB4, (uint8_t []){0x02}, 1, 0},
    {0xB7, (uint8_t []){0xC6}, 1, 0},
    {0xB6, (uint8_t []){0x2F}, 1, 0},
    {0x11, (uint8_t []){0xC0, 0xF0, 0x35}, 3, 0},
    {0xC1, (uint8_t []){0x15}, 1, 0},
    {0xC2, (uint8_t []){0xAF}, 1, 0},
    {0xC3, (uint8_t []){0x09}, 1, 0},
    {0xC5, (uint8_t []){0x22}, 1, 0},
    {0xC6, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 9, 0},
    {0x11, (uint8_t []){0xE0, 0x70, 0x00, 0x05, 0x03, 0x02, 0x20, 0x29, 0x01, 0x45, 0x30, 0x09, 0x07, 0x22, 0x29}, 15, 0},
    {0x11, (uint8_t []){0xE1, 0x70, 0x0C, 0x10, 0x0F, 0x0E, 0x09, 0x35, 0x64, 0x48, 0x3A, 0x14, 0x13, 0x2E, 0x30}, 15, 0},
    {0x11, (uint8_t []){0xE0, 0x70, 0x04, 0x0A, 0x0B, 0x0A, 0x27, 0x31, 0x55, 0x47, 0x29, 0x13, 0x13, 0x29, 0x2D}, 15, 0},
    {0x11, (uint8_t []){0xE1, 0x70, 0x08, 0x0E, 0x09, 0x08, 0x04, 0x33, 0x32, 0x49, 0x36, 0x14, 0x14, 0x2A, 0x2F}, 15, 0},
    {0x21, (uint8_t []){0x00}, 0, 0},
    {0xF0, (uint8_t []){0xC3}, 1, 0},
    {0xF0, (uint8_t []){0x96}, 1, 120},
    {0xF0, (uint8_t []){0xC3}, 1, 0},
    {0x29, (uint8_t []){0x00}, 0, 0},
    {0x2C, (uint8_t []){0x00}, 0, 0},
    //============ Gamma END===========
};

static esp_err_t panel_st7796_del(esp_lcd_panel_t *panel)
{
    st7796_panel_t *st7796 = (st7796_panel_t *)panel->user_data;

    if (st7796->reset_gpio_num >= 0) {
        gpio_reset_pin(st7796->reset_gpio_num);
    }
    // Delete MIPI DPI panel
    st7796->del(panel);
    ESP_LOGD(TAG, "del st7796 panel @%p", st7796);
    free(st7796);

    return ESP_OK;
}

static esp_err_t panel_st7796_init(esp_lcd_panel_t *panel)
{
    st7796_panel_t *st7796 = (st7796_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7796->io;
    const esp_panel_lcd_vendor_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        st7796->madctl_val,
    }, 1), TAG, "send command failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, (uint8_t[]) {
        st7796->colmod_val,
    }, 1), TAG, "send command failed");

    // vendor specific initialization, it can be different between manufacturers
    // should consult the LCD supplier for initialization sequence code
    if (st7796->init_cmds) {
        init_cmds = st7796->init_cmds;
        init_cmds_size = st7796->init_cmds_size;
    } else {
        init_cmds = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(esp_panel_lcd_vendor_init_cmd_t);
    }

    for (int i = 0; i < init_cmds_size; i++) {
        // Send command
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
    }
    ESP_LOGD(TAG, "send init commands success");

    ESP_RETURN_ON_ERROR(st7796->init(panel), TAG, "init MIPI DPI panel failed");

    return ESP_OK;
}

static esp_err_t panel_st7796_reset(esp_lcd_panel_t *panel)
{
    st7796_panel_t *st7796 = (st7796_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7796->io;

    // Perform hardware reset
    if (st7796->reset_gpio_num >= 0) {
        gpio_set_level(st7796->reset_gpio_num, st7796->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(st7796->reset_gpio_num, !st7796->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(120));
    } else if (io) { // Perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    return ESP_OK;
}

static esp_err_t panel_st7796_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    st7796_panel_t *st7796 = (st7796_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7796->io;
    uint8_t command = 0;

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, TAG, "invalid panel IO");

    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG, "send command failed");

    return ESP_OK;
}

static esp_err_t panel_st7796_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    st7796_panel_t *st7796 = (st7796_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7796->io;
    uint8_t madctl_val = st7796->madctl_val;

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, TAG, "invalid panel IO");

    // Control mirror through LCD command
    if (mirror_x) {
        madctl_val |= LCD_CMD_MX_BIT;
    } else {
        madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        madctl_val |= LCD_CMD_MY_BIT;
    } else {
        madctl_val &= ~LCD_CMD_MY_BIT;
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t []) {
        madctl_val
    }, 1), TAG, "send command failed");
    st7796->madctl_val = madctl_val;

    return ESP_OK;
}

static esp_err_t panel_st7796_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    st7796_panel_t *st7796 = (st7796_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7796->io;
    int command = 0;

    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG, "send command failed");
    return ESP_OK;
}
#endif

#endif // ESP_PANEL_DRIVERS_LCD_ENABLE_ST7796

/*
 * drv_tm_custom.c
 * Implementation of custom TM1638 driver
 */

#include "drv_tm_custom.h"
#include "drv_gpio.h"
#include "drv_system.h"
#include "c_types.h"
#include "common.h"
#include "console.h"
#include "obk_config.h"
#include <string.h>
#include <stdio.h>

// ===== Hardware mapping =====
#define TM1638_CMD_DATA     0x40  // Write data to display
#define TM1638_CMD_ADDR     0xC0  // Base address for GRID registers
#define TM1638_CMD_BRIGHT   0x88  // Brightness command (OR with value 0-7)
#define TM1638_CMD_KEY      0x42  // Read key scan

// GRID addresses for your board (GRID1=0x00, GRID2=0x02, ... GRID5=0x08)
#define ADDR_LED            0x00  // GRID1 - LEDs
#define ADDR_D0             0x02  // GRID2 - rightmost digit
#define ADDR_D1             0x04  // GRID3
#define ADDR_D2             0x06  // GRID4
#define ADDR_D3             0x08  // GRID5 - leftmost digit

// ===== GPIO pins (set via console or config) =====
static uint8_t gpio_stb = 8;   // STB
static uint8_t gpio_clk = 9;   // CLK
static uint8_t gpio_dio = 10;  // DIO

// ===== State =====
static uint8_t current_brightness = 2;
static uint8_t led_state = 0;   // bits: 0=LED0g,1=LED0r,2=LED1r,3=LED1g

// ===== 7-segment font (standard: a=bit0, b=bit1, c=bit2, d=bit3, e=bit4, f=bit5, g=bit6, dp=bit7) =====
static const uint8_t SEGMENT_FONT[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71, // F
    0x00  // (space)
};

// ===== Low-level functions =====
static void tm1638_start(void) {
    gpio_write(gpio_stb, 1);
    gpio_write(gpio_dio, 1);
    gpio_write(gpio_clk, 1);
}

static void tm1638_stop(void) {
    gpio_write(gpio_stb, 0);
    gpio_write(gpio_clk, 1);
    gpio_write(gpio_dio, 1);
    gpio_write(gpio_clk, 0);
    gpio_write(gpio_stb, 1);
}

static void tm1638_write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        gpio_write(gpio_clk, 0);
        gpio_write(gpio_dio, (data >> i) & 1);
        gpio_write(gpio_clk, 1);
    }
}

static uint8_t tm1638_read_byte(void) {
    uint8_t data = 0;
    gpio_set_mode(gpio_dio, GPIO_MODE_INPUT);
    for (uint8_t i = 0; i < 8; i++) {
        gpio_write(gpio_clk, 0);
        if (gpio_read(gpio_dio)) data |= (1 << i);
        gpio_write(gpio_clk, 1);
    }
    gpio_set_mode(gpio_dio, GPIO_MODE_OUTPUT);
    return data;
}

static void tm1638_send_command(uint8_t cmd) {
    gpio_write(gpio_stb, 0);
    tm1638_write_byte(cmd);
    gpio_write(gpio_stb, 1);
}

static void tm1638_send_data(uint8_t addr, uint8_t data) {
    gpio_write(gpio_stb, 0);
    tm1638_write_byte(TM1638_CMD_ADDR | (addr & 0x0F));
    tm1638_write_byte(data);
    gpio_write(gpio_stb, 1);
}

static void tm1638_update_display(void) {
    // Write all GRID registers (0x00..0x09)
    gpio_write(gpio_stb, 0);
    tm1638_write_byte(TM1638_CMD_ADDR);
    
    // GRID1 - LEDs (we manage this separately)
    tm1638_write_byte(led_state);
    
    // GRID2 - D0 (rightmost)
    tm1638_write_byte(display_buffer[0]);
    
    // GRID3 - D1
    tm1638_write_byte(display_buffer[1]);
    
    // GRID4 - D2
    tm1638_write_byte(display_buffer[2]);
    
    // GRID5 - D3 (leftmost)
    tm1638_write_byte(display_buffer[3]);
    
    // Fill remaining GRIDs (6-9) with 0
    for (uint8_t i = 0; i < 5; i++) {
        tm1638_write_byte(0x00);
    }
    
    gpio_write(gpio_stb, 1);
}

// ===== Public API =====

void drv_tm_custom_init(void) {
    // Initialize GPIOs
    gpio_set_mode(gpio_stb, GPIO_MODE_OUTPUT);
    gpio_set_mode(gpio_clk, GPIO_MODE_OUTPUT);
    gpio_set_mode(gpio_dio, GPIO_MODE_OUTPUT);
    
    tm1638_start();
    
    // Set brightness
    drv_tm_custom_set_brightness(current_brightness);
    
    // Clear display
    drv_tm_custom_clear();
    
    console_printf("[TMC] Custom TM1638 driver initialized (STB=%d, CLK=%d, DIO=%d)\n", 
                   gpio_stb, gpio_clk, gpio_dio);
}

void drv_tm_custom_set_brightness(uint8_t brightness) {
    if (brightness > 7) brightness = 7;
    current_brightness = brightness;
    tm1638_send_command(TM1638_CMD_BRIGHT | brightness);
}

void drv_tm_custom_clear(void) {
    memset(display_buffer, 0, 4);
    led_state = 0;
    tm1638_update_display();
}

void drv_tm_custom_print(const char *text) {
    uint8_t len = strlen(text);
    uint8_t pos = 0;
    uint8_t buffer[4] = {0, 0, 0, 0};
    
    for (uint8_t i = 0; i < len && pos < 4; i++) {
        char c = text[i];
        
        if (c == '.') {
            // Apply decimal point to previous character
            if (pos > 0) buffer[pos-1] |= 0x80;
            continue;
        }
        
        if (c >= '0' && c <= '9') {
            buffer[pos] = SEGMENT_FONT[c - '0'];
        } else if (c >= 'A' && c <= 'F') {
            buffer[pos] = SEGMENT_FONT[10 + (c - 'A')];
        } else if (c >= 'a' && c <= 'f') {
            buffer[pos] = SEGMENT_FONT[10 + (c - 'a')];
        } else if (c == ' ') {
            buffer[pos] = 0x00;
        } else {
            buffer[pos] = SEGMENT_FONT[16]; // space for unknown
        }
        pos++;
    }
    
    // Copy to display buffer (left-justified)
    memcpy(display_buffer, buffer, 4);
    tm1638_update_display();
}

void drv_tm_custom_print_num(uint32_t num, bool leading_zeros) {
    uint8_t digits[4];
    for (int8_t i = 3; i >= 0; i--) {
        digits[i] = num % 10;
        num /= 10;
    }
    
    uint8_t start = 0;
    if (!leading_zeros) {
        while (start < 3 && digits[start] == 0) start++;
    }
    
    for (uint8_t i = 0; i < 4; i++) {
        if (i < start) {
            display_buffer[i] = 0x00;
        } else {
            display_buffer[i] = SEGMENT_FONT[digits[i]];
        }
    }
    tm1638_update_display();
}

void drv_tm_custom_set_led(uint8_t position, uint8_t color) {
    if (position > 1) return;
    
    if (position == 0) {
        // LED0: green=bit0, red=bit1
        led_state &= ~0x03;
        if (color & 0x01) led_state |= 0x01; // green
        if (color & 0x02) led_state |= 0x02; // red
    } else {
        // LED1: red=bit2, green=bit3
        led_state &= ~0x0C;
        if (color & 0x02) led_state |= 0x04; // red
        if (color & 0x01) led_state |= 0x08; // green
    }
    
    tm1638_send_data(ADDR_LED, led_state);
}

void drv_tm_custom_set_leds(uint8_t state) {
    led_state = state;
    tm1638_send_data(ADDR_LED, led_state);
}

uint32_t drv_tm_custom_read_keys(void) {
    uint32_t keys = 0;
    
    gpio_write(gpio_stb, 0);
    tm1638_write_byte(TM1638_CMD_KEY);
    gpio_set_mode(gpio_dio, GPIO_MODE_INPUT);
    
    // Read 4 bytes (K1..K8)
    for (uint8_t i = 0; i < 4; i++) {
        keys |= ((uint32_t)tm1638_read_byte() << (i * 8));
    }
    
    gpio_set_mode(gpio_dio, GPIO_MODE_OUTPUT);
    gpio_write(gpio_stb, 1);
    
    return keys;
}

// ===== Console commands =====

static void cmd_tmc_print(char *args) {
    if (!args || strlen(args) == 0) {
        console_printf("Usage: TMC_Print <text> (max 4 chars, supports '.')\n");
        return;
    }
    drv_tm_custom_print(args);
}

static void cmd_tmc_led(char *args) {
    uint8_t num, color;
    if (sscanf(args, "%hhu %hhu", &num, &color) != 2) {
        console_printf("Usage: TMC_LED <0|1> <0=off|1=green|2=red|3=orange>\n");
        return;
    }
    drv_tm_custom_set_led(num, color);
}

static void cmd_tmc_readkeys(char *args) {
    uint32_t keys = drv_tm_custom_read_keys();
    console_printf("Keys: 0x%08X\n", keys);
    console_printf("  K1: %02X, K2: %02X, K3: %02X, K4: %02X\n",
                   (uint8_t)(keys & 0xFF),
                   (uint8_t)((keys >> 8) & 0xFF),
                   (uint8_t)((keys >> 16) & 0xFF),
                   (uint8_t)((keys >> 24) & 0xFF));
}

static void cmd_tmc_bright(char *args) {
    uint8_t val;
    if (sscanf(args, "%hhu", &val) != 1 || val > 7) {
        console_printf("Usage: TMC_Brightness <0-7>\n");
        return;
    }
    drv_tm_custom_set_brightness(val);
}

static void cmd_tmc_clear(char *args) {
    drv_tm_custom_clear();
}

void drv_tm_custom_register_commands(void) {
    console_register_command("TMC_Print", cmd_tmc_print);
    console_register_command("TMC_LED", cmd_tmc_led);
    console_register_command("TMC_ReadKeys", cmd_tmc_readkeys);
    console_register_command("TMC_Brightness", cmd_tmc_bright);
    console_register_command("TMC_Clear", cmd_tmc_clear);
    console_printf("[TMC] Commands registered\n");
}

// ===== Auto-start on boot (optional) =====
void drv_tm_custom_auto_start(void) {
    drv_tm_custom_init();
    drv_tm_custom_register_commands();
    drv_tm_custom_print("    ");
}
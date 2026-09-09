/*
 * drv_tm_custom.h
 * Custom TM1638 driver for 4-digit display + 2 bi-color LEDs (GRID1, GRID2-GRID5)
 * Based on user's hardware:
 *   - GRID2-GRID5: 4-digit common cathode display (D0-D3)
 *   - GRID1: 2 bi-color LEDs (SEG1-SEG4)
 *   - Buttons/encoder on SEG2 (K1, K2)
 *
 * Console commands:
 *   TMC_Print <string>          - display text/numbers (supports decimal point)
 *   TMC_LED <num> <color>       - set LED (0=off,1=green,2=red,3=orange)
 *   TMC_ReadKeys                - read buttons/encoder state
 *   TMC_Brightness <0-7>        - set display brightness
 *   TMC_Clear                   - clear display and LEDs
 */

#ifndef _DRV_TM_CUSTOM_H_
#define _DRV_TM_CUSTOM_H_

#include <stdint.h>
#include <stdbool.h>

// Initialize driver
void drv_tm_custom_init(void);

// Display functions
void drv_tm_custom_print(const char *text);          // print string (supports '.')
void drv_tm_custom_print_num(uint32_t num, bool leading_zeros); // print number
void drv_tm_custom_clear(void);                      // clear display + LEDs

// LED control (position: 0 or 1, color: 0=off, 1=green, 2=red, 3=orange)
void drv_tm_custom_set_led(uint8_t position, uint8_t color);
void drv_tm_custom_set_leds(uint8_t led_state);      // bit0=LED0g, bit1=LED0r, bit2=LED1r, bit3=LED1g

// Brightness (0-7)
void drv_tm_custom_set_brightness(uint8_t brightness);

// Key reading (returns 32-bit key scan data)
uint32_t drv_tm_custom_read_keys(void);

// Register console commands
void drv_tm_custom_register_commands(void);

#endif
#pragma once

#include "../obkdef.h"
#include "drv_idisplay.h"
#include <stdint.h>


#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

#define RED    rgb565(255,   0,   0) // 0xf800
#define GREEN  rgb565(  0, 255,   0) // 0x07e0
#define BLUE   rgb565(  0,   0, 255) // 0x001f
#define BLACK  rgb565(  0,   0,   0) // 0x0000
#define WHITE  rgb565(255, 255, 255) // 0xffff
#define GRAY   rgb565(128, 128, 128) // 0x8410
#define YELLOW rgb565(255, 255,   0) // 0xFFE0
#define CYAN   rgb565(  0, 156, 209) // 0x04FA
#define PURPLE rgb565(128,   0, 128) // 0x8010

#define ST7789_MADCTRL_MY  (0x01<<7) //Page Address Order 
#define ST7789_MADCTRL_MX  (0x01<<6) //Column Address Order
#define ST7789_MADCTRL_MV  (0x01<<5) //Page/Column Order
#define ST7789_MADCTRL_ML  (0x01<<4) //Line Address Order
#define ST7789_MADCTRL_RGB (0x01<<3) //RGB/BGR Order
#define ST7789_MADCTRL_MH  (0x01<<2) //Display Data Latch Order

typedef enum {DIRECTION0, DIRECTION90, DIRECTION180, DIRECTION270} DIRECTION;

typedef enum {
	SCROLL_RIGHT = 1,
	SCROLL_LEFT = 2,
	SCROLL_DOWN = 3,
	SCROLL_UP = 4,
} SCROLL_TYPE_t;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;	
} TFT_color_t;

typedef struct {
	/* interface settings */
	obk_spidevice_t     	spidev;
	obk_display_renderer_t 	renderer;
	int16_t 			dc_pin;        /* Data/command line pin */
	int16_t 			bl_pin;        /* Backlight pin */
	int16_t 			rs_pin;        /* Reset pin */
	uint8_t 			bl_channel;    /* Backlight like OBK channel */
	uint8_t 			brightness;    /* Brightness */
	/* display params */
	uint16_t 			width;
	uint16_t 			height;
	uint16_t 			offsetx;
	uint16_t 			offsety;
	uint16_t            rotation;
	/* font params */
	uint16_t 			font_direction;
	uint16_t 			font_scale;	
	int16_t 			clip_x1;       /* clip text rect */
	int16_t 			clip_y1;       /* clip text rect */
	int16_t 			clip_x2;       /* clip text rect */
	int16_t 			clip_y2;       /* clip text rect */
	/* frame params */
//	bool 				use_frame_buffer;
	uint16_t   		   *frame_buffer;
	int16_t 			frame_x;       /* frame x position */
	int16_t 			frame_y;       /* frame y position */
	uint16_t 			frame_w;       /* frame width */
	uint16_t 			frame_h;       /* frame height */ 
} TFT_t;


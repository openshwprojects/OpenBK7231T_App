#pragma once

#include "../obkdef.h"
#include "../new_common.h"
#include "../hal/hal_os_wrapper.h"
#include <stdint.h>

/**
 * @brief Display render structure
 */
struct obk_render_ops;
 
typedef struct obk_display_renderer {
	obk_service_t                   parent;	
	const struct obk_renderer_ops  *ops;
	obk_mutex_t                     lock;	
	void                           *user_data;
} obk_display_renderer_t;

/**
 * @brief Display operators
 */
struct obk_renderer_ops {    	
    void (*displayGetInfo)(uint16_t *w, uint16_t *h);
	void (*displayOn)(void);
	void (*displayOff)(void);
	void (*setBrightness)(uint8_t brightness);
	void (*setFont)(void *font);
	void (*setFontSize)(uint8_t font_size);
	uint16_t (*getTextWidth)(void *font, uint8_t font_size, const char * ascii, uint8_t len);
	uint16_t (*getTextHeight)(void *font, uint8_t font_size);
	int8_t (*beginFrame)(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color);
	int8_t (*endFrame)(uint16_t *hash);
	void (*drawPixel)(uint16_t x, uint16_t y, uint16_t color);
	void (*drawLine)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
	void (*drawFillRect)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
	void (*drawBitmap)(uint16_t x,uint16_t y,uint16_t w,uint16_t h, uint16_t *bitmap, uint16_t rcolor, uint16_t bkcolor);
	int  (*drawChar)(uint16_t x, uint16_t y, uint8_t ascii, uint16_t charColor, uint16_t bkgColor);
	int  (*drawString)(uint16_t x, uint16_t y, uint8_t * ascii, uint16_t charColor, uint16_t bkgColor);
	void (*setClipRect)(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
	void (*drawBezierCubic)(double x1, double y1, double x2, double y2,
                            double x3, double y3, double x4, double y4,	uint16_t color);
	void (*drawArc)(double x1, double y1, double x2, double y2, double rx, double ry, double rotation, int larg_arc, int sweep, uint16_t color);
};

/**
 * @brief SPI device structure
 */
typedef struct obk_gui_service {
	obk_service_t             *parent;
	obk_display_renderer_t    *renderer;
	//obk_display_config_t     config;
    void                      *user_data;
} obk_gui_service_t;

/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */
obk_err_t obk_display_renderer_register(obk_display_renderer_t *renderer);
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */							   
obk_err_t obk_display_attach_gui(obk_gui_service_t *service,
					            const char         *display_name,
					            void               *user_data);
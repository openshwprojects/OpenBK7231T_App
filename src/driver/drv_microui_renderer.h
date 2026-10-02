#pragma once

#include "../obkdef.h"
#include "../new_common.h"
#include "../hal/hal_os_wrapper.h"
#include <stdint.h>

#include "drv_idisplay.h"
#include "drv_microui_core.h"


int r_get_display_info(obk_display_renderer_t *renderer, uint16_t *w, uint16_t *h);

int16_t r_get_text_width(obk_display_renderer_t *renderer, mu_Font font, uint8_t font_size, const char *text, int len) {  
  if (renderer == NULL) return font_size*len;
  if (renderer->ops->getTextWidth == NULL) return font_size*len;
  return renderer->ops->getTextWidth(font, font_size, text, len);
} 

int16_t r_get_text_height(obk_display_renderer_t *renderer, mu_Font font, uint8_t font_size) {
  if (renderer == NULL) return font_size;
  if (renderer->ops->getTextHeight == NULL) return font_size;
  return renderer->ops->getTextHeight(font, font_size);
}

void r_setBrightness(obk_display_renderer_t *renderer, uint8_t brightness) {
  if (renderer == NULL) return;
  if (renderer->ops->getTextHeight == NULL) return;
  return renderer->ops->setBrightness(brightness);
	
}
int8_t r_frame_support(obk_display_renderer_t *renderer) {
	return (renderer->ops->beginFrame != NULL);
};
int8_t r_begin_frame(obk_display_renderer_t *renderer, mu_Rect *rect, mu_Color *color);
int8_t r_end_frame(obk_display_renderer_t *renderer, uint16_t *hash);
void r_draw_rect(obk_display_renderer_t *renderer, mu_Rect *rect, mu_Color *color);
void r_draw_text(obk_display_renderer_t *renderer, const char *text, mu_Vec2 *pos, mu_Color *color, uint8_t font_size);
void r_draw_icon(obk_display_renderer_t *renderer, uint16_t icon_id, mu_Rect *rect, mu_Color *color);
void r_setClipRect(obk_display_renderer_t *renderer, mu_Rect *rect);
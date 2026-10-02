#include "../obkdef.h"
#include "../obkhelper.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"

#include "../hal/hal_pins.h"
#include "../hal/hal_spi.h"

#include "drv_local.h"

#include "drv_idisplay.h"
#include "drv_microui_core.h"

#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
static mu_Color last_color;

int r_get_display_info(obk_display_renderer_t *renderer, uint16_t *w, uint16_t *h) {
  *w = 0; *h = 0;
  if (renderer == NULL) return -1;
  if (renderer->ops->displayGetInfo == NULL) return -1;
  renderer->ops->displayGetInfo(w, h);
  return 0;  
};

int8_t r_begin_frame(obk_display_renderer_t *renderer, mu_Rect *rect, mu_Color *color) {
  if (renderer == NULL) return -1;
  if (renderer->ops->beginFrame == NULL) return -1;
  return renderer->ops->beginFrame(rect->x, rect->y, rect->w, rect->h, rgb565(color->r,color->g,color->b));
}

int8_t r_end_frame(obk_display_renderer_t *renderer, uint16_t *hash) {
  if (renderer == NULL) return -1;
  if (renderer->ops->endFrame == NULL) return -1;
  return renderer->ops->endFrame(hash);
}

void r_draw_rect(obk_display_renderer_t *renderer, mu_Rect *rect, mu_Color *color) {
  if (renderer == NULL) return;
  if (renderer->ops->drawFillRect == NULL) return;
  renderer->ops->drawFillRect(rect->x, rect->y, rect->x+rect->w-1, rect->y+rect->h-1, rgb565(color->r,color->g,color->b));
  copy_color(&last_color, color);
}

void r_draw_text(obk_display_renderer_t *renderer, const char *text, mu_Vec2 *pos, mu_Color *color, uint8_t font_size) { 
  if (renderer == NULL) return;
  if (renderer->ops->drawString == NULL) return;
  if (renderer->ops->setFontSize == NULL) return;
  renderer->ops->setFontSize(font_size);
  renderer->ops->drawString(pos->x, pos->y, (uint8_t *)text, rgb565(color->r,color->g,color->b), rgb565(last_color.r,last_color.g,last_color.b));
}

void r_draw_icon(obk_display_renderer_t *renderer, uint16_t icon_id, mu_Rect *rect, mu_Color *color){
  if (renderer == NULL) return;
  if (renderer->ops->drawLine == NULL) return;
  uint16_t _color = rgb565(color->r,color->g,color->b);
  double scale = (rect->h<rect->w ? rect->h:rect->w)/24.0; 
  double x0, x1; 
  double y0, y1; 
  switch (icon_id) {
	case MU_ICON_CLOSE: {
		renderer->ops->drawLine(rect->x, rect->y, rect->x+rect->w-1, rect->y+rect->h-1, rgb565(color->r,color->g,color->b));
		renderer->ops->drawLine(rect->x, rect->y+rect->h-1, rect->x+rect->w-1, rect->y, rgb565(color->r,color->g,color->b));
		break;
	}
	case MU_ICON_CHECK:{
		/*
		<path d="M5 12l5 5l10 -10" />
		*/
		x0 = rect->x + 5*scale; y0 = rect->y + 12*scale;
		x1 = x0+5*scale; y1 = y0+5*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+10*scale; y1 = y0-10*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);		
		break;
	}
	case MU_ICON_COLLAPSED:{
#if 1		
		/*
		<path d="M11 9l3 3l-3 3" />
        <path d="M3 12a9 9 0 1 0 18 0a9 9 0 0 0 -18 0" />		
		*/
		x0 = rect->x + 11*scale; y0 = rect->y + 9*scale;
		x1=x0+3*scale; y1=y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1=x0-3*scale; y1=y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		x0 = rect->x + 3*scale; y0 = rect->y + 12*scale;	
		x1=x0+18*scale; y1=y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 9*scale, 9*scale, 0, 1, 0, _color);
		x0=x1; y0=y1;
		x1=x0-18*scale; y1=y0-0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 9*scale, 9*scale, 0, 0, 0, _color);
#endif
#if 0	
		/*
		<path d="M3 12h12" />
		<path d="M11 8l4 4l-4 4" />
		<path d="M12 21a9 9 0 0 0 0 -18" />
		*/
		x0 = rect->x + 3*scale; y0 = rect->y + 12*scale;
		renderer->ops->drawLine(x0, y0, x0+12*scale, y0, _color);
		
		x0 = rect->x + 11*scale; y0 = rect->y + 8*scale;
		x1 = x0+4*scale; y1 = y0+4*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0-4*scale; y1 = y0+4*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		x0 = rect->x + 12*scale; y0 = rect->y + 21*scale;	
		renderer->ops->drawArc(x0, y0, x0+0*scale, y0-18*scale, 9*scale, 9*scale, 0, 0, 0, _color);
#endif	
		break;
	}
	case MU_ICON_EXPANDED:{
#if 1
		/*
		<path d="M15 11l-3 3l-3 -3" />
		<path d="M12 3a9 9 0 1 0 0 18a9 9 0 0 0 0 -18" />
		*/
		x0 = rect->x + 15*scale; y0 = rect->y + 11*scale;
		x1=x0-3*scale; y1=y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1=x0-3*scale; y1=y0-3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		x0 = rect->x + 12*scale; y0 = rect->y + 3*scale;	
		x1=x0-0*scale; y1=y0+18*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 9*scale, 9*scale, 0, 1, 0, _color);
		x0=x1; y0=y1;
		x1=x0-0*scale; y1=y0-18*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 9*scale, 9*scale, 0, 0, 0, _color);
#endif		
#if 0		
		/*
		<path d="M12 3v12" />
		<path d="M16 11l-4 4l-4 -4" />
		<path d="M3 12a9 9 0 0 0 18 0" />
		*/		
		x0 = rect->x + 12*scale; y0 = rect->y + 3*scale;
		renderer->ops->drawLine(x0, y0, x0, y0+12*scale, _color);
		
		x0 = rect->x + 16*scale; y0 = rect->y + 11*scale;
		x1 = x0-4*scale; y1 = y0+4*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0-4*scale; y1 = y0-4*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		x0 = rect->x + 3*scale; y0 = rect->y + 12*scale;	
		renderer->ops->drawArc(x0, y0, x0+18*scale, y0, 9*scale, 9*scale, 0, 0, 0, _color);		
#endif
		break;
	}	
	case MU_ICON_ALIEN:{
		
		/* svg
		<path d="M11 17a2.5 2.5 0 0 0 2 0" />
		<path d="M12 3c-4.664 0 -7.396 2.331 -7.862 5.595a11.816 11.816 0 0 0 2 8.592a10.777 10.777 0 0 0 3.199 3.064c1.666 1 3.664 1 5.33 0a10.777 10.777 0 0 0 3.199 -3.064a11.89 11.89 0 0 0 2 -8.592c-.466 -3.265 -3.198 -5.595 -7.862 -5.595l-.004 0" />
		<path d="M8 11l2 2" />
		<path d="M16 11l-2 2" />
		*/		
		_color = rgb565(0,255,0);
		
		x0 = rect->x + 11*scale; y0 = rect->y + 17*scale;	
		renderer->ops->drawArc(x0, y0, x0+2*scale, y0+0, 2.5*scale, 2.5*scale, 0, 0, 0, _color);
		
		x0 = rect->x + 12*scale; y0 = rect->y + 3*scale;
		x1 = x0-7.862*scale; y1 = y0+5.595*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0-4.664*scale, y0+0, x0-7.396*scale, y0+2.331*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1=x0+2*scale; y1=y0+8.592*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 11.816*scale, 11.816*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1=x0+3.199*scale; y1=y0+3.064*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 10.777*scale, 10.777*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1=x0+5.33*scale; y1=y0+0*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+1.666*scale, y0+1*scale, x0+3.664*scale, y0+1*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1=x0+3.199*scale; y1=y0-3.064*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 10.777*scale, 10.777*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1=x0+2*scale; y1=y0-8.592*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 11.89*scale, 11.89*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1=x0-7.862*scale; y1=y0-5.595*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0-0.466*scale, y0-3.265*scale, x0-3.198*scale, y0-5.595*scale, x1, y1, _color);
		x0=x1; y0=y1;		
		x1=x0-0.004*scale; y1=y0+0*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		x0 = rect->x + 8*scale; y0 = rect->y + 11*scale;
		x1=x0+2*scale; y1=y0+2*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		x0 = rect->x + 16*scale; y0 = rect->y + 11*scale;
		x1=x0-2*scale; y1=y0+2*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		break;
	}
	case MU_ICON_WIFI:{
		/*
		<path d="M12 18l.01 0" />
		<path d="M9.172 15.172a4 4 0 0 1 5.656 0" />
		<path d="M6.343 12.343a8 8 0 0 1 11.314 0" />
		<path d="M3.515 9.515c4.686 -4.687 12.284 -4.687 17 0" />
		*/
		x0 = rect->x + 12*scale; y0 = rect->y + 18*scale;
		renderer->ops->drawLine(x0, y0, x0+0.01*scale, y0, _color);

		x0 = rect->x + 9.172*scale; y0 = rect->y + 15.172*scale;
		x1=x0+5.656*scale; y1=y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 4*scale, 4*scale, 0, 0, 1, _color);

		x0 = rect->x + 6.343*scale; y0 = rect->y + 12.343*scale;
		x1=x0+11.314*scale; y1=y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 8*scale, 8*scale, 0, 0, 1, _color);
		
		x0 = rect->x + 3.515*scale; y0 = rect->y + 9.515*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+4.686*scale, y0-4.687*scale, x0+12.284*scale, y0-4.687*scale, x0+17*scale, y0, _color);
		
		break;
	}
	case MU_ICON_WIFI_OFF:{
		/*
		<path d="M12 18l.01 0" />
		<path d="M9.172 15.172a4 4 0 0 1 5.656 0" />
		<path d="M6.343 12.343a7.963 7.963 0 0 1 3.864 -2.14m4.163 .155a7.965 7.965 0 0 1 3.287 2" />
		<path d="M3.515 9.515a12 12 0 0 1 3.544 -2.455m3.101 -.92a12 12 0 0 1 10.325 3.374" />
		<path d="M3 3l18 18" />
		*/
		x0 = rect->x + 12*scale; y0 = rect->y + 18*scale;
		renderer->ops->drawLine(x0, y0, x0+0.01*scale, y0, _color);

		x0 = rect->x + 9.172*scale; y0 = rect->y + 15.172*scale;
		x1=x0+5.656*scale; y1=y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 4*scale, 4*scale, 0, 0, 1, _color);

		x0 = rect->x + 6.343*scale; y0 = rect->y + 12.343*scale;
		x1=x0+3.864*scale; y1=y0-2.14*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 7.963*scale, 7.963*scale, 0, 0, 1, _color);		
		x0 = x1 + 4.163*scale; y0 = y1 + 0.155*scale;
		x1=x0+3.287*scale; y1=y0+2*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 7.965*scale, 7.965*scale, 0, 0, 1, _color);
		
		x0 = rect->x + 3.515*scale; y0 = rect->y + 9.515*scale;
		x1=x0+3.544*scale; y1=y0-2.455*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 12*scale, 12*scale, 0, 0, 1, _color);		
		x0 = x1 + 3.101*scale; y0 = y1 - 0.92*scale;
		x1=x0+10.325*scale; y1=y0+3.374*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 12*scale, 12*scale, 0, 0, 1, _color);
		
		x0 = rect->x + 3*scale; y0 = rect->y + 3*scale;
		renderer->ops->drawLine(x0, y0, x0+18*scale, y0+18*scale, _color);
		
		break;
	}
	case MU_ICON_HEALTH_OK: {
		/*
		<path d="M19.5 13.572l-7.5 7.428l-2.896 -2.868m-6.117 -8.104a5 5 0 0 1 9.013 -3.022a5 5 0 1 1 7.5 6.572" />
		<path d="M3 13h2l2 3l2 -6l1 3h3" />
		*/
		x0 = rect->x + 19.5*scale; y0 = rect->y + 13.572*scale;
		x1 = x0-7.52*scale; y1 = y0+7.4281*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, rgb565(255,0,0));
		x0=x1; y0=y1;
		x1 = x0-2.868*scale; y1 = y0-2.868*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, rgb565(255,0,0));
		x0=x1-6.117*scale; y0=y1-8.104*scale;
		x1 = x0+9.013*scale; y1 = y0-3.022*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 5*scale, 5*scale, 0, 0, 1, rgb565(255,0,0));
		x0=x1; y0=y1;
		x1 = x0+7.5*scale; y1 = y0+6.572*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 5*scale, 5*scale, 0, 1, 1, rgb565(255,0,0));
		
		x0 = rect->x + 3*scale; y0 = rect->y + 13*scale;
		x1 = x0+2*scale; y1 = y0+0*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+2*scale; y1 = y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+2*scale; y1 = y0-6*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+1*scale; y1 = y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+3*scale; y1 = y0+0*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		break;
	}
    case MU_ICON_HEALTH_CHECK: {
		/* 
		<path d="M15.03 17l-3.03 3l-7.5 -7.428a5 5 0 1 1 7.5 -6.566a5 5 0 1 1 7.922 6.102" />
		<path d="M19 16v3" />
		<path d="M19 22v.01" />
		*/
		x0 = rect->x + 15.03*scale; y0 = rect->y + 17*scale;
		x1 = x0-3.03*scale; y1 = y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, rgb565(255,0,0));
		x0=x1; y0=y1;
		x1 = x0-7.5*scale; y1 = y0-7.428*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, rgb565(255,0,0));
		x0=x1; y0=y1;
		x1 = x0+7.5*scale; y1 = y0-6.566*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 5*scale, 5*scale, 0, 1, 1, rgb565(255,0,0));
		x0=x1; y0=y1;
		x1 = x0+7.922*scale; y1 = y0+6.102*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 5*scale, 5*scale, 0, 1, 1, rgb565(255,0,0));
		
		x0 = rect->x + 19*scale; y0 = rect->y + 16*scale;
		x1 = x0+0*scale; y1 = y0+3*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		x0 = rect->x + 19*scale; y0 = rect->y + 22*scale;
		x1 = x0+0*scale; y1 = y0+0.01*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		
		break;
	}
	case MU_ICON_SETTINGS: {
		/*
		<path d="M10.325 4.317
		c.426 -1.756 2.924 -1.756 3.35 0
		a1.724 1.724 0 0 0 2.573 1.066
		c1.543 -.94 3.31 .826 2.37 2.37
		a1.724 1.724 0 0 0 1.065 2.572		
		c1.756 .426 1.756 2.924 0 3.35		
		a1.724 1.724 0 0 0 -1.066 2.573		
		c.94 1.543 -.826 3.31 -2.37 2.37		
		a1.724 1.724 0 0 0 -2.572 1.065		
		c-.426 1.756 -2.924 1.756 -3.35 0
		
		a1.724 1.724 0 0 0 -2.573 -1.066
		c-1.543 .94 -3.31 -.826 -2.37 -2.37
		a1.724 1.724 0 0 0 -1.065 -2.572
		c-1.756 -.426 -1.756 -2.924 0 -3.35
		a1.724 1.724 0 0 0 1.066 -2.573
		c-.94 -1.543 .826 -3.31 2.37 -2.37
		c1 .608 2.296 .07 2.572 -1.065" />
        <path d="M9 12a3 3 0 1 0 6 0a3 3 0 0 0 -6 0" />
		*/
		x0 = rect->x + 10.325*scale; y0 = rect->y + 4.317*scale;
		x1 = x0+3.35*scale; y1 = y0+0*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+0.426*scale, y0-1.756*scale, x0+2.924*scale, y0-1.756*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+2.573*scale; y1 = y0+1.066*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1 = x0+2.37*scale; y1 = y0+2.37*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+1.543*scale, y0-0.94*scale, x0+3.31*scale, y0+0.826*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+1.065*scale; y1 = y0+2.572*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;		
		x1 = x0+0*scale; y1 = y0+3.35*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+1.756*scale, y0+0.426*scale, x0+1.756*scale, y0+2.924*scale, x1, y1, _color);
		x0=x1; y0=y1;		
		x1 = x0-1.066*scale; y1 = y0+2.573*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;		
		x1 = x0-2.37*scale; y1 = y0+2.37*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+0.94*scale, y0+1.543*scale, x0-0.826*scale, y0+3.31*scale, x1, y1, _color);
		x0=x1; y0=y1;		
		x1 = x0-2.572*scale; y1 = y0+1.065*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;		
		x1 = x0-3.35*scale; y1 = y0+0*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0-0.426*scale, y0+1.756*scale, x0-2.924*scale, y0+1.756*scale, x1, y1, _color);
		x0=x1; y0=y1;		
		x1 = x0-2.573*scale; y1 = y0-1.066*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1 = x0-2.37*scale; y1 = y0-2.37*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0-1.543*scale, y0+0.94*scale, x0-3.31*scale, y0-0.826*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0-1.065*scale; y1 = y0-2.572*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1 = x0+0*scale; y1 = y0-3.35*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0-1.756*scale, y0-0.426*scale, x0-1.756*scale, y0-2.924*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+1.066*scale; y1 = y0-2.573*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 1.724*scale, 1.724*scale, 0, 0, 0, _color);
		x0=x1; y0=y1;
		x1 = x0+2.37*scale; y1 = y0-2.37*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0-0.94*scale, y0-1.543*scale, x0+0.826*scale, y0-3.31*scale, x1, y1, _color);
		x0=x1; y0=y1;
		x1 = x0+2.572*scale; y1 = y0-1.065*scale;
		renderer->ops->drawBezierCubic(x0, y0, x0+1*scale, y0+0.608*scale, x0+2.296*scale, y0+0.07*scale, x1, y1, _color);
		
		x0 = rect->x + 9*scale; y0 = rect->y + 12*scale;
		x1 = x0+6*scale; y1 = y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 3*scale, 3*scale, 0, 1, 0, _color);
		x0=x1; y0=y1;
		x1 = x0-6*scale; y1 = y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 3*scale, 3*scale, 0, 0, 0, _color);
		
		break;
	}
	case MU_ICON_POWER: {
		/*
		<path d="M7 6a7.75 7.75 0 1 0 10 0" />
		<path d="M12 4l0 8" />
		*/
		x0 = rect->x + 7*scale; y0 = rect->y + 6*scale;
		x1 = x0+10*scale; y1 = y0+0*scale;
		renderer->ops->drawArc(x0, y0, x1, y1, 7.75*scale, 7.75*scale, 0, 1, 0, _color);
		x0 = rect->x + 12*scale; y0 = rect->y + 4*scale;
		x1 = x0-0*scale; y1 = y0+8*scale;
		renderer->ops->drawLine(x0, y0, x1, y1, _color);
		break;
	}
	case MU_ICON_RESET: {
		/*
		<path d="M7 6a8 8 0 1 0 10 0" />
		<path d="M7 13a1 1 0 1 0 0 -4v7m2 0l-2 -4" />
		<path d="M11 15a1 1 0 0 0 1 1h0a1 1 0 0 0 1 -1v-2a1 1 0 0 0 -1 -1h0a1 1 0 0 1 -1 -1v-1a1 1 0 0 1 1 -1h0a1 1 0 0 1 1 1" />
		<path d="M15 9h2" />
		<path d="M16 9v7" />
		*/
		break;
	}	
  }
  
}

void r_setClipRect(obk_display_renderer_t *renderer, mu_Rect *rect) {
  if (renderer == NULL) return;
  if (renderer->ops->setClipRect == NULL) return;
  renderer->ops->setClipRect(rect->x, rect->y, rect->x+rect->w-1, rect->y+rect->h-1);
}

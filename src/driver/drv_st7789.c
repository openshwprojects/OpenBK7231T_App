#include "../obkdef.h"
#include "../obkhelper.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"

#include "../hal/hal_pins.h"

#include "drv_spibus.h"
#include "drv_st7789.h"
#include "drv_st7789_font7x15.h"


#define SWAPBYTE(u16) (((u16>>8)&0xFF) | (u16<<8))
#define lcdMIN(a, b)            ((a) < (b) ? (a) : (b))
#define lcdMAX(a, b)            ((a) > (b) ? (a) : (b))

static TFT_t tft;

void lcdInit(TFT_t * dev);
int8_t lcdBeginFrame(TFT_t * dev, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t bgColor);
int8_t lcdEndFrame(TFT_t * dev, uint16_t *hash);
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color);
void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDisplayOn(TFT_t * dev);
void lcdDisplayOff(TFT_t * dev);
void lcdSetBrightness(TFT_t * dev, uint8_t brightness);
void lcdDrawBitmap(TFT_t * dev, uint16_t x,uint16_t y,uint16_t w,uint16_t h, uint16_t *bitmap, uint16_t rcolor, uint16_t bkcolor);
int lcdDrawChar(TFT_t * dev, uint16_t x, uint16_t y, uint8_t ascii, uint16_t charColor, uint16_t bkgColor);
int lcdDrawString(TFT_t * dev, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t charColor, uint16_t bkgColor);
void lcdDrawBezierCubic(TFT_t * dev, 
                        double x1, double y1, double x2, double y2,
                        double x3, double y3, double x4, double y4,	
						uint16_t color);
void lcdDrawArc(TFT_t * dev, double x1, double y1, double x2, double y2, double rx, double ry, double rotation, int large_arc, int sweep, uint16_t color);
void lcdSetClipRect(TFT_t * dev, int16_t x1, int16_t y1, int16_t x2, int16_t y2);

void ops_displayGetInfo(uint16_t *w, uint16_t *h) {
	*w = tft.width;
	*h = tft.height;
}

void ops_displayOn(void) {	
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDisplayOn(&tft);
	obk_unlock_mutex(&(tft.renderer.lock));
}

void ops_displayOff(void) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDisplayOff(&tft);
	obk_unlock_mutex(&(tft.renderer.lock));
}
void ops_setBrightness(uint8_t brightness) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdSetBrightness(&tft, brightness);
	obk_unlock_mutex(&(tft.renderer.lock));	
}

void ops_setFont(void *font){
	// TODO:
}		
	
void ops_setFontSize(uint8_t font_size) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	tft.font_scale = font_size;
	obk_unlock_mutex(&(tft.renderer.lock));
}

uint16_t ops_getTextWidth(void *font, uint8_t font_size, const char *ascii, uint8_t len) {
	int res = 0;
	for (const char *p = (const char *)ascii; *p && len--; p++) {
		res += 7*font_size;
	}
	return res;
}
	
uint16_t ops_getTextHeight(void *font, uint8_t font_size) {
	return 13*font_size;
}

int8_t ops_BeginFrame(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t bgColor)
{
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return -1;
	return lcdBeginFrame(&tft, x, y, w, h, bgColor);
	obk_unlock_mutex(&(tft.renderer.lock));
}

int8_t ops_EndFrame(uint16_t *hash){
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return -1;
	return lcdEndFrame(&tft, hash);
	obk_unlock_mutex(&(tft.renderer.lock));	
}

void ops_drawPixel(uint16_t x, uint16_t y, uint16_t color) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDrawPixel(&tft, x, y, color);
	obk_unlock_mutex(&(tft.renderer.lock));
}
void ops_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDrawLine(&tft, x1, y1, x2, y2, color);
	obk_unlock_mutex(&(tft.renderer.lock));
}
void ops_drawFillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDrawFillRect(&tft, x1, y1, x2, y2, color);
	obk_unlock_mutex(&(tft.renderer.lock));
}
void ops_drawBitmap(uint16_t x,uint16_t y,uint16_t w,uint16_t h, uint16_t *bitmap, uint16_t rcolor, uint16_t bkcolor) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDrawBitmap(&tft, x, y, w, h, bitmap, rcolor, bkcolor);
	obk_unlock_mutex(&(tft.renderer.lock));	
}
int  ops_drawChar(uint16_t x, uint16_t y, uint8_t ascii, uint16_t charColor, uint16_t bkgColor) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return -1;
	int result = lcdDrawChar(&tft, x, y, ascii, charColor, bkgColor);
	obk_unlock_mutex(&(tft.renderer.lock));	
	return result;	
}
int  ops_drawString(uint16_t x, uint16_t y, uint8_t * ascii, uint16_t charColor, uint16_t bkgColor) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return -1;
	int result = lcdDrawString(&tft, x, y, ascii, charColor, bkgColor);
	obk_unlock_mutex(&(tft.renderer.lock));	
	return result;
}
void ops_drawBezierCubic( 
                        double x1, double y1, double x2, double y2,
                        double x3, double y3, double x4, double y4,	
						uint16_t color) 
{
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDrawBezierCubic( &tft, x1, y1, x2, y2, x3, y3, x4, y4, color);
	obk_unlock_mutex(&(tft.renderer.lock));	
}

void ops_drawArc(double x1, double y1, double x2, double y2, double rx, double ry, double rotation, int large_arc, int sweep, uint16_t color)
{
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdDrawArc( &tft, x1, y1, x2, y2, rx, ry, rotation, large_arc, sweep, color);
	obk_unlock_mutex(&(tft.renderer.lock));	
}
							
void ops_setClipRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	if (obk_lock_mutex(&(tft.renderer.lock), OBK_WAITING_FOREVER)) return;
	lcdSetClipRect(&tft, x1, y1, x2, y2);
	obk_unlock_mutex(&(tft.renderer.lock));	
}

static struct obk_renderer_ops obk_st7789_renderer_ops  =
{
	.displayGetInfo  = ops_displayGetInfo,
    .displayOn       = ops_displayOn,
    .displayOff      = ops_displayOff,
	.setBrightness   = ops_setBrightness,
	.setFont         = ops_setFont,
	.setFontSize     = ops_setFontSize,
	.getTextWidth    = ops_getTextWidth,
	.getTextHeight   = ops_getTextHeight,
	.beginFrame      = ops_BeginFrame,
	.endFrame        = ops_EndFrame,
	.drawPixel       = ops_drawPixel,
	.drawLine        = ops_drawLine,	
	.drawFillRect    = ops_drawFillRect,
	.drawBitmap      = ops_drawBitmap,
	.drawChar        = ops_drawChar,
	.drawString      = ops_drawString,
	.setClipRect     = ops_setClipRect,
	.drawBezierCubic = ops_drawBezierCubic,
	.drawArc         = ops_drawArc,
};

// startDriver ST7789 hspi NA IO15 IO2 NA|CH4 135 240 52 40 180
// startDriver ST7789 [SPI_NAME] [CS_PIN] [CS_LEVEL] [DC_PIN] [BL_PIN] [RESET_PIN] [DYSP_WIDTH] [DYSP_HEIGHT] [DYSP_W_OFS] [DYSP_H_OFS] [ROTATION]
void st7789_Init() {
	int arg_cnt = Tokenizer_GetArgsCount();
	int arg_i = 0;
	if(arg_cnt < 1) {
    	 ADDLOG_INFO(LOG_FEATURE_CMD, "\"startdriver ST7789\" needs at least one argument <SPI_NAME>, given ony %i" , Tokenizer_GetArgsCount() -1 );
    	 return;
    }
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789 driver init start...");
	/* TODO: load config from file st7789.json */
	/* spi & interface */
	tft.spidev.config.mode = OBK_SPI_MODE_3|OBK_SPI_MSB|OBK_SPI_3WIRE;
	tft.spidev.config.data_width = 8;
	tft.spidev.config.max_hz = 20000000L;	
	if (arg_i <= arg_cnt) arg_i++; //SPI NAME: hspi or cspi (normally not used) for esp8266, spi1, spi2 and etc. for other
	const char *spi_bus = Tokenizer_GetArg(arg_i);
	if (arg_i <= arg_cnt) arg_i++; //CS_PIN
	int nss_obk_pin = HAL_PIN_Find(Tokenizer_GetArg(arg_i));
	if (nss_obk_pin != -1)
		/* ! nss_pin must be actual GPIO pin, not obk pin index*/
		tft.spidev.config.nss_pin = HAL_GetGPIOPin(nss_obk_pin);
	else 
		tft.spidev.config.nss_pin = -1;
		
	/* TODO: nss_level */ 
	//if (Tokenizer_GetPin(3, 0)) tft.spidev.config.mode |= OBK_SPI_CS_HIGH;
	if (arg_i <= arg_cnt) arg_i++;
	tft.dc_pin = HAL_PIN_Find(Tokenizer_GetArg(arg_i));  /* Data\command line pin (obk pin index)*/
	if (arg_i <= arg_cnt) arg_i++;
	tft.rs_pin = HAL_PIN_Find(Tokenizer_GetArg(arg_i));  /* Reset pin (obk pin index)*/
	if (arg_i <= arg_cnt) arg_i++;
	
	tft.bl_channel = 0;
	if (arg_i > arg_cnt) {		
		tft.bl_pin = -1; /* Backlight not avaliable */
	} else {
		tft.bl_pin = HAL_PIN_Find(Tokenizer_GetArg(arg_i));  /* Backlight pin (obk pin index) or OBK_Channel*/	
		if (tft.bl_pin == -1) { // try parse it like channel
			char *bl_name = Tokenizer_GetArg(arg_i);
			if ((bl_name[0] =='C') && (bl_name[1] =='H')) {
				tft.bl_pin = atoi(bl_name+2);				
				tft.bl_channel = 1;
			}
		}
	}
	/* display params */
	if (arg_i <= arg_cnt) arg_i++;
	if (arg_i > arg_cnt) tft.width    = 240;//135;
	else tft.width   = Tokenizer_GetArgIntegerDefault(arg_i, 320); /* display width */
	if (arg_i <= arg_cnt) arg_i++;
	if (arg_i > arg_cnt) tft.height   = 135;//240;
	else tft.height  = Tokenizer_GetArgIntegerDefault(arg_i, 240); /* display height */
	if (arg_i <= arg_cnt) arg_i++;
	if (arg_i > arg_cnt) tft.offsetx  = 40;//52;
	else tft.offsetx = Tokenizer_GetArgIntegerDefault(arg_i, 0); /* display width ofset */
	if (arg_i <= arg_cnt) arg_i++;
	if (arg_i > arg_cnt) tft.offsety  = 52;//40;
	else tft.offsety = Tokenizer_GetArgIntegerDefault(arg_i, 0); /* display height ofset */
	if (arg_i <= arg_cnt) arg_i++;
	if (arg_i > arg_cnt) tft.rotation = 90;
	else tft.rotation = Tokenizer_GetArgIntegerDefault(arg_i, 0); /* display rotaion: 0, 90, 180, 270 */
	
	tft.font_direction = 0;
	tft.font_scale = 2;	
	tft.frame_buffer = NULL;
	tft.frame_w = 0;
	tft.frame_h = 0;
	tft.clip_x1 = 0;
	tft.clip_y1 = 0;
	tft.clip_x2 = tft.width-1;
	tft.clip_y2 = tft.height-1;
	
	tft.brightness = 50;
	
	if (tft.dc_pin != -1) {
		HAL_PIN_Setup_Output(tft.dc_pin);
		HAL_PIN_SetOutputValue(tft.dc_pin, 0);
	}	
	if (tft.bl_pin != -1) {
		if (tft.bl_channel) {
			//CHANNEL_Set_FloatPWM(tft.bl_pin, 0, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
			CHANNEL_Set(SPECIAL_CHANNEL_LEDPOWER, 0, 0);
			CHANNEL_Set(SPECIAL_CHANNEL_BRIGHTNESS, 0, 0);			
		} else {
			HAL_PIN_Setup_Output(tft.bl_pin);
			HAL_PIN_SetOutputValue(tft.bl_pin, 0);
		}
	}
	if (tft.rs_pin != -1) {
		HAL_PIN_Setup_Output(tft.rs_pin);
		HAL_PIN_SetOutputValue(tft.rs_pin, 1);
	}
		
	/* Attach to spi bus */	
	if (obk_spi_bus_attach_device(&tft.spidev, spi_bus, NULL) != OBK_EOK) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "ST7789 can't attach to bus [%s]" , spi_bus);
		return;
	}	
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789 attached to bus [%s], CS=IO%d, DC=IO%d, RS=IO%d, BL=%s%d" , spi_bus, 
	                                                                   tft.spidev.config.nss_pin, 
																	   HAL_GetGPIOPin(tft.dc_pin), 
																	   HAL_GetGPIOPin(tft.rs_pin),
																	   tft.bl_channel ? "СH":"IO",
																	   tft.bl_pin);
	
	memset(tft.renderer.parent.name, 0, OBK_NAME_MAX);
	memmove(tft.renderer.parent.name, "st7789", 6);
	tft.renderer.parent.list.next = NULL;
	tft.renderer.parent.list.prev = NULL; 
	tft.renderer.ops = &obk_st7789_renderer_ops;
	obk_display_renderer_register(&tft.renderer);
	
	lcdInit(&tft);	
	lcdDrawFillRect(&tft, 0, 0, tft.width-1, tft.height-1, BLACK);	
	lcdDrawString(&tft, (tft.width>>1)-(3*14), (tft.height>>1)-13, (uint8_t *)"ST7789", BLUE, BLACK);
	lcdDisplayOn(&tft);
	lcdSetBrightness(&tft, 50);
	//tft.font_scale = 1;
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789 driver init done.");
}

void st7789_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	/*
	if(bPreState)
		return;
	hprintf255(request, "<h2>SHTC3 Temperature=%.1fC, Humidity=%.0f%%</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
	*/
}

// stopDriver ST7789
void st7789_Stop() {
	
	
}

/**
 * @brief lcd_write_command 
 *
 * @param TFT_t * dev, uint8_t cmd
 *   
 * @return obk_err_t
 */
obk_err_t lcd_write_command(TFT_t * dev, uint8_t cmd)
{
	uint8_t xfer_buf = cmd;	
	
	HAL_PIN_SetOutputValue(dev->dc_pin, 0); /* command mode */	
	
	obk_spi_message_t message;
	message.send_buf = (uint8_t*)&xfer_buf;
	message.recv_buf = NULL;
	message.length   = 1;
	message.repeat   = 1;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;
	
	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return  OBK_EOK;
}

obk_err_t lcd_write_data_08(TFT_t * dev, uint8_t data)
{
	uint8_t xfer_buf = data;	
	
	HAL_PIN_SetOutputValue(dev->dc_pin, 1); /* data mode */
		
	obk_spi_message_t message;
	message.send_buf = (uint8_t*)&xfer_buf;
	message.recv_buf = NULL;
	message.length   = 1;
	message.repeat   = 1;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;
	
	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return  OBK_EOK;
}

obk_err_t lcd_write_data_16(TFT_t * dev, uint16_t data)
{
	uint16_t xfer_buf = data;	
	
	HAL_PIN_SetOutputValue(dev->dc_pin, 1); /* data mode */
	
	obk_spi_message_t message;
	message.send_buf = (uint8_t*)&xfer_buf;
	message.recv_buf = NULL;
	message.length   = 2;
	message.repeat   = 1;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;
	
	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return OBK_EOK;
}

obk_err_t lcd_write_data_32(TFT_t * dev, uint32_t data)
{
	uint32_t xfer_buf = data;	
	
	HAL_PIN_SetOutputValue(dev->dc_pin, 1); /* data mode */
	
	obk_spi_message_t message;
	message.send_buf = (uint8_t*)&xfer_buf;
	message.recv_buf = NULL;
	message.length   = 4;
	message.repeat   = 1;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;
	
	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return OBK_EOK;  
}

obk_err_t lcd_write_addr(TFT_t * dev, uint16_t addr1, uint16_t addr2)
{
	uint8_t xfer_buf[4];
	xfer_buf[0] = (addr1 >> 8) & 0xFF;
	xfer_buf[1] = addr1 & 0xFF;
	xfer_buf[2] = (addr2 >> 8) & 0xFF;
	xfer_buf[3] = addr2 & 0xFF;
	
	HAL_PIN_SetOutputValue(dev->dc_pin, 1); /* data mode */
	
	obk_spi_message_t message;
	message.send_buf = xfer_buf;
	message.recv_buf = NULL;
	message.length   = 4;
	message.repeat   = 1;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;
	
	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return OBK_EOK;
}

obk_err_t lcd_write_color(TFT_t * dev, uint16_t color, uint32_t size)
{
	uint8_t xfer_buf[2];
	//xfer_buf[0] = (color >> 8) & 0xFF;	
	//xfer_buf[1] = color & 0xFF;
	xfer_buf[0] = *(uint8_t*)(((uint8_t*)&color)+0);
	xfer_buf[1] = *(uint8_t*)(((uint8_t*)&color)+1);
	
	HAL_PIN_SetOutputValue(dev->dc_pin, 1); /* data mode */
	
	obk_spi_message_t message;	
	message.send_buf = xfer_buf;
	message.recv_buf = NULL;
	message.length   = 2;
	message.repeat   = size;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;

	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return OBK_EOK;
}

obk_err_t lcd_write_colors(TFT_t * dev, uint16_t * colors, uint32_t size)
{
	HAL_PIN_SetOutputValue(dev->dc_pin, 1); /* data mode */
	
	obk_spi_message_t message;
	message.send_buf = colors;
	message.recv_buf = NULL;
	message.length   = size<<1;
	message.repeat   = 1;
	message.cs_take    = 0;
	message.cs_release = 0;
	message.next = NULL;
	
	dev->spidev.bus->ops->xfer(&dev->spidev, &message);
	
	return OBK_EOK;
}

/**
 * @brief lcdInit 
 *
 * @param TFT_t * dev: display device pointer
 *   
 * @return obk_err_t
 */
void lcdInit(TFT_t * dev)
{
	/* take the bus */
	obk_spi_take_bus(&dev->spidev);
	
	if (dev->rs_pin != -1) {
		HAL_PIN_SetOutputValue(dev->rs_pin, 1);
		obk_delay_ms(10);
		HAL_PIN_SetOutputValue(dev->rs_pin, 0);
		obk_delay_ms(50);
		HAL_PIN_SetOutputValue(dev->rs_pin, 1);
		obk_delay_ms(150);
	} 
		
	/* activate slave select signal */
	if (dev->spidev.config.nss_pin != -1)
		obk_spi_take_device(&dev->spidev);
	
	//lcd_write_command(dev, 0x01); //Software Reset
	//obk_delay_ms(150);	

	lcd_write_command(dev, 0x11); //Sleep Out
	obk_delay_ms(10);
		
	lcd_write_command(dev, 0x3A); //Interface Pixel Format
	lcd_write_data_08(dev, 0x55);
		
	uint8_t mad_ctrl = 0;
	if(tft.rotation == 90) {
		mad_ctrl = ST7789_MADCTRL_MX|ST7789_MADCTRL_MV;
	} else if(tft.rotation == 180) {
		mad_ctrl = ST7789_MADCTRL_MX|ST7789_MADCTRL_MY;
	} else if(tft.rotation == 270) {
		mad_ctrl = ST7789_MADCTRL_MV|ST7789_MADCTRL_MY;
	}
	lcd_write_command(dev, 0x36); //Memory Data Access Control
	lcd_write_data_08(dev, mad_ctrl);
	
//	lcd_write_command(dev, 0x30);	//Partial sart/end address set
//	lcd_write_data_byte(dev, 0x00);
//	lcd_write_data_byte(dev, 0x00);
//	lcd_write_data_byte(dev, 0x00);
//	lcd_write_data_byte(dev, 0xA0-1);

	lcd_write_command(dev, 0x21);	//Display Inversion Of

	lcd_write_command(dev, 0x13);	//Normal Display Mode On
	//lcd_write_command(dev, 0x12);	//Partial Display Mode On

	lcd_write_command(dev, 0x29);	//Display ON
	
	/* deactivate slave select signal */
	if (dev->spidev.config.nss_pin != -1)
		obk_spi_release_device(&dev->spidev);	
	/* release the bus */
	obk_spi_release_bus(&dev->spidev);
}
/**
 * @brief lcdColorSize 
 *
 * @param 
 *   
 * @return color size for tft
 */
/*
int8_t lcdColorSize(TFT_t * dev) {
	return 2;
}
*/

/**
 * @brief lcdBeginFrame 
 *
 * @param x:frame X position
          y:frame Y position
		  w:frame width
          h:frame height
 *   
 * @return 0 - if OK, -1 if error
 */
int8_t lcdBeginFrame(TFT_t * dev, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t bgColor) {
	dev->frame_buffer = (uint16_t *)os_malloc(w*h*sizeof(uint16_t));
	if (dev->frame_buffer == NULL) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: cannot allocate frame buffer.");
		return -1;
	}
	dev->frame_x = x; dev->frame_y = y;
	dev->frame_w = w; dev->frame_h = h;
	uint16_t _color = SWAPBYTE(bgColor);
	for (int i = 0; i < w*h; i++) {
		dev->frame_buffer[i] = _color;
	}
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: begin frame: [%d,%d,%d,%d]", x, y, w, h);	
	return 0;
}
/**
 * @brief lcdFrameHash 
 *
 * @param w:frame width
          h:frame height
 *   
 * @return 0 - if OK, -1 if error
 */
uint16_t lcdFrameHash(TFT_t * dev) {
	if (dev->frame_buffer == NULL) return 0;
	uint16_t result = 0xAA55;
	for (uint16_t hash_i = 0; hash_i < (dev->frame_w*dev->frame_h); hash_i++) {
		result = result ^ dev->frame_buffer[hash_i];
	}
	return result;
}
/**
 * @brief lcdEndFrame 
 *
 * @param w:frame width
          h:frame height
 *   
 * @return 0 - if OK, -1 if error
 */
int8_t lcdEndFrame(TFT_t * dev, uint16_t *hash) {
    /* check if valid buffer */
	if (dev->frame_buffer == NULL) {
		dev->frame_x = 0; dev->frame_y = 0;
		dev->frame_w = 0; dev->frame_h = 0;
		return -1;
	}
	/* check hash */
	uint16_t _hash = 0;
	if (hash != NULL) {
		_hash = lcdFrameHash(dev);
		if (_hash == *hash) {/* frame has same content */
			*hash = _hash;
			_hash =	1;
		} else _hash = 0;
	}
	
	/* draw bitmap */
	if (_hash == 0) {
		/* take the bus */
		obk_spi_take_bus(&dev->spidev);
		
		lcd_write_command(dev, 0x2A);	// set column(x) address
		lcd_write_addr(dev, dev->frame_x + dev->offsetx, dev->frame_x+dev->frame_w-1+dev->offsetx);
		lcd_write_command(dev, 0x2B);	// set Page(y) address
		lcd_write_addr(dev, dev->frame_y + dev->offsety, dev->frame_y+dev->frame_h-1+dev->offsety);
		lcd_write_command(dev, 0x2C);	// Memory Write
		lcd_write_colors(dev, dev->frame_buffer, dev->frame_w*dev->frame_h);	
		
		/* release the bus */
		obk_spi_release_bus(&dev->spidev);	
	}
	
	/* free buffer */
	dev->frame_x = 0; dev->frame_y = 0;
	dev->frame_w = 0; dev->frame_h = 0;
	os_free(dev->frame_buffer);
	dev->frame_buffer = NULL;
	return 0;
}
/**
 * @brief lcdSetClipRect 
 *
 * @param x1:Start X coordinate
          y1:Start Y coordinate
          x2:End X coordinate
          y2:End Y coordinate
 *   
 * @return void
 */
void lcdSetClipRect(TFT_t * dev, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	if (x1 < 0) dev->clip_x1 = 0; else dev->clip_x1 = x1;
	if (y1 < 0) dev->clip_y1 = 0; else dev->clip_y1 = y1;
	if (x2 > (dev->width-1))  dev->clip_x2 = dev->width-1;  else dev->clip_x2 = x2;
	if (y2 > (dev->height-1)) dev->clip_y2 = dev->height-1; else dev->clip_y2 = y2;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: set clip: [%d,%d,%d,%d]", dev->clip_x1, dev->clip_y1, dev->clip_x2, dev->clip_y2);
}
/**
 * @brief lcdDrawPixel Draw pixel
 *
 * @param x:X coordinate
          y:Y coordinate
          color:color
 *   
 * @return void
 */
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color){
	if (x >= dev->width) return;
	if (y >= dev->height) return;
	uint16_t _color = SWAPBYTE(color);

	if (dev->frame_buffer) {
		int16_t _x = (x - dev->frame_x);
		int16_t _y = (y - dev->frame_y);
		if ((_x >= 0) && (_y >= 0) && (_x < dev->frame_w) && (_y < dev->frame_h))			
		    dev->frame_buffer[_y*dev->frame_w+_x] = _color;
		
	} else {
		uint16_t _x = x + dev->offsetx;
		uint16_t _y = y + dev->offsety;

		lcd_write_command(dev, 0x2A);	// set column(x) address
		lcd_write_addr(dev, _x, _x);
		lcd_write_command(dev, 0x2B);	// set Page(y) address
		lcd_write_addr(dev, _y, _y);
		lcd_write_command(dev, 0x2C);	// Memory Write
		lcd_write_color(dev, _color, 1);
	}
}

void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color){
	int i;
	int dx,dy;
	int sx,sy;
	int E;
	//ADDLOG_INFO(LOG_FEATURE_DRV, "ST7789 draw line");

	/* distance between two points */
	dx = ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = ( y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
			y1 += sy;
			E -= 2 * dx;
		}
	}

	/* inclination >= 1 */
	} else {
		E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}	
}

int8_t lcdIntersectRect(int16_t r1x1, int16_t r1y1, int16_t r1x2, int16_t r1y2,
                        int16_t r2x1, int16_t r2y1, int16_t r2x2, int16_t r2y2,
						int16_t *rx1, int16_t *ry1, int16_t *rx2, int16_t *ry2) {	
	if ((r1x1 >= r2x2) || (r2x1 >= r1x2) || (r1y1 >= r2y2) || (r2y1 >= r1y2)) return -1;
	*rx1 = lcdMAX(r1x1, r2x1);
	*ry1 = lcdMAX(r1y1, r2y1);
	*rx2 = lcdMIN(r1x2, r2x2);
	*ry2 = lcdMIN(r1y2, r2y2);
	if ((*rx1 <= *rx2) && (*ry1 <= *ry2)) return 0;
	else return -1;
}

/**
 * @brief lcdDrawFillRect 
 *
 * @param x1:Start X coordinate
          y1:Start Y coordinate
          x2:End X coordinate
          y2:End Y coordinate
          color:color
 *   
 * @return void
 */
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	if (x1 >= dev->width) return;
	if (x2 >= dev->width) x2=dev->width-1;
	if (y1 >= dev->height) return;
	if (y2 >= dev->height) y2=dev->height-1;
	uint16_t _color = SWAPBYTE(color);

	//ESP_LOGD(TAG,"offset(x)=%d offset(y)=%d",dev->_offsetx,dev->_offsety);

	if (dev->frame_buffer) {
		/* has rect intersects with frame ?*/
		int16_t _x1 = x1;
		int16_t _y1 = y1;
		int16_t _x2 = x2;
		int16_t _y2 = y2;
		if (!lcdIntersectRect(x1, y1, x2, y2, 
		                    dev->frame_x, dev->frame_y, 
							dev->frame_x+dev->frame_w-1, dev->frame_y+dev->frame_h-1,
							&_x1, &_y1, &_x2, &_y2)) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: fill rect: [%d,%d,%d,%d]", _x1, _y1, _x2-_x1, _y2-_y1);
			for (int16_t iy = _y1; iy <= _y2; iy++){
				for(int16_t ix = _x1; ix <= _x2; ix++) {
					dev->frame_buffer[(iy-dev->frame_y)*dev->frame_w+(ix-dev->frame_x)] = _color;
				}
			}
		}
	} else {
		uint16_t _x1 = x1 + dev->offsetx;
		uint16_t _x2 = x2 + dev->offsetx;
		uint16_t _y1 = y1 + dev->offsety;
		uint16_t _y2 = y2 + dev->offsety;
		
		/* take the bus */
		obk_spi_take_bus(&dev->spidev);
		
		lcd_write_command(dev, 0x2A);	// set column(x) address
		lcd_write_addr(dev, _x1, _x2);
		lcd_write_command(dev, 0x2B);	// set Page(y) address
		lcd_write_addr(dev, _y1, _y2);
		lcd_write_command(dev, 0x2C);	// Memory Write
		for(int i=_x1;i<=_x2;i++){
			uint16_t size = _y2-_y1+1;
			lcd_write_color(dev, _color, size);
		}
		
		/* release the bus */
		obk_spi_release_bus(&dev->spidev);
	}
}

/**
 * @brief lcdDrawBitmap  
 *
 * @param x:Start X coordinate
		  y:Start Y coordinate
          w:bitmap width
          h:bitmap height
          bitmap:Pointer to bitmap array
          rcolor:color in bitmap to replace
          bkcolor: new color
 *   
 * @return void
 */
void lcdDrawBitmap(TFT_t * dev, uint16_t x,uint16_t y,uint16_t w,uint16_t h, uint16_t *bitmap, uint16_t rcolor, uint16_t bkcolor) {
	uint16_t *bitmap_buf = 0;
	uint16_t _rcolor = SWAPBYTE(rcolor);
	uint16_t _bkcolor = SWAPBYTE(bkcolor); 
	if (_rcolor != _bkcolor) {
		bitmap_buf = (uint16_t *)os_malloc(sizeof(uint16_t)*w*h);
		if (bitmap_buf != 0) {
			int16_t px = w*h-1;
			for ( ; px >= 0; px-- ) {
				if (bitmap[px] == _rcolor) bitmap_buf[px] = _bkcolor;
				else                      bitmap_buf[px] = bitmap[px];
			}
			bitmap = bitmap_buf;
		}
	}
	lcd_write_command(dev, 0x2A);	// set column(x) address
	lcd_write_addr(dev, x, x+w-1);
	lcd_write_command(dev, 0x2B);	// set Page(y) address
	lcd_write_addr(dev, y, y+h-1);
	lcd_write_command(dev, 0x2C);	// Memory Write
	lcd_write_colors(dev, bitmap, w*h);
	if (bitmap_buf != 0)
		os_free(bitmap_buf);
}

/**
 * @brief lcdDrawChar Draw ASCII character
 *
 * @param x:X coordinate
          y:Y coordinate
          ascii: ascii code
          charColor: char color
          bkgColor: bkackground color
 *   
 * @return void
 */
int lcdDrawChar(TFT_t * dev, uint16_t x, uint16_t y, uint8_t ascii, uint16_t chColor, uint16_t bgColor) {
	int16_t i_x, j_y, sc_x, sc_y, pos_x, pos_y;
	uint16_t _chColor = chColor;
	uint16_t _bgColor = bgColor;
	int16_t _x1 = dev->clip_x1, _y1 = dev->clip_y1, _x2 = dev->clip_x2, _y2 = dev->clip_y2;
	
	if (dev->frame_buffer) { /*if use frame buffer, check intersect simbol rect with frame rect*/
		/* char rect */
		_x1 = x;
		_y1 = y;
		_x2 = x+dev->font_scale*7-1;
		_y2 = y+dev->font_scale*13-1;
		/* intersect with frame ? */
		if (lcdIntersectRect(_x1, _y1, _x2, _y2, 
		                    dev->frame_x, dev->frame_y, 
							dev->frame_x+dev->frame_w-1, dev->frame_y+dev->frame_h-1,
							&_x1, &_y1, &_x2, &_y2)) {
			return 0;
		}
		/* intersect with clip rect ? */
		if (lcdIntersectRect(_x1, _y1, _x2, _y2, 
		                    dev->clip_x1, dev->clip_y1, 
							dev->clip_x2, dev->clip_y2,
							&_x1, &_y1, &_x2, &_y2)) {
			return 0;
		}		
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: draw char: [%d,%d,%d,%d]", _x1, _y1, _x2-_x1, _y2-_y1);
	}
	uint16_t *char_buf = NULL;
	if (!dev->frame_buffer)
		char_buf = (uint16_t *)os_malloc(sizeof(uint16_t)*13*dev->font_scale*7*dev->font_scale);
	
	for (i_x=0;i_x<7;i_x++)	{
		for (sc_x=0;sc_x<dev->font_scale;sc_x++) {
			uint16_t ch = (NewBFontLAT[ ( (ascii-0x20)*14 + i_x+7) ] <<8) + NewBFontLAT[ ( (ascii-0x20)*14 + i_x) ];
			ch = ch<<1;
			for (j_y=0;j_y<13;j_y++) {
				uint16_t _color;
				if (ch & 0x8000) _color=_chColor; 
				else { 
					_color=_bgColor;
					if (dev->frame_buffer) {
						ch = ch<<1;
						continue;
					}
				}
				ch = ch<<1;
				for (sc_y=0;sc_y<dev->font_scale;sc_y++)
				{
					if (char_buf == NULL)	{
						pos_x = x + i_x*(dev->font_scale)+sc_x;
						pos_y = y + (12-j_y)*(dev->font_scale)+sc_y;
						if (dev->frame_buffer) {
							if ((pos_x >= _x1) && (pos_y >= _y1) && (pos_x <= _x2) && (pos_y <= _y2))								
								dev->frame_buffer[(pos_y-dev->frame_y)*dev->frame_w+(pos_x-dev->frame_x)] = SWAPBYTE(_color);
						} else 
							lcdDrawPixel(dev, pos_x, pos_y, _color);
					} else {
						pos_x = i_x*(dev->font_scale)+sc_x;
						pos_y = (12-j_y)*(dev->font_scale)+sc_y;
						*(uint16_t *)(char_buf+(pos_y*7*(dev->font_scale)+pos_x)) = SWAPBYTE(_color);
						//*(uint16_t *)(char_buf+(pos_x*13*(dev->font_scale)+pos_y)) = _color;
					}
				}
			}
		}
	}
	if (char_buf != 0) {
		uint16_t _x = x + dev->offsetx;
	    uint16_t _y = y + dev->offsety;
		lcd_write_command(dev, 0x2A);	// set column(x) address
		lcd_write_addr(dev, _x, _x+7*dev->font_scale-1);
		lcd_write_command(dev, 0x2B);	// set Page(y) address		
		lcd_write_addr(dev, _y, _y+13*dev->font_scale-1);
		lcd_write_command(dev, 0x2C);	// Memory Write
		lcd_write_colors(dev, char_buf, dev->font_scale*13*dev->font_scale*7);
	}
	if (char_buf != 0) os_free(char_buf);
	return 0;
}
/**
 * @brief lcdDrawString Draw ASCII string
 *
 * @param x:X coordinate
          y:Y coordinate
          *ascii: pointer to ascii string
          charColor: char color
          bkgColor: bkackground color
 *   
 * @return int
 */
int lcdDrawString(TFT_t * dev, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t chColor, uint16_t bgColor) {
	int length = strlen((char *)ascii);
	for(int i=0;i<length;i++) {
		if (dev->font_direction == 0) {
			lcdDrawChar(dev, x, y, ascii[i], chColor, bgColor);
			x=x+(7*dev->font_scale)-1;
		}
		if (dev->font_direction == 1) {
			lcdDrawChar(dev, x, y, ascii[i], chColor, bgColor);
			y=y+(14*dev->font_scale)-1;
		}
		if (dev->font_direction == 2) {
			lcdDrawChar(dev, x, y, ascii[i], chColor, bgColor);
			x=x-(7*dev->font_scale)-1;
		}
		if (dev->font_direction == 3) {
			y = lcdDrawChar(dev, x, y, ascii[i], chColor, bgColor);
			y=y-(14*dev->font_scale)-1;
		}
	}
	if (dev->font_direction == 0) return x;
	if (dev->font_direction == 2) return x;
	if (dev->font_direction == 1) return y;
	if (dev->font_direction == 3) return y;
	return 0;
}
// Вычисление одной координаты для кубической кривой Безье
static double lcdBezierCoord(double p0, double p1, double p2, double p3, double t) {
    double mt = 1.0 - t;          // (1 - t)
    double mt2 = mt * mt;         // (1 - t)^2
    double mt3 = mt2 * mt;        // (1 - t)^3
    double t2 = t * t;            // t^2
    double t3 = t2 * t;           // t^3

    return p0 * mt3 +
           3.0 * p1 * mt2 * t +
           3.0 * p2 * mt * t2 +
           p3 * t3;
}

void lcdDrawBezierCubic(TFT_t * dev, 
                        double x1, double y1, double x2, double y2,
                        double x3, double y3, double x4, double y4,	
						uint16_t color) {
	double xs = x1;
    xs = xs < x2 ? xs : x2;
	xs = xs < x3 ? xs : x3;
	xs = xs < x4 ? xs : x4;
	double xe = x1;
	xe = xe > x2 ? xe : x2;
	xe = xe > x3 ? xe : x3;
	xe = xe > x4 ? xe : x4;
	int steps = 2*(int)abs(xe-xs);
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: lcdBezier3[%0.1f,%0.1f,%0.1f,%0.1f][%d]", x1, y1, x4, y4, steps);
	uint16_t x0 = lcdBezierCoord(x1, x2, x3, x4, 0);
	uint16_t y0 = lcdBezierCoord(y1, y2, y3, y4, 0);
	for (int i = 1; i<=steps; i++) {
		double t = (double)i/(double)steps;//(x2-x1);
		uint16_t xi = lcdBezierCoord(x1, x2, x3, x4, t);
		uint16_t yi = lcdBezierCoord(y1, y2, y3, y4, t);
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: line[%0.1f,%0.1f,%0.1f,%0.1f][%0.1f]", x0, y0, xi, yi, t);
		lcdDrawLine(dev, x0, y0, xi, yi, color);
		x0 = xi; y0 = yi;
	}
}

#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(d)  ((d) * M_PI / 180.0)
#define RAD2DEG(r)  ((r) * 180.0 / M_PI)

//typedef struct { double x, y; } Point;

/*
 * Вычисляет центр эллипса и углы начала/конца дуги
 * по параметрам SVG-команды "A".
 *
 * Вход:
 *   x1, y1     — текущая точка (начало дуги)
 *   x2, y2     — конечная точка дуги
 *   rx, ry     — радиусы эллипса
 *   phi_deg    — поворот оси эллипса в градусах
 *   large_arc  — 0 или 1
 *   sweep      — 0 или 1
 *
 * Выход:
 *   *cx, *cy        — центр эллипса
 *   *theta_start   — начальный угол (радианы)
 *   *theta_end     — конечный угол (радианы)
 *
 * Возвращает 0 при успехе, -1 при ошибке.
 */
static int arc_to_center(
    double x1, double y1, double x2, double y2,
    double rx, double ry, double phi_deg,
    int large_arc, int sweep,
    double *cx, double *cy,
    double *theta_start, double *theta_end
) {
    // Вырожденный случай: точки совпадают
    if (x1 == x2 && y1 == y2) return -1;

    // Если радиусы нулевые — нет дуги
    if (rx == 0.0 || ry == 0.0) return -1;

    // Берём модули радиусов
    rx = fabs(rx);
    ry = fabs(ry);

    double phi = DEG2RAD(phi_deg);
    double cos_phi = cos(phi);
    double sin_phi = sin(phi);

    // Шаг 1: вычисляем (x1', y1') — начало дуги в повёрнутой системе
    double dx = (x1 - x2) / 2.0;
    double dy = (y1 - y2) / 2.0;
    double x1p =  cos_phi * dx + sin_phi * dy;
    double y1p = -sin_phi * dx + cos_phi * dy;

    // Шаг 2: корректируем радиусы, если они слишком малы
    double lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1.0) {
        double s = sqrt(lambda);
        rx *= s;
        ry *= s;
    }

    // Шаг 3: вычисляем центр в повёрнутой системе (cx', cy')
    double rx2 = rx * rx;
    double ry2 = ry * ry;
    double x1p2 = x1p * x1p;
    double y1p2 = y1p * y1p;

    double num = rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2;
    double den = rx2 * y1p2 + ry2 * x1p2;
    if (den == 0.0) return -1;

    double factor = (large_arc != sweep ? 1.0 : -1.0)
                  * sqrt(fmax(0.0, num / den));

    double cxp =  factor * (rx * y1p) / ry;
    double cyp = -factor * (ry * x1p) / rx;

    // Шаг 4: преобразуем центр обратно в исходную систему
    *cx = cos_phi * cxp - sin_phi * cyp + (x1 + x2) / 2.0;
    *cy = sin_phi * cxp + cos_phi * cyp + (y1 + y2) / 2.0;

    // Шаг 5: вычисляем начальный и конечный углы
    // Вектор от центра к начальной точке в повёрнутой системе
    double ux = (x1p - cxp) / rx;
    double uy = (y1p - cyp) / ry;
    // Вектор от центра к конечной точке в повёрнутой системе
    double vx = (-x1p - cxp) / rx;
    double vy = (-y1p - cyp) / ry;

    *theta_start = atan2(uy, ux);  // начальный угол

    // Размах дуги
    double dot = ux * vx + uy * vy;
    double cross = ux * vy - uy * vx;
    double dtheta = atan2(fabs(cross), dot);

    // Корректируем знак размаха
    if (cross < 0.0) dtheta = -dtheta;

    // Учитываем sweep-flag
    if (sweep == 0 && dtheta > 0.0)
        dtheta -= 2.0 * M_PI;
    else if (sweep == 1 && dtheta < 0.0)
        dtheta += 2.0 * M_PI;

    *theta_end = *theta_start + dtheta;

    return 0;
}
// Поворот точки вокруг начала координат на угол phi (в радианах)
static void rotate_point(double x0, double y0, double phi, double *x, double *y) {
    double c = cos(phi);
    double s = sin(phi);
    *x = x0 * c - y0 * s;
    *y = x0 * s + y0 * c;
}

// Точка на эллипсе с центром (cx,cy), радиусами rx,ry, поворотом phi, углом theta
static void ellipse_point(double cx, double cy, double rx, double ry,
                           double phi, double theta, double *x, double *y) {
    // Сначала точка на осевом эллипсе
    *x = rx * cos(theta);
    *y = ry * sin(theta);    
    // Поворот эллипса
    rotate_point(*x, *y, phi, x, y);
    // Перенос в центр
    *x = *x + cx;
    *y = *y + cy;    
}

/**
 * @brief Draws an elliptical arc from the current point to (x, y). The size and orientation 
 *        of the ellipse are defined by two radii (rx, ry) and an x-axis-rotation, which indicates 
 *        how the ellipse as a whole is rotated, in degrees, relative to the current coordinate system. 
 *        The center (cx, cy) of the ellipse is calculated automatically to satisfy the constraints 
 *        imposed by the other parameters. large-arc-flag and sweep-flag contribute to the automatic 
 *        calculations and help determine how the arc is drawn. 
 *
 * @param dev: pointer to dispay structure
 *        x1,y1: start point
 *        x2,y2: end point   
 *        rx,ry: radius
 *        rotaion: x-axis rotation
 *        large_arc:
 *        sweep: 
 *        color: color
 *   
 * @return 
 */
void lcdDrawArc(TFT_t * dev, double x1, double y1, double x2, double y2, double rx, double ry, double rotation, int large_arc, int sweep, uint16_t color)
{
	
	// Вычисляем центр и углы
    double cx, cy, theta_start, theta_end;

    if (arc_to_center(x1, y1, x2, y2, rx, ry, rotation,
                      large_arc, sweep,
                      &cx, &cy, &theta_start, &theta_end) != 0)
    {
        //fprintf(stderr, "Ошибка: некорректные параметры дуги\n");
        return;
    }	
	double dtheta = theta_end - theta_start;
	double rmax = rx > ry ? rx : ry;
	int steps = abs(dtheta*rmax);
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: lcdDrawArc[%0.1f,%0.1f,%0.1f,%0.1f][%d]", cx, cy, theta_start, theta_end, steps);
	double x0, y0;
	ellipse_point(cx, cy, rx, ry, DEG2RAD(rotation), theta_start, &x0, &y0);		
	for (int i = 1; i<steps; i++) {
		double t = (double)i/(double)steps;		
		double theta = theta_start + t * dtheta;
		double xi, yi;
		ellipse_point(cx, cy, rx, ry, DEG2RAD(rotation), theta, &xi, &yi);	
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "ST7789: lcdDrawArc[%0.1f,%0.1f,%0.1f,%0.1f]", x0, y0, xi, yi);
		lcdDrawLine(dev, x0, y0, xi, yi, color);
		x0 = xi; y0 = yi;
	}
}
/**
 * @brief Display OFF
 *
 * @param dev: pointer to dispay structure
 *   
 * @return 
 */
void lcdDisplayOff(TFT_t * dev) {
	if (dev->bl_pin != -1) {
		if (dev->bl_channel) {
			//CHANNEL_Set_FloatPWM(dev->bl_pin, 0, 0);
			CHANNEL_Set(SPECIAL_CHANNEL_LEDPOWER, 0, 0);
			CHANNEL_Set(SPECIAL_CHANNEL_BRIGHTNESS, 0, 0);			
		} else {			
			HAL_PIN_SetOutputValue(dev->bl_pin, 0);
		}
	}
	lcd_write_command(dev, 0x28);	// Display off
}
/**
 * @brief Display ON
 *
 * @param dev: pointer to dispay structure
 *   
 * @return 
 */
void lcdDisplayOn(TFT_t * dev) {
	lcd_write_command(dev, 0x29);	// Display on
	if (dev->bl_pin != -1) {
		if (dev->bl_channel) {
			//CHANNEL_Set_FloatPWM(dev->bl_pin, dev->brightness, 0);
			CHANNEL_Set(SPECIAL_CHANNEL_LEDPOWER, 1, 0);
			CHANNEL_Set(SPECIAL_CHANNEL_BRIGHTNESS, dev->brightness, 0);			
		} else {
			HAL_PIN_SetOutputValue(dev->bl_pin, 1);
		}
	}
}

/**
 * @brief lcdSetBrightness Write Display Brightness
 *
 * @param dev: pointer to dispay structure
 *        brightness: brightness         
 *   
 * @return 
 */
void lcdSetBrightness(TFT_t * dev, uint8_t brightness) {
	dev->brightness = brightness;
	if (dev->bl_pin != -1) {
		if (dev->bl_channel) {
			CHANNEL_Set(SPECIAL_CHANNEL_BRIGHTNESS, dev->brightness, 0);
		} else {			
			if (!dev->brightness)
				HAL_PIN_SetOutputValue(dev->bl_pin, 0);
			else
				HAL_PIN_SetOutputValue(dev->bl_pin, 1);
		}
	} else {
		lcd_write_command(dev, 0x51);	// Write Display Brightness
		lcd_write_data_08(dev, brightness);
	}
}

/*
#include <stdint.h>
#include <stddef.h>

typedef struct { int x, y; } Point;

// Пример функции установки пикселя (подставь свою)
extern void setPixel(int x, int y, uint32_t color);

void scanlineFill(const Point *poly, size_t n, uint32_t color) {
    if (n < 3) return;

    // Находим ограничивающий прямоугольник
    int minY = poly[0].y, maxY = poly[0].y;
    for (size_t i = 1; i < n; ++i) {
        if (poly[i].y < minY) minY = poly[i].y;
        if (poly[i].y > maxY) maxY = poly[i].y;
    }

    // Для каждой строки y
    for (int y = minY; y <= maxY; ++y) {
        // Собираем пересечения строки y с рёбрами
        double intersections[128]; // запас на случай сложного полигона
        size_t count = 0;

        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n; // следующее ребро
            int y1 = poly[i].y, y2 = poly[j].y;
            int x1 = poly[i].x, x2 = poly[j].x;

            // Пропускаем горизонтальные рёбра (они не дают уникальных пересечений)
            if (y1 == y2) continue;

            // Проверяем, пересекает ли ребро строку y
            if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                // Линейная интерполяция x на уровне y
                double t = (double)(y - y1) / (y2 - y1);
                double x = x1 + t * (x2 - x1);

                if (count < 128) {
                    intersections[count++] = x;
                }
            }
        }

        // Сортируем пересечения по x (простая сортировка вставками)
        for (size_t i = 1; i < count; ++i) {
            double key = intersections[i];
            size_t j = i;
            while (j > 0 && intersections[j - 1] > key) {
                intersections[j] = intersections[j - 1];
                --j;
            }
            intersections[j] = key;
        }

        // Рисуем отрезки между парами пересечений
        for (size_t i = 0; i + 1 < count; i += 2) {
            int xStart = (int)(intersections[i] + 0.5);
            int xEnd   = (int)(intersections[i + 1] + 0.5);

            for (int x = xStart; x <= xEnd; ++x) {
                setPixel(x, y, color);
            }
        }
    }
}

*/
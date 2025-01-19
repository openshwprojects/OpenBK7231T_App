#ifndef __DRV_SM16703P_H__
#define __DRV_SM16703P_H__

// Define the color order options
#define SM16703P_COLOR_ORDER_RGB 0x00
#define SM16703P_COLOR_ORDER_RBG 0x01
#define SM16703P_COLOR_ORDER_BRG 0x02
#define SM16703P_COLOR_ORDER_BGR 0x03
#define SM16703P_COLOR_ORDER_GRB 0x04
#define SM16703P_COLOR_ORDER_GBR 0x05

extern int color_order;
extern uint32_t pixel_count;

void SM16703P_Init();
void SM16703P_Show();
void SM16703P_setPixel(int pixel, int r, int g, int b);
void SM16703P_setPixelWithBrig(int pixel, int r, int g, int b);
void SM16703P_setAllPixels(int r, int g, int b);
void SM16703P_setMultiplePixel(int pixel, const uint8_t *data, bool push);
void SM16703P_GetPixel(uint32_t pixel, byte *dst);
bool SM16703P_VerifyPixel(uint32_t pixel, byte r, byte g, byte b);
void SM16703P_scaleAllPixels(int scale);

#endif // __DRV_SM16703P_H__
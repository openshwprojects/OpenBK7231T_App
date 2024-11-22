#ifndef __DRV_WS2811_H
#define __DRV_WS2811_H

// Define the color order options
#define WS2811_COLOR_ORDER_RGB 0x00
#define WS2811_COLOR_ORDER_RBG 0x01
#define WS2811_COLOR_ORDER_BRG 0x02
#define WS2811_COLOR_ORDER_BGR 0x03
#define WS2811_COLOR_ORDER_GRB 0x04
#define WS2811_COLOR_ORDER_GBR 0x05

#define WS2811_DMA_CH DMA_CH_2
#define WS2811_BAUD_RATE 16
// 6 is the default pin number as used for the device in https://www.elektroda.com/rtvforum/topic4083817.html
#define WS2811_DEFAULT_DATA_PIN 6

typedef struct ws2811Data_s {
	uint8_t *buf;
	bool ready;
	int pin_data;
} ws2811Data_t;

extern ws2811Data_t ws2811Data;

#endif // __DRV_WS2811_H
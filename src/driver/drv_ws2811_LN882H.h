#pragma once
// LN882H specific defines
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
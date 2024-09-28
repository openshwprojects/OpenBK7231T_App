#if PLATFORM_BK7231N || WINDOWS


#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#include "drv_spidma.h"
#include "drv_spiLED.h"

#if WINDOWS

void SPIDMA_Init(struct spi_message *msg) {

}
void SPIDMA_StartTX(struct spi_message *msg) {

}
void SPIDMA_StopTX(void) {

}

#endif
static uint8_t data_translate[4] = { 0b10001000, 0b10001110, 0b11101000, 0b11101110 };


uint8_t translate_2bit(uint8_t input) {
	//ADDLOG_INFO(LOG_FEATURE_CMD, "Translate 0x%02x to 0x%02x", (input & 0b00000011), data_translate[(input & 0b00000011)]);
	return data_translate[(input & 0b00000011)];
}

uint8_t reverse_translate_2bit(uint8_t input) {
	uint8_t i;
	for (i = 0; i < 4; ++i) {
		if (data_translate[i] == input) {
			return i;
		}
	}
	return 0;
}

byte reverse_translate_byte(uint8_t *input) {
	byte dst;
	dst = (reverse_translate_2bit(*input++) << 6);
	dst |= (reverse_translate_2bit(*input++) << 4);
	dst |= (reverse_translate_2bit(*input++) << 2);
	dst |= (reverse_translate_2bit(*input++) << 0);
	return dst;
}
void translate_byte(uint8_t input, uint8_t *dst) {
	// return 0x00000000 |
	// 	   translate_2bit((input >> 6)) |
	// 	   translate_2bit((input >> 4)) |
	// 	   translate_2bit((input >> 2)) |
	// 	   translate_2bit(input);
	*dst++ = translate_2bit((input >> 6));
	*dst++ = translate_2bit((input >> 4));
	*dst++ = translate_2bit((input >> 2));
	*dst++ = translate_2bit(input);

#if WINDOWS
	byte test = reverse_translate_byte(dst - 4);
	if (input != test) {
		printf("reverse_translate_byte is brken");
	}
#endif
}

spiLED_t spiLED;




void SPILED_InitDMA(int numBytes) {
	int i;

	spiLED.padding = 64;

	// Prepare buffer
	uint32_t buffer_size = spiLED.ofs + (numBytes * 4) + spiLED.padding; //Add `spiLED.ofs` bytes for "Reset"

	spiLED.buf = (UINT8 *)os_malloc(sizeof(UINT8) * (buffer_size)); //18LEDs x RGB x 4Bytes

	// Fill `spiLED.ofs` slice of the buffer with zero
	for (i = 0; i < spiLED.ofs; i++) {
		spiLED.buf[i] = 0;
	}
	// Fill data slice of the buffer with data bits that decode to black color
	for (i = spiLED.ofs; i < buffer_size - spiLED.padding; i++) {
		spiLED.buf[i] = 0b10001000;
	}
	// Fill `spiLED.padding` slice of the buffer with zero
	for (i = buffer_size - spiLED.padding; i < buffer_size; i++) {
		spiLED.buf[i] = 0;
	}

	spiLED.msg = os_malloc(sizeof(struct spi_message));
	spiLED.msg->send_buf = spiLED.buf;
	spiLED.msg->send_len = buffer_size;

	SPIDMA_Init(spiLED.msg);

	spiLED.ready = true;
}



void SPILED_SetRawHexString(int start_offset, const char *s, int push) {
	// start offset is in bytes, and we do 2 bits per dst byte, so *4
	uint8_t *dst = spiLED.buf + spiLED.ofs + start_offset * 4;

	// parse hex string like FFAABB0011 byte by byte
	while (s[0] && s[1]) {
		byte b;
		b = hexbyte(s);
		*dst++ = translate_2bit((b >> 6));
		*dst++ = translate_2bit((b >> 4));
		*dst++ = translate_2bit((b >> 2));
		*dst++ = translate_2bit(b);
		s += 2;
	}
	if (push) {
		SPIDMA_StartTX(spiLED.msg);
	}
}
void SPILED_SetRawBytes(int start_offset, byte *bytes, int numBytes, int push) {
	// start offset is in bytes, and we do 2 bits per dst byte, so *4
	uint8_t *dst = spiLED.buf + spiLED.ofs + start_offset * 4;

	// parse hex string like FFAABB0011 byte by byte
	for (int i = 0; i < numBytes; i++) {
		byte b = bytes[i];
		*dst++ = translate_2bit((b >> 6));
		*dst++ = translate_2bit((b >> 4));
		*dst++ = translate_2bit((b >> 2));
		*dst++ = translate_2bit(b);
	}
	if (push) {
		SPIDMA_StartTX(spiLED.msg);
	}
}

void SPILED_Init() {
	uint32_t val;
#if PLATFORM_BK7231N
	val = GFUNC_MODE_SPI_USE_GPIO_14;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	val = GFUNC_MODE_SPI_USE_GPIO_16_17;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	UINT32 param;
	param = PCLK_POSI_SPI;
	sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_26M, &param);

	param = PWD_SPI_CLK_BIT;
	sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, &param);
#else


#endif
}


#endif
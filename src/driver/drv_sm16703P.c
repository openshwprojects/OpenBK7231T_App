#if PLATFORM_BEKEN

#include "arm_arch.h"
#include "drv_model_pub.h"
#include "gpio.h"
#include "gpio_pub.h"
#include "icu_pub.h"
#include "include.h"
#include "intc_pub.h"
#include "sys_ctrl_pub.h"
#include "uart_pub.h"

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

static uint8_t data_translate[4] = {0b10001000, 0b10001110, 0b11101000, 0b11101110};

UINT8 *send_buf;
struct spi_message *spi_msg;
BOOLEAN initialized = false;
uint32_t pixel_count = 0;

static uint8_t translate_2bit(uint8_t input) {
	//ADDLOG_INFO(LOG_FEATURE_CMD, "Translate 0x%02x to 0x%02x", (input & 0b00000011), data_translate[(input & 0b00000011)]);
	return data_translate[(input & 0b00000011)];
}

static void translate_byte(uint8_t input, uint8_t *dst) {
	// return 0x00000000 |
	// 	   translate_2bit((input >> 6)) |
	// 	   translate_2bit((input >> 4)) |
	// 	   translate_2bit((input >> 2)) |
	// 	   translate_2bit(input);
	*dst++ = translate_2bit((input >> 6));
	*dst++ = translate_2bit((input >> 4));
	*dst++ = translate_2bit((input >> 2));
	*dst++ = translate_2bit(input);
}

static void SM16703P_setMultiplePixel(uint32_t pixel, UINT8 *data) {

	// Return if driver is not loaded
	if (!initialized)
		return;

	// Check max pixel
	if (pixel > pixel_count)
		pixel = pixel_count;

	// Iterate over pixel
	uint8_t *dst = spi_msg->send_buf + 2;
	for (uint32_t i = 0; i < pixel * 3; i++) {
		uint8_t input = *data++;
		*dst++ = translate_2bit((input >> 6));
		*dst++ = translate_2bit((input >> 4));
		*dst++ = translate_2bit((input >> 2));
		*dst++ = translate_2bit(input);
	}
}

commandResult_t SM16703P_CMD_setPixel(const void *context, const char *cmd, const char *args, int flags) {
	int pixel, r, g, b;
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() != 4) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not Enough Arguments for init SM16703P: Amount of LEDs missing");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	pixel = Tokenizer_GetArgIntegerRange(0, 0, 255);
	r = Tokenizer_GetArgIntegerRange(1, 0, 255);
	g = Tokenizer_GetArgIntegerRange(2, 0, 255);
	b = Tokenizer_GetArgIntegerRange(3, 0, 255);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Set Pixel %i to R %i G %i B %i", pixel, r, g, b);

	translate_byte(r, spi_msg->send_buf + (2 + 0 + (pixel * 3 * 4)));
	translate_byte(g, spi_msg->send_buf + (2 + 4 + (pixel * 3 * 4)));
	translate_byte(b, spi_msg->send_buf + (2 + 8 + (pixel * 3 * 4)));

	ADDLOG_INFO(LOG_FEATURE_CMD, "Raw Data 0x%02x 0x%02x 0x%02x 0x%02x - 0x%02x 0x%02x 0x%02x 0x%02x - 0x%02x 0x%02x 0x%02x 0x%02x",
				spi_msg->send_buf[2 + 0 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 1 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 2 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 3 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 4 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 5 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 6 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 7 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 8 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 9 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 10 + (pixel * 3 * 4)],
				spi_msg->send_buf[2 + 11 + (pixel * 3 * 4)]);

	return CMD_RES_OK;
}

static void SM16703P_Send(byte *data, int dataSize) {
	// ToDo
}

commandResult_t SM16703P_Start(const void *context, const char *cmd, const char *args, int flags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not Enough Arguments for init SM16703P: Amount of LEDs missing");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	pixel_count = Tokenizer_GetArgIntegerRange(0, 0, 255);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Register driver with %i LEDs", pixel_count);

	// Prepare buffer
	uint32_t buffer_size = 2 + (pixel_count * 3 * 4);			  //Add two bytes for "Reset"
	send_buf = (UINT8 *)os_malloc(sizeof(UINT8) * (buffer_size)); //18LEDs x RGB x 4Bytes
	int i;

	send_buf[0] = 0;
	send_buf[1] = 0;

	for (i = 2; i < buffer_size; i++) {
		send_buf[i] = 0b11101110;
	}

	spi_msg = os_malloc(sizeof(struct spi_message));
	spi_msg->send_buf = send_buf;
	spi_msg->send_len = buffer_size;

	SPIDMA_Init(spi_msg);

	initialized = true;

	return CMD_RES_OK;
}

static commandResult_t SM16703P_StartTX(const void *context, const char *cmd, const char *args, int flags) {
	if (!initialized)
		return CMD_RES_ERROR;

	SPIDMA_StartTX(spi_msg);
	return CMD_RES_OK;
}

// startDriver SM16703P
// backlog startDriver SM16703P; SM16703P_Test
void SM16703P_Init() {

	initialized = false;

	uint32_t val;

	val = GFUNC_MODE_SPI_USE_GPIO_14;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	val = GFUNC_MODE_SPI_USE_GPIO_16_17;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	UINT32 param;
	param = PCLK_POSI_SPI;
	sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_26M, &param);

	param = PWD_SPI_CLK_BIT;
	sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, &param);

	//cmddetail:{"name":"SM16703P_Test","args":"",
	//cmddetail:"descr":"qq",
	//cmddetail:"fn":"SM16703P_Test","file":"driver/drv_ucs1912.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Init", SM16703P_Start, NULL);
	//cmddetail:{"name":"SM16703P_Send","args":"",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"SM16703P_Send_Cmd","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Start", SM16703P_StartTX, NULL);
	//cmddetail:{"name":"SM16703P_Test_3xZero","args":"",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"SM16703P_Test_3xZero","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetPixel", SM16703P_CMD_setPixel, NULL);
	//cmddetail:{"name":"SM16703P_Test_3xOne","args":"",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"SM16703P_Test_3xOne","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	//CMD_RegisterCommand("SM16703P_Test_3xOne", SM16703P_Test_3xOne, NULL);
}
#endif

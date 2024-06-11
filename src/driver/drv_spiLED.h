

typedef struct spiLED_s {
	UINT8 *buf;
	struct spi_message *msg;
	BOOLEAN ready;
	// Number of empty bytes to send before pixel data on each frame
	// Likely not needed as the data line should be LOW (reset) between frames anyway
	uint32_t ofs;
	// Number of empty bytes to send after pixel data on each frame
	// Workaround to stuff the SPI buffer with empty bytes after a transmission (s.a. #1055)
	uint32_t padding;
} spiLED_t;

extern spiLED_t spiLED;

uint8_t translate_2bit(uint8_t input);
uint8_t reverse_translate_2bit(uint8_t input);
byte reverse_translate_byte(uint8_t *input);
void translate_byte(uint8_t input, uint8_t *dst);

void SPILED_InitDMA(int numBytes);

void SPILED_SetRawHexString(int start_offset, const char *s, int push);
void SPILED_SetRawBytes(int start_offset, byte *bytes, int numBytes, int push);
void SPILED_Init();
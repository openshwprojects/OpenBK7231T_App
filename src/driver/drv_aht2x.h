/*
 * AHT family command constants.
 *
 * Public OpenBeken compatibility note:
 * this driver is still registered as AHT2X and keeps the existing AHT2X_*
 * commands.  AHT10/AHT15 use the documented 0xE1 init command.  AHT20/AHT21-
 * style devices use the documented 0xBE 0x08 0x00 init sequence.  The family
 * shares the 0xAC 0x33 0x00 measurement command and 20-bit humidity/temperature
 * conversion format, while exact model auto-identification is not available.
 */
#define AHT2X_CMD_INI        0xBE
#define AHT2X_CMD_INI_AHT1X  0xE1
#define AHT2X_DAT_INI1       0x08
#define AHT2X_DAT_INI2       0x00
#define AHT2X_CMD_TMS        0xAC
#define AHT2X_DAT_TMS1       0x33
#define AHT2X_DAT_TMS2       0x00
#define AHT2X_CMD_RST        0xBA

#define AHT2X_DAT_BUSY       0x80
#define AHT2X_DAT_CALIBRATED 0x08

#define AHT2X_READ_LEN       6
#define AHT2X_READ_LEN_CRC   7
#define AHT2X_RAW_DIVISOR    1048576.0f

/* AHT-family CRC8 used by CRC-capable parts: polynomial 0x31, initial value 0xFF. */
#define AHT2X_CRC8_POLY      0x31
#define AHT2X_CRC8_INIT      0xFF

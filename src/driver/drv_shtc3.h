#define SHTC3_DELAY         4

/* Exported Macros -----------------------------------------------------------*/
#define SHTC3_I2C_ADDR						(0x70 << 1)
#define SHTC3_I2C_BUFFER_LEN_MAX			8

#define SHTC3_CMD_READ_ID					0xEFC8 /* command: read ID register */
#define SHTC3_CMD_SOFT_RESET				0x805D /* soft reset */
#define SHTC3_CMD_SLEEP						0xB098 /* sleep */
#define SHTC3_CMD_WAKEUP					0x3517 /* wakeup */

#define SHTC3_CMD_MEAS_T_RH_POLLING_NM		0x7866 /* meas. read T first, clock stretching disabled in normal mode */
#define SHTC3_CMD_MEAS_T_RH_CLOCKSTR_NM		0x7CA2 /* meas. read T first, clock stretching enabled in normal mode */
#define SHTC3_CMD_MEAS_RH_T_POLLING_NM		0x58E0 /* meas. read RH first, clock stretching disabled in normal mode */
#define SHTC3_CMD_MEAS_RH_T_CLOCKSTR_NM		0x5C24 /* meas. read RH first, clock stretching enabled in normal mode */
#define SHTC3_CMD_MEAS_T_RH_POLLING_LPM		0x609C /* meas. read T first, clock stretching disabled in low power mode */
#define SHTC3_CMD_MEAS_T_RH_CLOCKSTR_LPM	0x6458 /* meas. read T first, clock stretching enabled in low power mode */
#define SHTC3_CMD_MEAS_RH_T_POLLING_LPM		0x401A /* meas. read RH first, clock stretching disabled in low power mode */
#define SHTC3_CMD_MEAS_RH_T_CLOCKSTR_LPM	0x44DE /* meas. read RH first, clock stretching enabled in low power mode */
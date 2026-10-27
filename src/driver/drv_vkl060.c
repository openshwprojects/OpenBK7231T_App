/**
 * drv_vkl060.c - VKL060 segment LCD driver
 *
 * Ported from src/lcd_lktmzl02.c in pvvx/ZigbeeTLc.
 *
 * The LCD is a 6-digit (4 big + 2 small) segment display with °C/°F, "%",
 * battery and link symbols. Protocol is soft I2C bit-bang, open-drain
 * style (drive-low for "0", release-with-pullup for "1"), 7-bit address
 * 0x3E. All bus transactions are byte-for-byte identical to the original.
 *
 * Usage:
 *   startDriver VKL060 28 9 2 3
 *   (scl=28, sda=9, temp channel 2, humidity channel 3)
 *
 * Temperature channel value is in 0.1 °C (like channel type temperature_div10);
 * display unit defaults to Fahrenheit, change with VKL060_SetUnit C|F|N.
 *
 * Battery: the battery icon is shown automatically when the BATTERY driver is
 * running. The level comes from Battery_lastreading(OBK_BATT_LEVEL). Alternatively
 * pass a channel number in the 5th startDriver argument and its value (0..100) is
 * used instead. Icon: >=80%% three bars, >=50%% two, >=25%% one, below that empty
 * outline.
 */
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_vkl060.h"
#include "drv_battery.h"
#include "../hal/hal_pins.h"

#define VKL060_I2C_ADDR		0x3E // VKL060 (7-bit)

#define VKL060_BUF_SIZE		7

#define VKL060_BIT(n)		(1u << (n))

// Temperature symbol (bit pattern used by show_big_number_x10):
// 0x00 = "  ", 0x20 = "°Г", 0x40 = " -", 0x60 = "°F", 0x80 = " _",
// 0xA0 = "°C", 0xC0 = " =", 0xE0 = "°E"
#define TEMP_SYMBOL_NONE	0x00
#define TEMP_SYMBOL_F		0x60
#define TEMP_SYMBOL_C		0xA0

static softI2C_t g_i2c;
static uint8_t g_i2cAddress = VKL060_I2C_ADDR << 1; // 8-bit: 0x7C

static uint8_t g_displayBuff[VKL060_BUF_SIZE];
static uint8_t g_displayCmpBuff[VKL060_BUF_SIZE];
static uint8_t g_blinkFlg;
static uint8_t g_displayOff;

static uint8_t g_unit = TEMP_SYMBOL_F; // default display unit: Fahrenheit
static int g_chTemp = -1, g_chHumid = -1, g_chBatt = -1;
static bool g_connectedAuto = true;
static bool g_isWorking = false;

/* 0,1,2,3,4,5,6,7,8,9,A,b,C,d,E,F */
static const uint8_t display_numbers[] = {
		// 76543210
		0b011111010, // 0
		0b000001010, // 1
		0b011010110, // 2
		0b010011110, // 3
		0b000101110, // 4
		0b010111100, // 5
		0b011111100, // 6
		0b000011010, // 7
		0b011111110, // 8
		0b010111110, // 9
		0b001111110, // A
		0b011101100, // b
		0b011110000, // C
		0b011001110, // d
		0b011110100, // E
		0b001110100  // F
};
                    //76543210
#define LCD_SYM_b  0b011101100 // "b"
#define LCD_SYM_H  0b001101110 // "H"
#define LCD_SYM_h  0b001101100 // "h"
#define LCD_SYM_i  0b001000000 // "i"
#define LCD_SYM_L  0b011100000 // "L"
#define LCD_SYM_o  0b011001100 // "o"
#define LCD_SYM_t  0b011100100 // "t"
#define LCD_SYM_0  0b011111010 // "0"
#define LCD_SYM_A  0b001111110 // "A"
#define LCD_SYM_a  0b011011110 // "a"
#define LCD_SYM_P  0b001110110 // "P"
#define LCD_SYM_E  0b011110100 // "E"

#define LCD_SYM_BLE	0x07	// connect
#define LCD_SYM_BAT	0xf0	// battery

/* LCD controller initialize */
static const uint8_t lcd_init_cmd[] = {
		// LCD controller initialize:
		0xea, // System Set: Software Reset, Internal oscillator circuit
//		0xe8, // System Set: Internal oscillator circuit
		0xc8, // Mode Set: Display enable, 1/3 Bias
		0xbc, // Display control: 52 Hz, FRAME flip, low power mode1
//		0x80, // load data pointer
//		0xf0, // blink control off,  0xf2 - blink
//		0xfc, // All pixel control (APCTL): Normal
		0x0c, 0,0,0,0,0,0,0
};

/* LCD controller off - all chips sleep power 2..3 uA */
static const uint8_t lcd_off_cmd[] = { // sleep all 3.0 uA
		0xea, // Set IC Operation(ICSET): Software Reset, Internal oscillator circuit
		0xbc, // Display control (DISCTL): Power save mode 3, FRAME flip, Power save mode 1
		0xd0  // Mode Set (MODE SET): Display disable, 1/3 Bias, power saving
};

static bool VKL060_SendBytes(const uint8_t *buf, int size) {
	bool ok = Soft_I2C_Start(&g_i2c, g_i2cAddress);
	for (int i = 0; ok && i < size; i++) {
		ok = Soft_I2C_WriteByte(&g_i2c, buf[i]);
	}
	Soft_I2C_Stop(&g_i2c);
	return ok;
}

static bool VKL060_SendByte(uint8_t b) {
	return VKL060_SendBytes(&b, 1);
}

static void VKL060_SendToLcd(void) {
	if (!g_i2cAddress) {
		return;
	}
	uint8_t buf[8];
	buf[0] = 0x0b;
	buf[1] = g_displayCmpBuff[0];
	buf[2] = g_displayCmpBuff[1];
	buf[3] = g_displayCmpBuff[2];
	buf[4] = (g_displayCmpBuff[4] >> 4) | (g_displayCmpBuff[3] << 4);
	buf[5] = (g_displayCmpBuff[5] >> 4) | (g_displayCmpBuff[4] << 4);
	buf[6] = (g_displayCmpBuff[6] >> 4) | (g_displayCmpBuff[5] << 4);
	buf[7] = (g_displayCmpBuff[6] << 4);
	VKL060_SendBytes(buf, sizeof(buf));
	if (g_blinkFlg) {
		VKL060_SendByte(g_blinkFlg);
		if (g_blinkFlg > 0xf0) {
			g_blinkFlg = 0xf0;
		} else {
			g_blinkFlg = 0;
		}
	}
}

/* symbol: 0x00 = "  ", 0x60 = "°F", 0xA0 = "°C", 0xE0 = "°E" */
static void VKL060_ShowTempSymbol(uint8_t symbol) {
	if (symbol & 0x20) {
		g_displayBuff[3] |= VKL060_BIT(1);
	} else {
		g_displayBuff[3] &= ~(VKL060_BIT(1));
	}
	if (symbol & 0x40) {
		g_displayBuff[3] |= VKL060_BIT(0); //"-"
	} else {
		g_displayBuff[3] &= ~VKL060_BIT(0); //"-"
	}
	if (symbol & 0x80) {
		g_displayBuff[3] |= VKL060_BIT(2); // "_"
	} else {
		g_displayBuff[3] &= ~VKL060_BIT(2); // "_"
	}
}

/* "link lost" indicator, inverted like LKTMZL02 (USE_DISPLAY_CONNECT_SYMBOL=2):
 * symbol lights up when NOT connected. */
static void VKL060_SetConnected(bool state) {
	if (state) {
		g_displayBuff[0] &= ~LCD_SYM_BLE;
	} else {
		g_displayBuff[0] |= LCD_SYM_BLE;
	}
}

/* battery_level: 0..100 */
static void VKL060_SetBattery(bool state, uint8_t battery_level) {
	g_displayBuff[0] &= 0x0f;
	if (state) {
		g_displayBuff[0] |= VKL060_BIT(7);
		if (battery_level >= 25) {
			g_displayBuff[0] |= VKL060_BIT(5);
			if (battery_level >= 50) {
				g_displayBuff[0] |= VKL060_BIT(6);
				if (battery_level >= 80) {
					g_displayBuff[0] |= VKL060_BIT(4);
				}
			}
		}
	}
}

/* number: in 0.1 (-19995..19995), Show: -1999 .. -199.9 .. 199.9 .. 1999
 * symbol: 0x00 = "  ", 0x60 = "°F", 0xA0 = "°C", 0xE0 = "°E" */
static void VKL060_DrawTemp(int number, uint8_t symbol) {
	g_displayBuff[3] = 0;
	if (symbol & 0x20) {
		g_displayBuff[3] |= VKL060_BIT(1); // "°Г"
	}
	if (symbol & 0x40) {
		g_displayBuff[3] |= VKL060_BIT(0); //"-"
	}
	if (symbol & 0x80) {
		g_displayBuff[3] |= VKL060_BIT(2); // "_"
	}

	g_displayBuff[4] = 0;

	if (number > 19995) {
		g_displayBuff[1] = LCD_SYM_H; // "H"
		g_displayBuff[2] = LCD_SYM_i; // "i"
	} else if (number < -9995) {
		g_displayBuff[1] = LCD_SYM_L; // "L"
		g_displayBuff[2] = LCD_SYM_o; // "o"
	} else {
		/* number: -19995..19995 */
		g_displayBuff[1] = 0;
		g_displayBuff[2] = 0;
		if (number > 1999 || number < -1999) {
			/* number: -19995..-2000, 2000..19995 */
			// round(div 10)
			number += 5;
			number /= 10;
			// show no point: -1999..-200, 200..1999
		} else {
			// show point: -199.9..199.9
			g_displayBuff[3] |= VKL060_BIT(3); // point top
		}
		/* show: -1999..1999 */
		if (number < 0) {
			number = -number;
			if (number > 99) {
				g_displayBuff[1] = VKL060_BIT(0); // "-"
			} else {
				g_displayBuff[1] = VKL060_BIT(2); // "-"
			}
		}
		/* number: -99..1999 */
		if (number > 999) {
			g_displayBuff[0] |= VKL060_BIT(3); // "1" 1000..1999
		} else {
			g_displayBuff[0] &= ~(VKL060_BIT(3)); // "0" -999..999
		}
		if (number > 99) {
			g_displayBuff[1] |= display_numbers[number / 100 % 10];
		}
		if (number > 9) {
			g_displayBuff[2] |= display_numbers[number / 10 % 10];
		} else {
			g_displayBuff[2] |= LCD_SYM_0; // "0"
		}
		g_displayBuff[4] = display_numbers[number % 10];
	}
}

/* -9 .. 99 */
static void VKL060_DrawHumidity(int number, bool percent) {
	g_displayBuff[5] = 0;
	g_displayBuff[6] = percent ? VKL060_BIT(0) : 0;
	if (number > 99) {
		g_displayBuff[5] |= LCD_SYM_H; // "H"
		g_displayBuff[6] |= LCD_SYM_i; // "i"
	} else if (number < -9) {
		g_displayBuff[5] |= LCD_SYM_L; // "L"
		g_displayBuff[6] |= LCD_SYM_o; // "o"
	} else {
		if (number < 0) {
			number = -number;
			g_displayBuff[5] = VKL060_BIT(2); // "-"
		}
		if (number > 9) {
			g_displayBuff[5] = display_numbers[number / 10 % 10];
		}
		g_displayBuff[6] |= display_numbers[number % 10];
	}
}

static void VKL060_Update(void) {
	if (!g_displayOff
	 && memcmp(g_displayCmpBuff, g_displayBuff, sizeof(g_displayBuff))) {
		memcpy(g_displayCmpBuff, g_displayBuff, sizeof(g_displayBuff));
		VKL060_SendToLcd();
	}
}

static void VKL060_Off(void) {
	g_displayOff = 1;
	VKL060_SendBytes(lcd_off_cmd, sizeof(lcd_off_cmd));
}

static void VKL060_On(void) {
	g_displayOff = 0;
	// Soft_I2C functions return true on ACK/success (unlike ZigbeeTLc,
	// where the I2C send returns nonzero on error)
	if (VKL060_SendBytes(lcd_init_cmd, sizeof(lcd_init_cmd))) {
		rtos_delay_milliseconds(1);
		g_blinkFlg = 0;
		memset(g_displayBuff, 0xff, sizeof(g_displayBuff));
		memset(g_displayCmpBuff, 0, sizeof(g_displayCmpBuff));
		VKL060_Update();
		g_isWorking = true;
	} else {
		ADDLOG_INFO(LOG_FEATURE_DRV, "VKL060: LCD init failed (NACK), check SCL/SDA pins!");
		VKL060_Off();
		g_isWorking = false;
	}
}

/* Convert a channel value °C to °F */
static int VKL060_ConvertCx10ToFx10(int cx10) {
	return (cx10 * 18 + 5) / 10 + 320;
}

/* Show temperature. value_x10 is always in 0.1 °C (like channel type
 * temperature_div10); it is converted to °F when unit is Fahrenheit. */
void VKL060_ShowTemperature(int value_x10) {
	int v = value_x10;
	uint8_t symbol = g_unit;
	if (g_unit == TEMP_SYMBOL_F) {
		v = VKL060_ConvertCx10ToFx10(v);
	}
	VKL060_DrawTemp(v, symbol);
}

void VKL060_ShowHumidity(int percent) {
	VKL060_DrawHumidity(percent, true);
}

void VKL060_ShowBattery(uint8_t level) {
	VKL060_SetBattery(true, level);
}

static void VKL060_ShowErr(void) {
	g_displayBuff[0] &= LCD_SYM_BAT | LCD_SYM_BLE;
	g_displayBuff[1] = LCD_SYM_E; // "E"
	g_displayBuff[2] = LCD_SYM_E; // "E"
	g_displayBuff[3] = 0;
	g_displayBuff[4] = 0;
	g_displayBuff[5] = LCD_SYM_E; // "E"
	g_displayBuff[6] = LCD_SYM_E; // "E"
}

static void VKL060_ShowReset(void) {
	g_displayBuff[0] = 0;
	g_displayBuff[1] = LCD_SYM_o; // "o"
	g_displayBuff[2] = LCD_SYM_o; // "o"
	g_displayBuff[3] = 0;
	g_displayBuff[4] = 0;
	g_displayBuff[5] = LCD_SYM_o; // "o"
	g_displayBuff[6] = LCD_SYM_o; // "o"
	g_blinkFlg = 0xf2;
}

/*********************************************************************
 * Commands
 *********************************************************************/

commandResult_t VKL060_TestCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	VKL060_On();
	if (!g_isWorking) {
		return CMD_RES_ERROR;
	}
	// "88.8" big + "88" small + full battery
	g_displayBuff[0] = LCD_SYM_BAT;
	g_displayBuff[1] = display_numbers[8];
	g_displayBuff[2] = display_numbers[8];
	g_displayBuff[3] = TEMP_SYMBOL_C | VKL060_BIT(3); // "°C" + point
	g_displayBuff[4] = display_numbers[8];
	g_displayBuff[5] = display_numbers[8];
	g_displayBuff[6] = display_numbers[8] | VKL060_BIT(0); // "%"
	g_blinkFlg = 0;
	VKL060_Update();
	return CMD_RES_OK;
}

commandResult_t VKL060_OnCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	VKL060_On();
	return CMD_RES_OK;
}

commandResult_t VKL060_OffCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	VKL060_Off();
	return CMD_RES_OK;
}

commandResult_t VKL060_SetUnitCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	const char *s = Tokenizer_GetArg(0);
	if (!stricmp(s, "F")) {
		g_unit = TEMP_SYMBOL_F;
	} else if (!stricmp(s, "C")) {
		g_unit = TEMP_SYMBOL_C;
	} else if (!stricmp(s, "N")) {
		g_unit = TEMP_SYMBOL_NONE;
	} else {
		return CMD_RES_BAD_ARGUMENT;
	}
	ADDLOG_INFO(LOG_FEATURE_DRV, "VKL060: display unit set to %s", s);
	return CMD_RES_OK;
}

/* VKL060_ShowTemp <x10> - value in 0.1 °C, displayed using current unit */
commandResult_t VKL060_ShowTempCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int v = Tokenizer_GetArgInteger(0);
	VKL060_ShowTemperature(v);
	VKL060_Update();
	return CMD_RES_OK;
}

/* VKL060_ShowHumidity <percent> */
commandResult_t VKL060_ShowHumidityCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int v = Tokenizer_GetArgInteger(0);
	VKL060_ShowHumidity(v);
	VKL060_Update();
	return CMD_RES_OK;
}

/* VKL060_ShowBattery <0..100> */
commandResult_t VKL060_ShowBatteryCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int v = Tokenizer_GetArgInteger(0);
	if (v < 0) {
		v = 0;
	}
	if (v > 100) {
		v = 100;
	}
	VKL060_SetBattery(true, v);
	VKL060_Update();
	return CMD_RES_OK;
}

/* VKL060_SetConnected <0|1> - 1 = connected (link symbol off, inverted like LKTMZL02) */
commandResult_t VKL060_SetConnectedCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_connectedAuto = false;
	VKL060_SetConnected(Tokenizer_GetArgInteger(0) ? true : false);
	VKL060_Update();
	return CMD_RES_OK;
}

/* VKL060_ShowErr - sensor read error screen */
commandResult_t VKL060_ShowErrCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	VKL060_ShowErr();
	VKL060_Update();
	return CMD_RES_OK;
}

/* VKL060_ShowReset - "oo" screen (blinking) */
commandResult_t VKL060_ShowResetCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	VKL060_ShowReset();
	VKL060_Update();
	return CMD_RES_OK;
}

/*********************************************************************
 * Driver entry points
 *********************************************************************/

// startDriver VKL060 <scl> <sda> [tempCh] [humidCh] [battCh]
// defaults: scl = P28, sda = P9
void VKL060_Init() {
	g_i2c.pin_clk = Tokenizer_GetPin(1, 28);
	g_i2c.pin_data = Tokenizer_GetPin(2, 9);
	g_chTemp = Tokenizer_GetArgIntegerDefault(3, -1);
	g_chHumid = Tokenizer_GetArgIntegerDefault(4, -1);
	g_chBatt = Tokenizer_GetArgIntegerDefault(5, -1);

	if (!Soft_I2C_PreInit(&g_i2c)) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "VKL060: SCL/SDA pins stuck low, check wiring!");
	}
	rtos_delay_milliseconds(10);

	VKL060_On();

	//cmddetail:{"name":"VKL060_Test","args":"",
	//cmddetail:"descr":"Initialize LCD and show a test pattern (88.8 + 88%).",
	//cmddetail:"fn":"VKL060_TestCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"startDriver VKL060 28 9; VKL060_Test"}
	CMD_RegisterCommand("VKL060_Test", VKL060_TestCmd, NULL);
	//cmddetail:{"name":"VKL060_On","args":"",
	//cmddetail:"descr":"Switch the LCD display on.",
	//cmddetail:"fn":"VKL060_OnCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_On"}
	CMD_RegisterCommand("VKL060_On", VKL060_OnCmd, NULL);
	//cmddetail:{"name":"VKL060_Off","args":"",
	//cmddetail:"descr":"Switch the LCD display off (sleep mode, ~2-3 uA).",
	//cmddetail:"fn":"VKL060_OffCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_Off"}
	CMD_RegisterCommand("VKL060_Off", VKL060_OffCmd, NULL);
	//cmddetail:{"name":"VKL060_SetUnit","args":"[C|F|N]",
	//cmddetail:"descr":"Set the temperature display unit (C = Celsius, F = Fahrenheit default, N = no symbol).",
	//cmddetail:"fn":"VKL060_SetUnitCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_SetUnit C"}
	CMD_RegisterCommand("VKL060_SetUnit", VKL060_SetUnitCmd, NULL);
	//cmddetail:{"name":"VKL060_ShowTemp","args":"[x10]",
	//cmddetail:"descr":"Show temperature, value is in 0.1 C (e.g. 235 = 23.5 C), converted to the current unit.",
	//cmddetail:"fn":"VKL060_ShowTempCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_ShowTemp 235"}
	CMD_RegisterCommand("VKL060_ShowTemp", VKL060_ShowTempCmd, NULL);
	//cmddetail:{"name":"VKL060_ShowHumidity","args":"[percent]",
	//cmddetail:"descr":"Show humidity in percent (e.g. 62 = 62%).",
	//cmddetail:"fn":"VKL060_ShowHumidityCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_ShowHumidity 62"}
	CMD_RegisterCommand("VKL060_ShowHumidity", VKL060_ShowHumidityCmd, NULL);
	//cmddetail:{"name":"VKL060_ShowBattery","args":"[0..100]",
	//cmddetail:"descr":"Show the battery icon with the given level (percent).",
	//cmddetail:"fn":"VKL060_ShowBatteryCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_ShowBattery 100"}
	CMD_RegisterCommand("VKL060_ShowBattery", VKL060_ShowBatteryCmd, NULL);
	//cmddetail:{"name":"VKL060_SetConnected","args":"[0|1]",
	//cmddetail:"descr":"Set link indicator (inverted, like LKTMZL02: symbol shows when NOT connected). When not called, the driver shows the WiFi connection state automatically.",
	//cmddetail:"fn":"VKL060_SetConnectedCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_SetConnected 1"}
	CMD_RegisterCommand("VKL060_SetConnected", VKL060_SetConnectedCmd, NULL);
	//cmddetail:{"name":"VKL060_ShowErr","args":"",
	//cmddetail:"descr":"Show 'EE.EE' sensor error screen.",
	//cmddetail:"fn":"VKL060_ShowErrCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_ShowErr"}
	CMD_RegisterCommand("VKL060_ShowErr", VKL060_ShowErrCmd, NULL);
	//cmddetail:{"name":"VKL060_ShowReset","args":"",
	//cmddetail:"descr":"Show blinking 'oo.oo' reset screen.",
	//cmddetail:"fn":"VKL060_ShowResetCmd","file":"driver/drv_vkl060.c","requires":"",
	//cmddetail:"examples":"VKL060_ShowReset"}
	CMD_RegisterCommand("VKL060_ShowReset", VKL060_ShowResetCmd, NULL);
}

void VKL060_OnEverySecond() {
	if (!g_isWorking) {
		return;
	}
	if (g_connectedAuto) {
		VKL060_SetConnected(Main_IsConnectedToWiFi() ? true : false);
	}
	if (g_chTemp >= 0) {
		int v = CHANNEL_Get(g_chTemp);
		VKL060_ShowTemperature(v);
	}
	if (g_chHumid >= 0) {
		int v = CHANNEL_Get(g_chHumid);
		VKL060_ShowHumidity(v);
	}
	if (g_chBatt >= 0) {
		int v = CHANNEL_Get(g_chBatt);
		if (v < 0) {
			v = 0;
		}
		if (v > 100) {
			v = 100;
		}
		VKL060_SetBattery(true, v);
	} else if (DRV_IsRunning("BATTERY")) {
		int v = Battery_lastreading(OBK_BATT_LEVEL);
		if (v < 0) {
			v = 0;
		}
		if (v > 100) {
			v = 100;
		}
		VKL060_SetBattery(true, v);
	}
	VKL060_Update();
}

void VKL060_StopDriver() {
	VKL060_Off();
}

void VKL060_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState) {
		return;
	}
	hprintf255(request, "<h2>VKL060 LCD</h2>");
	if (!g_isWorking) {
		hprintf255(request, "WARNING: LCD init failed, check SCL/SDA pins (defaults P28/P9)!");
	}
	hprintf255(request, "Unit: %s, temp channel: %d, humidity channel: %d, battery channel: %d<br />",
		(g_unit == TEMP_SYMBOL_F) ? "F" : (g_unit == TEMP_SYMBOL_C) ? "C" : "none",
		g_chTemp, g_chHumid, g_chBatt);
}

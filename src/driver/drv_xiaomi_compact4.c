#include "../new_common.h"

#if ENABLE_DRIVER_XIAOMI_COMPACT4

#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "../hal/hal_flashVars.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_wifi.h"
#include "../httpserver/new_http.h"
#if ENABLE_HA_DISCOVERY
#include "../httpserver/hass.h"
#endif
#include "drv_local.h"

#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/uart.h"

#define XIAOMI_C4_PIN_PM25_TX 17
#define XIAOMI_C4_PIN_PM25_RX 16
#define XIAOMI_C4_UART_PORT UART_NUM_1
#define XIAOMI_C4_UART_BAUD 9600
#define XIAOMI_C4_UART_BUF_SIZE 256

#define XIAOMI_C4_PIN_I2C_SDA 14
#define XIAOMI_C4_PIN_I2C_SCL 27
#define XIAOMI_C4_I2C_PORT I2C_NUM_0
#define XIAOMI_C4_I2C_FREQ_HZ 100000
#define XIAOMI_C4_I2C_BRIGHTNESS 0x24
#define XIAOMI_C4_I2C_STATUS_LEDS 0x34
#define XIAOMI_C4_I2C_BUTTON_POWER 0x35
#define XIAOMI_C4_I2C_BUTTON_BRIGHTNESS 0x36
#define XIAOMI_C4_I2C_BUTTON_MODE 0x37

#define XIAOMI_C4_PIN_MOTOR_EN 2
#define XIAOMI_C4_PIN_MOTOR_PWM 26
#define XIAOMI_C4_MOTOR_LEDC_TIMER LEDC_TIMER_1
#define XIAOMI_C4_MOTOR_LEDC_CHANNEL LEDC_CHANNEL_5
#define XIAOMI_C4_MOTOR_LEDC_RES LEDC_TIMER_13_BIT
#define XIAOMI_C4_MOTOR_DUTY_50_PERCENT 4096
#define XIAOMI_C4_PIN_TACH 34
#define XIAOMI_C4_PIN_BUTTON_MODE 5
#define XIAOMI_C4_PIN_BUTTON_LIGHT 18
#define XIAOMI_C4_PIN_BUTTON_POWER 19
#define XIAOMI_C4_PIN_BUZZER 4
#define XIAOMI_C4_PIN_LED_RED 21
#define XIAOMI_C4_PIN_LED_ORANGE 32
#define XIAOMI_C4_PIN_LED_GREEN 33

#define XIAOMI_C4_CH_POWER 0
#define XIAOMI_C4_CH_MODE 1
#define XIAOMI_C4_CH_BRIGHTNESS 2
#define XIAOMI_C4_CH_CHILD_LOCK 3
#define XIAOMI_C4_CH_PM25 4
#define XIAOMI_C4_CH_MOTOR_RPM 5
#define XIAOMI_C4_CH_FILTER_USAGE 6
#define XIAOMI_C4_CH_FILTER_HEALTH 7
#define XIAOMI_C4_CH_REPLACE_FILTER 8
#define XIAOMI_C4_CH_FAV_SPEED 9
#define XIAOMI_C4_CH_NIGHT_SPEED 10
#define XIAOMI_C4_CH_P_FACTOR 11
#define XIAOMI_C4_CH_FILTER_LIFESPAN 12
#define XIAOMI_C4_CH_BUZZER 13

#define XIAOMI_C4_FLASH_BASE 48
#define XIAOMI_C4_VAR_POWER (XIAOMI_C4_FLASH_BASE + 0)
#define XIAOMI_C4_VAR_MODE (XIAOMI_C4_FLASH_BASE + 1)
#define XIAOMI_C4_VAR_BRIGHTNESS (XIAOMI_C4_FLASH_BASE + 2)
#define XIAOMI_C4_VAR_CHILD_LOCK (XIAOMI_C4_FLASH_BASE + 3)
#define XIAOMI_C4_VAR_FAV_SPEED (XIAOMI_C4_FLASH_BASE + 4)
#define XIAOMI_C4_VAR_NIGHT_SPEED (XIAOMI_C4_FLASH_BASE + 5)
#define XIAOMI_C4_VAR_P_FACTOR_X100 (XIAOMI_C4_FLASH_BASE + 6)
#define XIAOMI_C4_VAR_FILTER_LIFESPAN (XIAOMI_C4_FLASH_BASE + 7)
#define XIAOMI_C4_VAR_FILTER_USAGE (XIAOMI_C4_FLASH_BASE + 8)
#define XIAOMI_C4_VAR_BUZZER (XIAOMI_C4_FLASH_BASE + 9)

#define XIAOMI_C4_MODE_FAV 0
#define XIAOMI_C4_MODE_NIGHT 1
#define XIAOMI_C4_MODE_AUTO 2
#define XIAOMI_C4_BRIGHTNESS_FULL 0
#define XIAOMI_C4_BRIGHTNESS_MID 1
#define XIAOMI_C4_BRIGHTNESS_ZERO 2

#define XIAOMI_C4_BUTTON_LED_ON 0x3C
#define XIAOMI_C4_BUTTON_LED_OFF 0x00
#define XIAOMI_C4_STATUS_REPLACE_FILTER 0x01
#define XIAOMI_C4_STATUS_WIFI 0x02
#define XIAOMI_C4_STATUS_CHILD_LOCK 0x04
#define XIAOMI_C4_STATUS_MODE_AUTO 0x08
#define XIAOMI_C4_STATUS_MODE_NIGHT 0x10
#define XIAOMI_C4_STATUS_MODE_FAV 0x20

#define XIAOMI_C4_PM25_FRAME_LEN 20
#define XIAOMI_C4_PM25_MAX_VALID 2000
#define XIAOMI_C4_PM25_HIGH_LOG_THRESHOLD 300
#define XIAOMI_C4_PM25_POLL_SECONDS 5
#define XIAOMI_C4_MOTOR_RAMP_UP_PERCENT_PER_SEC 6
#define XIAOMI_C4_MOTOR_RAMP_DOWN_PERCENT_PER_SEC 12
#define XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS 0
#define XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS 365
#define XIAOMI_C4_FILTER_LIFESPAN_DEFAULT_DAYS 365
#define XIAOMI_C4_LONG_PRESS_MS 7000

static int g_power;
static int g_mode;
static int g_brightness;
static int g_childLock;
static int g_favSpeed;
static int g_nightSpeed;
static int g_pFactorX100;
static int g_filterLifespanDays;
static int g_buzzer;
static uint32_t g_filterUsageSeconds;
static int g_lastPm25 = -1;
static int g_lastMotorRpm;
static int g_motorTargetPercent;
static int g_motorCurrentPercent;
static int g_lastButtonPower;
static int g_lastButtonLight;
static int g_lastButtonMode;
static int g_powerPressTicks;
static int g_lightPressTicks;
static int g_modePressTicks;
static int g_modeLongHandled;
static uint32_t g_tachPulses;
static int g_pm25PollCountdown;
static int g_filterCountdown;
static int g_childLockNotificationTicks;
static int g_childLockNotificationFlips;
static int g_childLockNotificationState;
static int g_ignoreChannelChange;
static int g_initialized;
static uint8_t g_pm25RxBuf[XIAOMI_C4_UART_BUF_SIZE];
static int g_pm25RxLen;
static uint8_t g_pm25LastFrame[XIAOMI_C4_PM25_FRAME_LEN];
static int g_pm25LastRaw = -1;
static int g_pm25AcceptedFrames;
static int g_pm25RejectedChecksum;
static int g_pm25RejectedRange;
static i2c_master_bus_handle_t g_i2cBus;
static i2c_master_dev_handle_t g_i2cBrightness;
static i2c_master_dev_handle_t g_i2cStatusLeds;
static i2c_master_dev_handle_t g_i2cButtonPower;
static i2c_master_dev_handle_t g_i2cButtonBrightness;
static i2c_master_dev_handle_t g_i2cButtonMode;

static void XiaomiCompact4_ApplyState(int save);

static int XiaomiCompact4_ClampInt(int value, int min, int max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

static float XiaomiCompact4_ClampFloat(float value, float min, float max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

static int XiaomiCompact4_LoadInt(int index, int def, int min, int max) {
	int value = HAL_FlashVars_GetChannelValue(index);
	if (value < min || value > max) {
		value = def;
	}
	return value;
}

static void XiaomiCompact4_SaveInt(int index, int value) {
	HAL_FlashVars_SaveChannel(index, value);
}

static void XiaomiCompact4_EnsureFilterLifespan(void) {
	if (g_filterLifespanDays <= 0) {
		g_filterLifespanDays = XIAOMI_C4_FILTER_LIFESPAN_DEFAULT_DAYS;
		XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FILTER_LIFESPAN, g_filterLifespanDays);
	}
}

static const char *XiaomiCompact4_ModeToStr(int mode) {
	switch (mode) {
	case XIAOMI_C4_MODE_FAV:
		return "FAV";
	case XIAOMI_C4_MODE_NIGHT:
		return "NIGHT";
	default:
		return "AUTO";
	}
}

static const char *XiaomiCompact4_BrightnessToStr(int brightness) {
	switch (brightness) {
	case XIAOMI_C4_BRIGHTNESS_MID:
		return "MID";
	case XIAOMI_C4_BRIGHTNESS_ZERO:
		return "ZERO";
	default:
		return "FULL";
	}
}

static int XiaomiCompact4_ParseMode(const char *s) {
	if (!stricmp(s, "FAV")) return XIAOMI_C4_MODE_FAV;
	if (!stricmp(s, "NIGHT")) return XIAOMI_C4_MODE_NIGHT;
	return XIAOMI_C4_MODE_AUTO;
}

static int XiaomiCompact4_ParseBrightness(const char *s) {
	if (!stricmp(s, "MID")) return XIAOMI_C4_BRIGHTNESS_MID;
	if (!stricmp(s, "ZERO")) return XIAOMI_C4_BRIGHTNESS_ZERO;
	return XIAOMI_C4_BRIGHTNESS_FULL;
}

static void XiaomiCompact4_I2CInit(void) {
	i2c_master_bus_config_t busConfig = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = XIAOMI_C4_I2C_PORT,
		.sda_io_num = XIAOMI_C4_PIN_I2C_SDA,
		.scl_io_num = XIAOMI_C4_PIN_I2C_SCL,
		.flags.enable_internal_pullup = true,
	};
	if (i2c_new_master_bus(&busConfig, &g_i2cBus) != ESP_OK) {
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 I2C driver install failed");
		return;
	}

	i2c_device_config_t devConfig = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.scl_speed_hz = XIAOMI_C4_I2C_FREQ_HZ,
	};

	devConfig.device_address = XIAOMI_C4_I2C_BRIGHTNESS;
	i2c_master_bus_add_device(g_i2cBus, &devConfig, &g_i2cBrightness);
	devConfig.device_address = XIAOMI_C4_I2C_STATUS_LEDS;
	i2c_master_bus_add_device(g_i2cBus, &devConfig, &g_i2cStatusLeds);
	devConfig.device_address = XIAOMI_C4_I2C_BUTTON_POWER;
	i2c_master_bus_add_device(g_i2cBus, &devConfig, &g_i2cButtonPower);
	devConfig.device_address = XIAOMI_C4_I2C_BUTTON_BRIGHTNESS;
	i2c_master_bus_add_device(g_i2cBus, &devConfig, &g_i2cButtonBrightness);
	devConfig.device_address = XIAOMI_C4_I2C_BUTTON_MODE;
	i2c_master_bus_add_device(g_i2cBus, &devConfig, &g_i2cButtonMode);
}

static void XiaomiCompact4_I2CWrite1(int addr7, int value) {
	uint8_t data = (uint8_t)value;
	i2c_master_dev_handle_t dev;
	switch (addr7) {
	case XIAOMI_C4_I2C_BRIGHTNESS:
		dev = g_i2cBrightness;
		break;
	case XIAOMI_C4_I2C_STATUS_LEDS:
		dev = g_i2cStatusLeds;
		break;
	case XIAOMI_C4_I2C_BUTTON_POWER:
		dev = g_i2cButtonPower;
		break;
	case XIAOMI_C4_I2C_BUTTON_BRIGHTNESS:
		dev = g_i2cButtonBrightness;
		break;
	case XIAOMI_C4_I2C_BUTTON_MODE:
		dev = g_i2cButtonMode;
		break;
	default:
		dev = NULL;
		break;
	}
	if (dev == NULL) {
		return;
	}
	esp_err_t err = i2c_master_transmit(dev, &data, 1, 100);
	if (err != ESP_OK) {
		ADDLOG_DEBUG(LOG_FEATURE_DRV, "XiaomiCompact4 I2C write failed addr 0x%02X err %i", addr7, err);
	}
}

static void XiaomiCompact4_UARTInit(void) {
	uart_config_t uart_config = {
		.baud_rate = XIAOMI_C4_UART_BAUD,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	if (uart_is_driver_installed(XIAOMI_C4_UART_PORT)) {
		uart_driver_delete(XIAOMI_C4_UART_PORT);
	}
	uart_param_config(XIAOMI_C4_UART_PORT, &uart_config);
	uart_set_pin(XIAOMI_C4_UART_PORT, XIAOMI_C4_PIN_PM25_TX, XIAOMI_C4_PIN_PM25_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(XIAOMI_C4_UART_PORT, XIAOMI_C4_UART_BUF_SIZE, 0, 0, NULL, 0);
	g_pm25RxLen = 0;
}

static void XiaomiCompact4_UARTReadAvailable(void) {
	uint8_t tmp[64];
	int len = uart_read_bytes(XIAOMI_C4_UART_PORT, tmp, sizeof(tmp), 0);
	if (len <= 0) {
		return;
	}
	if (g_pm25RxLen + len > (int)sizeof(g_pm25RxBuf)) {
		g_pm25RxLen = 0;
	}
	memcpy(g_pm25RxBuf + g_pm25RxLen, tmp, len);
	g_pm25RxLen += len;
}

static uint8_t XiaomiCompact4_UARTPeek(int index) {
	if (index < 0 || index >= g_pm25RxLen) {
		return 0;
	}
	return g_pm25RxBuf[index];
}

static void XiaomiCompact4_UARTConsume(int count) {
	if (count <= 0) {
		return;
	}
	if (count >= g_pm25RxLen) {
		g_pm25RxLen = 0;
		return;
	}
	memmove(g_pm25RxBuf, g_pm25RxBuf + count, g_pm25RxLen - count);
	g_pm25RxLen -= count;
}

static int XiaomiCompact4_PM25FrameChecksumValid(const uint8_t *frame) {
	uint8_t sum = 0;
	for (int i = 0; i < XIAOMI_C4_PM25_FRAME_LEN; i++) {
		sum += frame[i];
	}
	return sum == 0;
}

static void XiaomiCompact4_PM25FrameToHex(const uint8_t *frame, char *out, int outLen) {
	int pos = 0;
	for (int i = 0; i < XIAOMI_C4_PM25_FRAME_LEN && pos < outLen - 1; i++) {
		pos += snprintf(out + pos, outLen - pos, "%02X%s", frame[i], i == XIAOMI_C4_PM25_FRAME_LEN - 1 ? "" : " ");
	}
	out[outLen - 1] = '\0';
}

static void XiaomiCompact4_MotorPWMInit(void) {
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.timer_num = XIAOMI_C4_MOTOR_LEDC_TIMER,
		.duty_resolution = XIAOMI_C4_MOTOR_LEDC_RES,
		.freq_hz = 1000,
		.clk_cfg = SOC_MOD_CLK_RC_FAST,
	};
	ledc_channel_config_t channel = {
		.gpio_num = XIAOMI_C4_PIN_MOTOR_PWM,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = XIAOMI_C4_MOTOR_LEDC_CHANNEL,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = XIAOMI_C4_MOTOR_LEDC_TIMER,
		.duty = 0,
		.hpoint = 0,
	};
	ledc_timer_config(&timer);
	ledc_channel_config(&channel);
}

static void XiaomiCompact4_MotorPWMSet(int frequency) {
	if (frequency <= 0) {
		ledc_stop(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_CHANNEL, 0);
		return;
	}
	ledc_set_freq(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_TIMER, frequency);
	ledc_set_duty(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_CHANNEL, XIAOMI_C4_MOTOR_DUTY_50_PERCENT);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_CHANNEL);
}

static int XiaomiCompact4_MotorPercentToFrequency(int percent) {
	return (int)((float)percent * 4.12f + 100.0f);
}

static void XiaomiCompact4_ApplyMotorRamp(void) {
	if (!g_power) {
		g_motorCurrentPercent = 0;
		XiaomiCompact4_MotorPWMSet(0);
		return;
	}
	if (g_motorCurrentPercent < g_motorTargetPercent) {
		g_motorCurrentPercent += XIAOMI_C4_MOTOR_RAMP_UP_PERCENT_PER_SEC;
		if (g_motorCurrentPercent > g_motorTargetPercent) {
			g_motorCurrentPercent = g_motorTargetPercent;
		}
	} else if (g_motorCurrentPercent > g_motorTargetPercent) {
		g_motorCurrentPercent -= XIAOMI_C4_MOTOR_RAMP_DOWN_PERCENT_PER_SEC;
		if (g_motorCurrentPercent < g_motorTargetPercent) {
			g_motorCurrentPercent = g_motorTargetPercent;
		}
	}
	XiaomiCompact4_MotorPWMSet(XiaomiCompact4_MotorPercentToFrequency(g_motorCurrentPercent));
}

static void XiaomiCompact4_SetLedPWM(int pin, float level) {
	if (level <= 0.0f) {
		HAL_PIN_PWM_Update(pin, 0);
		return;
	}
	HAL_PIN_PWM_Update(pin, XiaomiCompact4_ClampFloat(level * 100.0f, 0.0f, 100.0f));
}

static int XiaomiCompact4_IsWiFiConnected(void) {
	const char *ip = HAL_GetMyIPString();
	return ip && strcmp(ip, "0.0.0.0");
}

static int XiaomiCompact4_ReplaceFilter(void) {
	if (g_filterLifespanDays <= 0) {
		return 1;
	}
	uint32_t limit = (uint32_t)g_filterLifespanDays * 24U * 3600U;
	return g_filterUsageSeconds > limit;
}

static int XiaomiCompact4_FilterHealth(void) {
	if (g_filterLifespanDays <= 0) {
		return 0;
	}
	return XiaomiCompact4_ClampInt(100 - (int)((100U * g_filterUsageSeconds) / ((uint32_t)g_filterLifespanDays * 24U * 3600U)), 0, 100);
}

static int XiaomiCompact4_FilterUsageDays(void) {
	return (int)(g_filterUsageSeconds / (24U * 3600U));
}

static void XiaomiCompact4_ResetFilter(void) {
	XiaomiCompact4_EnsureFilterLifespan();
	g_filterUsageSeconds = 0;
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetChannels(void) {
	g_ignoreChannelChange = 1;
	CHANNEL_Set(XIAOMI_C4_CH_POWER, g_power, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_MODE, g_mode, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_BRIGHTNESS, g_brightness, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_CHILD_LOCK, g_childLock, CHANNEL_SET_FLAG_SILENT);
	if (g_lastPm25 >= 0) {
		CHANNEL_Set(XIAOMI_C4_CH_PM25, g_lastPm25, CHANNEL_SET_FLAG_SILENT);
	}
	CHANNEL_Set(XIAOMI_C4_CH_MOTOR_RPM, g_lastMotorRpm, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_FILTER_USAGE, (int)g_filterUsageSeconds, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_FILTER_HEALTH, XiaomiCompact4_FilterHealth(), CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_REPLACE_FILTER, XiaomiCompact4_ReplaceFilter(), CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_FAV_SPEED, g_favSpeed, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_NIGHT_SPEED, g_nightSpeed, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_P_FACTOR, g_pFactorX100, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_FILTER_LIFESPAN, g_filterLifespanDays, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_BUZZER, g_buzzer, CHANNEL_SET_FLAG_SILENT);
	g_ignoreChannelChange = 0;
}

static void XiaomiCompact4_SaveState(void) {
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_POWER, g_power);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_MODE, g_mode);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_BRIGHTNESS, g_brightness);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_CHILD_LOCK, g_childLock);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FAV_SPEED, g_favSpeed);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_NIGHT_SPEED, g_nightSpeed);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_P_FACTOR_X100, g_pFactorX100);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FILTER_LIFESPAN, g_filterLifespanDays);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FILTER_USAGE, (int)g_filterUsageSeconds);
	XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_BUZZER, g_buzzer);
}

static void XiaomiCompact4_StartChildLockNotification(void) {
	g_childLockNotificationTicks = 1;
	g_childLockNotificationFlips = 8;
	g_childLockNotificationState = 0;
	HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, g_buzzer ? 50 : 0);
}

static void XiaomiCompact4_UpdateHID(void) {
	int isFullBacklight = g_brightness == XIAOMI_C4_BRIGHTNESS_FULL;
	int isNoBacklight = g_brightness == XIAOMI_C4_BRIGHTNESS_ZERO;
	int disableHid = isNoBacklight || !g_power;
	int expectedBacklight = (isFullBacklight && g_power) ? 8 : 3;
	float brightnessCoefficient = isFullBacklight ? 1.0f : 0.6f;
	int status = 0;

	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BRIGHTNESS, ((expectedBacklight == 8 ? 0 : expectedBacklight) << 4) | 0x01);
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BUTTON_MODE, disableHid ? XIAOMI_C4_BUTTON_LED_OFF : XIAOMI_C4_BUTTON_LED_ON);
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BUTTON_BRIGHTNESS, disableHid ? XIAOMI_C4_BUTTON_LED_OFF : XIAOMI_C4_BUTTON_LED_ON);
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BUTTON_POWER, XIAOMI_C4_BUTTON_LED_ON);

	if (!disableHid) {
		if (g_mode == XIAOMI_C4_MODE_FAV) status |= XIAOMI_C4_STATUS_MODE_FAV;
		else if (g_mode == XIAOMI_C4_MODE_NIGHT) status |= XIAOMI_C4_STATUS_MODE_NIGHT;
		else status |= XIAOMI_C4_STATUS_MODE_AUTO;
		if (XiaomiCompact4_IsWiFiConnected()) status |= XIAOMI_C4_STATUS_WIFI;
		if (XiaomiCompact4_ReplaceFilter()) status |= XIAOMI_C4_STATUS_REPLACE_FILTER;
	}
	if (g_childLock) {
		status |= XIAOMI_C4_STATUS_CHILD_LOCK;
	}
	if (g_childLockNotificationFlips > 0 && g_childLockNotificationState) {
		status ^= XIAOMI_C4_STATUS_CHILD_LOCK;
	}
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_STATUS_LEDS, status);

	if (disableHid || g_lastPm25 < 0) {
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_GREEN, 0);
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_ORANGE, 0);
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_RED, 0);
	} else {
		float green = 0.0f, orange = 0.0f, red = 0.0f;
		int pm = g_lastPm25;
		if (pm < 20) {
			green = 1.0f;
		} else if (pm < 40) {
			green = 0.6f;
			orange = 1.0f;
		} else if (pm < 60) {
			green = 0.35f;
			orange = 1.0f;
		} else if (pm < 100) {
			orange = 1.0f;
			red = 0.35f;
		} else {
			orange = 1.0f;
			red = 1.0f;
		}
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_GREEN, green * brightnessCoefficient);
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_ORANGE, orange * brightnessCoefficient);
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_RED, red * brightnessCoefficient);
	}
}

static void XiaomiCompact4_UpdateMotor(void) {
	if (!g_power) {
		g_motorTargetPercent = 0;
		g_motorCurrentPercent = 0;
		XiaomiCompact4_MotorPWMSet(0);
		return;
	}
	float motorSetpoint = 0.0f;
	if (g_mode == XIAOMI_C4_MODE_AUTO) {
		if (g_lastPm25 >= 0) {
			motorSetpoint = XiaomiCompact4_ClampFloat((float)g_lastPm25 * ((float)g_pFactorX100 / 100.0f), 0.0f, 100.0f);
		} else {
			motorSetpoint = g_favSpeed;
		}
	} else if (g_mode == XIAOMI_C4_MODE_FAV) {
		motorSetpoint = g_favSpeed;
	} else {
		motorSetpoint = g_nightSpeed;
	}
	g_motorTargetPercent = XiaomiCompact4_ClampInt((int)(motorSetpoint + 0.5f), 0, 100);
}

static void XiaomiCompact4_ApplyState(int save) {
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_EN, g_power ? 1 : 0);
	XiaomiCompact4_UpdateHID();
	XiaomiCompact4_UpdateMotor();
	XiaomiCompact4_SetChannels();
	if (save) {
		XiaomiCompact4_SaveState();
	}
}

static void XiaomiCompact4_SetPower(int value) {
	g_power = value ? 1 : 0;
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetMode(int value) {
	g_mode = XiaomiCompact4_ClampInt(value, XIAOMI_C4_MODE_FAV, XIAOMI_C4_MODE_AUTO);
	if (g_brightness == XIAOMI_C4_BRIGHTNESS_ZERO) {
		g_brightness = XIAOMI_C4_BRIGHTNESS_MID;
	}
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetBrightness(int value) {
	g_brightness = XiaomiCompact4_ClampInt(value, XIAOMI_C4_BRIGHTNESS_FULL, XIAOMI_C4_BRIGHTNESS_ZERO);
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetChildLock(int value) {
	g_childLock = value ? 1 : 0;
	XiaomiCompact4_StartChildLockNotification();
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_ProcessClick(int pin) {
	if (pin == XIAOMI_C4_PIN_BUTTON_POWER) {
		if (g_childLock) {
			XiaomiCompact4_StartChildLockNotification();
		} else {
			XiaomiCompact4_SetPower(!g_power);
		}
	} else if (pin == XIAOMI_C4_PIN_BUTTON_LIGHT) {
		if (!g_power) return;
		if (g_childLock) {
			XiaomiCompact4_StartChildLockNotification();
		} else {
			XiaomiCompact4_SetBrightness((g_brightness + 1) % 3);
		}
	} else if (pin == XIAOMI_C4_PIN_BUTTON_MODE) {
		if (!g_power) return;
		if (g_childLock) {
			XiaomiCompact4_StartChildLockNotification();
		} else {
			XiaomiCompact4_SetMode((g_mode + 1) % 3);
		}
	}
}

static void XiaomiCompact4_ServiceButton(int pin, int *last, int *pressTicks, int *longHandled) {
	int pressed = HAL_PIN_ReadDigitalInput(pin) == 0;
	if (pressed && !*last) {
		*pressTicks = 0;
		if (longHandled) *longHandled = 0;
	}
	if (pressed) {
		(*pressTicks)++;
		if (longHandled && !*longHandled && *pressTicks >= (XIAOMI_C4_LONG_PRESS_MS / 100)) {
			*longHandled = 1;
			XiaomiCompact4_SetChildLock(!g_childLock);
		}
	} else if (*last) {
		if (!longHandled || !*longHandled) {
			if (*pressTicks <= 10) {
				XiaomiCompact4_ProcessClick(pin);
			}
		}
		*pressTicks = 0;
	}
	*last = pressed;
}

static void XiaomiCompact4_TachISR(int gpio) {
	g_tachPulses++;
}

static void XiaomiCompact4_SendPM25Query(void) {
	static const byte query[] = { 0x11, 0x02, 0x0B, 0x01, 0xE1 };
	uart_write_bytes(XIAOMI_C4_UART_PORT, query, sizeof(query));
}

static void XiaomiCompact4_ProcessPM25UART(void) {
	XiaomiCompact4_UARTReadAvailable();
	while (g_pm25RxLen >= 3) {
		if (XiaomiCompact4_UARTPeek(0) != 0x16 || XiaomiCompact4_UARTPeek(1) != 0x11 || XiaomiCompact4_UARTPeek(2) != 0x0B) {
			XiaomiCompact4_UARTConsume(1);
			continue;
		}
		if (g_pm25RxLen < XIAOMI_C4_PM25_FRAME_LEN) {
			break;
		}
		uint8_t frame[XIAOMI_C4_PM25_FRAME_LEN];
		memcpy(frame, g_pm25RxBuf, XIAOMI_C4_PM25_FRAME_LEN);
		XiaomiCompact4_UARTConsume(XIAOMI_C4_PM25_FRAME_LEN);
		if (!XiaomiCompact4_PM25FrameChecksumValid(frame)) {
			g_pm25RejectedChecksum++;
			if ((g_pm25RejectedChecksum % 20) == 1) {
				char hex[XIAOMI_C4_PM25_FRAME_LEN * 3];
				XiaomiCompact4_PM25FrameToHex(frame, hex, sizeof(hex));
				ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 bad checksum frame: %s", hex);
			}
			continue;
		}
		int pm25 = (((int)frame[15]) << 8) | frame[16];
		memcpy(g_pm25LastFrame, frame, sizeof(g_pm25LastFrame));
		g_pm25LastRaw = pm25;
		if (pm25 > XIAOMI_C4_PM25_MAX_VALID) {
			g_pm25RejectedRange++;
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 out of range %i", pm25);
			continue;
		}
		if (pm25 >= XIAOMI_C4_PM25_HIGH_LOG_THRESHOLD) {
			char hex[XIAOMI_C4_PM25_FRAME_LEN * 3];
			XiaomiCompact4_PM25FrameToHex(frame, hex, sizeof(hex));
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 high %i frame: %s", pm25, hex);
		}
		g_pm25AcceptedFrames++;
		g_lastPm25 = pm25;
		XiaomiCompact4_UpdateHID();
		XiaomiCompact4_UpdateMotor();
		XiaomiCompact4_SetChannels();
	}
}

static void XiaomiCompact4_ServiceNotification(void) {
	if (g_childLockNotificationFlips <= 0) {
		HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, 0);
		return;
	}
	g_childLockNotificationTicks--;
	if (g_childLockNotificationTicks <= 0) {
		g_childLockNotificationTicks = 3;
		g_childLockNotificationState = !g_childLockNotificationState;
		g_childLockNotificationFlips--;
		if (g_childLockNotificationFlips == 0) {
			HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, 0);
		} else {
			HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, (g_buzzer && g_childLockNotificationState) ? 50 : 0);
		}
		XiaomiCompact4_UpdateHID();
	}
}

void XiaomiCompact4_RunQuickTick(void) {
	if (!g_initialized) return;
	XiaomiCompact4_ProcessPM25UART();
	XiaomiCompact4_ServiceButton(XIAOMI_C4_PIN_BUTTON_POWER, &g_lastButtonPower, &g_powerPressTicks, NULL);
	XiaomiCompact4_ServiceButton(XIAOMI_C4_PIN_BUTTON_LIGHT, &g_lastButtonLight, &g_lightPressTicks, NULL);
	XiaomiCompact4_ServiceButton(XIAOMI_C4_PIN_BUTTON_MODE, &g_lastButtonMode, &g_modePressTicks, &g_modeLongHandled);
	XiaomiCompact4_ServiceNotification();
}

void XiaomiCompact4_RunEverySecond(void) {
	if (!g_initialized) return;
	g_pm25PollCountdown--;
	if (g_pm25PollCountdown <= 0) {
		g_pm25PollCountdown = XIAOMI_C4_PM25_POLL_SECONDS;
		XiaomiCompact4_SendPM25Query();
	}
	g_filterCountdown--;
	if (g_filterCountdown <= 0) {
		g_filterCountdown = 60;
		if (g_power) {
			g_filterUsageSeconds += 60;
			XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FILTER_USAGE, (int)g_filterUsageSeconds);
		}
	}
	g_lastMotorRpm = (int)((g_tachPulses * 60U) / 15U);
	g_tachPulses = 0;
	XiaomiCompact4_ApplyMotorRamp();
	XiaomiCompact4_UpdateHID();
	XiaomiCompact4_SetChannels();
}

void XiaomiCompact4_Stop(void) {
	g_motorTargetPercent = 0;
	g_motorCurrentPercent = 0;
	XiaomiCompact4_MotorPWMSet(0);
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_EN, 0);
	HAL_DetachInterrupt(XIAOMI_C4_PIN_TACH);
	g_initialized = 0;
}

void XiaomiCompact4_OnChannelChanged(int ch, int value) {
	if (g_ignoreChannelChange || !g_initialized) return;
	switch (ch) {
	case XIAOMI_C4_CH_POWER:
		XiaomiCompact4_SetPower(value);
		break;
	case XIAOMI_C4_CH_MODE:
		XiaomiCompact4_SetMode(value);
		break;
	case XIAOMI_C4_CH_BRIGHTNESS:
		XiaomiCompact4_SetBrightness(value);
		break;
	case XIAOMI_C4_CH_CHILD_LOCK:
		XiaomiCompact4_SetChildLock(value);
		break;
	case XIAOMI_C4_CH_FAV_SPEED:
		g_favSpeed = XiaomiCompact4_ClampInt(value, 0, 100);
		XiaomiCompact4_ApplyState(1);
		break;
	case XIAOMI_C4_CH_NIGHT_SPEED:
		g_nightSpeed = XiaomiCompact4_ClampInt(value, 0, 100);
		XiaomiCompact4_ApplyState(1);
		break;
	case XIAOMI_C4_CH_P_FACTOR:
		g_pFactorX100 = XiaomiCompact4_ClampInt(value, 1, 1000);
		XiaomiCompact4_ApplyState(1);
		break;
	case XIAOMI_C4_CH_FILTER_LIFESPAN:
		g_filterLifespanDays = XiaomiCompact4_ClampInt(value, XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS, XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS);
		XiaomiCompact4_ApplyState(1);
		break;
	case XIAOMI_C4_CH_BUZZER:
		g_buzzer = value ? 1 : 0;
		if (!g_buzzer) {
			HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, 0);
		}
		XiaomiCompact4_ApplyState(1);
		break;
	}
}

static commandResult_t CMD_XiaomiCompact4_SetPower(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	XiaomiCompact4_SetPower(Tokenizer_GetArgInteger(0));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetMode(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	XiaomiCompact4_SetMode(XiaomiCompact4_ParseMode(Tokenizer_GetArg(0)));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetBrightness(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	XiaomiCompact4_SetBrightness(XiaomiCompact4_ParseBrightness(Tokenizer_GetArg(0)));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetChildLock(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	XiaomiCompact4_SetChildLock(Tokenizer_GetArgInteger(0));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_ResetFilter(const void *context, const char *cmd, const char *args, int cmdFlags) {
	XiaomiCompact4_ResetFilter();
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetFavSpeed(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_favSpeed = XiaomiCompact4_ClampInt(Tokenizer_GetArgInteger(0), 0, 100);
	XiaomiCompact4_ApplyState(1);
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetNightSpeed(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_nightSpeed = XiaomiCompact4_ClampInt(Tokenizer_GetArgInteger(0), 0, 100);
	XiaomiCompact4_ApplyState(1);
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetPFactor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_pFactorX100 = XiaomiCompact4_ClampInt((int)(Tokenizer_GetArgFloat(0) * 100.0f), 1, 1000);
	XiaomiCompact4_ApplyState(1);
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetFilterLifespan(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_filterLifespanDays = XiaomiCompact4_ClampInt(Tokenizer_GetArgInteger(0), XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS, XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS);
	XiaomiCompact4_ApplyState(1);
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_PM25Stats(const void *context, const char *cmd, const char *args, int cmdFlags) {
	char hex[XIAOMI_C4_PM25_FRAME_LEN * 3];
	XiaomiCompact4_PM25FrameToHex(g_pm25LastFrame, hex, sizeof(hex));
	ADDLOG_INFO(LOG_FEATURE_CMD, "PM25 raw=%i accepted=%i rejected_checksum=%i rejected_range=%i rx_len=%i last_frame=%s",
		g_pm25LastRaw, g_pm25AcceptedFrames, g_pm25RejectedChecksum, g_pm25RejectedRange, g_pm25RxLen, hex);
	return CMD_RES_OK;
}

void XiaomiCompact4_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	char tmpA[8];
	if (bPreState) {
		if (http_getArg(request->url, "x4resetfilter", tmpA, sizeof(tmpA))) {
			XiaomiCompact4_ResetFilter();
		}
		return;
	}
	hprintf255(request, "<h3>Xiaomi Compact 4</h3>");
	hprintf255(request, "PM2.5: %i ug/m3, Motor: %i rpm, Output/Target: %i%%/%i%%<br>",
		g_lastPm25, g_lastMotorRpm, g_motorCurrentPercent, g_motorTargetPercent);
	hprintf255(request, "Filter: %i%% health, %i/%i d used, Replace: %s<br>",
		XiaomiCompact4_FilterHealth(), XiaomiCompact4_FilterUsageDays(), g_filterLifespanDays, XiaomiCompact4_ReplaceFilter() ? "yes" : "no");
	poststr(request, "<table><tr><td><form action=\"index\">");
	poststr(request, "<input type=\"hidden\" name=\"x4resetfilter\" value=\"1\">");
	poststr(request, "<input type=\"submit\" value=\"Reset Filter\"/></form></td></tr></table>");
}

static void XiaomiCompact4_SetupChannels(void) {
	CHANNEL_SetLabel(XIAOMI_C4_CH_POWER, "Power", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_MODE, "Mode", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_BRIGHTNESS, "Brightness", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_CHILD_LOCK, "Child Lock", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_PM25, "PM2.5", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_MOTOR_RPM, "Motor Speed", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FILTER_USAGE, "Filter Usage", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FILTER_HEALTH, "Filter Health", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_REPLACE_FILTER, "Replace Filter", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FAV_SPEED, "Favorite Motor Speed", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_NIGHT_SPEED, "Night Motor Speed", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_P_FACTOR, "Auto Motor Speed", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FILTER_LIFESPAN, "Filter Lifespan", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_BUZZER, "Buzzer", 1);
	CHANNEL_SetType(XIAOMI_C4_CH_POWER, ChType_Toggle);
	CHANNEL_SetType(XIAOMI_C4_CH_MODE, ChType_Enum);
	CHANNEL_SetType(XIAOMI_C4_CH_BRIGHTNESS, ChType_Enum);
	CHANNEL_SetType(XIAOMI_C4_CH_CHILD_LOCK, ChType_Toggle);
	CHANNEL_SetType(XIAOMI_C4_CH_PM25, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_MOTOR_RPM, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_FILTER_USAGE, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_FILTER_HEALTH, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_REPLACE_FILTER, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_FAV_SPEED, ChType_Percent);
	CHANNEL_SetType(XIAOMI_C4_CH_NIGHT_SPEED, ChType_Percent);
	CHANNEL_SetType(XIAOMI_C4_CH_P_FACTOR, ChType_Dimmer1000);
	CHANNEL_SetType(XIAOMI_C4_CH_FILTER_LIFESPAN, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_BUZZER, ChType_Toggle);
	CMD_ExecuteCommand("SetChannelEnum 1 0:FAV 1:NIGHT 2:AUTO", 0);
	CMD_ExecuteCommand("SetChannelEnum 2 0:FULL 1:MID 2:ZERO", 0);
}

#if ENABLE_HA_DISCOVERY
static void XiaomiCompact4_HassSetName(HassDeviceInfo *dev_info, const char *name) {
	cJSON_ReplaceItemInObject(dev_info->root, "name", cJSON_CreateString(name));
}

static void XiaomiCompact4_HassSetCommandTopic(HassDeviceInfo *dev_info, const char *command) {
	char topic[96];
	snprintf(topic, sizeof(topic), "cmnd/%s/%s", CFG_GetMQTTClientId(), command);
	cJSON_ReplaceItemInObject(dev_info->root, "cmd_t", cJSON_CreateString(topic));
}

static void XiaomiCompact4_HassPublish(const char *topic, HassDeviceInfo *dev_info) {
	if (dev_info == NULL) {
		return;
	}
	MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(dev_info);
}

static void XiaomiCompact4_HassClearOldSensor(const char *topic, int ch) {
	HassDeviceInfo *dev_info = hass_init_sensor_device_info(CUSTOM_SENSOR, ch, -1, -1, 1);
	if (dev_info == NULL) {
		return;
	}
	MQTT_QueuePublish(topic, dev_info->channel, "", OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(dev_info);
}

static void XiaomiCompact4_HassClearOldRelay(const char *topic, int ch, ENTITY_TYPE type) {
	HassDeviceInfo *dev_info = hass_init_relay_device_info(ch, type, false);
	if (dev_info == NULL) {
		return;
	}
	MQTT_QueuePublish(topic, dev_info->channel, "", OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(dev_info);
}

static void XiaomiCompact4_HassPublishSwitch(const char *topic, int ch, const char *name) {
	HassDeviceInfo *dev_info = hass_init_relay_device_info(ch, RELAY, false);
	if (dev_info == NULL) {
		return;
	}
	XiaomiCompact4_HassSetName(dev_info, name);
	XiaomiCompact4_HassPublish(topic, dev_info);
}

static void XiaomiCompact4_HassQueueChannelState(int ch, int value) {
	char channel[16];
	char state[4];
	snprintf(channel, sizeof(channel), "%i/get", ch);
	snprintf(state, sizeof(state), "%i", value ? 1 : 0);
	MQTT_QueuePublish(CFG_GetMQTTClientId(), channel, state, 0);
}

static void XiaomiCompact4_HassQueueChannelValue(int ch, int value) {
	char channel[16];
	char state[8];
	snprintf(channel, sizeof(channel), "%i/get", ch);
	snprintf(state, sizeof(state), "%i", value);
	MQTT_QueuePublish(CFG_GetMQTTClientId(), channel, state, 0);
}

static void XiaomiCompact4_HassPublishSelect(const char *topic, int ch, const char *name, const char **options, int optionCount) {
	char stateTopic[16];
	char commandTopic[16];
	snprintf(stateTopic, sizeof(stateTopic), "~/%i/get", ch);
	snprintf(commandTopic, sizeof(commandTopic), "~/%i/set", ch);
	HassDeviceInfo *dev_info = hass_createSelectEntityIndexed(stateTopic, commandTopic, optionCount, options, name);
	if (dev_info == NULL) {
		return;
	}
	XiaomiCompact4_HassPublish(topic, dev_info);
}

static void XiaomiCompact4_HassPublishSensor(const char *topic, int ch, const char *name, const char *unit, const char *deviceClass, const char *icon) {
	HassDeviceInfo *dev_info = hass_init_sensor_device_info(CUSTOM_SENSOR, ch, -1, -1, 1);
	if (dev_info == NULL) {
		return;
	}
	XiaomiCompact4_HassSetName(dev_info, name);
	cJSON_DeleteItemFromObject(dev_info->root, "pl_on");
	cJSON_DeleteItemFromObject(dev_info->root, "pl_off");
	if (unit) {
		cJSON_AddStringToObject(dev_info->root, "unit_of_meas", unit);
	}
	if (deviceClass) {
		cJSON_AddStringToObject(dev_info->root, "dev_cla", deviceClass);
	}
	if (icon) {
		cJSON_AddStringToObject(dev_info->root, "icon", icon);
	}
	XiaomiCompact4_HassPublish(topic, dev_info);
}

static void XiaomiCompact4_HassPublishNumber(const char *topic, int ch, const char *name, const char *command, float min, float max, float step, const char *unit, const char *deviceClass, const char *valueTemplate) {
	HassDeviceInfo *dev_info = hass_init_sensor_device_info(HASS_PERCENT, ch, -1, -1, 1);
	if (dev_info == NULL) {
		return;
	}
	XiaomiCompact4_HassSetName(dev_info, name);
	XiaomiCompact4_HassSetCommandTopic(dev_info, command);
	cJSON_DeleteItemFromObject(dev_info->root, "pl_on");
	cJSON_DeleteItemFromObject(dev_info->root, "pl_off");
	cJSON_ReplaceItemInObject(dev_info->root, "min", cJSON_CreateNumber(min));
	cJSON_ReplaceItemInObject(dev_info->root, "max", cJSON_CreateNumber(max));
	cJSON_ReplaceItemInObject(dev_info->root, "step", cJSON_CreateNumber(step));
	cJSON_DeleteItemFromObject(dev_info->root, "stat_cla");
	if (unit) {
		cJSON_ReplaceItemInObject(dev_info->root, "unit_of_meas", cJSON_CreateString(unit));
	} else {
		cJSON_DeleteItemFromObject(dev_info->root, "unit_of_meas");
	}
	if (valueTemplate) {
		cJSON_AddStringToObject(dev_info->root, "val_tpl", valueTemplate);
	}
	if (deviceClass) {
		cJSON_AddStringToObject(dev_info->root, "dev_cla", deviceClass);
	}
	cJSON_DeleteItemFromObject(dev_info->root, "entity_category");
	cJSON_AddStringToObject(dev_info->root, "entity_category", "config");
	XiaomiCompact4_HassPublish(topic, dev_info);
}

void XiaomiCompact4_OnHassDiscovery(const char *topic) {
	HassDeviceInfo *dev_info;
	static const char *modeOptions[] = { "FAV", "NIGHT", "AUTO" };
	static const char *brightnessOptions[] = { "FULL", "MID", "ZERO" };

	XiaomiCompact4_HassClearOldRelay(topic, XIAOMI_C4_CH_POWER, LIGHT_ON_OFF);
	XiaomiCompact4_HassPublishSwitch(topic, XIAOMI_C4_CH_POWER, "Power");
	XiaomiCompact4_HassPublishSwitch(topic, XIAOMI_C4_CH_BUZZER, "Buzzer");
	XiaomiCompact4_HassPublishSelect(topic, XIAOMI_C4_CH_MODE, "Mode", modeOptions, 3);
	XiaomiCompact4_HassPublishSelect(topic, XIAOMI_C4_CH_BRIGHTNESS, "Brightness", brightnessOptions, 3);
	XiaomiCompact4_HassQueueChannelState(XIAOMI_C4_CH_POWER, g_power);
	XiaomiCompact4_HassQueueChannelState(XIAOMI_C4_CH_BUZZER, g_buzzer);
	XiaomiCompact4_HassQueueChannelValue(XIAOMI_C4_CH_MODE, g_mode);
	XiaomiCompact4_HassQueueChannelValue(XIAOMI_C4_CH_BRIGHTNESS, g_brightness);

	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_PM25, "PM2.5", "ug/m3", NULL, "mdi:air-filter");
	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_MOTOR_RPM, "Motor Speed", "rpm", NULL, "mdi:fan");
	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_FILTER_USAGE, "Filter Usage", "s", "duration", "mdi:air-filter");
	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_FILTER_HEALTH, "Filter Health", "%", NULL, "mdi:air-filter");

	XiaomiCompact4_HassClearOldSensor(topic, XIAOMI_C4_CH_REPLACE_FILTER);
	dev_info = hass_init_binary_sensor_device_info(XIAOMI_C4_CH_REPLACE_FILTER, true);
	if (dev_info) {
		XiaomiCompact4_HassSetName(dev_info, "Replace Filter");
		cJSON_AddStringToObject(dev_info->root, "dev_cla", "problem");
		cJSON_AddStringToObject(dev_info->root, "icon", "mdi:air-filter");
		XiaomiCompact4_HassPublish(topic, dev_info);
	}

	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_FAV_SPEED, "Favorite Motor Speed", "XiaomiCompact4_SetFavSpeed", 0, 100, 1, NULL, NULL, NULL);
	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_NIGHT_SPEED, "Night Motor Speed", "XiaomiCompact4_SetNightSpeed", 0, 100, 1, NULL, NULL, NULL);
	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_P_FACTOR, "Auto Motor Speed", "XiaomiCompact4_SetPFactor", 0.01f, 10.0f, 0.01f, NULL, NULL, "{{ ((value | float(0)) / 100) | round(2) }}");
	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_FILTER_LIFESPAN, "Filter Lifespan", "XiaomiCompact4_SetFilterLifespan", XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS, XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS, 1, "d", "duration", NULL);

	dev_info = hass_init_button_device_info("reset", "XiaomiCompact4_ResetFilter", "1", HASS_CATEGORY_CONFIG);
	if (dev_info) {
		XiaomiCompact4_HassSetName(dev_info, "Reset Filter");
		XiaomiCompact4_HassPublish(topic, dev_info);
	}
}

#endif

bool XiaomiCompact4_ShouldSkipGenericHassDiscovery(int ch) {
	if (!g_initialized) {
		return false;
	}
	if (ch == XIAOMI_C4_CH_POWER || ch == XIAOMI_C4_CH_BUZZER) {
		return true;
	}
	if (ch == XIAOMI_C4_CH_MODE || ch == XIAOMI_C4_CH_BRIGHTNESS) {
		return true;
	}
	return ch >= XIAOMI_C4_CH_PM25 && ch <= XIAOMI_C4_CH_FILTER_LIFESPAN;
}

void XiaomiCompact4_Init(void) {
	//cmddetail:{"name":"XiaomiCompact4_SetPower","args":"[0/1]","descr":"Set Xiaomi Compact 4 power","fn":"CMD_XiaomiCompact4_SetPower","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4","examples":"XiaomiCompact4_SetPower 1"}
	CMD_RegisterCommand("XiaomiCompact4_SetPower", CMD_XiaomiCompact4_SetPower, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetMode", CMD_XiaomiCompact4_SetMode, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetBrightness", CMD_XiaomiCompact4_SetBrightness, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetChildLock", CMD_XiaomiCompact4_SetChildLock, NULL);
	CMD_RegisterCommand("XiaomiCompact4_ResetFilter", CMD_XiaomiCompact4_ResetFilter, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetFavSpeed", CMD_XiaomiCompact4_SetFavSpeed, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetNightSpeed", CMD_XiaomiCompact4_SetNightSpeed, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetPFactor", CMD_XiaomiCompact4_SetPFactor, NULL);
	CMD_RegisterCommand("XiaomiCompact4_SetFilterLifespan", CMD_XiaomiCompact4_SetFilterLifespan, NULL);
	CMD_RegisterCommand("XiaomiCompact4_PM25Stats", CMD_XiaomiCompact4_PM25Stats, NULL);

	g_power = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_POWER, 0, 0, 1);
	g_mode = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_MODE, XIAOMI_C4_MODE_AUTO, XIAOMI_C4_MODE_FAV, XIAOMI_C4_MODE_AUTO);
	g_brightness = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_BRIGHTNESS, XIAOMI_C4_BRIGHTNESS_FULL, XIAOMI_C4_BRIGHTNESS_FULL, XIAOMI_C4_BRIGHTNESS_ZERO);
	g_childLock = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_CHILD_LOCK, 0, 0, 1);
	g_favSpeed = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_FAV_SPEED, 30, 0, 100);
	g_nightSpeed = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_NIGHT_SPEED, 10, 0, 100);
	g_pFactorX100 = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_P_FACTOR_X100, 100, 1, 1000);
	g_filterLifespanDays = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_FILTER_LIFESPAN, XIAOMI_C4_FILTER_LIFESPAN_DEFAULT_DAYS, XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS, XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS);
	XiaomiCompact4_EnsureFilterLifespan();
	g_filterUsageSeconds = (uint32_t)XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_FILTER_USAGE, 0, 0, 0x7FFFFFFF);
	g_buzzer = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_BUZZER, 1, 0, 1);
	g_pm25PollCountdown = 1;
	g_filterCountdown = 60;

	HAL_PIN_Setup_Output(XIAOMI_C4_PIN_MOTOR_EN);
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_EN, g_power ? 1 : 0);
	HAL_PIN_Setup_Input_Pullup(XIAOMI_C4_PIN_BUTTON_POWER);
	HAL_PIN_Setup_Input_Pullup(XIAOMI_C4_PIN_BUTTON_LIGHT);
	HAL_PIN_Setup_Input_Pullup(XIAOMI_C4_PIN_BUTTON_MODE);
	HAL_PIN_Setup_Input(XIAOMI_C4_PIN_TACH);
	HAL_AttachInterrupt(XIAOMI_C4_PIN_TACH, INTERRUPT_RISING, XiaomiCompact4_TachISR);

	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_BUZZER, 1000);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_LED_RED, 1000);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_LED_ORANGE, 1000);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_LED_GREEN, 1000);
	HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, 0);
	XiaomiCompact4_MotorPWMInit();

	XiaomiCompact4_I2CInit();

	XiaomiCompact4_UARTInit();

	XiaomiCompact4_SetupChannels();
	g_initialized = 1;
	XiaomiCompact4_ApplyState(0);
	ADDLOG_INFO(LOG_FEATURE_DRV, "XiaomiCompact4 initialized for ESP32-WROOM-32D hardware");
}

#endif

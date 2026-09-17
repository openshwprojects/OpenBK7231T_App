#include "../new_common.h"

#if ENABLE_DRIVER_XIAOMI_COMPACT4

#include "../new_cfg.h"
#include "../new_pins.h"
#include "../quicktick.h"
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
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "hal/wdt_hal.h"
#include "soc/rtc.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define XIAOMI_C4_PIN_PM25_TX 17
#define XIAOMI_C4_PIN_PM25_RX 16
#define XIAOMI_C4_UART_PORT UART_NUM_2
#define XIAOMI_C4_UART_BAUD 9600
#define XIAOMI_C4_UART_SOURCE_CLK UART_SCLK_REF_TICK
#define XIAOMI_C4_UART_BUF_SIZE 512
#define XIAOMI_C4_UART_QUEUE_SIZE 10
#define XIAOMI_C4_UART_TX_DONE_TIMEOUT_MS 20
#if 0
#define XIAOMI_C4_UART_LOG_BYTES 64
#endif
#define XIAOMI_C4_WARNING_LOG_INTERVAL 20

/*
 * These trace points were invaluable while correlating UART, I2C, touch and
 * motor failures with the logic analyzer and ESP32 coredumps. Keep their text
 * beside the code, but exclude them from normal builds so the driver does not
 * flood the shared logger. Re-enable this block only for a dedicated hardware
 * investigation.
 */
#if 0
#define XIAOMI_C4_FORENSIC_TRACE_ENABLED 1
#define XIAOMI_C4_FORENSIC_LOG(...) ADDLOG_EXTRADEBUG(__VA_ARGS__)
#endif

#define XIAOMI_C4_PIN_I2C_SDA 14
#define XIAOMI_C4_PIN_I2C_SCL 27
#define XIAOMI_C4_I2C_PORT I2C_NUM_0
#define XIAOMI_C4_I2C_FREQ_HZ 100000
#define XIAOMI_C4_I2C_BRIGHTNESS 0x24
#define XIAOMI_C4_I2C_STATUS_LEDS 0x34
#define XIAOMI_C4_I2C_BUTTON_POWER 0x35
#define XIAOMI_C4_I2C_BUTTON_BRIGHTNESS 0x36
#define XIAOMI_C4_I2C_BUTTON_MODE 0x37
#define XIAOMI_C4_I2C_DEVICE_COUNT 5
#define XIAOMI_C4_I2C_TX_TIMEOUT_MS 25
#define XIAOMI_C4_I2C_LOCK_TIMEOUT_MS 50
#define XIAOMI_C4_I2C_REFRESH_MS 60000
#define XIAOMI_C4_I2C_RECOVERY_COOLDOWN_MS 1000
#if 0
/*
 * Forensic breadcrumb reference
 *
 * This retained the last Xiaomi lifecycle stage across warm reboots. It was
 * useful while locating the I2C and motor-PWM failures, but it also touched
 * RTC memory and sampled SDA/SCL in production paths. Keep the implementation
 * for a future instrumented build without compiling it into release firmware.
 */
#define XIAOMI_C4_I2C_BREADCRUMB_MAGIC 0x58433449U
enum {
	XIAOMI_C4_I2C_STAGE_NONE = 0,
	XIAOMI_C4_I2C_STAGE_PRE_DRIVER,
	XIAOMI_C4_I2C_STAGE_INIT_BEGIN,
	XIAOMI_C4_I2C_STAGE_BUS_READY,
	XIAOMI_C4_I2C_STAGE_DEVICE_ADD_BEGIN,
	XIAOMI_C4_I2C_STAGE_DEVICE_ADD_RETURN,
	XIAOMI_C4_I2C_STAGE_INIT_DONE,
	XIAOMI_C4_I2C_STAGE_TX_BEGIN,
	XIAOMI_C4_I2C_STAGE_TX_RETURN_OK,
	XIAOMI_C4_I2C_STAGE_TX_RETURN_ERROR,
	XIAOMI_C4_I2C_STAGE_RECOVERY_QUEUED,
	XIAOMI_C4_I2C_STAGE_RECOVERY_BEGIN,
	XIAOMI_C4_I2C_STAGE_RECOVERY_SHUTDOWN_DONE,
	XIAOMI_C4_I2C_STAGE_RECOVERY_INIT_DONE,
	XIAOMI_C4_STAGE_DRIVER_INIT_ENTRY,
	XIAOMI_C4_STAGE_COMMANDS_REGISTERED,
	XIAOMI_C4_STAGE_CONFIG_LOADED,
	XIAOMI_C4_STAGE_GPIO_CONFIG_DONE,
	XIAOMI_C4_STAGE_TACH_SHUTDOWN_BEGIN,
	XIAOMI_C4_STAGE_TACH_SHUTDOWN_DONE,
	XIAOMI_C4_STAGE_TACH_GPIO_CONFIG_BEGIN,
	XIAOMI_C4_STAGE_TACH_GPIO_CONFIG_DONE,
	XIAOMI_C4_STAGE_TACH_ISR_SERVICE_BEGIN,
	XIAOMI_C4_STAGE_TACH_ISR_SERVICE_DONE,
	XIAOMI_C4_STAGE_TACH_HANDLER_ADD_BEGIN,
	XIAOMI_C4_STAGE_TACH_HANDLER_ADD_DONE,
	XIAOMI_C4_STAGE_AUX_PWM_BEGIN,
	XIAOMI_C4_STAGE_AUX_PWM_DONE,
	XIAOMI_C4_STAGE_MOTOR_PWM_BEGIN,
	XIAOMI_C4_STAGE_MOTOR_PWM_DONE,
	XIAOMI_C4_STAGE_MOTOR_PWM_TIMER_BEGIN,
	XIAOMI_C4_STAGE_MOTOR_PWM_TIMER_DONE,
	XIAOMI_C4_STAGE_MOTOR_PWM_CHANNEL_BEGIN,
	XIAOMI_C4_STAGE_MOTOR_PWM_CHANNEL_DONE,
	XIAOMI_C4_STAGE_MOTOR_PWM_STOP_BEGIN,
	XIAOMI_C4_STAGE_MOTOR_PWM_STOP_DONE,
	XIAOMI_C4_STAGE_NVS_SAVE_BEGIN,
	XIAOMI_C4_STAGE_NVS_SAVE_DONE,
};

typedef struct {
	uint32_t magic;
	uint32_t magicInverse;
	uint32_t sequence;
	uint32_t stage;
	uint32_t uptimeMs;
	int32_t address;
	int32_t value;
	int32_t error;
	int32_t sda;
	int32_t scl;
	uint32_t recoveryPending;
} XiaomiCompact4_I2CBreadcrumb;

static RTC_NOINIT_ATTR XiaomiCompact4_I2CBreadcrumb g_i2cBreadcrumb;

#define XiaomiCompact4_I2CBreadcrumbMark(...)
#endif

// Breadcrumb call sites remain commented beside the operations they once traced.

#define XIAOMI_C4_PIN_MOTOR_EN 2
#define XIAOMI_C4_PIN_MOTOR_PWM 26
#define XIAOMI_C4_MOTOR_LEDC_TIMER LEDC_TIMER_1
#define XIAOMI_C4_MOTOR_LEDC_CHANNEL LEDC_CHANNEL_5
#define XIAOMI_C4_MOTOR_LEDC_RES LEDC_TIMER_13_BIT
#define XIAOMI_C4_MOTOR_DUTY_50_PERCENT 4096
#define XIAOMI_C4_MOTOR_MIN_FREQUENCY 100
#define XIAOMI_C4_MOTOR_MAX_FREQUENCY 512
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
// Flash slot 0 stays reserved for layout compatibility; startup is always off.
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
#if 0
#define XIAOMI_C4_PM25_HIGH_LOG_THRESHOLD 300
#endif
#define XIAOMI_C4_PM25_POLL_SECONDS 5
#define XIAOMI_C4_PM25_STARTUP_DEFAULT 5
// Count complete request/response windows so split UART reads do not look stale.
#define XIAOMI_C4_PM25_STALE_QUERY_WINDOWS 3
#define XIAOMI_C4_PM25_RECOVERY_QUERY_WINDOWS 2
static const uint8_t g_pm25Query[] = { 0x11, 0x02, 0x0B, 0x01, 0xE1 };
// Keep normal fan changes smooth in both directions; power-off still stops immediately.
#define XIAOMI_C4_MOTOR_RAMP_UP_PERCENT_PER_SEC 6
#define XIAOMI_C4_MOTOR_RAMP_DOWN_PERCENT_PER_SEC 6
// RPM is sampled every second for control, but reported less often to avoid MQTT churn.
#define XIAOMI_C4_MOTOR_RPM_PUBLISH_SECONDS 3
#define XIAOMI_C4_FILTER_USAGE_TICK_SECONDS 60
#define XIAOMI_C4_FILTER_CHECKPOINT_SECONDS 3600
#if 0
// Forensic reference threshold used by the disabled NVS timing monitor.
#define XIAOMI_C4_NVS_SLOW_MS 50
#endif
#define XIAOMI_C4_TACH_STORM_EDGES_PER_SECOND 2000
#define XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS 1
#define XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS 365
#define XIAOMI_C4_FILTER_LIFESPAN_DEFAULT_DAYS 365
#define XIAOMI_C4_CLICK_MAX_MS 1000
#define XIAOMI_C4_LONG_PRESS_MS 7000
#define XIAOMI_C4_TICKS_FOR_MS(ms) (((ms) + QUICK_TMR_DURATION - 1) / QUICK_TMR_DURATION)
#define XIAOMI_C4_QUICKTICK_STALE_FAILSAFE_SECONDS 3
#define XIAOMI_C4_FAILSAFE_REASON_QUICKTICK_STALE 1
#define XIAOMI_C4_FAILSAFE_REASON_PM25_STALE 2
#define XIAOMI_C4_FAILSAFE_REASON_MOTOR_PWM 3
#define XIAOMI_C4_RTC_WDT_TIMEOUT_MS 10000

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
static int g_filterUsageDirty;
static int g_filterCheckpointCountdown;
static int g_lastPm25 = -1;
static int g_lastMotorRpm;
static int g_motorTargetPercent;
static int g_motorCurrentPercent;
#if 0
// Validation-only motor transition counters.
static uint32_t g_motorTargetChanges;
#endif
static int g_motorPwmReady;
static int g_motorEnabled;
static int g_motorAppliedFrequency = -1;
static int g_motorPwmLastError;
static uint32_t g_motorPwmErrors;
#if 0
static uint32_t g_motorFrequencyChanges;
#endif
static portMUX_TYPE g_motorEnableMux = portMUX_INITIALIZER_UNLOCKED;
static int g_lastButtonPower;
static int g_lastButtonLight;
static int g_lastButtonMode;
static int g_powerPressTicks;
static int g_lightPressTicks;
static int g_modePressTicks;
static int g_modeLongHandled;
#if 0
// Validation-only touch edge and decision counters.
static uint32_t g_buttonPressEdges[3];
static uint32_t g_buttonReleaseEdges[3];
static uint32_t g_buttonClicks[3];
static uint32_t g_buttonIgnored[3];
static uint32_t g_buttonLongPresses[3];
#endif
static int g_wifiConnectedLast = -1;
static int g_wifiLedBlinkPhase;
#if 0
// Validation-only Wi-Fi state-transition counters.
static uint32_t g_wifiConnectionChanges;
static uint32_t g_wifiBlinkTransitions;
#endif
static volatile uint32_t g_tachPulses;
static int g_tachIsrAttached;
static uint32_t g_tachTotalEdges;
static uint32_t g_tachLastWindowEdges;
#if 0
static uint32_t g_tachMaxWindowEdges;
#endif
static uint32_t g_tachStormWindows;
#if 0
static uint32_t g_tachLastActiveSecond;
#endif
static int g_pm25PollCountdown;
static int g_filterCountdown;
static int g_childLockNotificationTicks;
static int g_childLockNotificationFlips;
static int g_childLockNotificationState;
static int g_ignoreChannelChange;
static int g_initialized;
static uint8_t g_pm25RxBuf[XIAOMI_C4_UART_BUF_SIZE];
static int g_pm25RxLen;
#if 0
// Validation-only PM frame snapshots and transport history.
static uint8_t g_pm25LastFrame[XIAOMI_C4_PM25_FRAME_LEN];
static int g_pm25LastRaw = -1;
static int g_pm25AcceptedFrames;
#endif
static int g_pm25RejectedChecksum;
static int g_pm25RejectedRange;
#if 0
static int g_pm25ScratchResets;
static int g_pm25MaxRxLen;
#endif
static int g_pm25UartBufferFull;
static int g_pm25UartFifoOvf;
#if 0
static int g_pm25UartEvents;
static int g_pm25UartDataEvents;
#endif
#if 0
static int g_pm25UartBreakEvents;
static int g_pm25UartFrameErrors;
static int g_pm25UartParityErrors;
#endif
static int g_pm25UartReadErrors;
#if 0
static int g_pm25UartWriteErrors;
static int g_pm25UartTxWaitErrors;
static int g_pm25UartInitErrors;
#endif
#if 0
static uint32_t g_pm25TxCompleted;
static uint32_t g_pm25RxChunks;
static int g_pm25LastRxChunk;
static int g_pm25MaxRxChunk;
#endif
static uint32_t g_pm25QueryAttempts;
#if 0
static uint32_t g_pm25QueriesSent;
static uint32_t g_pm25EmptyQueryWindows;
static uint32_t g_pm25NoRxQueryWindows;
static uint32_t g_pm25NoValidQueryWindows;
static uint32_t g_pm25QueryEchoes;
static uint32_t g_pm25HeaderCandidates;
static uint32_t g_pm25DiscardedBytes;
static uint32_t g_pm25ChecksumResyncs;
static uint32_t g_pm25LastValidSecond;
static uint32_t g_pm25MaxValidGapSeconds;
static uint32_t g_pm25LastValidQuery;
static uint32_t g_pm25MaxValidQueryGap;
#endif
static uint32_t g_pm25ConsecutiveInvalidWindows;
static uint32_t g_pm25ConsecutiveRecoveryWindows;
#if 0
static uint32_t g_pm25FailsafeActivations;
#endif
static uint32_t g_pm25FailsafeRecoveries;
#if 0
static int g_pm25FrameSeenSinceQuery;
#endif
static int g_pm25ValidFrameSeenSinceQuery;
#if 0
static uint32_t g_pm25RxBytesSinceQuery;
static uint32_t g_pm25LastQueryTickMs;
static uint32_t g_pm25LastRxTickMs;
#endif
static int g_failsafeActive;
static int g_failsafeReason;
static int g_quickTickStaleSeconds;
static uint32_t g_diagQuickTicks;
#if 0
static uint32_t g_diagEverySeconds;
#endif
#if 0
// Validation-only throughput, stack and deferred-update counters.
static uint32_t g_diagPm25RxBytes;
static UBaseType_t g_diagQuickStackMin = (UBaseType_t)-1;
static uint32_t g_pm25DeferredUpdates;
static uint32_t g_hidDeferredRequests;
#endif
static i2c_master_bus_handle_t g_i2cBus;
static i2c_master_dev_handle_t g_i2cBrightness;
static i2c_master_dev_handle_t g_i2cStatusLeds;
static i2c_master_dev_handle_t g_i2cButtonPower;
static i2c_master_dev_handle_t g_i2cButtonBrightness;
static i2c_master_dev_handle_t g_i2cButtonMode;
static uint32_t g_i2cInitAttempts;
#if 0
static uint32_t g_i2cInitErrors;
#endif
static uint32_t g_i2cWriteRequests;
static uint32_t g_i2cWriteAttempts;
#if 0
static uint32_t g_i2cWriteSuccesses;
static uint32_t g_i2cWriteFailures;
static uint32_t g_i2cSkippedUnchanged;
#endif
static uint32_t g_i2cLockTimeouts;
static uint32_t g_i2cRecoveryAttempts;
#if 0
static uint32_t g_i2cRecoverySuccesses;
#endif
#if 0
static uint32_t g_i2cRecoveryFailures;
#endif
#if 0
static uint32_t g_i2cMaxWriteMs;
static uint32_t g_i2cLastRecoveryMs;
#endif
static uint32_t g_i2cRecoveryRequestedMs;
#if 0
static uint32_t g_i2cRecoverySuppressedWrites;
#endif
static int g_i2cRecoveryPending;
#if 0
static uint32_t g_i2cNullHandleWrites;
static uint32_t g_i2cUnknownAddressWrites;
#endif
static uint32_t g_i2cConsecutiveFailures;
#if 0
static uint32_t g_i2cMaxConsecutiveFailures;
#endif
#if 0
static uint32_t g_i2cLastFailureMs;
#endif
#if 0
static int g_i2cLastAddress = -1;
static int g_i2cLastValue = -1;
#endif
static int g_i2cLastError;
static int g_i2cLastRecoveryError;
static int g_i2cLastSda = -1;
static int g_i2cLastScl = -1;
#if 0
static uint32_t g_i2cDeviceAttempts[XIAOMI_C4_I2C_DEVICE_COUNT];
static uint32_t g_i2cDeviceSuccesses[XIAOMI_C4_I2C_DEVICE_COUNT];
static uint32_t g_i2cDeviceFailures[XIAOMI_C4_I2C_DEVICE_COUNT];
static int g_i2cDeviceLastValue[XIAOMI_C4_I2C_DEVICE_COUNT] = { -1, -1, -1, -1, -1 };
#endif
static int g_i2cDeviceLastSuccessValue[XIAOMI_C4_I2C_DEVICE_COUNT] = { -1, -1, -1, -1, -1 };
static int g_i2cDeviceLastError[XIAOMI_C4_I2C_DEVICE_COUNT];
static uint32_t g_i2cDeviceLastSuccessMs[XIAOMI_C4_I2C_DEVICE_COUNT];
static SemaphoreHandle_t g_i2cMutex;
static QueueHandle_t g_pm25UartQueue;
#if 0
// Forensic NVS timing counters; retained for a future instrumented build.
static uint32_t g_nvsSaveAttempts;
static uint32_t g_nvsSaveCompletions;
static uint32_t g_nvsSlowSaves;
static uint32_t g_nvsMaxSaveMs;
static uint32_t g_nvsLastSaveMs;
static int g_nvsLastIndex = -1;
static int g_nvsLastValue;
#endif

static void XiaomiCompact4_ApplyState(int save);
static uint32_t XiaomiCompact4_UptimeMs(void);
static int XiaomiCompact4_I2CReady(void);

#if 0
/* Converts retained stage IDs into readable names for forensic UART output. */
static const char *XiaomiCompact4_I2CStageName(uint32_t stage) {
	switch (stage) {
	case XIAOMI_C4_I2C_STAGE_PRE_DRIVER: return "pre_driver";
	case XIAOMI_C4_I2C_STAGE_INIT_BEGIN: return "init_begin";
	case XIAOMI_C4_I2C_STAGE_BUS_READY: return "bus_ready";
	case XIAOMI_C4_I2C_STAGE_DEVICE_ADD_BEGIN: return "device_add_begin";
	case XIAOMI_C4_I2C_STAGE_DEVICE_ADD_RETURN: return "device_add_return";
	case XIAOMI_C4_I2C_STAGE_INIT_DONE: return "init_done";
	case XIAOMI_C4_I2C_STAGE_TX_BEGIN: return "tx_begin";
	case XIAOMI_C4_I2C_STAGE_TX_RETURN_OK: return "tx_return_ok";
	case XIAOMI_C4_I2C_STAGE_TX_RETURN_ERROR: return "tx_return_error";
	case XIAOMI_C4_I2C_STAGE_RECOVERY_QUEUED: return "recovery_queued";
	case XIAOMI_C4_I2C_STAGE_RECOVERY_BEGIN: return "recovery_begin";
	case XIAOMI_C4_I2C_STAGE_RECOVERY_SHUTDOWN_DONE: return "recovery_shutdown_done";
	case XIAOMI_C4_I2C_STAGE_RECOVERY_INIT_DONE: return "recovery_init_done";
	case XIAOMI_C4_STAGE_DRIVER_INIT_ENTRY: return "driver_init_entry";
	case XIAOMI_C4_STAGE_COMMANDS_REGISTERED: return "commands_registered";
	case XIAOMI_C4_STAGE_CONFIG_LOADED: return "config_loaded";
	case XIAOMI_C4_STAGE_GPIO_CONFIG_DONE: return "gpio_config_done";
	case XIAOMI_C4_STAGE_TACH_SHUTDOWN_BEGIN: return "tach_shutdown_begin";
	case XIAOMI_C4_STAGE_TACH_SHUTDOWN_DONE: return "tach_shutdown_done";
	case XIAOMI_C4_STAGE_TACH_GPIO_CONFIG_BEGIN: return "tach_gpio_config_begin";
	case XIAOMI_C4_STAGE_TACH_GPIO_CONFIG_DONE: return "tach_gpio_config_done";
	case XIAOMI_C4_STAGE_TACH_ISR_SERVICE_BEGIN: return "tach_isr_service_begin";
	case XIAOMI_C4_STAGE_TACH_ISR_SERVICE_DONE: return "tach_isr_service_done";
	case XIAOMI_C4_STAGE_TACH_HANDLER_ADD_BEGIN: return "tach_handler_add_begin";
	case XIAOMI_C4_STAGE_TACH_HANDLER_ADD_DONE: return "tach_handler_add_done";
	case XIAOMI_C4_STAGE_AUX_PWM_BEGIN: return "aux_pwm_begin";
	case XIAOMI_C4_STAGE_AUX_PWM_DONE: return "aux_pwm_done";
	case XIAOMI_C4_STAGE_MOTOR_PWM_BEGIN: return "motor_pwm_begin";
	case XIAOMI_C4_STAGE_MOTOR_PWM_DONE: return "motor_pwm_done";
	case XIAOMI_C4_STAGE_MOTOR_PWM_TIMER_BEGIN: return "motor_pwm_timer_begin";
	case XIAOMI_C4_STAGE_MOTOR_PWM_TIMER_DONE: return "motor_pwm_timer_done";
	case XIAOMI_C4_STAGE_MOTOR_PWM_CHANNEL_BEGIN: return "motor_pwm_channel_begin";
	case XIAOMI_C4_STAGE_MOTOR_PWM_CHANNEL_DONE: return "motor_pwm_channel_done";
	case XIAOMI_C4_STAGE_MOTOR_PWM_STOP_BEGIN: return "motor_pwm_stop_begin";
	case XIAOMI_C4_STAGE_MOTOR_PWM_STOP_DONE: return "motor_pwm_stop_done";
	case XIAOMI_C4_STAGE_NVS_SAVE_BEGIN: return "nvs_save_begin";
	case XIAOMI_C4_STAGE_NVS_SAVE_DONE: return "nvs_save_done";
	default: return "none";
	}
}

static int XiaomiCompact4_I2CBreadcrumbValid(void) {
	return g_i2cBreadcrumb.magic == XIAOMI_C4_I2C_BREADCRUMB_MAGIC
		&& g_i2cBreadcrumb.magicInverse == ~XIAOMI_C4_I2C_BREADCRUMB_MAGIC;
}

static void XiaomiCompact4_I2CBreadcrumbInitialize(void) {
	if (XiaomiCompact4_I2CBreadcrumbValid()) return;
	g_i2cBreadcrumb.magic = XIAOMI_C4_I2C_BREADCRUMB_MAGIC;
	g_i2cBreadcrumb.magicInverse = ~XIAOMI_C4_I2C_BREADCRUMB_MAGIC;
	g_i2cBreadcrumb.sequence = 0;
	g_i2cBreadcrumb.stage = XIAOMI_C4_I2C_STAGE_NONE;
	g_i2cBreadcrumb.uptimeMs = 0;
	g_i2cBreadcrumb.address = -1;
	g_i2cBreadcrumb.value = -1;
	g_i2cBreadcrumb.error = ESP_OK;
	g_i2cBreadcrumb.sda = -1;
	g_i2cBreadcrumb.scl = -1;
	g_i2cBreadcrumb.recoveryPending = 0;
}

static void XiaomiCompact4_I2CBreadcrumbMark(uint32_t stage, int address, int value,
		esp_err_t error, int recoveryPending) {
	XiaomiCompact4_I2CBreadcrumbInitialize();
	g_i2cBreadcrumb.sequence++;
	g_i2cBreadcrumb.stage = stage;
	g_i2cBreadcrumb.uptimeMs = XiaomiCompact4_UptimeMs();
	g_i2cBreadcrumb.address = address;
	g_i2cBreadcrumb.value = value;
	g_i2cBreadcrumb.error = error;
	g_i2cBreadcrumb.sda = gpio_get_level(XIAOMI_C4_PIN_I2C_SDA);
	g_i2cBreadcrumb.scl = gpio_get_level(XIAOMI_C4_PIN_I2C_SCL);
	g_i2cBreadcrumb.recoveryPending = recoveryPending ? 1U : 0U;
}
#endif

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

static int XiaomiCompact4_LoadFilterLifespan(void) {
	int value = HAL_FlashVars_GetChannelValue(XIAOMI_C4_VAR_FILTER_LIFESPAN);
	if (value < XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS
		|| value > XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS) {
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 invalid stored filter lifespan %i; using %i days without overwriting flash",
			value, XIAOMI_C4_FILTER_LIFESPAN_DEFAULT_DAYS);
		return XIAOMI_C4_FILTER_LIFESPAN_DEFAULT_DAYS;
	}
	return value;
}

static int XiaomiCompact4_EffectivePm25(void) {
	return g_lastPm25 >= 0 ? g_lastPm25 : XIAOMI_C4_PM25_STARTUP_DEFAULT;
}

#if 0
/*
 * Forensic NVS timing reference
 *
 * This version measured save latency and recorded interrupted writes. Normal
 * firmware only needs to save the value and clear the filter dirty flag.
 */
static void XiaomiCompact4_SaveInt(int index, int value) {
	uint32_t startMs = XiaomiCompact4_UptimeMs();
	g_nvsSaveAttempts++;
	g_nvsLastIndex = index;
	g_nvsLastValue = value;
	XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_NVS_SAVE_BEGIN,
		index, value, ESP_OK, g_i2cRecoveryPending);
	HAL_FlashVars_SaveChannel(index, value);
	g_nvsLastSaveMs = XiaomiCompact4_UptimeMs() - startMs;
	g_nvsSaveCompletions++;
	if (g_nvsLastSaveMs > g_nvsMaxSaveMs) g_nvsMaxSaveMs = g_nvsLastSaveMs;
	if (index == XIAOMI_C4_VAR_FILTER_USAGE) g_filterUsageDirty = 0;
	XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_NVS_SAVE_DONE,
		index, value, ESP_OK, g_i2cRecoveryPending);
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 NVS save completed index=%i value=%i duration_ms=%u attempts=%u completed=%u",
		index, value, (unsigned int)g_nvsLastSaveMs,
		(unsigned int)g_nvsSaveAttempts, (unsigned int)g_nvsSaveCompletions);
	if (g_nvsLastSaveMs >= XIAOMI_C4_NVS_SLOW_MS) {
		g_nvsSlowSaves++;
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 slow NVS save index=%i value=%i duration_ms=%u slow=%u",
			index, value, (unsigned int)g_nvsLastSaveMs,
			(unsigned int)g_nvsSlowSaves);
	}
}
#else
static void XiaomiCompact4_SaveInt(int index, int value) {
	HAL_FlashVars_SaveChannel(index, value);
	if (index == XIAOMI_C4_VAR_FILTER_USAGE) g_filterUsageDirty = 0;
}
#endif

#if 0
// Converts modes for the disabled validation traces and commands.
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
#endif

static int XiaomiCompact4_ParseMode(const char *s) {
	if (!stricmp(s, "FAV") || !strcmp(s, "0")) return XIAOMI_C4_MODE_FAV;
	if (!stricmp(s, "NIGHT") || !strcmp(s, "1")) return XIAOMI_C4_MODE_NIGHT;
	return XIAOMI_C4_MODE_AUTO;
}

static int XiaomiCompact4_ParseBrightness(const char *s) {
	if (!stricmp(s, "MID") || !strcmp(s, "1")) return XIAOMI_C4_BRIGHTNESS_MID;
	if (!stricmp(s, "ZERO") || !strcmp(s, "2")) return XIAOMI_C4_BRIGHTNESS_ZERO;
	return XIAOMI_C4_BRIGHTNESS_FULL;
}

static int XiaomiCompact4_I2CIndexForAddress(int addr7) {
	switch (addr7) {
	case XIAOMI_C4_I2C_BRIGHTNESS: return 0;
	case XIAOMI_C4_I2C_STATUS_LEDS: return 1;
	case XIAOMI_C4_I2C_BUTTON_POWER: return 2;
	case XIAOMI_C4_I2C_BUTTON_BRIGHTNESS: return 3;
	case XIAOMI_C4_I2C_BUTTON_MODE: return 4;
	default: return -1;
	}
}

static const char *XiaomiCompact4_I2CNameForAddress(int addr7) {
	switch (addr7) {
	case XIAOMI_C4_I2C_BRIGHTNESS: return "brightness";
	case XIAOMI_C4_I2C_STATUS_LEDS: return "status_leds";
	case XIAOMI_C4_I2C_BUTTON_POWER: return "button_power";
	case XIAOMI_C4_I2C_BUTTON_BRIGHTNESS: return "button_brightness";
	case XIAOMI_C4_I2C_BUTTON_MODE: return "button_mode";
	default: return "unknown";
	}
}

static i2c_master_dev_handle_t XiaomiCompact4_I2CHandleForAddress(int addr7) {
	switch (addr7) {
	case XIAOMI_C4_I2C_BRIGHTNESS: return g_i2cBrightness;
	case XIAOMI_C4_I2C_STATUS_LEDS: return g_i2cStatusLeds;
	case XIAOMI_C4_I2C_BUTTON_POWER: return g_i2cButtonPower;
	case XIAOMI_C4_I2C_BUTTON_BRIGHTNESS: return g_i2cButtonBrightness;
	case XIAOMI_C4_I2C_BUTTON_MODE: return g_i2cButtonMode;
	default: return NULL;
	}
}

// Forensic trace reference: follows I2C handle creation, teardown and bus ownership.
static void XiaomiCompact4_I2CAddDevice(i2c_device_config_t *devConfig, int addr7,
		i2c_master_dev_handle_t *handle) {
	devConfig->device_address = addr7;
	*handle = NULL;
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_DEVICE_ADD_BEGIN,
		// addr7, -1, ESP_OK, g_i2cRecoveryPending);
	esp_err_t err = i2c_master_bus_add_device(g_i2cBus, devConfig, handle);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_DEVICE_ADD_RETURN,
		// addr7, -1, err, g_i2cRecoveryPending);
	if (err != ESP_OK) {
#if 0
		g_i2cInitErrors++;
#endif
		g_i2cLastError = err;
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C add failed addr=0x%02X name=%s err=%i handle=%p",
			addr7, XiaomiCompact4_I2CNameForAddress(addr7), err, *handle);
		return;
	}
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 I2C add ok addr=0x%02X name=%s err=%i handle=%p",
		addr7, XiaomiCompact4_I2CNameForAddress(addr7), err, *handle);
#endif
}

static void XiaomiCompact4_I2CInit(void) {
	g_i2cInitAttempts++;
	g_i2cLastSda = gpio_get_level(XIAOMI_C4_PIN_I2C_SDA);
	g_i2cLastScl = gpio_get_level(XIAOMI_C4_PIN_I2C_SCL);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_INIT_BEGIN,
		// -1, -1, ESP_OK, g_i2cRecoveryPending);
	for (int i = 0; i < XIAOMI_C4_I2C_DEVICE_COUNT; i++) {
		g_i2cDeviceLastSuccessValue[i] = -1;
		g_i2cDeviceLastSuccessMs[i] = 0;
		g_i2cDeviceLastError[i] = ESP_ERR_INVALID_STATE;
	}
	if (g_i2cMutex == NULL) {
		g_i2cMutex = xSemaphoreCreateMutex();
		if (g_i2cMutex == NULL) {
#if 0
			g_i2cInitErrors++;
#endif
			g_i2cLastError = ESP_ERR_NO_MEM;
			// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_INIT_DONE,
				// -1, -1, ESP_ERR_NO_MEM, g_i2cRecoveryPending);
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 I2C mutex create failed");
			return;
		}
	}
	i2c_master_bus_config_t busConfig = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = XIAOMI_C4_I2C_PORT,
		.sda_io_num = XIAOMI_C4_PIN_I2C_SDA,
		.scl_io_num = XIAOMI_C4_PIN_I2C_SCL,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 I2C init begin attempt=%u port=%i sda=%i scl=%i line_sda=%i line_scl=%i freq=%i pullup=1 existing_bus=%p",
		(unsigned int)g_i2cInitAttempts, (int)XIAOMI_C4_I2C_PORT,
		XIAOMI_C4_PIN_I2C_SDA, XIAOMI_C4_PIN_I2C_SCL, g_i2cLastSda, g_i2cLastScl,
		XIAOMI_C4_I2C_FREQ_HZ, g_i2cBus);
#endif
	esp_err_t err = i2c_new_master_bus(&busConfig, &g_i2cBus);
	if (err != ESP_OK) {
		// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_INIT_DONE,
			// -1, -1, err, g_i2cRecoveryPending);
#if 0
		g_i2cInitErrors++;
#endif
		g_i2cLastError = err;
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C bus install failed attempt=%u err=%i bus=%p",
			(unsigned int)g_i2cInitAttempts, err, g_i2cBus);
		return;
	}
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_BUS_READY,
		// -1, -1, ESP_OK, g_i2cRecoveryPending);
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 I2C bus install ok err=%i bus=%p", err, g_i2cBus);
#endif

	i2c_device_config_t devConfig = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.scl_speed_hz = XIAOMI_C4_I2C_FREQ_HZ,
	};

	XiaomiCompact4_I2CAddDevice(&devConfig, XIAOMI_C4_I2C_BRIGHTNESS, &g_i2cBrightness);
	XiaomiCompact4_I2CAddDevice(&devConfig, XIAOMI_C4_I2C_STATUS_LEDS, &g_i2cStatusLeds);
	XiaomiCompact4_I2CAddDevice(&devConfig, XIAOMI_C4_I2C_BUTTON_POWER, &g_i2cButtonPower);
	XiaomiCompact4_I2CAddDevice(&devConfig, XIAOMI_C4_I2C_BUTTON_BRIGHTNESS, &g_i2cButtonBrightness);
	XiaomiCompact4_I2CAddDevice(&devConfig, XIAOMI_C4_I2C_BUTTON_MODE, &g_i2cButtonMode);
	err = XiaomiCompact4_I2CReady() ? ESP_OK
		: (g_i2cLastError == ESP_OK ? ESP_ERR_INVALID_STATE : g_i2cLastError);
	g_i2cLastError = err;
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_INIT_DONE,
		// -1, -1, err, g_i2cRecoveryPending);
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 I2C init done attempt=%u errors=%u bus=%p handles=[%p,%p,%p,%p,%p]",
		(unsigned int)g_i2cInitAttempts, (unsigned int)g_i2cInitErrors, g_i2cBus,
		g_i2cBrightness, g_i2cStatusLeds, g_i2cButtonPower,
		g_i2cButtonBrightness, g_i2cButtonMode);
#endif
}

static void XiaomiCompact4_I2CRemoveDevice(i2c_master_dev_handle_t *handle, int addr7) {
	if (*handle == NULL) return;
	esp_err_t err = i2c_master_bus_rm_device(*handle);
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 I2C remove addr=0x%02X name=%s handle=%p err=%i",
		addr7, XiaomiCompact4_I2CNameForAddress(addr7), *handle, err);
#endif
	if (err == ESP_OK) *handle = NULL;
}

static void XiaomiCompact4_I2CShutdown(void) {
	if (g_i2cMutex != NULL
		&& xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(XIAOMI_C4_I2C_LOCK_TIMEOUT_MS)) != pdTRUE) {
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 I2C shutdown lock timeout");
		return;
	}
	XiaomiCompact4_I2CRemoveDevice(&g_i2cBrightness, XIAOMI_C4_I2C_BRIGHTNESS);
	XiaomiCompact4_I2CRemoveDevice(&g_i2cStatusLeds, XIAOMI_C4_I2C_STATUS_LEDS);
	XiaomiCompact4_I2CRemoveDevice(&g_i2cButtonPower, XIAOMI_C4_I2C_BUTTON_POWER);
	XiaomiCompact4_I2CRemoveDevice(&g_i2cButtonBrightness, XIAOMI_C4_I2C_BUTTON_BRIGHTNESS);
	XiaomiCompact4_I2CRemoveDevice(&g_i2cButtonMode, XIAOMI_C4_I2C_BUTTON_MODE);
	if (g_i2cBus != NULL) {
		esp_err_t err = i2c_del_master_bus(g_i2cBus);
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 I2C bus delete bus=%p err=%i", g_i2cBus, err);
#endif
		if (err == ESP_OK) g_i2cBus = NULL;
	}
	if (g_i2cMutex != NULL) {
		xSemaphoreGive(g_i2cMutex);
		vSemaphoreDelete(g_i2cMutex);
		g_i2cMutex = NULL;
	}
}

static int XiaomiCompact4_I2CReady(void) {
	return g_i2cBus != NULL
		&& g_i2cBrightness != NULL
		&& g_i2cStatusLeds != NULL
		&& g_i2cButtonPower != NULL
		&& g_i2cButtonBrightness != NULL
		&& g_i2cButtonMode != NULL;
}

static int XiaomiCompact4_I2CStopped(void) {
	return g_i2cBus == NULL
		&& g_i2cBrightness == NULL
		&& g_i2cStatusLeds == NULL
		&& g_i2cButtonPower == NULL
		&& g_i2cButtonBrightness == NULL
		&& g_i2cButtonMode == NULL;
}

static int XiaomiCompact4_I2CServiceRecovery(void) {
	uint32_t now;
	int ready;

	if (!g_i2cRecoveryPending) return 0;
	now = XiaomiCompact4_UptimeMs();
	if (now - g_i2cRecoveryRequestedMs < XIAOMI_C4_I2C_RECOVERY_COOLDOWN_MS) return 0;

	g_i2cRecoveryAttempts++;
#if 0
	g_i2cLastRecoveryMs = now;
#endif
	g_i2cLastSda = gpio_get_level(XIAOMI_C4_PIN_I2C_SDA);
	g_i2cLastScl = gpio_get_level(XIAOMI_C4_PIN_I2C_SCL);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_RECOVERY_BEGIN,
		// g_i2cLastAddress, g_i2cLastValue, g_i2cLastError, 1);
	ADDLOG_WARN(LOG_FEATURE_DRV,
		"XiaomiCompact4 I2C deferred recovery begin attempt=%u requested_ms=%u now_ms=%u sda=%i scl=%i",
		(unsigned int)g_i2cRecoveryAttempts, (unsigned int)g_i2cRecoveryRequestedMs,
		(unsigned int)now, g_i2cLastSda, g_i2cLastScl);

	XiaomiCompact4_I2CShutdown();
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_RECOVERY_SHUTDOWN_DONE,
		// g_i2cLastAddress, g_i2cLastValue,
		// XiaomiCompact4_I2CStopped() ? ESP_OK : ESP_ERR_INVALID_STATE, 1);
	if (!XiaomiCompact4_I2CStopped()) {
#if 0
		g_i2cRecoveryFailures++;
#endif
		g_i2cLastRecoveryError = ESP_ERR_INVALID_STATE;
		g_i2cRecoveryRequestedMs = XiaomiCompact4_UptimeMs();
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C deferred recovery teardown incomplete attempt=%u bus=%p handles=[%p,%p,%p,%p,%p]",
			(unsigned int)g_i2cRecoveryAttempts, g_i2cBus, g_i2cBrightness,
			g_i2cStatusLeds, g_i2cButtonPower, g_i2cButtonBrightness, g_i2cButtonMode);
		return 1;
	}
	XiaomiCompact4_I2CInit();
	ready = XiaomiCompact4_I2CReady();
	g_i2cLastRecoveryError = ready ? ESP_OK
		: (g_i2cLastError == ESP_OK ? ESP_ERR_INVALID_STATE : g_i2cLastError);
	if (ready) {
#if 0
		g_i2cRecoverySuccesses++;
#endif
		g_i2cRecoveryPending = 0;
	} else {
#if 0
		g_i2cRecoveryFailures++;
#endif
		g_i2cRecoveryRequestedMs = XiaomiCompact4_UptimeMs();
	}
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_RECOVERY_INIT_DONE,
		// g_i2cLastAddress, g_i2cLastValue, g_i2cLastRecoveryError, g_i2cRecoveryPending);
	ADDLOG_WARN(LOG_FEATURE_DRV,
		"XiaomiCompact4 I2C deferred recovery done attempt=%u ready=%i err=%i pending=%i bus=%p",
		(unsigned int)g_i2cRecoveryAttempts, ready, g_i2cLastRecoveryError,
		g_i2cRecoveryPending, g_i2cBus);
	return 1;
}

static void XiaomiCompact4_I2CRecordFailure(int index, esp_err_t err, uint32_t now) {
#if 0
	g_i2cWriteFailures++;
#endif
	g_i2cConsecutiveFailures++;
#if 0
	if (g_i2cConsecutiveFailures > g_i2cMaxConsecutiveFailures) {
		g_i2cMaxConsecutiveFailures = g_i2cConsecutiveFailures;
	}
#endif
#if 0
	g_i2cLastFailureMs = now;
#else
	(void)now;
#endif
	g_i2cLastError = err;
	if (index >= 0) {
#if 0
		g_i2cDeviceFailures[index]++;
#endif
		g_i2cDeviceLastError[index] = err;
	}
}

static void XiaomiCompact4_I2CWrite1(int addr7, int value) {
	uint8_t data = (uint8_t)value;
	int index = XiaomiCompact4_I2CIndexForAddress(addr7);
	i2c_master_dev_handle_t dev = XiaomiCompact4_I2CHandleForAddress(addr7);
	uint32_t now = XiaomiCompact4_UptimeMs();
	esp_err_t err = ESP_OK;
	uint32_t elapsed;

	g_i2cWriteRequests++;
#if 0
	g_i2cLastAddress = addr7;
	g_i2cLastValue = data;
#endif
	if (index >= 0) {
#if 0
		g_i2cDeviceLastValue[index] = data;
#endif
		if (g_i2cRecoveryPending) {
#if 0
			g_i2cRecoverySuppressedWrites++;
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 I2C write suppressed during recovery addr=0x%02X value=0x%02X suppressed=%u",
				addr7, data, (unsigned int)g_i2cRecoverySuppressedWrites);
#endif
			return;
		}
		if (g_i2cDeviceLastError[index] == ESP_OK
			&& g_i2cDeviceLastSuccessValue[index] == data
			&& now - g_i2cDeviceLastSuccessMs[index] < XIAOMI_C4_I2C_REFRESH_MS) {
#if 0
			g_i2cSkippedUnchanged++;
#endif
			return;
		}
	}
	if (dev == NULL || index < 0) {
		XiaomiCompact4_I2CRecordFailure(index, ESP_ERR_INVALID_STATE, now);
#if 0
		if (index >= 0) {
			g_i2cNullHandleWrites++;
		} else {
			g_i2cUnknownAddressWrites++;
		}
#endif
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C write unavailable req=%u addr=0x%02X name=%s value=0x%02X index=%i handle=%p",
			(unsigned int)g_i2cWriteRequests, addr7, XiaomiCompact4_I2CNameForAddress(addr7),
			data, index, dev);
		return;
	}
	if (g_i2cMutex == NULL
		|| xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(XIAOMI_C4_I2C_LOCK_TIMEOUT_MS)) != pdTRUE) {
		g_i2cLockTimeouts++;
		XiaomiCompact4_I2CRecordFailure(index, ESP_ERR_TIMEOUT, now);
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C lock timeout req=%u addr=0x%02X value=0x%02X count=%u",
			(unsigned int)g_i2cWriteRequests, addr7, data, (unsigned int)g_i2cLockTimeouts);
		return;
	}

	g_i2cWriteAttempts++;
#if 0
	g_i2cDeviceAttempts[index]++;
#endif
	now = XiaomiCompact4_UptimeMs();
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_TX_BEGIN,
		// addr7, data, ESP_OK, 0);
	err = i2c_master_transmit(dev, &data, 1, XIAOMI_C4_I2C_TX_TIMEOUT_MS);
	elapsed = XiaomiCompact4_UptimeMs() - now;
	// XiaomiCompact4_I2CBreadcrumbMark(err == ESP_OK
		// ? XIAOMI_C4_I2C_STAGE_TX_RETURN_OK : XIAOMI_C4_I2C_STAGE_TX_RETURN_ERROR,
		// addr7, data, err, 0);
#if 0
	if (elapsed > g_i2cMaxWriteMs) g_i2cMaxWriteMs = elapsed;
#endif
	if (err != ESP_OK) {
		g_i2cLastSda = gpio_get_level(XIAOMI_C4_PIN_I2C_SDA);
		g_i2cLastScl = gpio_get_level(XIAOMI_C4_PIN_I2C_SCL);
		if (err == ESP_ERR_TIMEOUT && !g_i2cRecoveryPending) {
			g_i2cRecoveryPending = 1;
			g_i2cRecoveryRequestedMs = XiaomiCompact4_UptimeMs();
			// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_RECOVERY_QUEUED,
				// addr7, data, err, 1);
		}
	}
	xSemaphoreGive(g_i2cMutex);

	if (err != ESP_OK) {
		XiaomiCompact4_I2CRecordFailure(index, err, XiaomiCompact4_UptimeMs());
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C write failed req=%u seq=%u addr=0x%02X name=%s value=0x%02X err=%i elapsed_ms=%u consecutive=%u sda=%i scl=%i recovery_pending=%i task=%s",
			(unsigned int)g_i2cWriteRequests, (unsigned int)g_i2cWriteAttempts,
			addr7, XiaomiCompact4_I2CNameForAddress(addr7), data, err, (unsigned int)elapsed,
			(unsigned int)g_i2cConsecutiveFailures, g_i2cLastSda, g_i2cLastScl,
			g_i2cRecoveryPending, pcTaskGetName(NULL));
		return;
	}
	g_i2cConsecutiveFailures = 0;
	g_i2cLastError = ESP_OK;
#if 0
	g_i2cWriteSuccesses++;
	g_i2cDeviceSuccesses[index]++;
	uint32_t successMs = XiaomiCompact4_UptimeMs();
	g_i2cDeviceLastSuccessValue[index] = data;
	g_i2cDeviceLastSuccessMs[index] = successMs;
#else
	g_i2cDeviceLastSuccessValue[index] = data;
	g_i2cDeviceLastSuccessMs[index] = XiaomiCompact4_UptimeMs();
#endif
	g_i2cDeviceLastError[index] = ESP_OK;
#if 0
	if (XIAOMI_C4_FORENSIC_TRACE_ENABLED && g_loglevel >= LOG_EXTRADEBUG) {
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C write ok req=%u seq=%u addr=0x%02X name=%s value=0x%02X elapsed_ms=%u task=%s",
			(unsigned int)g_i2cWriteRequests, (unsigned int)g_i2cWriteAttempts,
			addr7, XiaomiCompact4_I2CNameForAddress(addr7), data, (unsigned int)elapsed,
			pcTaskGetName(NULL));
	}
#endif
}

static uint32_t XiaomiCompact4_UptimeMs(void) {
	return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

#if 0
// Formats raw UART bytes for the disabled validation traces and commands.
static void XiaomiCompact4_BytesToHex(const uint8_t *data, int len, char *out, int outLen) {
	int pos = 0;
	if (outLen <= 0) return;
	for (int i = 0; i < len && pos < outLen - 1; i++) {
		int written = snprintf(out + pos, outLen - pos, "%02X%s", data[i], i == len - 1 ? "" : " ");
		if (written < 0 || written >= outLen - pos) {
			break;
		}
		pos += written;
	}
	out[pos] = '\0';
}
#endif

// Forensic trace reference: captures the selected clock, actual baud and UART ownership.
static void XiaomiCompact4_UARTInit(void) {
	uart_config_t uart_config = {
		.baud_rate = XIAOMI_C4_UART_BAUD,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		// APB can fall from 80 MHz to 40 MHz after TX under DFS, corrupting the delayed sensor reply.
		.source_clk = XIAOMI_C4_UART_SOURCE_CLK,
	};
	esp_err_t err;
#if 0
	uint32_t actualBaud = 0;
	uint32_t sourceClockHz = 0;
	esp_err_t sourceClockErr = uart_get_sclk_freq(XIAOMI_C4_UART_SOURCE_CLK, &sourceClockHz);
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 UART init begin port=%i tx=%i rx=%i baud=%i source=%i source_hz=%u source_err=%i secondary_uart_flag=%i installed=[%i,%i,%i]",
		(int)XIAOMI_C4_UART_PORT, XIAOMI_C4_PIN_PM25_TX, XIAOMI_C4_PIN_PM25_RX,
		XIAOMI_C4_UART_BAUD, (int)XIAOMI_C4_UART_SOURCE_CLK, (unsigned int)sourceClockHz,
		sourceClockErr, CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART),
		uart_is_driver_installed(UART_NUM_0), uart_is_driver_installed(UART_NUM_1),
		uart_is_driver_installed(UART_NUM_2));
	if (sourceClockErr != ESP_OK) g_pm25UartInitErrors++;
#endif
	if (uart_is_driver_installed(XIAOMI_C4_UART_PORT)) {
		err = uart_driver_delete(XIAOMI_C4_UART_PORT);
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 UART delete existing port=%i err=%i", (int)XIAOMI_C4_UART_PORT, err);
#endif
#if 0
		if (err != ESP_OK) g_pm25UartInitErrors++;
#endif
	}
	g_pm25UartQueue = NULL;
	err = uart_param_config(XIAOMI_C4_UART_PORT, &uart_config);
	if (err != ESP_OK) {
#if 0
		g_pm25UartInitErrors++;
#endif
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART param config failed err=%i", err);
	}
	err = uart_set_pin(XIAOMI_C4_UART_PORT, XIAOMI_C4_PIN_PM25_TX, XIAOMI_C4_PIN_PM25_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err != ESP_OK) {
#if 0
		g_pm25UartInitErrors++;
#endif
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART pin config failed err=%i", err);
	}
	err = uart_driver_install(XIAOMI_C4_UART_PORT, XIAOMI_C4_UART_BUF_SIZE, 0, XIAOMI_C4_UART_QUEUE_SIZE, &g_pm25UartQueue, 0);
	if (err != ESP_OK) {
#if 0
		g_pm25UartInitErrors++;
#endif
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART driver install failed err=%i", err);
		g_pm25UartQueue = NULL;
	} else {
		// uart_driver_install() configures ESP-IDF's default 10-symbol RX timeout.
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 UART RX timeout uses driver default symbols=10");
#endif
		err = uart_flush_input(XIAOMI_C4_UART_PORT);
		if (err != ESP_OK) {
#if 0
			g_pm25UartInitErrors++;
#endif
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART initial RX flush failed err=%i", err);
		}
	}
#if 0
	err = uart_get_baudrate(XIAOMI_C4_UART_PORT, &actualBaud);
	if (err != ESP_OK) g_pm25UartInitErrors++;
	g_pm25RxLen = 0;
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 UART init done port=%i driver=%i queue=%p baud=%u baud_err=%i init_errors=%i",
		(int)XIAOMI_C4_UART_PORT, uart_is_driver_installed(XIAOMI_C4_UART_PORT),
		(void *)g_pm25UartQueue, (unsigned int)actualBaud, err, g_pm25UartInitErrors);
#else
	g_pm25RxLen = 0;
#endif
}

static void XiaomiCompact4_UARTAppendBytes(const uint8_t *data, int len) {
	if (len <= 0) {
		return;
	}
#if 0
	g_diagPm25RxBytes += len;
	g_pm25RxBytesSinceQuery += len;
#endif
	if (g_pm25RxLen + len > (int)sizeof(g_pm25RxBuf)) {
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART scratch reset q=%u old_len=%i append=%i",
			(unsigned int)g_pm25QueryAttempts, g_pm25RxLen, len);
		g_pm25RxLen = 0;
#if 0
		g_pm25ScratchResets++;
#endif
	}
	if (len > (int)sizeof(g_pm25RxBuf)) {
		data += len - (int)sizeof(g_pm25RxBuf);
		len = sizeof(g_pm25RxBuf);
	}
	memcpy(g_pm25RxBuf + g_pm25RxLen, data, len);
	g_pm25RxLen += len;
#if 0
	if (g_pm25RxLen > g_pm25MaxRxLen) {
		g_pm25MaxRxLen = g_pm25RxLen;
	}
#endif
}

static void XiaomiCompact4_UARTReadAvailable(void) {
	uint8_t tmp[64];
	while (1) {
		int len = uart_read_bytes(XIAOMI_C4_UART_PORT, tmp, sizeof(tmp), 0);
		if (len < 0) {
			g_pm25UartReadErrors++;
			if ((g_pm25UartReadErrors % XIAOMI_C4_WARNING_LOG_INTERVAL) == 1) {
				ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART read error %i", len);
			}
			return;
		}
		if (len == 0) {
			return;
		}
#if 0
		g_pm25RxChunks++;
		g_pm25LastRxChunk = len;
		if (len > g_pm25MaxRxChunk) g_pm25MaxRxChunk = len;
		g_pm25LastRxTickMs = XiaomiCompact4_UptimeMs();
#endif
#if 0
		if (XIAOMI_C4_FORENSIC_TRACE_ENABLED && g_loglevel >= LOG_EXTRADEBUG) {
			char hex[XIAOMI_C4_UART_LOG_BYTES * 3];
			int logLen = len > XIAOMI_C4_UART_LOG_BYTES ? XIAOMI_C4_UART_LOG_BYTES : len;
			XiaomiCompact4_BytesToHex(tmp, logLen, hex, sizeof(hex));
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 UART RX q=%u chunk=%u len=%i delta_ms=%u data=%s%s",
				(unsigned int)g_pm25QueryAttempts, (unsigned int)g_pm25RxChunks, len,
				(unsigned int)(g_pm25LastRxTickMs - g_pm25LastQueryTickMs), hex,
				len > logLen ? " ..." : "");
		}
#endif
		XiaomiCompact4_UARTAppendBytes(tmp, len);
	}
}

static void XiaomiCompact4_UARTHandleOverflow(int *counter, const char *name) {
	(*counter)++;
	if ((*counter % XIAOMI_C4_WARNING_LOG_INTERVAL) == 1) {
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 %s count %i", name, *counter);
	}
	uart_flush_input(XIAOMI_C4_UART_PORT);
	xQueueReset(g_pm25UartQueue);
	g_pm25RxLen = 0;
}

static void XiaomiCompact4_UARTProcessEvents(void) {
	uart_event_t event;
	size_t buffered = 0;
	if (g_pm25UartQueue == NULL) {
		return;
	}
	while (xQueueReceive(g_pm25UartQueue, &event, 0)) {
#if 0
		g_pm25UartEvents++;
#endif
		uart_get_buffered_data_len(XIAOMI_C4_UART_PORT, &buffered);
		switch (event.type) {
		case UART_DATA:
#if 0
			g_pm25UartDataEvents++;
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 UART event DATA q=%u size=%u timeout=%i buffered=%u",
				(unsigned int)g_pm25QueryAttempts, (unsigned int)event.size,
				event.timeout_flag, (unsigned int)buffered);
#endif
			break;
		case UART_BREAK:
#if 0
			g_pm25UartBreakEvents++;
#endif
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART event BREAK q=%u size=%u buffered=%u",
				(unsigned int)g_pm25QueryAttempts, (unsigned int)event.size, (unsigned int)buffered);
			break;
		case UART_FRAME_ERR:
#if 0
			g_pm25UartFrameErrors++;
#endif
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART event FRAME_ERR q=%u size=%u buffered=%u",
				(unsigned int)g_pm25QueryAttempts, (unsigned int)event.size, (unsigned int)buffered);
			break;
		case UART_PARITY_ERR:
#if 0
			g_pm25UartParityErrors++;
#endif
			ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART event PARITY_ERR q=%u size=%u buffered=%u",
				(unsigned int)g_pm25QueryAttempts, (unsigned int)event.size, (unsigned int)buffered);
			break;
		case UART_BUFFER_FULL:
			XiaomiCompact4_UARTHandleOverflow(&g_pm25UartBufferFull, "UART_BUFFER_FULL");
			return;
		case UART_FIFO_OVF:
			XiaomiCompact4_UARTHandleOverflow(&g_pm25UartFifoOvf, "UART_FIFO_OVF");
			return;
		default:
#if 0
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 UART event type=%i q=%u size=%u buffered=%u",
				(int)event.type, (unsigned int)g_pm25QueryAttempts,
				(unsigned int)event.size, (unsigned int)buffered);
#endif
			break;
		}
	}
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

#if 0
// Formats complete PM frames for the disabled validation traces and commands.
static void XiaomiCompact4_PM25FrameToHex(const uint8_t *frame, char *out, int outLen) {
	XiaomiCompact4_BytesToHex(frame, XIAOMI_C4_PM25_FRAME_LEN, out, outLen);
}
#endif

static void XiaomiCompact4_SetMotorEnabled(int enabled) {
	enabled = enabled ? 1 : 0;
	portENTER_CRITICAL(&g_motorEnableMux);
	// Recheck under the clamp lock so a concurrent Power OFF or stop always wins.
	if (enabled && (!g_initialized || !g_power || g_failsafeActive || !g_motorPwmReady)) {
		enabled = 0;
	}
	if (g_motorEnabled == enabled) {
		portEXIT_CRITICAL(&g_motorEnableMux);
		return;
	}
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_EN, enabled);
	g_motorEnabled = enabled;
	portEXIT_CRITICAL(&g_motorEnableMux);
}

static int XiaomiCompact4_RecordMotorPWMError(const char *operation, int value, esp_err_t err) {
	g_motorPwmReady = 0;
	g_motorPwmLastError = err;
	g_motorPwmErrors++;
	ADDLOG_ERROR(LOG_FEATURE_DRV,
		"XiaomiCompact4 motor PWM %s failed value=%i err=%i name=%s errors=%u",
		operation, value, err, esp_err_to_name(err), (unsigned int)g_motorPwmErrors);
	return 0;
}

static int XiaomiCompact4_MotorPWMInit(void) {
	esp_err_t err;
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.timer_num = XIAOMI_C4_MOTOR_LEDC_TIMER,
		.duty_resolution = XIAOMI_C4_MOTOR_LEDC_RES,
		.freq_hz = XIAOMI_C4_MOTOR_MIN_FREQUENCY,
		.clk_cfg = SOC_MOD_CLK_RC_FAST,
	};
	ledc_channel_config_t channel = {
		.gpio_num = XIAOMI_C4_PIN_MOTOR_PWM,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = XIAOMI_C4_MOTOR_LEDC_CHANNEL,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = XIAOMI_C4_MOTOR_LEDC_TIMER,
		.duty = XIAOMI_C4_MOTOR_DUTY_50_PERCENT,
		.hpoint = 0,
	};

	// GPIO2 is the physical safety boundary. Keep it low until PWM is known-good.
	XiaomiCompact4_SetMotorEnabled(0);
	g_motorPwmReady = 0;
	g_motorAppliedFrequency = -1;
	g_motorPwmLastError = ESP_OK;
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_TIMER_BEGIN,
		// XIAOMI_C4_MOTOR_LEDC_TIMER, timer.freq_hz, ESP_OK, 0);
	err = ledc_timer_config(&timer);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_TIMER_DONE,
		// XIAOMI_C4_MOTOR_LEDC_TIMER, timer.freq_hz, err, 0);
	if (err != ESP_OK) {
		return XiaomiCompact4_RecordMotorPWMError("timer init", timer.freq_hz, err);
	}
	// A warm reset can leave this private channel active, so stop it before rebuilding it.
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_STOP_BEGIN,
		// XIAOMI_C4_PIN_MOTOR_PWM, XIAOMI_C4_MOTOR_LEDC_CHANNEL, ESP_OK, 0);
	err = ledc_stop(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_CHANNEL, 0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_STOP_DONE,
		// XIAOMI_C4_PIN_MOTOR_PWM, XIAOMI_C4_MOTOR_LEDC_CHANNEL, err, 0);
	if (err != ESP_OK) {
		return XiaomiCompact4_RecordMotorPWMError("pre-init stop",
			XIAOMI_C4_MOTOR_LEDC_CHANNEL, err);
	}
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_CHANNEL_BEGIN,
		// XIAOMI_C4_PIN_MOTOR_PWM, XIAOMI_C4_MOTOR_LEDC_CHANNEL, ESP_OK, 0);
	err = ledc_channel_config(&channel);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_CHANNEL_DONE,
		// XIAOMI_C4_PIN_MOTOR_PWM, XIAOMI_C4_MOTOR_LEDC_CHANNEL, err, 0);
	if (err != ESP_OK) {
		return XiaomiCompact4_RecordMotorPWMError("channel init",
			XIAOMI_C4_MOTOR_LEDC_CHANNEL, err);
	}

	// Duty is fixed by the hardware protocol. Runtime speed changes only touch frequency.
	g_motorPwmReady = 1;
	g_motorAppliedFrequency = XIAOMI_C4_MOTOR_MIN_FREQUENCY;
	return 1;
}

static int XiaomiCompact4_MotorPWMSetFrequency(int frequency) {
	esp_err_t err;

	if (!g_motorPwmReady) {
		return XiaomiCompact4_RecordMotorPWMError("not ready", frequency, ESP_ERR_INVALID_STATE);
	}
	if (frequency < XIAOMI_C4_MOTOR_MIN_FREQUENCY
		|| frequency > XIAOMI_C4_MOTOR_MAX_FREQUENCY) {
		return XiaomiCompact4_RecordMotorPWMError("frequency range", frequency, ESP_ERR_INVALID_ARG);
	}
	if (g_motorAppliedFrequency == frequency) {
		return 1;
	}

	err = ledc_set_freq(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_TIMER, frequency);
	if (err != ESP_OK) {
		return XiaomiCompact4_RecordMotorPWMError("frequency update", frequency, err);
	}
	g_motorAppliedFrequency = frequency;
#if 0
	g_motorFrequencyChanges++;
#endif
	return 1;
}

static void XiaomiCompact4_ForceMotorFailsafe(int reason) {
	int logFault = !g_failsafeActive || g_failsafeReason != reason;

	// Publish the fault state before taking the clamp lock so no task can re-enable GPIO2.
	g_failsafeActive = 1;
	g_failsafeReason = reason;
	if (reason == XIAOMI_C4_FAILSAFE_REASON_MOTOR_PWM) {
		// A broken PWM path cannot recover safely while the driver remains active.
		g_power = 0;
	}
	XiaomiCompact4_SetMotorEnabled(0);
	if (logFault) {
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 motor failsafe active, reason %i", reason);
	}
	g_motorTargetPercent = 0;
	g_motorCurrentPercent = 0;
}

static void XiaomiCompact4_ClearPM25Failsafe(void) {
	if (!g_failsafeActive || g_failsafeReason != XIAOMI_C4_FAILSAFE_REASON_PM25_STALE) {
		return;
	}
	g_failsafeActive = 0;
	g_failsafeReason = 0;
	g_pm25FailsafeRecoveries++;
	ADDLOG_INFO(LOG_FEATURE_DRV,
		"XiaomiCompact4 PM25 failsafe recovered after %u valid query windows; recoveries=%u",
		(unsigned int)g_pm25ConsecutiveRecoveryWindows,
		(unsigned int)g_pm25FailsafeRecoveries);
	XiaomiCompact4_ApplyState(0);
}

static int XiaomiCompact4_MotorPercentToFrequency(int percent) {
	percent = XiaomiCompact4_ClampInt(percent, 0, 100);
	return XIAOMI_C4_MOTOR_MIN_FREQUENCY
		+ (percent * (XIAOMI_C4_MOTOR_MAX_FREQUENCY - XIAOMI_C4_MOTOR_MIN_FREQUENCY)) / 100;
}

static void XiaomiCompact4_ApplyMotorRamp(void) {
	int frequency;

	if (!g_power || g_failsafeActive) {
		g_motorCurrentPercent = 0;
		XiaomiCompact4_SetMotorEnabled(0);
		return;
	}
	if (!g_motorPwmReady) {
		XiaomiCompact4_ForceMotorFailsafe(XIAOMI_C4_FAILSAFE_REASON_MOTOR_PWM);
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

	frequency = XiaomiCompact4_MotorPercentToFrequency(g_motorCurrentPercent);
	if (!XiaomiCompact4_MotorPWMSetFrequency(frequency)) {
		XiaomiCompact4_ForceMotorFailsafe(XIAOMI_C4_FAILSAFE_REASON_MOTOR_PWM);
		return;
	}
	// GPIO2 is deliberately last; SetMotorEnabled rechecks state under its lock.
	XiaomiCompact4_SetMotorEnabled(1);
}

static void XiaomiCompact4_SetLedPWM(int pin, float level) {
	if (level <= 0.0f) {
		HAL_PIN_PWM_Update(pin, 0);
		return;
	}
	HAL_PIN_PWM_Update(pin, XiaomiCompact4_ClampFloat(level * 100.0f, 0.0f, 100.0f));
}

static void XiaomiCompact4_StopPWMOutputLow(int pin) {
	HAL_PIN_PWM_Stop(pin);
	// PWM release resets the GPIO. Reclaim it low so the attached output cannot float on.
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

static void XiaomiCompact4_BlankHID(void) {
	if (!XiaomiCompact4_I2CReady()) return;
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_STATUS_LEDS, XIAOMI_C4_BUTTON_LED_OFF);
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BUTTON_POWER, XIAOMI_C4_BUTTON_LED_OFF);
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BUTTON_BRIGHTNESS, XIAOMI_C4_BUTTON_LED_OFF);
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_BUTTON_MODE, XIAOMI_C4_BUTTON_LED_OFF);
}

static int XiaomiCompact4_IsWiFiConnected(void) {
	return Main_IsConnectedToWiFi();
}

// Forensic trace reference: records standalone Wi-Fi LED state changes and blink phases.
static void XiaomiCompact4_ServiceWiFiIndicator(void) {
	int connected = XiaomiCompact4_IsWiFiConnected();
	if (connected != g_wifiConnectedLast) {
#if 0
		g_wifiConnectionChanges++;
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 WiFi indicator state connected=%i previous=%i changes=%u ip=%s",
			connected, g_wifiConnectedLast, (unsigned int)g_wifiConnectionChanges,
			HAL_GetMyIPString() ? HAL_GetMyIPString() : "null");
#endif
		g_wifiConnectedLast = connected;
	}
	if (connected) {
		g_wifiLedBlinkPhase = 1;
	} else {
		g_wifiLedBlinkPhase = !g_wifiLedBlinkPhase;
#if 0
		g_wifiBlinkTransitions++;
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 WiFi indicator blink phase=%i transitions=%u",
			g_wifiLedBlinkPhase, (unsigned int)g_wifiBlinkTransitions);
#endif
	}
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
	g_filterUsageSeconds = 0;
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetChannels(int publishMotorRpm) {
	g_ignoreChannelChange = 1;
	CHANNEL_Set(XIAOMI_C4_CH_POWER, g_power, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_MODE, g_mode, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_BRIGHTNESS, g_brightness, CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set(XIAOMI_C4_CH_CHILD_LOCK, g_childLock, CHANNEL_SET_FLAG_SILENT);
	if (g_lastPm25 >= 0) {
		CHANNEL_Set(XIAOMI_C4_CH_PM25, g_lastPm25, CHANNEL_SET_FLAG_SILENT);
	}
	if (publishMotorRpm) {
		CHANNEL_Set(XIAOMI_C4_CH_MOTOR_RPM, g_lastMotorRpm, CHANNEL_SET_FLAG_SILENT);
	}
	CHANNEL_Set(XIAOMI_C4_CH_FILTER_USAGE, XiaomiCompact4_FilterUsageDays(), CHANNEL_SET_FLAG_SILENT);
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
		if (XiaomiCompact4_IsWiFiConnected() || g_wifiLedBlinkPhase) status |= XIAOMI_C4_STATUS_WIFI;
		if (XiaomiCompact4_ReplaceFilter()) status |= XIAOMI_C4_STATUS_REPLACE_FILTER;
	}
	if (g_childLock) {
		status |= XIAOMI_C4_STATUS_CHILD_LOCK;
	}
	if (g_childLockNotificationFlips > 0 && g_childLockNotificationState) {
		status ^= XIAOMI_C4_STATUS_CHILD_LOCK;
	}
	XiaomiCompact4_I2CWrite1(XIAOMI_C4_I2C_STATUS_LEDS, status);

	if (disableHid) {
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_GREEN, 0);
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_ORANGE, 0);
		XiaomiCompact4_SetLedPWM(XIAOMI_C4_PIN_LED_RED, 0);
	} else {
		float green = 0.0f, orange = 0.0f, red = 0.0f;
		int pm = XiaomiCompact4_EffectivePm25();
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

// Forensic trace reference: explains each requested motor-target transition.
static void XiaomiCompact4_UpdateMotorTarget(void) {
#if 0
	int previousTarget = g_motorTargetPercent;
#endif
	int effectivePm25 = XiaomiCompact4_EffectivePm25();
	int newTarget = 0;

	if (!g_power || g_failsafeActive) {
		g_motorTargetPercent = newTarget;
		g_motorCurrentPercent = 0;
#if 0
		if (previousTarget != newTarget) {
			g_motorTargetChanges++;
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 motor target %i->%i power=%i mode=%s pm=%i pfactor_x100=%i failsafe=%i changes=%u",
				previousTarget, newTarget, g_power, XiaomiCompact4_ModeToStr(g_mode), effectivePm25,
				g_pFactorX100, g_failsafeActive, (unsigned int)g_motorTargetChanges);
		}
#endif
		return;
	}
	float motorSetpoint = 0.0f;
	if (g_mode == XIAOMI_C4_MODE_AUTO) {
		motorSetpoint = XiaomiCompact4_ClampFloat((float)effectivePm25 * ((float)g_pFactorX100 / 100.0f), 0.0f, 100.0f);
	} else if (g_mode == XIAOMI_C4_MODE_FAV) {
		motorSetpoint = g_favSpeed;
	} else {
		motorSetpoint = g_nightSpeed;
	}
	newTarget = XiaomiCompact4_ClampInt((int)(motorSetpoint + 0.5f), 0, 100);
	g_motorTargetPercent = newTarget;
#if 0
	if (previousTarget != newTarget) {
		g_motorTargetChanges++;
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 motor target %i->%i power=%i mode=%s pm=%i pfactor_x100=%i fav=%i night=%i changes=%u",
			previousTarget, newTarget, g_power, XiaomiCompact4_ModeToStr(g_mode), effectivePm25,
			g_pFactorX100, g_favSpeed, g_nightSpeed, (unsigned int)g_motorTargetChanges);
	}
#endif
}

static void XiaomiCompact4_ApplyState(int save) {
	// Power OFF is immediate. Power ON is deferred until the main task confirms valid PWM.
	if (!g_power || g_failsafeActive) {
		XiaomiCompact4_SetMotorEnabled(0);
	}
	// HID I2C is applied by RunEverySecond on the main task, never by a caller's task.
#if 0
	g_hidDeferredRequests++;
#endif
	XiaomiCompact4_UpdateMotorTarget();
	XiaomiCompact4_SetChannels(1);
	if (save) {
		XiaomiCompact4_SaveState();
	}
}

static void XiaomiCompact4_SetPower(int value) {
	if (value && !g_motorPwmReady) {
		// Keep the UI and saved state OFF when the hardware cannot honor Power ON.
		XiaomiCompact4_ForceMotorFailsafe(XIAOMI_C4_FAILSAFE_REASON_MOTOR_PWM);
		XiaomiCompact4_ApplyState(0);
		return;
	}
	g_power = value ? 1 : 0;
	XiaomiCompact4_ApplyState(1);
}

static int XiaomiCompact4_SetFilterLifespanDays(int value) {
	if (value < XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS
		|| value > XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS) {
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 rejected filter lifespan %i; valid range is %i-%i days",
			value, XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS,
			XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS);
		XiaomiCompact4_SetChannels(0);
		return 0;
	}
	g_filterLifespanDays = value;
	XiaomiCompact4_ApplyState(1);
	return 1;
}

static void XiaomiCompact4_SetFavoriteSpeed(int value) {
	g_favSpeed = XiaomiCompact4_ClampInt(value, 0, 100);
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetNightSpeed(int value) {
	g_nightSpeed = XiaomiCompact4_ClampInt(value, 0, 100);
	XiaomiCompact4_ApplyState(1);
}

static void XiaomiCompact4_SetPFactorX100(int value) {
	g_pFactorX100 = XiaomiCompact4_ClampInt(value, 1, 1000);
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

#if 0
// Maps touch GPIOs to the disabled validation counters.
static int XiaomiCompact4_ButtonIndex(int pin) {
	if (pin == XIAOMI_C4_PIN_BUTTON_POWER) return 0;
	if (pin == XIAOMI_C4_PIN_BUTTON_LIGHT) return 1;
	if (pin == XIAOMI_C4_PIN_BUTTON_MODE) return 2;
	return -1;
}
#endif

static const char *XiaomiCompact4_ButtonName(int pin) {
	if (pin == XIAOMI_C4_PIN_BUTTON_POWER) return "power";
	if (pin == XIAOMI_C4_PIN_BUTTON_LIGHT) return "light";
	if (pin == XIAOMI_C4_PIN_BUTTON_MODE) return "mode";
	return "unknown";
}

// Forensic trace reference: records touch edges, duration decisions and ignored taps.
static void XiaomiCompact4_ServiceButton(int pin, int *last, int *pressTicks, int *longHandled) {
#if 0
	int index = XiaomiCompact4_ButtonIndex(pin);
#endif
	int pressed = HAL_PIN_ReadDigitalInput(pin) == 0;
	if (pressed && !*last) {
		*pressTicks = 0;
		if (longHandled) *longHandled = 0;
#if 0
		if (index >= 0) g_buttonPressEdges[index]++;
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 button press name=%s pin=%i presses=%u tick_ms=%i",
			XiaomiCompact4_ButtonName(pin), pin,
			index >= 0 ? (unsigned int)g_buttonPressEdges[index] : 0U,
			QUICK_TMR_DURATION);
#endif
	}
	if (pressed) {
		(*pressTicks)++;
		if (longHandled && !*longHandled
			&& *pressTicks >= XIAOMI_C4_TICKS_FOR_MS(XIAOMI_C4_LONG_PRESS_MS)) {
			*longHandled = 1;
#if 0
			if (index >= 0) g_buttonLongPresses[index]++;
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 button long name=%s pin=%i duration_ms=%i threshold_ms=%i count=%u",
				XiaomiCompact4_ButtonName(pin), pin,
				*pressTicks * QUICK_TMR_DURATION, XIAOMI_C4_LONG_PRESS_MS,
				index >= 0 ? (unsigned int)g_buttonLongPresses[index] : 0U);
#endif
			XiaomiCompact4_SetChildLock(!g_childLock);
		}
	} else if (*last) {
		int durationMs = *pressTicks * QUICK_TMR_DURATION;
#if 0
		if (index >= 0) g_buttonReleaseEdges[index]++;
#endif
		if (!longHandled || !*longHandled) {
			if (*pressTicks <= XIAOMI_C4_TICKS_FOR_MS(XIAOMI_C4_CLICK_MAX_MS)) {
#if 0
				if (index >= 0) g_buttonClicks[index]++;
				XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
					"XiaomiCompact4 button click name=%s pin=%i duration_ms=%i ticks=%i clicks=%u",
					XiaomiCompact4_ButtonName(pin), pin, durationMs, *pressTicks,
					index >= 0 ? (unsigned int)g_buttonClicks[index] : 0U);
#endif
				XiaomiCompact4_ProcessClick(pin);
			} else {
				ADDLOG_WARN(LOG_FEATURE_DRV,
					"XiaomiCompact4 button ignored name=%s pin=%i duration_ms=%i ticks=%i max_click_ms=%i",
					XiaomiCompact4_ButtonName(pin), pin, durationMs, *pressTicks,
					XIAOMI_C4_CLICK_MAX_MS);
			}
		}
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 button release name=%s pin=%i duration_ms=%i ticks=%i releases=%u long_handled=%i",
			XiaomiCompact4_ButtonName(pin), pin, durationMs, *pressTicks,
			index >= 0 ? (unsigned int)g_buttonReleaseEdges[index] : 0U,
			longHandled ? *longHandled : 0);
#endif
		*pressTicks = 0;
	}
	*last = pressed;
}

static void IRAM_ATTR XiaomiCompact4_TachISR(void *arg) {
	(void)arg;
	g_tachPulses++;
}

// Forensic trace reference: records driver-local GPIO34 ISR setup and teardown results.
static int XiaomiCompact4_TachInit(void) {
#if 0
	int reusedService = 0;
#endif
	gpio_config_t config = {
		.pin_bit_mask = 1ULL << XIAOMI_C4_PIN_TACH,
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_POSEDGE,
	};
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_GPIO_CONFIG_BEGIN,
		// XIAOMI_C4_PIN_TACH, GPIO_INTR_POSEDGE, ESP_OK, 0);
	esp_err_t err = gpio_config(&config);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_GPIO_CONFIG_DONE,
		// XIAOMI_C4_PIN_TACH, GPIO_INTR_POSEDGE, err, 0);
	if (err != ESP_OK) {
		ADDLOG_ERROR(LOG_FEATURE_DRV,
			"XiaomiCompact4 tach GPIO setup failed pin=%i err=%i name=%s",
			XIAOMI_C4_PIN_TACH, err, esp_err_to_name(err));
		return 0;
	}

	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_ISR_SERVICE_BEGIN,
		// XIAOMI_C4_PIN_TACH, 0, ESP_OK, 0);
	err = gpio_install_isr_service(0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_ISR_SERVICE_DONE,
		// XIAOMI_C4_PIN_TACH, 0, err, 0);
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
		ADDLOG_ERROR(LOG_FEATURE_DRV,
			"XiaomiCompact4 tach ISR service failed pin=%i err=%i name=%s",
			XIAOMI_C4_PIN_TACH, err, esp_err_to_name(err));
		return 0;
	}
#if 0
	reusedService = err == ESP_ERR_INVALID_STATE;
#endif

	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_HANDLER_ADD_BEGIN,
		// XIAOMI_C4_PIN_TACH, GPIO_INTR_POSEDGE, ESP_OK, 0);
	err = gpio_isr_handler_add(XIAOMI_C4_PIN_TACH, XiaomiCompact4_TachISR, NULL);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_HANDLER_ADD_DONE,
		// XIAOMI_C4_PIN_TACH, GPIO_INTR_POSEDGE, err, 0);
	if (err != ESP_OK) {
		ADDLOG_ERROR(LOG_FEATURE_DRV,
			"XiaomiCompact4 tach ISR attach failed pin=%i err=%i name=%s",
			XIAOMI_C4_PIN_TACH, err, esp_err_to_name(err));
		return 0;
	}

	g_tachIsrAttached = 1;
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 tach ISR active pin=%i edge=rising pull=none service=%s",
		XIAOMI_C4_PIN_TACH, reusedService ? "reused" : "installed");
#endif
	return 1;
}

static void XiaomiCompact4_TachShutdown(void) {
	if (!g_tachIsrAttached) return;

#if 0
	esp_err_t disableErr = gpio_intr_disable(XIAOMI_C4_PIN_TACH);
	esp_err_t removeErr = gpio_isr_handler_remove(XIAOMI_C4_PIN_TACH);
	g_tachIsrAttached = 0;
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 tach ISR shutdown pin=%i disable_err=%i remove_err=%i",
		XIAOMI_C4_PIN_TACH, disableErr, removeErr);
#else
	gpio_intr_disable(XIAOMI_C4_PIN_TACH);
	gpio_isr_handler_remove(XIAOMI_C4_PIN_TACH);
	g_tachIsrAttached = 0;
#endif
}

static void XiaomiCompact4_SendPM25Query(void) {
	if (g_pm25QueryAttempts > 0) {
		// Judge the previous request only after its full five-second response window.
#if 0
		if (!g_pm25FrameSeenSinceQuery) g_pm25EmptyQueryWindows++;
		if (g_pm25RxBytesSinceQuery == 0) g_pm25NoRxQueryWindows++;
		if (!g_pm25ValidFrameSeenSinceQuery) g_pm25NoValidQueryWindows++;
#endif
		if (g_pm25ValidFrameSeenSinceQuery) {
			g_pm25ConsecutiveInvalidWindows = 0;
			if (g_failsafeActive && g_failsafeReason == XIAOMI_C4_FAILSAFE_REASON_PM25_STALE) {
				g_pm25ConsecutiveRecoveryWindows++;
				if (g_pm25ConsecutiveRecoveryWindows >= XIAOMI_C4_PM25_RECOVERY_QUERY_WINDOWS) {
					XiaomiCompact4_ClearPM25Failsafe();
				}
			} else {
				g_pm25ConsecutiveRecoveryWindows = 0;
			}
		} else {
			g_pm25ConsecutiveInvalidWindows++;
			g_pm25ConsecutiveRecoveryWindows = 0;
			if (g_pm25ConsecutiveInvalidWindows >= XIAOMI_C4_PM25_STALE_QUERY_WINDOWS
				&& (!g_failsafeActive || g_failsafeReason == XIAOMI_C4_FAILSAFE_REASON_PM25_STALE)) {
#if 0
				if (!g_failsafeActive) g_pm25FailsafeActivations++;
#endif
				XiaomiCompact4_ForceMotorFailsafe(XIAOMI_C4_FAILSAFE_REASON_PM25_STALE);
			}
		}
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 PM25 previous q=%u rx_bytes=%u candidate=%i valid=%i scratch=%i invalid_windows=%u recovery_windows=%u",
			(unsigned int)g_pm25QueryAttempts, (unsigned int)g_pm25RxBytesSinceQuery,
			g_pm25FrameSeenSinceQuery, g_pm25ValidFrameSeenSinceQuery, g_pm25RxLen,
			(unsigned int)g_pm25ConsecutiveInvalidWindows,
			(unsigned int)g_pm25ConsecutiveRecoveryWindows);
#endif
	}
#if 0
	g_pm25FrameSeenSinceQuery = 0;
#endif
	g_pm25ValidFrameSeenSinceQuery = 0;
#if 0
	g_pm25RxBytesSinceQuery = 0;
#endif
	g_pm25QueryAttempts++;
#if 0
	g_pm25LastQueryTickMs = XiaomiCompact4_UptimeMs();
	if (XIAOMI_C4_FORENSIC_TRACE_ENABLED && g_loglevel >= LOG_EXTRADEBUG) {
		uint32_t actualBaud = 0;
		uint32_t sourceClockHz = 0;
		size_t buffered = 0;
		esp_err_t baudErr = uart_get_baudrate(XIAOMI_C4_UART_PORT, &actualBaud);
		esp_err_t sourceClockErr = uart_get_sclk_freq(XIAOMI_C4_UART_SOURCE_CLK, &sourceClockHz);
		esp_err_t bufferedErr = uart_get_buffered_data_len(XIAOMI_C4_UART_PORT, &buffered);
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 UART TX begin q=%u port=%i driver=%i baud=%u baud_err=%i source=%i source_hz=%u source_err=%i buffered=%u buffered_err=%i data=11 02 0B 01 E1",
			(unsigned int)g_pm25QueryAttempts, (int)XIAOMI_C4_UART_PORT,
			uart_is_driver_installed(XIAOMI_C4_UART_PORT), (unsigned int)actualBaud,
			baudErr, (int)XIAOMI_C4_UART_SOURCE_CLK, (unsigned int)sourceClockHz,
			sourceClockErr, (unsigned int)buffered, bufferedErr);
	}
#endif
	int written = uart_write_bytes(XIAOMI_C4_UART_PORT, g_pm25Query, sizeof(g_pm25Query));
#if 0
	if (written == (int)sizeof(g_pm25Query)) {
		g_pm25QueriesSent++;
	} else {
#else
	if (written != (int)sizeof(g_pm25Query)) {
#endif
#if 0
		g_pm25UartWriteErrors++;
#endif
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART write error q=%u written=%i expected=%i",
			(unsigned int)g_pm25QueryAttempts, written, (int)sizeof(g_pm25Query));
	}
	esp_err_t waitErr = uart_wait_tx_done(XIAOMI_C4_UART_PORT, pdMS_TO_TICKS(XIAOMI_C4_UART_TX_DONE_TIMEOUT_MS));
#if 0
	if (waitErr == ESP_OK && written == (int)sizeof(g_pm25Query)) {
		g_pm25TxCompleted++;
	} else if (waitErr != ESP_OK) {
#else
	if (waitErr != ESP_OK) {
#endif
#if 0
		g_pm25UartTxWaitErrors++;
#endif
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 UART TX wait failed q=%u err=%i written=%i",
			(unsigned int)g_pm25QueryAttempts, waitErr, written);
	}
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 UART TX done q=%u written=%i wait_err=%i elapsed_ms=%u qwrite=%u txdone=%u",
		(unsigned int)g_pm25QueryAttempts, written, waitErr,
		(unsigned int)(XiaomiCompact4_UptimeMs() - g_pm25LastQueryTickMs),
		(unsigned int)g_pm25QueriesSent, (unsigned int)g_pm25TxCompleted);
#endif
}

// Forensic trace reference: exposes parser resynchronization and complete sensor frames.
static void XiaomiCompact4_ProcessPM25UART(void) {
	XiaomiCompact4_UARTProcessEvents();
	XiaomiCompact4_UARTReadAvailable();
	while (g_pm25RxLen >= 3) {
		if (g_pm25RxLen < (int)sizeof(g_pm25Query)
			&& memcmp(g_pm25RxBuf, g_pm25Query, g_pm25RxLen) == 0) {
			break;
		}
		if (g_pm25RxLen >= (int)sizeof(g_pm25Query)
			&& memcmp(g_pm25RxBuf, g_pm25Query, sizeof(g_pm25Query)) == 0) {
#if 0
			g_pm25QueryEchoes++;
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 query echo q=%u scratch=%i",
				(unsigned int)g_pm25QueryAttempts, g_pm25RxLen);
#endif
			XiaomiCompact4_UARTConsume(sizeof(g_pm25Query));
			continue;
		}
		if (XiaomiCompact4_UARTPeek(0) != 0x16 || XiaomiCompact4_UARTPeek(1) != 0x11 || XiaomiCompact4_UARTPeek(2) != 0x0B) {
#if 0
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 discard q=%u byte=%02X scratch=%i",
				(unsigned int)g_pm25QueryAttempts, XiaomiCompact4_UARTPeek(0), g_pm25RxLen);
			g_pm25DiscardedBytes++;
#endif
			XiaomiCompact4_UARTConsume(1);
			continue;
		}
		if (g_pm25RxLen < XIAOMI_C4_PM25_FRAME_LEN) {
			break;
		}
		uint8_t frame[XIAOMI_C4_PM25_FRAME_LEN];
		memcpy(frame, g_pm25RxBuf, XIAOMI_C4_PM25_FRAME_LEN);
#if 0
		g_pm25HeaderCandidates++;
#endif
#if 0
		g_pm25FrameSeenSinceQuery = 1;
#endif
		int checksumValid = XiaomiCompact4_PM25FrameChecksumValid(frame);
#if 0
		char hex[XIAOMI_C4_PM25_FRAME_LEN * 3] = { 0 };
		int hexReady = 0;
		if (XIAOMI_C4_FORENSIC_TRACE_ENABLED && g_loglevel >= LOG_EXTRADEBUG) {
			XiaomiCompact4_PM25FrameToHex(frame, hex, sizeof(hex));
			hexReady = 1;
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 PM25 candidate q=%u cand=%u checksum=%i delta_ms=%u frame=%s",
				(unsigned int)g_pm25QueryAttempts, (unsigned int)g_pm25HeaderCandidates,
				checksumValid, (unsigned int)(XiaomiCompact4_UptimeMs() - g_pm25LastQueryTickMs), hex);
		}
#endif
		if (!checksumValid) {
			g_pm25RejectedChecksum++;
#if 0
			g_pm25ChecksumResyncs++;
#endif
			if ((g_pm25RejectedChecksum % XIAOMI_C4_WARNING_LOG_INTERVAL) == 1) {
				ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 bad checksum q=%u rejected=%i",
					(unsigned int)g_pm25QueryAttempts, g_pm25RejectedChecksum);
			}
#if 0
			if (XIAOMI_C4_FORENSIC_TRACE_ENABLED && g_loglevel >= LOG_EXTRADEBUG) {
				if (!hexReady) XiaomiCompact4_PM25FrameToHex(frame, hex, sizeof(hex));
				XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
					"XiaomiCompact4 PM25 bad checksum q=%u rejected=%i frame=%s",
					(unsigned int)g_pm25QueryAttempts, g_pm25RejectedChecksum, hex);
			}
#endif
			// Preserve any valid frame that may begin inside a corrupted candidate.
			XiaomiCompact4_UARTConsume(1);
			continue;
		}
		XiaomiCompact4_UARTConsume(XIAOMI_C4_PM25_FRAME_LEN);
#if 0
		int df3Df4 = (((int)frame[5]) << 8) | frame[6];
#endif
		int df13Df14 = (((int)frame[15]) << 8) | frame[16];
		int pm25 = df13Df14;
#if 0
		memcpy(g_pm25LastFrame, frame, sizeof(g_pm25LastFrame));
		g_pm25LastRaw = pm25;
#endif
		if (pm25 > XIAOMI_C4_PM25_MAX_VALID) {
			g_pm25RejectedRange++;
			if ((g_pm25RejectedRange % XIAOMI_C4_WARNING_LOG_INTERVAL) == 1) {
				ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 out of range q=%u rejected=%i",
					(unsigned int)g_pm25QueryAttempts, g_pm25RejectedRange);
			}
#if 0
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
				"XiaomiCompact4 PM25 out of range q=%u rejected=%i df3_4=%i df13_14=%i",
				(unsigned int)g_pm25QueryAttempts, g_pm25RejectedRange, df3Df4, df13Df14);
#endif
			continue;
		}
#if 0
		if (pm25 >= XIAOMI_C4_PM25_HIGH_LOG_THRESHOLD && XIAOMI_C4_FORENSIC_TRACE_ENABLED && g_loglevel >= LOG_EXTRADEBUG) {
			if (!hexReady) XiaomiCompact4_PM25FrameToHex(frame, hex, sizeof(hex));
			XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 PM25 high %i frame: %s", pm25, hex);
		}
		if (g_pm25LastValidSecond > 0) {
			uint32_t gap = g_diagEverySeconds - g_pm25LastValidSecond;
			if (gap > g_pm25MaxValidGapSeconds) g_pm25MaxValidGapSeconds = gap;
		}
		if (g_pm25LastValidQuery > 0) {
			uint32_t gap = g_pm25QueriesSent - g_pm25LastValidQuery;
			if (gap > g_pm25MaxValidQueryGap) g_pm25MaxValidQueryGap = gap;
		}
		g_pm25LastValidSecond = g_diagEverySeconds;
		g_pm25LastValidQuery = g_pm25QueriesSent;
#endif
		g_pm25ValidFrameSeenSinceQuery = 1;
#if 0
		g_pm25AcceptedFrames++;
#endif
		g_lastPm25 = pm25;
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 PM25 accepted q=%u accepted=%i df3_4=%i df13_14=%i selected=%i delta_ms=%u",
			(unsigned int)g_pm25QueryAttempts, g_pm25AcceptedFrames, df3Df4, df13Df14,
			pm25, (unsigned int)(XiaomiCompact4_UptimeMs() - g_pm25LastQueryTickMs));
		// Keep I2C and channel publication off the small, latency-sensitive quick task.
		g_pm25DeferredUpdates++;
#endif
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
#if 0
		g_hidDeferredRequests++;
#endif
	}
}

void XiaomiCompact4_RunQuickTick(void) {
	if (!g_initialized) return;
	XiaomiCompact4_ProcessPM25UART();
	XiaomiCompact4_ServiceButton(XIAOMI_C4_PIN_BUTTON_POWER, &g_lastButtonPower, &g_powerPressTicks, NULL);
	XiaomiCompact4_ServiceButton(XIAOMI_C4_PIN_BUTTON_LIGHT, &g_lastButtonLight, &g_lightPressTicks, NULL);
	XiaomiCompact4_ServiceButton(XIAOMI_C4_PIN_BUTTON_MODE, &g_lastButtonMode, &g_modePressTicks, &g_modeLongHandled);
	XiaomiCompact4_ServiceNotification();
	g_diagQuickTicks++;
#if 0
	// Validation reference: periodically sampled quick-task stack headroom.
	if ((g_diagQuickTicks % 10U) == 0U) {
		UBaseType_t stackFree = uxTaskGetStackHighWaterMark(NULL);
		if (stackFree < g_diagQuickStackMin) g_diagQuickStackMin = stackFree;
	}
#endif
}

void XiaomiCompact4_RunEverySecond(void) {
	static int motorRpmPublishCountdown = 0;
	static uint32_t lastQuickTicks = 0;
	int publishMotorRpm = 0;
	int i2cRecoveryAttempted;

	if (!g_initialized) return;
#if 0
	g_diagEverySeconds++;
#endif
	if (g_diagQuickTicks == lastQuickTicks) {
		g_quickTickStaleSeconds++;
		if (g_power && g_quickTickStaleSeconds >= XIAOMI_C4_QUICKTICK_STALE_FAILSAFE_SECONDS) {
			XiaomiCompact4_ForceMotorFailsafe(XIAOMI_C4_FAILSAFE_REASON_QUICKTICK_STALE);
		}
	} else {
		lastQuickTicks = g_diagQuickTicks;
		g_quickTickStaleSeconds = 0;
	}
	g_pm25PollCountdown--;
	if (g_pm25PollCountdown <= 0) {
		g_pm25PollCountdown = XIAOMI_C4_PM25_POLL_SECONDS;
		XiaomiCompact4_SendPM25Query();
	}
	g_filterCountdown--;
	if (g_filterCountdown <= 0) {
		g_filterCountdown = XIAOMI_C4_FILTER_USAGE_TICK_SECONDS;
		if (g_power) {
			g_filterUsageSeconds += XIAOMI_C4_FILTER_USAGE_TICK_SECONDS;
			g_filterUsageDirty = 1;
		}
	}
	g_filterCheckpointCountdown--;
	if (g_filterCheckpointCountdown <= 0) {
		g_filterCheckpointCountdown = XIAOMI_C4_FILTER_CHECKPOINT_SECONDS;
		if (g_filterUsageDirty) {
			XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FILTER_USAGE, (int)g_filterUsageSeconds);
		}
	}
	g_tachLastWindowEdges = g_tachPulses;
	g_tachPulses = 0;
	g_tachTotalEdges += g_tachLastWindowEdges;
#if 0
	if (g_tachLastWindowEdges > g_tachMaxWindowEdges) g_tachMaxWindowEdges = g_tachLastWindowEdges;
	if (g_tachLastWindowEdges > 0) g_tachLastActiveSecond = g_diagEverySeconds;
#endif
	if (g_tachLastWindowEdges >= XIAOMI_C4_TACH_STORM_EDGES_PER_SECOND) {
		g_tachStormWindows++;
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 tach edge storm edges_per_sec=%u total=%u windows=%u",
			(unsigned int)g_tachLastWindowEdges, (unsigned int)g_tachTotalEdges,
			(unsigned int)g_tachStormWindows);
	}
	g_lastMotorRpm = (int)((g_tachLastWindowEdges * 60U) / 15U);
	motorRpmPublishCountdown--;
	if (motorRpmPublishCountdown <= 0) {
		motorRpmPublishCountdown = XIAOMI_C4_MOTOR_RPM_PUBLISH_SECONDS;
		publishMotorRpm = 1;
	}
	// Consume PM/control updates on the main task before advancing the motor ramp.
	i2cRecoveryAttempted = XiaomiCompact4_I2CServiceRecovery();
	XiaomiCompact4_UpdateMotorTarget();
	XiaomiCompact4_ApplyMotorRamp();
	XiaomiCompact4_ServiceWiFiIndicator();
	// Give a recreated bus one quiet cycle before sending the next CMS update.
	if (!i2cRecoveryAttempted) XiaomiCompact4_UpdateHID();
	XiaomiCompact4_SetChannels(publishMotorRpm);
}

int XiaomiCompact4_ShouldFeedWDT(void) {
	if (!g_initialized) {
		return 1;
	}
	if (g_quickTickStaleSeconds <= XIAOMI_C4_QUICKTICK_STALE_FAILSAFE_SECONDS) {
		return 1;
	}
	if ((g_quickTickStaleSeconds % 5) == 4) {
		ADDLOG_WARN(LOG_FEATURE_DRV, "XiaomiCompact4 QuickTick stalled for %i seconds; withholding WDT feed", g_quickTickStaleSeconds);
	}
	return 0;
}

void XiaomiCompact4_Stop(void) {
	esp_err_t err;

	// Make the appliance safe before flash writes or peripheral teardown can block.
	g_initialized = 0;
	XiaomiCompact4_SetMotorEnabled(0);
	g_motorTargetPercent = 0;
	g_motorCurrentPercent = 0;
	// Leave the front panel dark before persistence or peripheral teardown can block.
	XiaomiCompact4_BlankHID();
	if (g_filterUsageDirty) {
		XiaomiCompact4_SaveInt(XIAOMI_C4_VAR_FILTER_USAGE, (int)g_filterUsageSeconds);
	}
	if (g_motorPwmReady) {
		err = ledc_stop(LEDC_LOW_SPEED_MODE, XIAOMI_C4_MOTOR_LEDC_CHANNEL, 0);
		if (err != ESP_OK) {
			XiaomiCompact4_RecordMotorPWMError("shutdown", XIAOMI_C4_MOTOR_LEDC_CHANNEL, err);
		}
	}
	g_motorPwmReady = 0;
	g_motorAppliedFrequency = -1;
	// Release HAL-owned channels while keeping every attached output electrically off.
	XiaomiCompact4_StopPWMOutputLow(XIAOMI_C4_PIN_BUZZER);
	XiaomiCompact4_StopPWMOutputLow(XIAOMI_C4_PIN_LED_RED);
	XiaomiCompact4_StopPWMOutputLow(XIAOMI_C4_PIN_LED_ORANGE);
	XiaomiCompact4_StopPWMOutputLow(XIAOMI_C4_PIN_LED_GREEN);
	XiaomiCompact4_TachShutdown();
	XiaomiCompact4_I2CShutdown();
	g_i2cRecoveryPending = 0;
	if (uart_is_driver_installed(XIAOMI_C4_UART_PORT)) {
		esp_err_t err = uart_driver_delete(XIAOMI_C4_UART_PORT);
#if 0
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV, "XiaomiCompact4 UART shutdown port=%i err=%i",
			(int)XIAOMI_C4_UART_PORT, err);
#else
		(void)err;
#endif
	}
	g_pm25UartQueue = NULL;
}

#if 0
/*
 * Forensic retained-state report
 *
 * Enable together with the breadcrumb implementation and ExtraDebug trace
 * binding when another warm-reboot investigation needs the last lifecycle
 * stage on UART.
 */
static void XiaomiCompact4_LogRetainedI2CBreadcrumb(void) {
	if (XiaomiCompact4_I2CBreadcrumbValid()) {
		// Forensic trace reference: identifies the last completed stage after a warm reboot.
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 retained I2C breadcrumb seq=%u stage=%u(%s) uptime_ms=%u addr=0x%02X value=0x%02X err=%i sda=%i scl=%i recovery_pending=%u",
			(unsigned int)g_i2cBreadcrumb.sequence, (unsigned int)g_i2cBreadcrumb.stage,
			XiaomiCompact4_I2CStageName(g_i2cBreadcrumb.stage),
			(unsigned int)g_i2cBreadcrumb.uptimeMs, (unsigned int)g_i2cBreadcrumb.address,
			(unsigned int)g_i2cBreadcrumb.value, (int)g_i2cBreadcrumb.error,
			(int)g_i2cBreadcrumb.sda, (int)g_i2cBreadcrumb.scl,
			(unsigned int)g_i2cBreadcrumb.recoveryPending);
	} else {
		XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
			"XiaomiCompact4 retained I2C breadcrumb invalid magic=0x%08X",
			(unsigned int)g_i2cBreadcrumb.magic);
	}
}
#endif

void XiaomiCompact4_ConfigureWDT(void) {
	wdt_hal_context_t rtcWdt = RWDT_HAL_CONTEXT_DEFAULT();
	uint32_t timeoutTicks = (uint32_t)((uint64_t)XIAOMI_C4_RTC_WDT_TIMEOUT_MS
		* rtc_clk_slow_freq_get_hz() / 1000ULL);

	// Keep the hardware watchdog local to this appliance; other ESP-IDF builds are unaffected.
	wdt_hal_init(&rtcWdt, WDT_RWDT, 0, false);
	wdt_hal_write_protect_disable(&rtcWdt);
	wdt_hal_config_stage(&rtcWdt, WDT_STAGE0, timeoutTicks, WDT_STAGE_ACTION_RESET_SYSTEM);
	wdt_hal_config_stage(&rtcWdt, WDT_STAGE1, timeoutTicks, WDT_STAGE_ACTION_RESET_RTC);
	wdt_hal_enable(&rtcWdt);
	wdt_hal_write_protect_enable(&rtcWdt);
}

void XiaomiCompact4_PreDriverSafetyInit(int safeMode) {
	// The motor clamp is identical in normal and safe-mode startup.
	(void)safeMode;
#if 0
	// Forensic reference: report the retained stage before replacing it.
	XiaomiCompact4_LogRetainedI2CBreadcrumb();
#endif
	// A diagnostic build can preserve the failed-boot stage while safe mode suppresses the driver.
	// if (!safeMode) {
		// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_PRE_DRIVER,
			// -1, safeMode, ESP_OK, 0);
	// }
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_EN, 0);
	HAL_PIN_Setup_Output(XIAOMI_C4_PIN_MOTOR_EN);
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_EN, 0);
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_PWM, 0);
	HAL_PIN_Setup_Output(XIAOMI_C4_PIN_MOTOR_PWM);
	HAL_PIN_SetOutputValue(XIAOMI_C4_PIN_MOTOR_PWM, 0);
#if 0
	XIAOMI_C4_FORENSIC_LOG(LOG_FEATURE_DRV,
		"XiaomiCompact4 pre-driver motor clamp safe_mode=%i enable_pin=%i enable_level=%i pwm_pin=%i pwm_level=%i",
		safeMode, XIAOMI_C4_PIN_MOTOR_EN,
		HAL_PIN_ReadDigitalInput(XIAOMI_C4_PIN_MOTOR_EN), XIAOMI_C4_PIN_MOTOR_PWM,
		HAL_PIN_ReadDigitalInput(XIAOMI_C4_PIN_MOTOR_PWM));
#endif
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
		XiaomiCompact4_SetFavoriteSpeed(value);
		break;
	case XIAOMI_C4_CH_NIGHT_SPEED:
		XiaomiCompact4_SetNightSpeed(value);
		break;
	case XIAOMI_C4_CH_P_FACTOR:
		XiaomiCompact4_SetPFactorX100(value);
		break;
	case XIAOMI_C4_CH_FILTER_LIFESPAN:
		XiaomiCompact4_SetFilterLifespanDays(value);
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
	XiaomiCompact4_SetFavoriteSpeed(Tokenizer_GetArgInteger(0));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetNightSpeed(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	XiaomiCompact4_SetNightSpeed(Tokenizer_GetArgInteger(0));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetPFactor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	XiaomiCompact4_SetPFactorX100((int)(Tokenizer_GetArgFloat(0) * 100.0f));
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_SetFilterLifespan(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	return XiaomiCompact4_SetFilterLifespanDays(Tokenizer_GetArgInteger(0))
		? CMD_RES_OK : CMD_RES_BAD_ARGUMENT;
}

#if 0
/*
 * Validation command reference
 *
 * These reports exposed raw PM UART frames, I2C transaction history and touch
 * timing while the hardware was under test. Release firmware keeps only the
 * normal warnings and recovery behavior, so none of these commands is built.
 */
static commandResult_t CMD_XiaomiCompact4_PM25Stats(const void *context, const char *cmd, const char *args, int cmdFlags) {
	char hex[XIAOMI_C4_PM25_FRAME_LEN * 3];
	char rxHex[XIAOMI_C4_UART_LOG_BYTES * 3];
	uint32_t actualBaud = 0;
	uint32_t sourceClockHz = 0;
	size_t buffered = 0;
	esp_err_t baudErr = uart_get_baudrate(XIAOMI_C4_UART_PORT, &actualBaud);
	esp_err_t sourceClockErr = uart_get_sclk_freq(XIAOMI_C4_UART_SOURCE_CLK, &sourceClockHz);
	esp_err_t bufferedErr = uart_get_buffered_data_len(XIAOMI_C4_UART_PORT, &buffered);
	uint32_t validAge = g_pm25LastValidSecond > 0 ? g_diagEverySeconds - g_pm25LastValidSecond : g_diagEverySeconds;
	uint32_t validQueryGap = g_pm25LastValidQuery > 0 ? g_pm25QueriesSent - g_pm25LastValidQuery : g_pm25QueriesSent;
	XiaomiCompact4_PM25FrameToHex(g_pm25LastFrame, hex, sizeof(hex));
	int rxLogLen = g_pm25RxLen > XIAOMI_C4_UART_LOG_BYTES ? XIAOMI_C4_UART_LOG_BYTES : g_pm25RxLen;
	XiaomiCompact4_BytesToHex(g_pm25RxBuf, rxLogLen, rxHex, sizeof(rxHex));
	ADDLOG_INFO(LOG_FEATURE_CMD, "PM25 raw=%i q=%u qok=%u txdone=%u qempty=%u qnorx=%u qnovalid=%u echo=%u cand=%u drop=%u resync=%u accepted=%i rejected_checksum=%i rejected_range=%i age=%u max_age=%u qgap=%u max_qgap=%u invalid_windows=%u recovery_windows=%u stale_limit=%i recovery_limit=%i stale_activations=%u stale_recoveries=%u qt=%u sec=%u qt_stack_min=%u deferred=%u qt_stale=%i failsafe=%i reason=%i",
		g_pm25LastRaw, (unsigned int)g_pm25QueryAttempts, (unsigned int)g_pm25QueriesSent,
		(unsigned int)g_pm25TxCompleted, (unsigned int)g_pm25EmptyQueryWindows,
		(unsigned int)g_pm25NoRxQueryWindows, (unsigned int)g_pm25NoValidQueryWindows,
		(unsigned int)g_pm25QueryEchoes,
		(unsigned int)g_pm25HeaderCandidates, (unsigned int)g_pm25DiscardedBytes,
		(unsigned int)g_pm25ChecksumResyncs, g_pm25AcceptedFrames, g_pm25RejectedChecksum,
		g_pm25RejectedRange, (unsigned int)validAge, (unsigned int)g_pm25MaxValidGapSeconds,
		(unsigned int)validQueryGap, (unsigned int)g_pm25MaxValidQueryGap,
		(unsigned int)g_pm25ConsecutiveInvalidWindows,
		(unsigned int)g_pm25ConsecutiveRecoveryWindows,
		XIAOMI_C4_PM25_STALE_QUERY_WINDOWS, XIAOMI_C4_PM25_RECOVERY_QUERY_WINDOWS,
		(unsigned int)g_pm25FailsafeActivations, (unsigned int)g_pm25FailsafeRecoveries,
		(unsigned int)g_diagQuickTicks, (unsigned int)g_diagEverySeconds,
		(unsigned int)g_diagQuickStackMin, (unsigned int)g_pm25DeferredUpdates,
		g_quickTickStaleSeconds, g_failsafeActive, g_failsafeReason);
	ADDLOG_INFO(LOG_FEATURE_CMD, "PM25UART port=%i driver=%i baud=%u baud_err=%i source=%i source_hz=%u source_err=%i init_err=%i chunks=%u last_chunk=%i max_chunk=%i rx_bytes=%u rx_len=%i rx_max=%i rx_resets=%i events=%i data_events=%i break=%i frame_err=%i parity_err=%i buf_full=%i fifo_ovf=%i read_err=%i write_err=%i txwait_err=%i buffered=%u buffered_err=%i",
		(int)XIAOMI_C4_UART_PORT, uart_is_driver_installed(XIAOMI_C4_UART_PORT),
		(unsigned int)actualBaud, baudErr, (int)XIAOMI_C4_UART_SOURCE_CLK,
		(unsigned int)sourceClockHz, sourceClockErr, g_pm25UartInitErrors,
		(unsigned int)g_pm25RxChunks, g_pm25LastRxChunk, g_pm25MaxRxChunk,
		(unsigned int)g_diagPm25RxBytes, g_pm25RxLen, g_pm25MaxRxLen, g_pm25ScratchResets,
		g_pm25UartEvents, g_pm25UartDataEvents, g_pm25UartBreakEvents,
		g_pm25UartFrameErrors, g_pm25UartParityErrors, g_pm25UartBufferFull,
		g_pm25UartFifoOvf, g_pm25UartReadErrors, g_pm25UartWriteErrors,
		g_pm25UartTxWaitErrors, (unsigned int)buffered, bufferedErr);
	ADDLOG_INFO(LOG_FEATURE_CMD, "PM25BUF q=%u rx_since_query=%u last_query_ms=%u last_rx_ms=%u scratch=%s%s last_frame=%s",
		(unsigned int)g_pm25QueryAttempts, (unsigned int)g_pm25RxBytesSinceQuery,
		(unsigned int)g_pm25LastQueryTickMs, (unsigned int)g_pm25LastRxTickMs,
		rxHex, g_pm25RxLen > rxLogLen ? " ..." : "", hex);
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"MOTOR power=%i mode=%s pm=%i pfactor_x100=%i target_pct=%i current_pct=%i target_hz=%i current_hz=%i rpm=%i target_changes=%u",
		g_power, XiaomiCompact4_ModeToStr(g_mode), XiaomiCompact4_EffectivePm25(), g_pFactorX100,
		g_motorTargetPercent, g_motorCurrentPercent,
		(g_power && !g_failsafeActive) ? XiaomiCompact4_MotorPercentToFrequency(g_motorTargetPercent) : 0,
		(g_power && !g_failsafeActive) ? XiaomiCompact4_MotorPercentToFrequency(g_motorCurrentPercent) : 0,
		g_lastMotorRpm, (unsigned int)g_motorTargetChanges);
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"MOTORPWM ready=%i enabled=%i applied_hz=%i min_hz=%i max_hz=%i freq_changes=%u errors=%u last_err=%i",
		g_motorPwmReady, g_motorEnabled, g_motorAppliedFrequency,
		XIAOMI_C4_MOTOR_MIN_FREQUENCY, XIAOMI_C4_MOTOR_MAX_FREQUENCY,
		(unsigned int)g_motorFrequencyChanges, (unsigned int)g_motorPwmErrors,
		g_motorPwmLastError);
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_I2CStats(const void *context, const char *cmd, const char *args, int cmdFlags) {
	static const int addresses[XIAOMI_C4_I2C_DEVICE_COUNT] = {
		XIAOMI_C4_I2C_BRIGHTNESS,
		XIAOMI_C4_I2C_STATUS_LEDS,
		XIAOMI_C4_I2C_BUTTON_POWER,
		XIAOMI_C4_I2C_BUTTON_BRIGHTNESS,
		XIAOMI_C4_I2C_BUTTON_MODE,
	};
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"I2C init=%u init_err=%u port=%i sda=%i scl=%i freq=%i bus=%p req=%u writes=%u ok=%u fail=%u skipped=%u deferred=%u lock_timeout=%u max_write_ms=%u null=%u unknown=%u consecutive=%u max_consecutive=%u last_addr=0x%02X last_value=0x%02X last_err=%i last_sda=%i last_scl=%i last_ok_ms=%u last_fail_ms=%u",
		(unsigned int)g_i2cInitAttempts, (unsigned int)g_i2cInitErrors,
		(int)XIAOMI_C4_I2C_PORT, XIAOMI_C4_PIN_I2C_SDA, XIAOMI_C4_PIN_I2C_SCL,
		XIAOMI_C4_I2C_FREQ_HZ, g_i2cBus, (unsigned int)g_i2cWriteRequests,
		(unsigned int)g_i2cWriteAttempts,
		(unsigned int)g_i2cWriteSuccesses, (unsigned int)g_i2cWriteFailures,
		(unsigned int)g_i2cSkippedUnchanged, (unsigned int)g_hidDeferredRequests,
		(unsigned int)g_i2cLockTimeouts,
		(unsigned int)g_i2cMaxWriteMs,
		(unsigned int)g_i2cNullHandleWrites, (unsigned int)g_i2cUnknownAddressWrites,
		(unsigned int)g_i2cConsecutiveFailures, (unsigned int)g_i2cMaxConsecutiveFailures,
		g_i2cLastAddress, g_i2cLastValue, g_i2cLastError, g_i2cLastSda, g_i2cLastScl,
		(unsigned int)g_i2cLastSuccessMs, (unsigned int)g_i2cLastFailureMs);
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"I2CRECOVERY attempts=%u ok=%u fail=%u pending=%i requested_ms=%u last_ms=%u last_err=%i suppressed=%u cooldown_ms=%i tx_timeout_ms=%i refresh_ms=%i mutex=%p wifi_connected=%i wifi_blink_phase=%i wifi_changes=%u wifi_blinks=%u",
		(unsigned int)g_i2cRecoveryAttempts, (unsigned int)g_i2cRecoverySuccesses,
		(unsigned int)g_i2cRecoveryFailures, g_i2cRecoveryPending,
		(unsigned int)g_i2cRecoveryRequestedMs, (unsigned int)g_i2cLastRecoveryMs,
		g_i2cLastRecoveryError, (unsigned int)g_i2cRecoverySuppressedWrites,
		XIAOMI_C4_I2C_RECOVERY_COOLDOWN_MS, XIAOMI_C4_I2C_TX_TIMEOUT_MS,
		XIAOMI_C4_I2C_REFRESH_MS, g_i2cMutex,
		XiaomiCompact4_IsWiFiConnected(), g_wifiLedBlinkPhase,
		(unsigned int)g_wifiConnectionChanges, (unsigned int)g_wifiBlinkTransitions);
#if 0
	/* Forensic reference: retained I2C stage from the previous warm reboot. */
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"I2CBREADCRUMB valid=%i seq=%u stage=%u name=%s uptime_ms=%u addr=0x%02X value=0x%02X err=%i sda=%i scl=%i recovery_pending=%u",
		XiaomiCompact4_I2CBreadcrumbValid(), (unsigned int)g_i2cBreadcrumb.sequence,
		(unsigned int)g_i2cBreadcrumb.stage, XiaomiCompact4_I2CStageName(g_i2cBreadcrumb.stage),
		(unsigned int)g_i2cBreadcrumb.uptimeMs, (unsigned int)g_i2cBreadcrumb.address,
		(unsigned int)g_i2cBreadcrumb.value, (int)g_i2cBreadcrumb.error,
		(int)g_i2cBreadcrumb.sda, (int)g_i2cBreadcrumb.scl,
		(unsigned int)g_i2cBreadcrumb.recoveryPending);
#endif
	for (int i = 0; i < XIAOMI_C4_I2C_DEVICE_COUNT; i++) {
		int addr7 = addresses[i];
		ADDLOG_INFO(LOG_FEATURE_CMD,
			"I2CDEV index=%i addr=0x%02X name=%s handle=%p writes=%u ok=%u fail=%u last_value=0x%02X last_ok_value=0x%02X last_ok_ms=%u last_err=%i",
			i, addr7, XiaomiCompact4_I2CNameForAddress(addr7),
			XiaomiCompact4_I2CHandleForAddress(addr7),
			(unsigned int)g_i2cDeviceAttempts[i], (unsigned int)g_i2cDeviceSuccesses[i],
			(unsigned int)g_i2cDeviceFailures[i], g_i2cDeviceLastValue[i],
			g_i2cDeviceLastSuccessValue[i], (unsigned int)g_i2cDeviceLastSuccessMs[i],
			g_i2cDeviceLastError[i]);
	}
	return CMD_RES_OK;
}

static commandResult_t CMD_XiaomiCompact4_ButtonStats(const void *context, const char *cmd, const char *args, int cmdFlags) {
	static const int pins[3] = {
		XIAOMI_C4_PIN_BUTTON_POWER,
		XIAOMI_C4_PIN_BUTTON_LIGHT,
		XIAOMI_C4_PIN_BUTTON_MODE,
	};
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"BUTTONS tick_ms=%i click_max_ms=%i click_max_ticks=%i long_ms=%i long_ticks=%i",
		QUICK_TMR_DURATION, XIAOMI_C4_CLICK_MAX_MS,
		XIAOMI_C4_TICKS_FOR_MS(XIAOMI_C4_CLICK_MAX_MS), XIAOMI_C4_LONG_PRESS_MS,
		XIAOMI_C4_TICKS_FOR_MS(XIAOMI_C4_LONG_PRESS_MS));
	for (int i = 0; i < 3; i++) {
		ADDLOG_INFO(LOG_FEATURE_CMD,
			"BUTTON index=%i name=%s pin=%i level=%i presses=%u releases=%u clicks=%u ignored=%u long=%u",
			i, XiaomiCompact4_ButtonName(pins[i]), pins[i], HAL_PIN_ReadDigitalInput(pins[i]),
			(unsigned int)g_buttonPressEdges[i], (unsigned int)g_buttonReleaseEdges[i],
			(unsigned int)g_buttonClicks[i], (unsigned int)g_buttonIgnored[i],
			(unsigned int)g_buttonLongPresses[i]);
	}
	return CMD_RES_OK;
}

#if 0
/*
 * Forensic NVS and tachometer report retained for future fault builds. It is
 * deliberately absent from release command registration and command discovery.
 */
static commandResult_t CMD_XiaomiCompact4_ForensicStats(const void *context, const char *cmd, const char *args, int cmdFlags) {
	uint32_t tachAge = g_tachLastActiveSecond > 0
		? g_diagEverySeconds - g_tachLastActiveSecond : g_diagEverySeconds;
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"FORENSIC nvs_attempts=%u nvs_completed=%u nvs_unfinished=%u nvs_slow=%u nvs_last_ms=%u nvs_max_ms=%u nvs_last_index=%i nvs_last_value=%i filter_dirty=%i filter_checkpoint_in=%i",
		(unsigned int)g_nvsSaveAttempts, (unsigned int)g_nvsSaveCompletions,
		(unsigned int)(g_nvsSaveAttempts - g_nvsSaveCompletions),
		(unsigned int)g_nvsSlowSaves, (unsigned int)g_nvsLastSaveMs,
		(unsigned int)g_nvsMaxSaveMs, g_nvsLastIndex, g_nvsLastValue,
		g_filterUsageDirty, g_filterCheckpointCountdown);
	ADDLOG_INFO(LOG_FEATURE_CMD,
		"TACH attached=%i last_edges=%u total_edges=%u max_edges_per_sec=%u storm_threshold=%i storm_windows=%u last_active_age=%u rpm=%i",
		g_tachIsrAttached, (unsigned int)g_tachLastWindowEdges,
		(unsigned int)g_tachTotalEdges, (unsigned int)g_tachMaxWindowEdges,
		XIAOMI_C4_TACH_STORM_EDGES_PER_SECOND, (unsigned int)g_tachStormWindows,
		(unsigned int)tachAge, g_lastMotorRpm);
	return CMD_RES_OK;
}
#endif
#endif

void XiaomiCompact4_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	char tmpA[8];
	if (bPreState) {
		if (http_getArg(request->url, "x4resetfilter", tmpA, sizeof(tmpA))) {
			XiaomiCompact4_ResetFilter();
		}
		return;
	}
	poststr(request, "<table><tr><td><form action=\"index\">");
	poststr(request, "<input type=\"hidden\" name=\"x4resetfilter\" value=\"1\">");
	poststr(request, "<input type=\"submit\" value=\"Reset filter\"/></form></td></tr></table>");
}

static void XiaomiCompact4_SetupChannels(void) {
	CHANNEL_SetLabel(XIAOMI_C4_CH_POWER, "Power", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_MODE, "Mode", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_BRIGHTNESS, "Brightness", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_CHILD_LOCK, "Child lock", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_PM25, "PM2.5 (ug/m3)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_MOTOR_RPM, "Motor speed (rpm)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FILTER_USAGE, "Filter usage (days)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FILTER_HEALTH, "Filter health (%)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_REPLACE_FILTER, "Filter replacement due", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FAV_SPEED, "Favorite speed (%)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_NIGHT_SPEED, "Night speed (%)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_P_FACTOR, "Auto PM multiplier (x0.01)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_FILTER_LIFESPAN, "Filter lifespan (days)", 1);
	CHANNEL_SetLabel(XIAOMI_C4_CH_BUZZER, "Buzzer", 1);
	CHANNEL_SetType(XIAOMI_C4_CH_POWER, ChType_Toggle);
	CHANNEL_SetType(XIAOMI_C4_CH_MODE, ChType_Enum);
	CHANNEL_SetType(XIAOMI_C4_CH_BRIGHTNESS, ChType_Enum);
	CHANNEL_SetType(XIAOMI_C4_CH_CHILD_LOCK, ChType_Toggle);
	CHANNEL_SetType(XIAOMI_C4_CH_PM25, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_MOTOR_RPM, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_FILTER_USAGE, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_FILTER_HEALTH, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_REPLACE_FILTER, ChType_ReadOnlyEnum);
	CHANNEL_SetType(XIAOMI_C4_CH_FAV_SPEED, ChType_Percent);
	CHANNEL_SetType(XIAOMI_C4_CH_NIGHT_SPEED, ChType_Percent);
	CHANNEL_SetType(XIAOMI_C4_CH_P_FACTOR, ChType_Dimmer1000);
	CHANNEL_SetType(XIAOMI_C4_CH_FILTER_LIFESPAN, ChType_ReadOnly);
	CHANNEL_SetType(XIAOMI_C4_CH_BUZZER, ChType_Toggle);
	CMD_ExecuteCommand("SetChannelEnum 1 0:FAV 1:NIGHT 2:AUTO", 0);
	CMD_ExecuteCommand("SetChannelEnum 2 0:FULL 1:MID 2:ZERO", 0);
	CMD_ExecuteCommand("SetChannelEnum 8 0:No 1:Yes", 0);
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
	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_MOTOR_RPM, "Motor speed", "rpm", NULL, "mdi:fan");
	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_FILTER_USAGE, "Filter usage", "d", "duration", "mdi:air-filter");
	XiaomiCompact4_HassPublishSensor(topic, XIAOMI_C4_CH_FILTER_HEALTH, "Filter health", "%", NULL, "mdi:air-filter");

	XiaomiCompact4_HassClearOldSensor(topic, XIAOMI_C4_CH_REPLACE_FILTER);
	dev_info = hass_init_binary_sensor_device_info(XIAOMI_C4_CH_REPLACE_FILTER, true);
	if (dev_info) {
		XiaomiCompact4_HassSetName(dev_info, "Filter replacement due");
		cJSON_AddStringToObject(dev_info->root, "dev_cla", "problem");
		cJSON_AddStringToObject(dev_info->root, "icon", "mdi:air-filter");
		XiaomiCompact4_HassPublish(topic, dev_info);
	}

	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_FAV_SPEED, "Favorite motor speed", "XiaomiCompact4_SetFavSpeed", 0, 100, 1, NULL, NULL, NULL);
	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_NIGHT_SPEED, "Night motor speed", "XiaomiCompact4_SetNightSpeed", 0, 100, 1, NULL, NULL, NULL);
	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_P_FACTOR, "Auto PM multiplier", "XiaomiCompact4_SetPFactor", 0.01f, 10.0f, 0.01f, NULL, NULL, "{{ ((value | float(0)) / 100) | round(2) }}");
	XiaomiCompact4_HassPublishNumber(topic, XIAOMI_C4_CH_FILTER_LIFESPAN, "Filter lifespan", "XiaomiCompact4_SetFilterLifespan", XIAOMI_C4_FILTER_LIFESPAN_MIN_DAYS, XIAOMI_C4_FILTER_LIFESPAN_MAX_DAYS, 1, "d", "duration", NULL);

	dev_info = hass_init_button_device_info("reset", "XiaomiCompact4_ResetFilter", "1", HASS_CATEGORY_CONFIG);
	if (dev_info) {
		XiaomiCompact4_HassSetName(dev_info, "Reset filter");
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

bool XiaomiCompact4_ShouldUseCompactIndexLabels(int ch) {
	return g_initialized && ch >= XIAOMI_C4_CH_POWER && ch <= XIAOMI_C4_CH_BUZZER;
}

void XiaomiCompact4_Init(void) {
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_DRIVER_INIT_ENTRY,
		// -1, -1, ESP_OK, 0);
	//cmddetail:{"name":"XiaomiCompact4_SetPower","args":"[0/1]",
	//cmddetail:"descr":"Turns the Xiaomi Compact 4 purifier on or off",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetPower","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetPower 1"}
	CMD_RegisterCommand("XiaomiCompact4_SetPower", CMD_XiaomiCompact4_SetPower, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetMode","args":"[FAV/NIGHT/AUTO or 0-2]",
	//cmddetail:"descr":"Selects the favorite, night, or automatic purifier mode",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetMode","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetMode AUTO"}
	CMD_RegisterCommand("XiaomiCompact4_SetMode", CMD_XiaomiCompact4_SetMode, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetBrightness","args":"[FULL/MID/ZERO or 0-2]",
	//cmddetail:"descr":"Sets the Xiaomi Compact 4 display brightness",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetBrightness","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetBrightness MID"}
	CMD_RegisterCommand("XiaomiCompact4_SetBrightness", CMD_XiaomiCompact4_SetBrightness, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetChildLock","args":"[0/1]",
	//cmddetail:"descr":"Disables or enables the Xiaomi Compact 4 child lock",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetChildLock","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetChildLock 1"}
	CMD_RegisterCommand("XiaomiCompact4_SetChildLock", CMD_XiaomiCompact4_SetChildLock, NULL);
	//cmddetail:{"name":"XiaomiCompact4_ResetFilter","args":"",
	//cmddetail:"descr":"Resets the Xiaomi Compact 4 filter usage counter",
	//cmddetail:"fn":"CMD_XiaomiCompact4_ResetFilter","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_ResetFilter"}
	CMD_RegisterCommand("XiaomiCompact4_ResetFilter", CMD_XiaomiCompact4_ResetFilter, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetFavSpeed","args":"[0-100]",
	//cmddetail:"descr":"Sets the favorite-mode motor speed percentage",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetFavSpeed","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetFavSpeed 70"}
	CMD_RegisterCommand("XiaomiCompact4_SetFavSpeed", CMD_XiaomiCompact4_SetFavSpeed, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetNightSpeed","args":"[0-100]",
	//cmddetail:"descr":"Sets the night-mode motor speed percentage",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetNightSpeed","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetNightSpeed 30"}
	CMD_RegisterCommand("XiaomiCompact4_SetNightSpeed", CMD_XiaomiCompact4_SetNightSpeed, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetPFactor","args":"[0.01-10.00]",
	//cmddetail:"descr":"Sets the PM2.5 multiplier used by automatic mode",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetPFactor","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetPFactor 1.51"}
	CMD_RegisterCommand("XiaomiCompact4_SetPFactor", CMD_XiaomiCompact4_SetPFactor, NULL);
	//cmddetail:{"name":"XiaomiCompact4_SetFilterLifespan","args":"[1-365]",
	//cmddetail:"descr":"Sets the expected filter lifespan in days",
	//cmddetail:"fn":"CMD_XiaomiCompact4_SetFilterLifespan","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	//cmddetail:"examples":"XiaomiCompact4_SetFilterLifespan 365"}
	CMD_RegisterCommand("XiaomiCompact4_SetFilterLifespan", CMD_XiaomiCompact4_SetFilterLifespan, NULL);
#if 0
	// Diagnostic metadata reference: {"name":"XiaomiCompact4_PM25Stats","args":"",
	// "descr":"Shows PM2.5 parser, UART, freshness, failsafe, and motor diagnostics",
	// "fn":"CMD_XiaomiCompact4_PM25Stats","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	// "examples":"XiaomiCompact4_PM25Stats"}
	CMD_RegisterCommand("XiaomiCompact4_PM25Stats", CMD_XiaomiCompact4_PM25Stats, NULL);
	// Diagnostic metadata reference: {"name":"XiaomiCompact4_I2CStats","args":"",
	// "descr":"Shows Xiaomi Compact 4 I2C initialization, write, and recovery diagnostics",
	// "fn":"CMD_XiaomiCompact4_I2CStats","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	// "examples":"XiaomiCompact4_I2CStats"}
	CMD_RegisterCommand("XiaomiCompact4_I2CStats", CMD_XiaomiCompact4_I2CStats, NULL);
	// Diagnostic metadata reference: {"name":"XiaomiCompact4_ButtonStats","args":"",
	// "descr":"Shows Xiaomi Compact 4 button timing and edge diagnostics",
	// "fn":"CMD_XiaomiCompact4_ButtonStats","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	// "examples":"XiaomiCompact4_ButtonStats"}
	CMD_RegisterCommand("XiaomiCompact4_ButtonStats", CMD_XiaomiCompact4_ButtonStats, NULL);
	// Forensic metadata reference: {"name":"XiaomiCompact4_ForensicStats","args":"",
	// "descr":"Shows Xiaomi Compact 4 NVS and tachometer forensic counters",
	// "fn":"CMD_XiaomiCompact4_ForensicStats","file":"driver/drv_xiaomi_compact4.c","requires":"XiaomiCompact4",
	// "examples":"XiaomiCompact4_ForensicStats"}
	CMD_RegisterCommand("XiaomiCompact4_ForensicStats", CMD_XiaomiCompact4_ForensicStats, NULL);
#endif
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_COMMANDS_REGISTERED,
		// -1, -1, ESP_OK, 0);

	// The appliance always starts off; persisted Power must never start the motor after a reboot.
	g_power = 0;
	g_mode = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_MODE, XIAOMI_C4_MODE_AUTO, XIAOMI_C4_MODE_FAV, XIAOMI_C4_MODE_AUTO);
	g_brightness = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_BRIGHTNESS, XIAOMI_C4_BRIGHTNESS_FULL, XIAOMI_C4_BRIGHTNESS_FULL, XIAOMI_C4_BRIGHTNESS_ZERO);
	g_childLock = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_CHILD_LOCK, 0, 0, 1);
	g_favSpeed = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_FAV_SPEED, 30, 0, 100);
	g_nightSpeed = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_NIGHT_SPEED, 10, 0, 100);
	g_pFactorX100 = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_P_FACTOR_X100, 100, 1, 1000);
	g_filterLifespanDays = XiaomiCompact4_LoadFilterLifespan();
	g_filterUsageSeconds = (uint32_t)XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_FILTER_USAGE, 0, 0, 0x7FFFFFFF);
	g_buzzer = XiaomiCompact4_LoadInt(XIAOMI_C4_VAR_BUZZER, 1, 0, 1);
	g_wifiConnectedLast = -1;
	g_wifiLedBlinkPhase = 0;
#if 0
	g_wifiConnectionChanges = 0;
	g_wifiBlinkTransitions = 0;
#endif
	g_pm25PollCountdown = 1;
	g_filterCountdown = XIAOMI_C4_FILTER_USAGE_TICK_SECONDS;
	g_filterCheckpointCountdown = XIAOMI_C4_FILTER_CHECKPOINT_SECONDS;
	g_filterUsageDirty = 0;
	g_pm25ConsecutiveInvalidWindows = 0;
	g_pm25ConsecutiveRecoveryWindows = 0;
#if 0
	g_pm25FrameSeenSinceQuery = 0;
#endif
	g_pm25ValidFrameSeenSinceQuery = 0;
#if 0
	g_pm25RxBytesSinceQuery = 0;
#endif
	g_failsafeActive = 0;
	g_failsafeReason = 0;
	g_quickTickStaleSeconds = 0;
	g_motorPwmReady = 0;
	g_motorEnabled = -1;
	g_motorAppliedFrequency = -1;
	g_motorPwmLastError = ESP_OK;
	g_motorPwmErrors = 0;
#if 0
	g_motorFrequencyChanges = 0;
#endif
	g_i2cRecoveryPending = 0;
	g_i2cRecoveryRequestedMs = 0;
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_CONFIG_LOADED,
		// -1, -1, ESP_OK, 0);

	HAL_PIN_Setup_Output(XIAOMI_C4_PIN_MOTOR_EN);
	XiaomiCompact4_SetMotorEnabled(0);
	HAL_PIN_Setup_Input_Pullup(XIAOMI_C4_PIN_BUTTON_POWER);
	HAL_PIN_Setup_Input_Pullup(XIAOMI_C4_PIN_BUTTON_LIGHT);
	HAL_PIN_Setup_Input_Pullup(XIAOMI_C4_PIN_BUTTON_MODE);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_GPIO_CONFIG_DONE,
		// -1, -1, ESP_OK, 0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_SHUTDOWN_BEGIN,
		// XIAOMI_C4_PIN_TACH, -1, ESP_OK, 0);
	XiaomiCompact4_TachShutdown();
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_TACH_SHUTDOWN_DONE,
		// XIAOMI_C4_PIN_TACH, -1, ESP_OK, 0);
	g_tachPulses = 0;
	XiaomiCompact4_TachInit();

	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_BEGIN,
		// XIAOMI_C4_PIN_BUZZER, 1000, ESP_OK, 0);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_BUZZER, 1000);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_DONE,
		// XIAOMI_C4_PIN_BUZZER, 1000, ESP_OK, 0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_BEGIN,
		// XIAOMI_C4_PIN_LED_RED, 1000, ESP_OK, 0);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_LED_RED, 1000);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_DONE,
		// XIAOMI_C4_PIN_LED_RED, 1000, ESP_OK, 0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_BEGIN,
		// XIAOMI_C4_PIN_LED_ORANGE, 1000, ESP_OK, 0);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_LED_ORANGE, 1000);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_DONE,
		// XIAOMI_C4_PIN_LED_ORANGE, 1000, ESP_OK, 0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_BEGIN,
		// XIAOMI_C4_PIN_LED_GREEN, 1000, ESP_OK, 0);
	HAL_PIN_PWM_Start(XIAOMI_C4_PIN_LED_GREEN, 1000);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_AUX_PWM_DONE,
		// XIAOMI_C4_PIN_LED_GREEN, 1000, ESP_OK, 0);
	HAL_PIN_PWM_Update(XIAOMI_C4_PIN_BUZZER, 0);
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_BEGIN,
		// XIAOMI_C4_PIN_MOTOR_PWM, XIAOMI_C4_MOTOR_MIN_FREQUENCY, ESP_OK, 0);
	if (!XiaomiCompact4_MotorPWMInit()) {
		XiaomiCompact4_ForceMotorFailsafe(XIAOMI_C4_FAILSAFE_REASON_MOTOR_PWM);
	}
	// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_STAGE_MOTOR_PWM_DONE,
		// XIAOMI_C4_PIN_MOTOR_PWM, XIAOMI_C4_MOTOR_MIN_FREQUENCY,
		// g_motorPwmReady ? ESP_OK : g_motorPwmLastError, 0);

	XiaomiCompact4_I2CInit();
	if (!XiaomiCompact4_I2CReady()) {
		g_i2cRecoveryPending = 1;
		g_i2cRecoveryRequestedMs = XiaomiCompact4_UptimeMs();
		// XiaomiCompact4_I2CBreadcrumbMark(XIAOMI_C4_I2C_STAGE_RECOVERY_QUEUED,
			// -1, -1, g_i2cLastError, 1);
		ADDLOG_WARN(LOG_FEATURE_DRV,
			"XiaomiCompact4 I2C startup incomplete; deferred recovery queued err=%i bus=%p",
			g_i2cLastError, g_i2cBus);
	}

	XiaomiCompact4_UARTInit();

	XiaomiCompact4_SetupChannels();
	g_initialized = 1;
	XiaomiCompact4_ServiceWiFiIndicator();
	XiaomiCompact4_ApplyState(0);
	ADDLOG_INFO(LOG_FEATURE_DRV, "XiaomiCompact4 initialized for ESP32-WROOM-32D hardware");
}

#endif

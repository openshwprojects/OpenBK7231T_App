
#include "../obk_config.h"

#if ENABLE_DRIVER_ARISTON

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../hal/hal_flashVars.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "drv_deviceclock.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"
#include "drv_ariston.h"

// Externs for MQTT
extern int MQTT_PublishMain_StringString(const char* sChannel, const char* valueStr, int flags);
extern int MQTT_PublishMain_StringFloat(const char* sChannel, float f, int idx, int flags);
extern const char *CFG_GetDeviceName(void);
extern const char *CFG_GetMQTTClientId(void);

#ifndef OBK_PUBLISH_FLAG_RAW_TOPIC_NAME
#define OBK_PUBLISH_FLAG_RAW_TOPIC_NAME 8
#endif
#ifndef OBK_PUBLISH_FLAG_RETAIN
#define OBK_PUBLISH_FLAG_RETAIN 2
#endif

// -------------------------------------------------------
// Protocol constants
//
// Our frames (ESP / WiFi module) use header:  C3 41
// Boiler frames use header:                   3C C3 14
//
// We are the MASTER. We initiate all exchanges.
// The boiler always responds.
// -------------------------------------------------------

#define ARISTON_HDR0        0xC3
#define ARISTON_HDR1        0x41  // Our address byte

#define ARISTON_BOILER_SOF  0x3C  // Boiler frame start-of-frame
#define ARISTON_BOILER_HDR0 0xC3
#define ARISTON_BOILER_HDR1 0x14  // Boiler address byte

#define ARISTON_CMD_TEMP    0x23
#define ARISTON_CMD_STARTUP 0x24
#define ARISTON_CMD_PARAM   0x25
#define ARISTON_CMD_WIFI    0x33  // WiFi state report (we send periodically)
#define ARISTON_CMD_INIT    0x52

// WiFi state values seen in logs (register CC 4B)
#define WIFI_STATE_STA_CONNECTED  0x08
#define WIFI_STATE_AP_MODE        0x04
#define WIFI_STATE_OFF            0x00

// UART port
static int g_ariston_uart = 1;

static ariston_ctx_t g_ctx;

static ariston_seq_t g_seq = SEQ_IDLE;
static int g_seq_timer = 0;      // counts up every second
static int g_wifi_report_timer = 0;
static int g_poll_toggle = 0;
static int g_runtime_cycle_counter = 0;
static uint32_t g_next_poll_tick = 0; // sub-second steady-state poll scheduler
static int g_param_poll_index = 0;
static int g_startup_wifi_stage = 0;
// Normalized control state. These are updated from both CMD25 reads and CMD33 pushes,
// then reused by MQTT, HA, and the local HTTP page.
static int g_power_state = -1;        // 0=off, 1=on, 2=standby, 3=boost
static int g_app_power_state = -1;    // 05 21 (UI/app power), informational
static int g_mode_state = -1;         // 0=eco, 1=manual (current mapping)
static int g_anti_legionella = -1;    // 1=ON, 0=OFF
static int g_boost_state = -1;         // 1=boost active, 0=not active
static uint8_t g_last_param_reg_hi = 0;
static uint8_t g_last_param_reg_lo = 0;
static float g_energy_session_wh = 0.0f;
static float g_energy_total_wh = 0.0f;
static float g_energy_today_wh = 0.0f;
static float g_energy_unsaved_wh = 0.0f;
static int g_energy_save_seconds = 0;
static uint32_t g_energy_save_counter = 0;
static int g_ariston_energy_loaded = 0;
static uint32_t g_energy_last_tick = 0;
static int g_energy_day_key = -1;
static int g_ariston_discovery_sent = 0;
static int g_ariston_mqtt_was_ready = 0;
static int g_ntp_sync_prev_synced = 0;
static int g_ntp_sync_last_day_key = -1;
static int g_ntp_sync_last_minute_of_day = -1;
static int g_on_time_baseline_valid = 0;
static int g_on_time_last_minute_key = 0;
static int g_on_time_raw_companion = -1;
static uint8_t g_ariston_identity_mac[6] = { 0 };
static char g_ariston_identity_sn[13] = { 0 };
static int g_ariston_identity_valid = 0;
static int g_ariston_identity_warned = 0;

#define ARISTON_CLOCK_STALE_MS 120000U
#define ARISTON_ON_TIME_MAX_DELTA_MINUTES 180
#define ARISTON_ENERGY_SAVE_DELTA_WH 50.0f
#define ARISTON_ENERGY_SAVE_INTERVAL_SECONDS 1800

#define ARISTON_PWR_OFF      0
#define ARISTON_PWR_ON       1
#define ARISTON_PWR_STANDBY  2
#define ARISTON_PWR_BOOST    3

typedef enum {
    ARISTON_CLOCK_SRC_UNKNOWN = 0,
    ARISTON_CLOCK_SRC_BOILER  = 1,
    ARISTON_CLOCK_SRC_NTP     = 2,
    ARISTON_CLOCK_SRC_UPTIME  = 3
} ariston_clock_source_t;

static ariston_clock_source_t g_energy_day_source = ARISTON_CLOCK_SRC_UNKNOWN;

typedef struct {
    uint8_t hi;
    uint8_t lo;
} ariston_reg_t;

typedef enum {
    ARISTON_ID_OK = 0,
    ARISTON_ID_ERR_MAC_EMPTY,
    ARISTON_ID_ERR_MAC_LENGTH,
    ARISTON_ID_ERR_MAC_NON_HEX,
    ARISTON_ID_ERR_SN_EMPTY,
    ARISTON_ID_ERR_SN_LENGTH,
    ARISTON_ID_ERR_SN_NON_PRINTABLE
} ariston_identity_result_t;

// Startup CMD25 sweep captured from original module behavior.
static const ariston_reg_t s_startup_param_regs[] = {
    { 0x05, 0x21 }, { 0xDD, 0x27 }, { 0x06, 0x21 }, { 0x08, 0x21 }, { 0x09, 0x21 },
    { 0x0A, 0x21 }, { 0x11, 0x23 }, { 0x44, 0x24 }, { 0x45, 0x24 }, { 0x46, 0x24 },
    { 0x47, 0x24 }, { 0x48, 0x24 }, { 0x49, 0x24 }, { 0x03, 0x8F }, { 0x04, 0x8F },
    { 0x05, 0x8F }, { 0xD8, 0x89 }, { 0xD8, 0x8F }, { 0xDD, 0x8F }, { 0x72, 0x8A },
    { 0xDF, 0x89 }, { 0x5C, 0x2D }, { 0x5E, 0x2D }, { 0x60, 0x2D }, { 0x79, 0x2D },
    { 0x7A, 0x2D }, { 0x7B, 0x2D }, { 0x7D, 0x2D }, { 0xD6, 0x2A }, { 0xD7, 0x2A },
    { 0x14, 0x21 }, { 0x18, 0x21 }, { 0x1E, 0x21 }, { 0xDF, 0x27 }, { 0xCC, 0x4B }
};

// Forward declarations
static commandResult_t Cmd_Ariston_SetTemp(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_Power(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_Boost(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_Standby(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_Mode(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_AntiLegionella(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_EnergyReset(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_OnTimeReset(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_Uart(const void *context, const char *cmd, const char *args, int cmdFlags);
static commandResult_t Cmd_Ariston_Discovery(const void *context, const char *cmd, const char *args, int cmdFlags);
static void Ariston_PublishSensors(void);

static void Ariston_ReinitUart(void) {
#if PLATFORM_ESPIDF
    if (!CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART)) {
        CFG_SetFlag(OBK_FLAG_USE_SECONDARY_UART, true);
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
            "Ariston: forcing secondary UART (setFlag 26=1) for ESP-IDF transport");
    }
#endif
    UART_InitUARTEx(g_ariston_uart, 9600, 0, 0);
    UART_InitReceiveRingBufferEx(g_ariston_uart, 256);
}

// -------------------------------------------------------
// Low-level TX: send a frame FROM the ESP (C3 41 header)
// Frame: C3 41 [cmd] [len] [payload...] [checksum]
// Checksum = sum of all bytes starting from C3
// -------------------------------------------------------
static void Ariston_SendFrame(uint8_t cmd, const uint8_t *payload, uint8_t len) {
    uint8_t chk = ARISTON_HDR0 + ARISTON_HDR1 + cmd + len;

    UART_SendByteEx(g_ariston_uart, ARISTON_HDR0);
    UART_SendByteEx(g_ariston_uart, ARISTON_HDR1);
    UART_SendByteEx(g_ariston_uart, cmd);
    UART_SendByteEx(g_ariston_uart, len);

    for (int i = 0; i < len; i++) {
        UART_SendByteEx(g_ariston_uart, payload[i]);
        chk += payload[i];
    }

    UART_SendByteEx(g_ariston_uart, chk);
    addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV, "Ariston TX: CMD=%02X LEN=%d", cmd, len);
}

// -------------------------------------------------------
// CMD 0x52  — identity announcement
// We replicate what the original ESP sends at startup.
// From logs (plug in log 1):
//   C3 41 52 09  03 0F 01 <MAC[6]>              (MAC)
//   C3 41 52 0F  03 10 01 <SN[12 ASCII]>        (serial)
//   C3 41 52 12  03 29 00 0E 73 <X> <SN[12]>    (combined/session)
// -------------------------------------------------------
static void Ariston_SendInit(void) {
    uint8_t serial_payload[15];
    uint8_t combined_payload[18];

    if (!g_ariston_identity_valid) {
        if (!g_ariston_identity_warned) {
            addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
                "Ariston: identity not set, init sequence blocked. Use startDriver Ariston <MAC> <SN>");
            g_ariston_identity_warned = 1;
        }
        return;
    }

    // Frame 1: MAC announcement
    const uint8_t mac_payload[] = {
        0x03, 0x0F, 0x01,
        g_ariston_identity_mac[0], g_ariston_identity_mac[1], g_ariston_identity_mac[2],
        g_ariston_identity_mac[3], g_ariston_identity_mac[4], g_ariston_identity_mac[5]
    };
    Ariston_SendFrame(ARISTON_CMD_INIT, mac_payload, sizeof(mac_payload));
    rtos_delay_milliseconds(20);

    // Frame 2: Serial number in ASCII (12 chars).
    serial_payload[0] = 0x03;
    serial_payload[1] = 0x10;
    serial_payload[2] = 0x01;
    memcpy(serial_payload + 3, g_ariston_identity_sn, 12);
    Ariston_SendFrame(ARISTON_CMD_INIT, serial_payload, sizeof(serial_payload));
    rtos_delay_milliseconds(20);

    // Frame 3: Combined/session payload required by original startup handshake.
    // Layout mirrors captures: 03 29 00 0E 73 <xx> <SN[12]>
    combined_payload[0] = 0x03;
    combined_payload[1] = 0x29;
    combined_payload[2] = 0x00;
    combined_payload[3] = 0x0E;
    combined_payload[4] = 0x73;
    combined_payload[5] = g_ariston_identity_mac[5];
    memcpy(combined_payload + 6, g_ariston_identity_sn, 12);
    Ariston_SendFrame(ARISTON_CMD_INIT, combined_payload, sizeof(combined_payload));
    rtos_delay_milliseconds(20);

    // Original boot sequence sends 02 2C 00 retries before 03 21 00.
    const uint8_t init_retry[] = { 0x02, 0x2C, 0x00 };
    for (int i = 0; i < 3; i++) {
        Ariston_SendFrame(ARISTON_CMD_INIT, init_retry, sizeof(init_retry));
        rtos_delay_milliseconds(200);
    }

    // Final first-phase init request (03 21 00).
    const uint8_t init_req[] = { 0x03, 0x21, 0x00 };
    Ariston_SendFrame(ARISTON_CMD_INIT, init_req, sizeof(init_req));

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Sent CMD 52 init sequence 1");
}

static void Ariston_SendInit2(void) {
    // Second init request (02 0B 00) -> Boiler ACKs this with 1D FE 00 13
    const uint8_t init2_req[] = { 0x02, 0x0B, 0x00 };
    Ariston_SendFrame(ARISTON_CMD_INIT, init2_req, sizeof(init2_req));
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Sent CMD 52 init sequence 2");
}

// -------------------------------------------------------
// CMD 0x24 — startup request
// From logs: C3 41 24 02 90 01 BB
// -------------------------------------------------------
static void Ariston_SendStartup(void) {
    const uint8_t payload[] = { 0x90, 0x01 };
    Ariston_SendFrame(ARISTON_CMD_STARTUP, payload, sizeof(payload));
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Sent CMD 24 startup request");
}

// -------------------------------------------------------
// CMD 0x25 — parameter poll
// We send a 2-byte register address. Boiler replies with value.
// Call repeatedly with different register IDs.
// From power-up logs: ESP polls many registers sequentially.
// -------------------------------------------------------
static void Ariston_SendParamQuery(uint8_t reg_hi, uint8_t reg_lo) {
    const uint8_t payload[] = { reg_hi, reg_lo };
    g_last_param_reg_hi = reg_hi;
    g_last_param_reg_lo = reg_lo;
    Ariston_SendFrame(ARISTON_CMD_PARAM, payload, sizeof(payload));
}

// -------------------------------------------------------
// CMD 0x23 — temperature poll + setpoint injection
// We send a list of register IDs we want the boiler to fill.
// The boiler replies with values for those registers.
// We also embed TEMP_SET in our query so the boiler knows our target.
//
// From logs power up seq line 86:
//   C3 41 23 0C  4A D8  4B D8  5E D9  5F D9  6E 9E  71 9E  (TEMP REQUEST SET)
// Boiler replies (line 87):
//   3C C3 14 23 0C  8F 00 00 00  5C 28 00 00  12 01  B2 02
//   Decoded: TEMP_CUR=27.4, TEMP_SET=69°C (0x02B2=690, /10=69)
//
// Simpler polling frame (line 80, TEMP REQUEST SENSORS):
//   C3 41 23 0C  60 10  68 13  69 13  6A 13  6B 13  43 51
// Boiler replies with actual water temps.
// -------------------------------------------------------
static void Ariston_SendTempPoll(void) {
    // Poll water temperature sensors
    const uint8_t temp_req[] = {
        0x60, 0x10,   // TEMP_AVG register
        0x68, 0x13,
        0x69, 0x13,
        0x6A, 0x13,
        0x6B, 0x13,
        0x43, 0x51
    };
    g_ctx.last_temp_query = ARISTON_QUERY_WATER_TEMP;
    Ariston_SendFrame(ARISTON_CMD_TEMP, temp_req, sizeof(temp_req));
}

static void Ariston_SendSetpointPoll(void) {
    // Poll current setpoint. Boiler replies with TEMP_CUR and TEMP_SET.
    // This is also where we can read back what the boiler is using.
    const uint8_t set_req[] = {
        0x4A, 0xD8,
        0x4B, 0xD8,
        0x5E, 0xD9,
        0x5F, 0xD9,
        0x6E, 0x9E,
        0x71, 0x9E
    };
    g_ctx.last_temp_query = ARISTON_QUERY_SETPOINT;
    Ariston_SendFrame(ARISTON_CMD_TEMP, set_req, sizeof(set_req));
}

static void Ariston_SendTelemetryPoll(void) {
    // Poll Time to Temp and Showers (returns a 7-byte payload where [2]=showers, [3]=time_to_temp)
    const uint8_t req[] = {
        0xD3, 0x4B,
        0xC4, 0x4B,
        0xD9, 0x46,
        0x5E, 0x47,
        0xCB, 0x4B,
        0xCC, 0x4B
    };
    g_ctx.last_temp_query = ARISTON_QUERY_TELEMETRY;
    Ariston_SendFrame(ARISTON_CMD_TEMP, req, sizeof(req));
}

static void Ariston_SendPowerPoll(void) {
    // Poll Heater Power (returns a 10-byte payload where [2-3] = heater power in W)
    // We send exactly what the app sends for the "HEATER 1" group:
    // 4F 9D 50 9D 4C 9E 4D 9E 57 D1
    const uint8_t req[] = {
        0x4F, 0x9D,
        0x50, 0x9D, // Heater Power register
        0x4C, 0x9E,
        0x4D, 0x9E,
        0x57, 0xD1
    };
    g_ctx.last_temp_query = ARISTON_QUERY_POWER;
    Ariston_SendFrame(ARISTON_CMD_TEMP, req, sizeof(req));
}

static void Ariston_SendOnTimePoll(void) {
    // Poll Power On Time
    const uint8_t req[] = {
        0xDA, 0x40,
        0xC7, 0x9C,
        0xC8, 0x9C,
        0x18, 0x45,
        0xD1, 0x3D,
        0xCF, 0x3D
    };
    g_ctx.last_temp_query = ARISTON_QUERY_ON_TIME;
    Ariston_SendFrame(ARISTON_CMD_TEMP, req, sizeof(req));
}

static void Ariston_SendStatus3Poll(void) {
    // STATUS3 block carries weekday/date/hour fields.
    const uint8_t req[] = {
        0xD1, 0x40,
        0xDE, 0x40,
        0xDF, 0x40,
        0xDD, 0x40,
        0xDC, 0x40,
        0xDB, 0x40
    };
    g_ctx.last_temp_query = ARISTON_QUERY_STATUS3;
    Ariston_SendFrame(ARISTON_CMD_TEMP, req, sizeof(req));
}

// -------------------------------------------------------
// CMD 0x33 — WiFi state push
// We send our WiFi state to the boiler periodically (~every 10s).
// From logs: C3 41 33 03  CC 4B 08  (STATE=8, STA connected)
// -------------------------------------------------------
static void Ariston_SendWifiState(uint8_t state) {
    const uint8_t payload[] = { 0xCC, 0x4B, state };
    Ariston_SendFrame(ARISTON_CMD_WIFI, payload, sizeof(payload));
    addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV, "Ariston: Sent WiFi state %02X", state);
}

static const char *Ariston_WeekdayName(int raw) {
    static const char *names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    if (raw < 0 || raw > 6) {
        return "Unknown";
    }
    return names[raw];
}

static int Ariston_GetUptimeDayKey(void) {
    uint32_t now = xTaskGetTickCount();
    uint64_t sec = ((uint64_t)now * (uint64_t)portTICK_PERIOD_MS) / 1000ULL;
    return (int)(sec / 86400ULL);
}

static int Ariston_IsClockFresh(uint32_t now, uint32_t thenTick) {
    uint32_t diffTicks;
    uint32_t diffMs;

    if (thenTick == 0) {
        return 0;
    }
    diffTicks = now - thenTick;
    diffMs = diffTicks * (uint32_t)portTICK_PERIOD_MS;
    return (diffMs <= ARISTON_CLOCK_STALE_MS) ? 1 : 0;
}

static int Ariston_GetBoilerDayKey(uint32_t now, int *outKey) {
    if (!outKey) {
        return 0;
    }
    if (!g_ctx.clock_has_status3) {
        return 0;
    }
    if (!Ariston_IsClockFresh(now, g_ctx.clock_status3_tick)) {
        return 0;
    }
    if (g_ctx.clock_year2 < 0 || g_ctx.clock_year2 > 99) {
        return 0;
    }
    if (g_ctx.clock_month < 1 || g_ctx.clock_month > 12) {
        return 0;
    }
    if (g_ctx.clock_day < 1 || g_ctx.clock_day > 31) {
        return 0;
    }
    *outKey = (2000 + g_ctx.clock_year2) * 10000 + g_ctx.clock_month * 100 + g_ctx.clock_day;
    return 1;
}

static int Ariston_IsLeapYear(int year) {
    if ((year % 4) != 0) {
        return 0;
    }
    if ((year % 100) != 0) {
        return 1;
    }
    return (year % 400) == 0 ? 1 : 0;
}

static int Ariston_DaysInMonth(int year, int month) {
    static const uint8_t daysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && Ariston_IsLeapYear(year)) {
        return 29;
    }
    return daysPerMonth[month - 1];
}

static int Ariston_CalendarToDayIndex(int year, int month, int day, int *outDayIndex) {
    int y;
    int m;
    int days = 0;

    if (!outDayIndex) {
        return 0;
    }
    if (year < 2000 || year > 2099) {
        return 0;
    }
    if (month < 1 || month > 12) {
        return 0;
    }
    if (day < 1 || day > Ariston_DaysInMonth(year, month)) {
        return 0;
    }
    for (y = 2000; y < year; y++) {
        days += Ariston_IsLeapYear(y) ? 366 : 365;
    }
    for (m = 1; m < month; m++) {
        days += Ariston_DaysInMonth(year, m);
    }
    days += (day - 1);
    *outDayIndex = days;
    return 1;
}

static int Ariston_GetBoilerMinuteKey(uint32_t now, int *outMinuteKey) {
    int dayIndex;
    int year;
    if (!outMinuteKey) {
        return 0;
    }
    if (!g_ctx.clock_has_status3 || !g_ctx.clock_has_minute) {
        return 0;
    }
    if (!Ariston_IsClockFresh(now, g_ctx.clock_status3_tick) ||
        !Ariston_IsClockFresh(now, g_ctx.clock_minute_tick)) {
        return 0;
    }
    if (g_ctx.clock_year2 < 0 || g_ctx.clock_year2 > 99 ||
        g_ctx.clock_month < 1 || g_ctx.clock_month > 12 ||
        g_ctx.clock_day < 1 || g_ctx.clock_day > 31 ||
        g_ctx.clock_hour < 0 || g_ctx.clock_hour > 23 ||
        g_ctx.clock_minute < 0 || g_ctx.clock_minute > 59) {
        return 0;
    }
    year = 2000 + g_ctx.clock_year2;
    if (!Ariston_CalendarToDayIndex(year, g_ctx.clock_month, g_ctx.clock_day, &dayIndex)) {
        return 0;
    }
    *outMinuteKey = dayIndex * 1440 + g_ctx.clock_hour * 60 + g_ctx.clock_minute;
    return 1;
}

static int Ariston_UpdateOnTimeRuntime(void) {
    uint32_t now = xTaskGetTickCount();
    int minuteKey;
    int deltaMinutes;

    if (!Ariston_GetBoilerMinuteKey(now, &minuteKey)) {
        g_on_time_baseline_valid = 0;
        return 0;
    }
    if (!g_on_time_baseline_valid) {
        g_on_time_baseline_valid = 1;
        g_on_time_last_minute_key = minuteKey;
        return 0;
    }

    deltaMinutes = minuteKey - g_on_time_last_minute_key;
    g_on_time_last_minute_key = minuteKey;

    if (deltaMinutes <= 0) {
        return 0;
    }
    if (deltaMinutes > ARISTON_ON_TIME_MAX_DELTA_MINUTES) {
        addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
                  "Ariston: ON Time baseline rebase due to clock jump (+%d min)", deltaMinutes);
        return 0;
    }
    if (g_ctx.heater_active == 1) {
        g_ctx.on_time += deltaMinutes;
        return 1;
    }
    return 0;
}

static int Ariston_GetNtpDayKey(int *outKey) {
    int year;
    int month;
    int day;
    if (!outKey) {
        return 0;
    }
    if (!TIME_IsTimeSynced()) {
        return 0;
    }
    year = TIME_GetYear();
    month = TIME_GetMonth();
    day = TIME_GetMDay();
    if (year < 2000 || year > 2099) {
        return 0;
    }
    if (month < 1 || month > 12) {
        return 0;
    }
    if (day < 1 || day > 31) {
        return 0;
    }
    *outKey = year * 10000 + month * 100 + day;
    return 1;
}

static ariston_clock_source_t Ariston_SelectClockSource(uint32_t now, int *outDayKey) {
    int dayKey;
    if (Ariston_GetBoilerDayKey(now, &dayKey)) {
        if (outDayKey) {
            *outDayKey = dayKey;
        }
        return ARISTON_CLOCK_SRC_BOILER;
    }
    if (Ariston_GetNtpDayKey(&dayKey)) {
        if (outDayKey) {
            *outDayKey = dayKey;
        }
        return ARISTON_CLOCK_SRC_NTP;
    }
    if (outDayKey) {
        *outDayKey = Ariston_GetUptimeDayKey();
    }
    return ARISTON_CLOCK_SRC_UPTIME;
}

static int Ariston_IsTrustedSource(ariston_clock_source_t src) {
    return (src == ARISTON_CLOCK_SRC_BOILER || src == ARISTON_CLOCK_SRC_NTP) ? 1 : 0;
}

static const char *Ariston_ClockSourceName(ariston_clock_source_t src) {
    if (src == ARISTON_CLOCK_SRC_BOILER) {
        return "boiler";
    }
    if (src == ARISTON_CLOCK_SRC_NTP) {
        return "ntp";
    }
    if (src == ARISTON_CLOCK_SRC_UPTIME) {
        return "uptime";
    }
    return "unknown";
}

static int Ariston_SendClockTupleFromNtp(const char *reason) {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int wday;
    uint8_t payload[18];

    if (!TIME_IsTimeSynced()) {
        return 0;
    }
    year = TIME_GetYear();
    month = TIME_GetMonth();
    day = TIME_GetMDay();
    hour = TIME_GetHour();
    minute = TIME_GetMinute();
    wday = TIME_GetWeekDay();
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || wday < 0 || wday > 6) {
        return 0;
    }

    payload[0] = 0xDB; payload[1] = 0x40; payload[2] = (uint8_t)hour;
    payload[3] = 0xDA; payload[4] = 0x40; payload[5] = (uint8_t)minute;
    payload[6] = 0xDE; payload[7] = 0x40; payload[8] = (uint8_t)wday;
    payload[9] = 0xDF; payload[10] = 0x40; payload[11] = (uint8_t)(year % 100);
    payload[12] = 0xDD; payload[13] = 0x40; payload[14] = (uint8_t)month;
    payload[15] = 0xDC; payload[16] = 0x40; payload[17] = (uint8_t)day;

    Ariston_SendFrame(ARISTON_CMD_WIFI, payload, sizeof(payload));
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
              "Ariston: Sent CMD33 clock sync (%s) %04d-%02d-%02d %02d:%02d wday=%d",
              reason ? reason : "periodic", year, month, day, hour, minute, wday);
    return 1;
}

static void Ariston_MaybeSendNtpClockSync(void) {
    int dayKey;
    int minuteOfDay;
    int nowSynced;
    int minute;

    nowSynced = TIME_IsTimeSynced() ? 1 : 0;
    if (!nowSynced) {
        g_ntp_sync_prev_synced = 0;
        return;
    }

    if (!Ariston_GetNtpDayKey(&dayKey)) {
        return;
    }
    minute = TIME_GetMinute();
    if (minute < 0 || minute > 59) {
        return;
    }
    minuteOfDay = TIME_GetHour() * 60 + minute;
    if (minuteOfDay < 0 || minuteOfDay > 1439) {
        return;
    }

    if (!g_ntp_sync_prev_synced) {
        if (Ariston_SendClockTupleFromNtp("ntp_synced")) {
            g_ntp_sync_last_day_key = dayKey;
            g_ntp_sync_last_minute_of_day = minuteOfDay;
        }
        g_ntp_sync_prev_synced = 1;
        return;
    }

    if (dayKey != g_ntp_sync_last_day_key) {
        if (Ariston_SendClockTupleFromNtp("day_change")) {
            g_ntp_sync_last_day_key = dayKey;
            g_ntp_sync_last_minute_of_day = minuteOfDay;
        }
        return;
    }

    if (minute == 0 && minuteOfDay != g_ntp_sync_last_minute_of_day) {
        if (Ariston_SendClockTupleFromNtp("hourly")) {
            g_ntp_sync_last_day_key = dayKey;
            g_ntp_sync_last_minute_of_day = minuteOfDay;
        }
    }
}

static void Ariston_UpdateDailyEnergyKey(void) {
    uint32_t now = xTaskGetTickCount();
    int newKey;
    ariston_clock_source_t newSource = Ariston_SelectClockSource(now, &newKey);
    int shouldReset = 0;
    int oldKey = g_energy_day_key;
    ariston_clock_source_t oldSource = g_energy_day_source;

    g_ctx.clock_using_uptime_fallback = (newSource == ARISTON_CLOCK_SRC_UPTIME) ? 1 : 0;

    if (g_energy_day_key < 0) {
        g_energy_day_key = newKey;
        g_energy_day_source = newSource;
        return;
    }

    if (oldSource == ARISTON_CLOCK_SRC_UNKNOWN) {
        g_energy_day_key = newKey;
        g_energy_day_source = newSource;
        return;
    }

    if (newSource == oldSource) {
        if (newKey != oldKey) {
            shouldReset = 1;
        }
    } else if (Ariston_IsTrustedSource(oldSource) && Ariston_IsTrustedSource(newSource)) {
        if (newKey != oldKey) {
            shouldReset = 1;
        }
    } else {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                  "Ariston: clock source switch %s -> %s (key %d -> %d), no reset",
                  Ariston_ClockSourceName(oldSource), Ariston_ClockSourceName(newSource),
                  oldKey, newKey);
    }

    g_energy_day_key = newKey;
    g_energy_day_source = newSource;

    if (shouldReset) {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                  "Ariston: day rollover (%s) key %d -> %d, resetting Energy Today",
                  Ariston_ClockSourceName(newSource), oldKey, newKey);
        g_energy_today_wh = 0.0f;
        g_energy_save_seconds = 9999;
    }
}

static void Ariston_PublishClockAndDayState(int force) {
#if ENABLE_MQTT
    static float last_energy_today_wh = -1.0f;
    uint32_t now;
    ariston_clock_source_t src;

    if (!MQTT_IsReady()) {
        return;
    }

    now = xTaskGetTickCount();
    src = Ariston_SelectClockSource(now, 0);
    g_ctx.clock_using_uptime_fallback = (src == ARISTON_CLOCK_SRC_UPTIME) ? 1 : 0;

    if (force || (g_energy_today_wh - last_energy_today_wh > 0.05f) || (last_energy_today_wh - g_energy_today_wh > 0.05f)) {
        MQTT_PublishMain_StringFloat("ariston/energy_today_kwh", g_energy_today_wh / 1000.0f, 3, OBK_PUBLISH_FLAG_RETAIN);
        last_energy_today_wh = g_energy_today_wh;
    }
#else
    (void)force;
#endif
}

static int Ariston_IsPowerOnLike(void) {
    return (g_power_state == ARISTON_PWR_ON || g_power_state == ARISTON_PWR_BOOST) ? 1 : 0;
}

static const char *Ariston_GetPowerStateString(void) {
    if (g_power_state == ARISTON_PWR_OFF) {
        return "Off";
    }
    if (g_power_state == ARISTON_PWR_ON) {
        return "On";
    }
    if (g_power_state == ARISTON_PWR_STANDBY) {
        return "Standby";
    }
    if (g_power_state == ARISTON_PWR_BOOST) {
        return "Boost";
    }
    return "Unknown";
}

static const char *Ariston_GetModeStateString(void) {
    if (g_mode_state == 0) {
        return "ECO";
    }
    if (g_mode_state == 1) {
        return "Manual";
    }
    return "Unknown";
}

static const char *Ariston_GetBoostStateString(void) {
    if (g_boost_state == 0) {
        return "OFF";
    }
    if (g_boost_state == 1) {
        return "ON";
    }
    return "Unknown";
}

static const char *Ariston_GetAntiLegionellaString(void) {
    if (g_anti_legionella == 0) {
        return "OFF";
    }
    if (g_anti_legionella == 1) {
        return "ON";
    }
    return "Unknown";
}

static const char *Ariston_GetHeaterActivityString(void) {
    if (g_ctx.heater_active == 0) {
        return "OFF";
    }
    if (g_ctx.heater_active == 1) {
        return "ON";
    }
    return "Unknown";
}

static const char *Ariston_GetHAModeString(void) {
    if (g_boost_state == 1 || g_power_state == ARISTON_PWR_BOOST) {
        return "performance";
    }
    if (g_power_state == ARISTON_PWR_OFF || g_power_state == ARISTON_PWR_STANDBY) {
        return "off";
    }
    if (g_mode_state == 0) {
        return "eco";
    }
    if (g_mode_state == 1) {
        return "electric";
    }
    return NULL;
}

// Single normalization point for the boiler control registers used by both
// CMD25 read replies and CMD33 pushed state updates.
static void Ariston_ApplyControlRegisterValue(uint8_t rhi, uint8_t rlo, uint8_t v, int sourceIsCmd33) {
    if (rhi == 0x05 && rlo == 0x21) {
        g_app_power_state = (v == 0x00) ? 0 : 1;
        if (g_app_power_state == 0) {
            g_power_state = ARISTON_PWR_OFF;
            g_boost_state = 0;
            g_ctx.heater_active = 0;
            g_ctx.heater_power = 0;
        }
        return;
    }

    if (rhi == 0x06 && rlo == 0x21) {
        g_mode_state = (v == 0x01) ? 0 : 1;
        return;
    }

    if (rhi == 0x0A && rlo == 0x21) {
        g_anti_legionella = (v == 0x00) ? 0 : 1;
        return;
    }

    if (rhi == 0xDD && rlo == 0x27) {
        if (v == 0x05) {
            g_power_state = ARISTON_PWR_STANDBY;
            g_boost_state = 0;
            g_ctx.heater_active = 0;
            g_ctx.heater_power = 0;
            return;
        }
        if (v == 0x01) {
            if (g_app_power_state == 1) {
                g_power_state = ARISTON_PWR_ON;
                g_boost_state = 0;
            } else if (sourceIsCmd33) {
                addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV,
                    "Ariston: Ignoring DD27=01 while app power is OFF");
            }
            return;
        }
        if (v == 0x09) {
            if (g_app_power_state == 1) {
                g_power_state = ARISTON_PWR_BOOST;
                g_boost_state = 1;
                g_mode_state = 1;
            } else if (sourceIsCmd33) {
                addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV,
                    "Ariston: Ignoring DD27=09 while app power is OFF");
            }
        }
    }
}

static void Ariston_PublishControlStateSnapshot(int force) {
#if ENABLE_MQTT
    const char *power_str;
    const char *mode_str;
    const char *ha_mode;
    static int last_power_state = -999;
    static int last_is_on = -999;
    static int last_app_power_state = -999;
    static int last_mode_state = -999;
    static int last_anti_legionella = -999;
    static int last_boost_state = -999;
    static char last_ha_mode[24] = { 0 };
    if (!MQTT_IsReady()) return;

    power_str = Ariston_GetPowerStateString();
    mode_str = Ariston_GetModeStateString();
    ha_mode = Ariston_GetHAModeString();

    if (force || g_power_state != last_power_state) {
        MQTT_PublishMain_StringString("ariston/power_state", power_str, OBK_PUBLISH_FLAG_RETAIN);
        last_power_state = g_power_state;
    }
    if (g_power_state >= 0) {
        int is_on = Ariston_IsPowerOnLike();
        if (force || is_on != last_is_on) {
            MQTT_PublishMain_StringString("ariston/power", is_on ? "1" : "0", OBK_PUBLISH_FLAG_RETAIN);
            last_is_on = is_on;
        }
    }
    if (g_app_power_state >= 0 && (force || g_app_power_state != last_app_power_state)) {
        MQTT_PublishMain_StringString("ariston/app_power", g_app_power_state ? "1" : "0", OBK_PUBLISH_FLAG_RETAIN);
        last_app_power_state = g_app_power_state;
    }
    if (g_mode_state >= 0 && (force || g_mode_state != last_mode_state)) {
        MQTT_PublishMain_StringString("ariston/mode", mode_str, OBK_PUBLISH_FLAG_RETAIN);
        last_mode_state = g_mode_state;
    }
    if (g_anti_legionella >= 0 && (force || g_anti_legionella != last_anti_legionella)) {
        MQTT_PublishMain_StringString("ariston/anti_legionella", g_anti_legionella ? "1" : "0", OBK_PUBLISH_FLAG_RETAIN);
        last_anti_legionella = g_anti_legionella;
    }
    if (g_boost_state >= 0 && (force || g_boost_state != last_boost_state)) {
        MQTT_PublishMain_StringString("ariston/boost", g_boost_state ? "1" : "0", OBK_PUBLISH_FLAG_RETAIN);
        last_boost_state = g_boost_state;
    }
    if (ha_mode && (force || strcmp(ha_mode, last_ha_mode))) {
        MQTT_PublishMain_StringString("ariston/ha_mode", ha_mode, OBK_PUBLISH_FLAG_RETAIN);
        strncpy(last_ha_mode, ha_mode, sizeof(last_ha_mode) - 1);
        last_ha_mode[sizeof(last_ha_mode) - 1] = '\0';
    }
#endif
}

static void Ariston_PublishControlStates(void) {
    Ariston_PublishControlStateSnapshot(0);
}

static void Ariston_PublishAllStatesNow(void) {
#if ENABLE_MQTT
    if (!MQTT_IsReady()) {
        return;
    }
    Ariston_PublishControlStateSnapshot(1);

    MQTT_PublishMain_StringFloat("ariston/water_temp", g_ctx.water_temp, 1, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/target_temp", g_ctx.target_temp, 1, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/current_temp", g_ctx.current_temp, 1, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/heater_power", (float)g_ctx.heater_power, 0, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/heater_active", (float)g_ctx.heater_active, 0, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/showers", (float)g_ctx.showers, 0, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/time_to_temp", (float)g_ctx.time_to_temp, 0, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/on_time", (float)g_ctx.on_time, 0, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/energy_est_kwh", g_energy_session_wh / 1000.0f, 3, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/energy_total_kwh", g_energy_total_wh / 1000.0f, 3, OBK_PUBLISH_FLAG_RETAIN);
    MQTT_PublishMain_StringFloat("ariston/energy_today_kwh", g_energy_today_wh / 1000.0f, 3, OBK_PUBLISH_FLAG_RETAIN);
    Ariston_PublishClockAndDayState(1);
#endif
}

static void Ariston_SaveEnergyTotalIfNeeded(int force) {
    ARISTON_ENERGY_DATA data;
    if (!g_ariston_energy_loaded) {
        return;
    }
    if (!force &&
        g_energy_unsaved_wh < ARISTON_ENERGY_SAVE_DELTA_WH &&
        g_energy_save_seconds < ARISTON_ENERGY_SAVE_INTERVAL_SECONDS) {
        return;
    }
    data.TotalWh = g_energy_total_wh;
    data.save_counter = ++g_energy_save_counter;
    if (HAL_SetAristonEnergyStatus(&data)) {
        g_energy_unsaved_wh = 0.0f;
        g_energy_save_seconds = 0;
    }
}

static void Ariston_LoadEnergyTotal(void) {
    ARISTON_ENERGY_DATA data;

    g_energy_total_wh = 0.0f;
    g_energy_today_wh = 0.0f;
    g_energy_day_key = -1;
    g_energy_save_counter = 0;
    g_energy_unsaved_wh = 0.0f;
    g_energy_save_seconds = 0;
    memset(&data, 0, sizeof(data));
    if (HAL_GetAristonEnergyStatus(&data)) {
        if (data.TotalWh >= 0.0f) {
            g_energy_total_wh = data.TotalWh;
        }
        g_energy_save_counter = data.save_counter;
    }
    g_ctx.energy_today_wh = g_energy_today_wh;
    g_ariston_energy_loaded = 1;
}

static void Ariston_ApplyHeaterActivity(void) {
    if (g_ctx.heater_active == 0) {
        g_ctx.heater_power = 0;
        return;
    }
    if (g_ctx.heater_active == 1 && g_ctx.heater_power_nominal > 0) {
        g_ctx.heater_power = g_ctx.heater_power_nominal;
        return;
    }
    // Unknown activity or no nominal power yet: keep current reported value.
}

static void Ariston_UpdateEnergyEstimate(void) {
    uint32_t now = xTaskGetTickCount();
    if (g_energy_last_tick == 0) {
        g_energy_last_tick = now;
        return;
    }
    uint32_t dt_ticks = now - g_energy_last_tick;
    g_energy_last_tick = now;
    if (dt_ticks == 0) {
        return;
    }
    Ariston_UpdateDailyEnergyKey();

    // Integrate real elapsed time only while heater reports active power.
    if (g_ctx.heater_power > 0) {
        float dt_hours = ((float)dt_ticks * (float)portTICK_PERIOD_MS) / 3600000.0f;
        float add_wh = (float)g_ctx.heater_power * dt_hours;
        if (add_wh > 0.0f) {
            g_energy_session_wh += add_wh;
            g_energy_total_wh += add_wh;
            g_energy_today_wh += add_wh;
            g_ctx.energy_today_wh = g_energy_today_wh;
            g_energy_unsaved_wh += add_wh;
        }
    }
}

static void Ariston_ResetEnergyTotal(void) {
    g_energy_session_wh = 0.0f;
    g_energy_total_wh = 0.0f;
    g_energy_today_wh = 0.0f;
    g_ctx.energy_today_wh = 0.0f;
    g_energy_unsaved_wh = 0.0f;
    g_energy_save_counter = 0;
    g_energy_day_key = -1;
    g_energy_day_source = ARISTON_CLOCK_SRC_UNKNOWN;
    g_energy_last_tick = xTaskGetTickCount();
    g_energy_save_seconds = 9999;
    Ariston_SaveEnergyTotalIfNeeded(1);
}

static void Ariston_SendStartupWifiStage(int stage) {
    if (stage == 0) {
        // Seen early in original startup.
        const uint8_t p[] = { 0xDD, 0x9A, 0x00, 0xCC, 0x4B, 0x00, 0xCC, 0x4B, 0x05 };
        Ariston_SendFrame(ARISTON_CMD_WIFI, p, sizeof(p));
    } else if (stage == 1) {
        const uint8_t p[] = { 0xCC, 0x4B, 0x06 };
        Ariston_SendFrame(ARISTON_CMD_WIFI, p, sizeof(p));
    } else if (stage == 2) {
        // Send startup clock tuple only when OBK clock is already synced.
        // Avoid sending stale hardcoded date/time.
        if (!Ariston_SendClockTupleFromNtp("startup")) {
            addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV,
                      "Ariston: Startup clock tuple skipped (NTP not synced yet)");
        }
    } else if (stage == 3) {
        // Original sends a duplicated "connecting" payload during startup.
        const uint8_t p[] = { 0xCC, 0x4B, 0x06, 0xCC, 0x4B, 0x06 };
        Ariston_SendFrame(ARISTON_CMD_WIFI, p, sizeof(p));
    } else {
        Ariston_SendWifiState(WIFI_STATE_STA_CONNECTED);
    }
    addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV, "Ariston: Startup WiFi stage %d", stage);
}

// -------------------------------------------------------
// CMD 0x33 — change target temperature
// From logs: the way to SET temperature is to send a CMD 33
// with register 0x79 2D and value (temp_degC * 10) in Little Endian.
// Example: 45°C → 450 = 0x01C2 → bytes: C2 01
// However log analysis for "standby to on" may clarify exact encoding.
// -------------------------------------------------------
static void Ariston_SendSetTemp(void) {
    int raw = (int)(g_ctx.target_temp * 10.0f);
    uint8_t payload[] = {
        0x79, 0x2D,          // register ID (big-endian in frame)
        raw & 0xFF,          // value LSB
        (raw >> 8) & 0xFF    // value MSB
    };
    Ariston_SendFrame(ARISTON_CMD_WIFI, payload, sizeof(payload));
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Set target temp %.1f -> raw %d", g_ctx.target_temp, raw);
}

static void Ariston_SendControlWriteRaw(const uint8_t *payload, uint8_t len) {
    if (payload == 0 || len == 0) {
        return;
    }
    Ariston_SendFrame(ARISTON_CMD_WIFI, payload, len);
}

// Steady-state poll cadence. We keep the hot telemetry on CMD23 and only refresh
// slower control registers occasionally so the wire traffic stays close to captures.
static void Ariston_SendNextPoll(void) {
    if (g_poll_toggle == 0) {
        Ariston_SendTempPoll();
    } else if (g_poll_toggle == 1) {
        Ariston_SendSetpointPoll();
    } else if (g_poll_toggle == 2) {
        Ariston_SendTelemetryPoll();
    } else if (g_poll_toggle == 3) {
        Ariston_SendPowerPoll();
    } else if (g_poll_toggle == 4) {
        Ariston_SendOnTimePoll();
    } else {
        // Keep runtime traffic close to original: mostly CMD23 telemetry.
        // Refresh control/status registers at low duty cycle:
        // 05 21 (app power), 06 21 (mode), 0A 21 (anti-leg), DD 27 (power/boost).
        int phase = g_runtime_cycle_counter & 0x0F;
        if (phase == 0) {
            Ariston_SendParamQuery(0x05, 0x21);
        } else if (phase == 4) {
            Ariston_SendParamQuery(0x06, 0x21);
        } else if (phase == 8) {
            Ariston_SendParamQuery(0x0A, 0x21);
        } else if (phase == 12) {
            Ariston_SendParamQuery(0xDD, 0x27);
        } else {
            Ariston_SendStatus3Poll();
        }
        g_runtime_cycle_counter++;
    }
    g_poll_toggle++;
    if (g_poll_toggle >= 6) {
        g_poll_toggle = 0;
    }
}

static void Ariston_SendPowerState(uint8_t state) {
    // Publish an optimistic UI/HA-facing state immediately after the write.
    // The next boiler CMD25/CMD33 reply is still the source of truth and will
    // reconcile anything that differs.
    // state: 0x01 = ON, 0x00 = OFF (05 21), 0x05 = Standby, 0x09 = Boost
    if (state == 0x00) {
        const uint8_t payload[] = { 0x05, 0x21, 0x00 };
        // Keep OFF sticky locally until boiler confirms.
        g_app_power_state = 0;
        g_power_state = ARISTON_PWR_OFF;
        g_boost_state = 0;
        g_ctx.heater_active = 0;
        g_ctx.heater_power = 0;
        Ariston_SendControlWriteRaw(payload, sizeof(payload));
    } else if (state == 0x01) {
        // Ordered wake + power-on is more robust than bundled write.
        const uint8_t wake_app[] = { 0x05, 0x21, 0x01 };
        const uint8_t power_on[] = { 0xDD, 0x27, 0x01 };
        // Mark local intent so incoming DD27=01 is not rejected as stale.
        g_app_power_state = 1;
        g_power_state = ARISTON_PWR_ON;
        g_boost_state = 0;
        Ariston_SendControlWriteRaw(wake_app, sizeof(wake_app));
        rtos_delay_milliseconds(80);
        Ariston_SendControlWriteRaw(power_on, sizeof(power_on));
    } else if (state == 0x05) {
        const uint8_t payload[] = { 0xDD, 0x27, 0x05 };
        g_power_state = ARISTON_PWR_STANDBY;
        g_boost_state = 0;
        g_ctx.heater_active = 0;
        g_ctx.heater_power = 0;
        Ariston_SendControlWriteRaw(payload, sizeof(payload));
    } else if (state == 0x09) {
        // Gate boost by confirmed app power state; do not auto-wake from OFF/unknown.
        if (g_app_power_state != 1 || g_power_state == ARISTON_PWR_OFF) {
            addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Ariston: Boost ignored while power is OFF");
            return;
        }
        const uint8_t payload[] = { 0xDD, 0x27, 0x09 };
        g_power_state = ARISTON_PWR_BOOST;
        g_boost_state = 1;
        g_mode_state = 1;
        Ariston_SendControlWriteRaw(payload, sizeof(payload));
    }
    Ariston_PublishControlStates();
    Ariston_PublishSensors();
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Requested power state %02X (awaiting boiler confirm)", state);
}

static void Ariston_SendModeState(uint8_t mode) {
    uint8_t payload[] = { 0x06, 0x21, mode };
    Ariston_SendFrame(ARISTON_CMD_WIFI, payload, sizeof(payload));
    // Wire mapping (confirmed): 0x01=ECO, 0x00=Manual.
    g_mode_state = (mode == 0x01) ? 0 : 1;
    Ariston_PublishControlStates();
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Set mode %s (%02X)", g_mode_state ? "Manual" : "ECO", mode);
}

static void Ariston_SendAntiLegionella(uint8_t enabled) {
    uint8_t payload[] = { 0x0A, 0x21, enabled ? 0x01 : 0x00 };
    Ariston_SendFrame(ARISTON_CMD_WIFI, payload, sizeof(payload));
    g_anti_legionella = enabled ? 1 : 0;
    Ariston_PublishControlStates();
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Anti-legionella %s", enabled ? "ON" : "OFF");
}

// -------------------------------------------------------
// MQTT publish
// -------------------------------------------------------
static void Ariston_PublishSensors(void) {
#if ENABLE_MQTT
    if (!MQTT_IsReady()) return;
    static float last_water = -1, last_target = -1, last_room = -1;
    static int last_heater = -1;
    static int last_heater_active = -2;

    if (g_ctx.water_temp != last_water) {
        MQTT_PublishMain_StringFloat("ariston/water_temp", g_ctx.water_temp, 1, OBK_PUBLISH_FLAG_RETAIN);
        last_water = g_ctx.water_temp;
    }
    if (g_ctx.target_temp != last_target) {
        MQTT_PublishMain_StringFloat("ariston/target_temp", g_ctx.target_temp, 1, OBK_PUBLISH_FLAG_RETAIN);
        last_target = g_ctx.target_temp;
    }
    if (g_ctx.current_temp != last_room) {
        MQTT_PublishMain_StringFloat("ariston/current_temp", g_ctx.current_temp, 1, OBK_PUBLISH_FLAG_RETAIN);
        last_room = g_ctx.current_temp;
    }
    if (g_ctx.heater_power != last_heater) {
        MQTT_PublishMain_StringFloat("ariston/heater_power", (float)g_ctx.heater_power, 0, OBK_PUBLISH_FLAG_RETAIN);
        last_heater = g_ctx.heater_power;
    }
    if (g_ctx.heater_active != last_heater_active) {
        MQTT_PublishMain_StringFloat("ariston/heater_active", (float)g_ctx.heater_active, 0, OBK_PUBLISH_FLAG_RETAIN);
        last_heater_active = g_ctx.heater_active;
    }
    static int last_showers = -1;
    if (g_ctx.showers != last_showers) {
        MQTT_PublishMain_StringFloat("ariston/showers", (float)g_ctx.showers, 0, OBK_PUBLISH_FLAG_RETAIN);
        last_showers = g_ctx.showers;
    }
    static int last_time_to_temp = -1;
    if (g_ctx.time_to_temp != last_time_to_temp) {
        MQTT_PublishMain_StringFloat("ariston/time_to_temp", (float)g_ctx.time_to_temp, 0, OBK_PUBLISH_FLAG_RETAIN);
        last_time_to_temp = g_ctx.time_to_temp;
    }
    static int last_on_time = -1;
    if (g_ctx.on_time != last_on_time) {
        MQTT_PublishMain_StringFloat("ariston/on_time", (float)g_ctx.on_time, 0, OBK_PUBLISH_FLAG_RETAIN);
        last_on_time = g_ctx.on_time;
    }
    static float last_energy_est_wh = -1.0f;
    static float last_energy_total_wh = -1.0f;
    float energy_est_wh = g_energy_session_wh;
    float energy_total_wh = g_energy_total_wh;
    float delta = energy_est_wh - last_energy_est_wh;
    if (delta < 0.0f) {
        delta = -delta;
    }
    if (delta > 0.05f) {
        MQTT_PublishMain_StringFloat("ariston/energy_est_kwh", energy_est_wh / 1000.0f, 3, OBK_PUBLISH_FLAG_RETAIN);
        last_energy_est_wh = energy_est_wh;
    }
    delta = energy_total_wh - last_energy_total_wh;
    if (delta < 0.0f) {
        delta = -delta;
    }
    if (delta > 0.05f) {
        MQTT_PublishMain_StringFloat("ariston/energy_total_kwh", energy_total_wh / 1000.0f, 3, OBK_PUBLISH_FLAG_RETAIN);
        last_energy_total_wh = energy_total_wh;
    }
    Ariston_PublishClockAndDayState(0);
#endif
}

// -------------------------------------------------------
// Parse a fully-received boiler frame (3C C3 14 header)
// -------------------------------------------------------
static void Ariston_ProcessBoilerFrame(void) {
    addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV,
              "Ariston BOILER RX: CMD=%02X LEN=%d", g_ctx.rx_cmd, g_ctx.rx_len);

    switch (g_ctx.rx_cmd) {

        case ARISTON_CMD_INIT: {
            // Boiler acknowledging our second init
            // From logs seq 13: 3C C3 14 52 04  1D FE 00 13
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Boiler ACK CMD52");
            if (g_seq == SEQ_WAIT_INIT_ACK) {
                g_seq = SEQ_PARAM_POLL;
                g_seq_timer = 0;
                g_param_poll_index = 0;
                g_startup_wifi_stage = 0;
            }
            break;
        }

        case ARISTON_CMD_STARTUP: {
            // Boiler ACK to startup
            // From logs seq 11: 3C C3 14 24 01 00 FC
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Boiler ACK CMD24");
            if (g_seq == SEQ_WAIT_STARTUP_ACK) {
                g_seq = SEQ_SEND_INIT_2;
                g_seq_timer = 0;
            }
            break;
        }

        case ARISTON_CMD_PARAM: {
            // Boiler replied to one of our CMD 25 parameter queries.
            addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV, "Ariston: Boiler CMD25 param reply (len=%d)", g_ctx.rx_len);
            if (g_ctx.rx_len >= 1) {
                Ariston_ApplyControlRegisterValue(g_last_param_reg_hi, g_last_param_reg_lo, g_ctx.rx_payload[0], 0);
                Ariston_PublishControlStates();
            }
            // After a few param replies, advance to running state.
            if (g_seq == SEQ_PARAM_POLL) {
                // We will advance to RUNNING after the OnEverySecond timer expires.
            }
            break;
        }

        case ARISTON_CMD_TEMP: {
            // Boiler replied to our CMD 23 temperature query.
            // We use g_ctx.last_temp_query to know exactly how to decode the reply.
            switch (g_ctx.last_temp_query) {
                case ARISTON_QUERY_WATER_TEMP:
                    if (g_ctx.rx_len >= 10) {
                        float best = -1.0f;
                        int pairs = g_ctx.rx_len / 2;
                        if (pairs > 5) {
                            pairs = 5;
                        }
                        for (int p = 0; p < pairs; p++) {
                            int ofs = p * 2;
                            uint16_t raw = (uint16_t)g_ctx.rx_payload[ofs] | ((uint16_t)g_ctx.rx_payload[ofs + 1] << 8);
                            float val = raw / 10.0f;
                            if (val >= 5.0f && val <= 95.0f) {
                                best = val;
                                break;
                            }
                        }
                        if (best > 0.0f) {
                            g_ctx.water_temp = best;
                            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Water temp=%.1f C", g_ctx.water_temp);
                        }
                    }
                    break;

                case ARISTON_QUERY_SETPOINT:
                    if (g_ctx.rx_len == 12) {
                        uint16_t raw_cur = (uint16_t)g_ctx.rx_payload[8] | ((uint16_t)g_ctx.rx_payload[9] << 8);
                        float temp_cur = raw_cur / 10.0f;
                        if (temp_cur >= 0.0f && temp_cur <= 95.0f) {
                            g_ctx.current_temp = temp_cur;
                        }

                        uint16_t raw_set = (uint16_t)g_ctx.rx_payload[10] | ((uint16_t)g_ctx.rx_payload[11] << 8);
                        float temp_set = raw_set / 10.0f;
                        if (temp_set >= 20.0f && temp_set <= 90.0f) {
                            // First time setup, OR if not actively overridden by user recently
                            g_ctx.target_temp = temp_set;
                            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Target=%.1f°C, Current=%.1f°C", temp_set, temp_cur);
                        }
                    }
                    break;

                case ARISTON_QUERY_TELEMETRY:
                    if (g_ctx.rx_len == 7) {
                        // byte1 in this block correlates with relay activity in captures:
                        // 0x01 = heater OFF, 0x04 = heater ON.
                        if (g_ctx.rx_payload[1] == 0x01) {
                            g_ctx.heater_active = 0;
                        } else if (g_ctx.rx_payload[1] == 0x04) {
                            g_ctx.heater_active = 1;
                        }
                        g_ctx.showers = g_ctx.rx_payload[2];
                        g_ctx.time_to_temp = g_ctx.rx_payload[3];
                        Ariston_ApplyHeaterActivity();
                        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                                  "Ariston: Telemetry activity=%02X heater=%d Showers=%d TimeToTemp=%d",
                                  g_ctx.rx_payload[1], g_ctx.heater_active, g_ctx.showers, g_ctx.time_to_temp);
                    }
                    break;

                case ARISTON_QUERY_POWER:
                    if (g_ctx.rx_len == 10) {
                        g_ctx.heater_power_nominal = (int)g_ctx.rx_payload[2] | ((int)g_ctx.rx_payload[3] << 8);
                        g_ctx.heater_power = g_ctx.heater_power_nominal;
                        Ariston_ApplyHeaterActivity();
                        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                                  "Ariston: Heater power raw=%dW effective=%dW",
                                  g_ctx.heater_power_nominal, g_ctx.heater_power);
                    }
                    break;
                    
                case ARISTON_QUERY_ON_TIME:
                    if (g_ctx.rx_len == 6) {
                        uint32_t nowTick = xTaskGetTickCount();
                        g_on_time_raw_companion = (int)g_ctx.rx_payload[0] | ((int)g_ctx.rx_payload[1] << 8);
                        if (g_ctx.rx_payload[0] <= 59) {
                            g_ctx.clock_minute = (int)g_ctx.rx_payload[0];
                            g_ctx.clock_has_minute = 1;
                            g_ctx.clock_minute_tick = nowTick;
                        }
                        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                                  "Ariston: Minute companion raw=%d minute=%d",
                                  g_on_time_raw_companion, g_ctx.clock_minute);
                    }
                    break;

                case ARISTON_QUERY_STATUS3:
                    if (g_ctx.rx_len == 6) {
                        uint32_t nowTick = xTaskGetTickCount();
                        int year2 = (int)g_ctx.rx_payload[2];
                        int month = (int)g_ctx.rx_payload[3];
                        int day = (int)g_ctx.rx_payload[4];
                        int hour = (int)g_ctx.rx_payload[5];
                        int wday = (int)g_ctx.rx_payload[1];

                        g_ctx.clock_status3_raw0 = (int)g_ctx.rx_payload[0];
                        if (year2 >= 0 && year2 <= 99 &&
                            month >= 1 && month <= 12 &&
                            day >= 1 && day <= 31 &&
                            hour >= 0 && hour <= 23) {
                            g_ctx.clock_year2 = year2;
                            g_ctx.clock_month = month;
                            g_ctx.clock_day = day;
                            g_ctx.clock_hour = hour;
                            g_ctx.clock_has_status3 = 1;
                            g_ctx.clock_status3_tick = nowTick;
                        }
                        if (wday >= 0 && wday <= 6) {
                            g_ctx.clock_weekday_raw = wday;
                        }

                        addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                                  "Ariston: STATUS3 raw0=%d wday=%d date=20%02d-%02d-%02d hour=%02d",
                                  g_ctx.clock_status3_raw0, g_ctx.clock_weekday_raw,
                                  g_ctx.clock_year2, g_ctx.clock_month, g_ctx.clock_day, g_ctx.clock_hour);
                    }
                    break;

                default:
                    addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Ariston: Unsolicited CMD 23 reply len=%d", g_ctx.rx_len);
                    break;
            }

            // Immediately mark as none so we don't accidentally parse random unsolicited replies
            g_ctx.last_temp_query = ARISTON_QUERY_NONE;
            Ariston_PublishSensors();
            break;
        }

        case ARISTON_CMD_WIFI: {
            // Boiler sending CMD 33 register/value tuples.
            for (int i = 0; i + 2 < g_ctx.rx_len; i += 3) {
                uint8_t rhi = g_ctx.rx_payload[i + 0];
                uint8_t rlo = g_ctx.rx_payload[i + 1];
                uint8_t v   = g_ctx.rx_payload[i + 2];
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
                          "Ariston: Boiler CMD33 reg=%02X%02X state=%02X",
                          rhi, rlo, v);
                Ariston_ApplyControlRegisterValue(rhi, rlo, v, 1);
            }
            Ariston_PublishControlStates();
            break;
        }

        default:
            addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV,
                      "Ariston: Unknown boiler CMD %02X (len=%d)", g_ctx.rx_cmd, g_ctx.rx_len);
            break;
    }
}

// -------------------------------------------------------
// Receive state machine
// We are listening for boiler frames: 3C C3 14 [cmd] [len] [payload] [chk]
// States track this 3-byte SOF sequence.
// -------------------------------------------------------
void Ariston_RunFrame(void) {
    int avail = UART_GetDataSizeEx(g_ariston_uart);
    while (avail > 0) {
        uint8_t byte = UART_GetByteEx(g_ariston_uart, 0);
        UART_ConsumeBytesEx(g_ariston_uart, 1);
        avail--;

        switch (g_ctx.rx_state) {

            case ARISTON_STATE_INIT:
                // fallthrough to SOF
            case ARISTON_STATE_WAIT_SOF:
                // Wait for 0x3C (boiler SOF)
                if (byte == ARISTON_BOILER_SOF) {
                    g_ctx.rx_state = ARISTON_STATE_WAIT_HDR0;
                }
                break;

            case ARISTON_STATE_WAIT_HDR0:
                if (byte == ARISTON_BOILER_HDR0) {
                    // C3: start checksum from here
                    g_ctx.current_checksum = byte;
                    g_ctx.rx_state = ARISTON_STATE_WAIT_HDR1;
                } else {
                    g_ctx.rx_state = ARISTON_STATE_WAIT_SOF;
                }
                break;

            case ARISTON_STATE_WAIT_HDR1:
                if (byte == ARISTON_BOILER_HDR1) {
                    g_ctx.current_checksum += byte;
                    g_ctx.rx_state = ARISTON_STATE_WAIT_CMD;
                } else {
                    g_ctx.rx_state = ARISTON_STATE_WAIT_SOF;
                }
                break;

            case ARISTON_STATE_WAIT_CMD:
                g_ctx.rx_cmd = byte;
                g_ctx.current_checksum += byte;
                g_ctx.rx_state = ARISTON_STATE_WAIT_LEN;
                break;

            case ARISTON_STATE_WAIT_LEN:
                g_ctx.rx_len = byte;
                g_ctx.current_checksum += byte;
                g_ctx.rx_idx = 0;
                if (g_ctx.rx_len > ARISTON_MAX_PAYLOAD) {
                    addLogAdv(LOG_ERROR, LOG_FEATURE_DRV, "Ariston RX overflow len=%d", g_ctx.rx_len);
                    g_ctx.rx_state = ARISTON_STATE_WAIT_SOF;
                } else if (g_ctx.rx_len == 0) {
                    g_ctx.rx_state = ARISTON_STATE_WAIT_CHK;
                } else {
                    g_ctx.rx_state = ARISTON_STATE_WAIT_PAYLOAD;
                }
                break;

            case ARISTON_STATE_WAIT_PAYLOAD:
                g_ctx.rx_payload[g_ctx.rx_idx++] = byte;
                g_ctx.current_checksum += byte;
                if (g_ctx.rx_idx >= g_ctx.rx_len) {
                    g_ctx.rx_state = ARISTON_STATE_WAIT_CHK;
                }
                break;

            case ARISTON_STATE_WAIT_CHK:
                if (byte == g_ctx.current_checksum) {
                    Ariston_ProcessBoilerFrame();
                } else {
                    addLogAdv(LOG_ERROR, LOG_FEATURE_DRV,
                              "Ariston chk error: expected=%02X got=%02X cmd=%02X",
                              g_ctx.current_checksum, byte, g_ctx.rx_cmd);
                }
                g_ctx.rx_state = ARISTON_STATE_WAIT_SOF;
                break;
        }
    }

    if (g_seq == SEQ_RUNNING) {
        uint32_t now = xTaskGetTickCount();
        uint32_t step_ticks = 500U / (uint32_t)portTICK_PERIOD_MS;
        if (step_ticks == 0) {
            step_ticks = 1;
        }
        if (g_next_poll_tick == 0) {
            g_next_poll_tick = now + step_ticks;
        } else if ((int32_t)(now - g_next_poll_tick) >= 0) {
            Ariston_SendNextPoll();
            g_next_poll_tick = now + step_ticks;
        }
    }
}

// -------------------------------------------------------
// Startup sequence driver (called from OnEverySecond)
// -------------------------------------------------------
static void Ariston_DriveSequence(void) {
    if (!g_ariston_identity_valid) {
        if (!g_ariston_identity_warned) {
            addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
                "Ariston: sequence blocked, identity missing. Use startDriver Ariston <MAC> <SN>");
            g_ariston_identity_warned = 1;
        }
        g_seq = SEQ_IDLE;
        g_seq_timer = 0;
        return;
    }

    g_seq_timer++;

    switch (g_seq) {

        case SEQ_IDLE:
            // Start init after 2 seconds
            if (g_seq_timer >= 2) {
                g_seq = SEQ_SEND_INIT_1;
                g_seq_timer = 0;
            }
            break;

        case SEQ_SEND_INIT_1:
            Ariston_SendInit();
            // Boiler doesn't ACK the first init sequence, we immediately proceed to startup
            g_seq = SEQ_SEND_STARTUP;
            g_seq_timer = 0;
            break;

        case SEQ_SEND_STARTUP:
            Ariston_SendStartup();
            g_seq = SEQ_WAIT_STARTUP_ACK;
            g_seq_timer = 0;
            break;

        case SEQ_WAIT_STARTUP_ACK:
            // Some captures show occasional parser misses on CMD24 ACK.
            // Proceeding to init2 after a short grace period avoids boot-looping.
            if (g_seq_timer >= 1) {
                if (g_seq_timer >= 3) {
                    addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Ariston: CMD24 ACK timeout, proceeding to init2");
                }
                g_seq = SEQ_SEND_INIT_2;
                g_seq_timer = 0;
            }
            break;

        case SEQ_SEND_INIT_2:
            Ariston_SendInit2();
            g_seq = SEQ_WAIT_INIT_ACK;
            g_seq_timer = 0;
            break;

        case SEQ_WAIT_INIT_ACK:
            // ProcessBoilerFrame advances us when CMD52 ACK arrives.
            if (g_seq_timer >= 3) {
                addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Ariston: CMD52 ACK timeout, retrying");
                g_seq = SEQ_SEND_INIT_1;
                g_seq_timer = 0;
                g_param_poll_index = 0;
                g_startup_wifi_stage = 0;
            }
            break;

        case SEQ_PARAM_POLL:
            // Send a broad startup CMD25 sweep and startup CMD33 transition states.
            if (g_startup_wifi_stage == 0) {
                Ariston_SendStartupWifiStage(0);
                g_startup_wifi_stage = 1;
            }

            if (g_param_poll_index >= (int)(sizeof(s_startup_param_regs) / sizeof(s_startup_param_regs[0])) / 4
                && g_startup_wifi_stage == 1) {
                Ariston_SendStartupWifiStage(1);
                g_startup_wifi_stage = 2;
            }
            if (g_param_poll_index >= (int)(sizeof(s_startup_param_regs) / sizeof(s_startup_param_regs[0])) / 2
                && g_startup_wifi_stage == 2) {
                Ariston_SendStartupWifiStage(2);
                g_startup_wifi_stage = 3;
            }
            if (g_param_poll_index >= 3 * ((int)(sizeof(s_startup_param_regs) / sizeof(s_startup_param_regs[0])) / 4)
                && g_startup_wifi_stage == 3) {
                Ariston_SendStartupWifiStage(3);
                g_startup_wifi_stage = 4;
            }

            for (int burst = 0; burst < 4; burst++) {
                int count = (int)(sizeof(s_startup_param_regs) / sizeof(s_startup_param_regs[0]));
                if (g_param_poll_index >= count) {
                    break;
                }
                Ariston_SendParamQuery(s_startup_param_regs[g_param_poll_index].hi,
                    s_startup_param_regs[g_param_poll_index].lo);
                g_param_poll_index++;
            }

            if (g_param_poll_index >= (int)(sizeof(s_startup_param_regs) / sizeof(s_startup_param_regs[0]))
                && g_startup_wifi_stage == 4) {
                Ariston_SendStartupWifiStage(4);
                g_startup_wifi_stage = 5;
            }

            if (g_param_poll_index >= (int)(sizeof(s_startup_param_regs) / sizeof(s_startup_param_regs[0]))
                && g_seq_timer >= 2) {
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Entering running state");
                g_seq = SEQ_RUNNING;
                g_seq_timer = 0;
                g_wifi_report_timer = 0;
                g_poll_toggle = 0;
                g_next_poll_tick = xTaskGetTickCount();
            }
            break;

        case SEQ_RUNNING:
            // Handled separately in the timers below
            break;
    }
}

// -------------------------------------------------------
// Called every second from the driver framework
// -------------------------------------------------------
void Ariston_OnEverySecond(void) {
    Ariston_DriveSequence();

#if ENABLE_MQTT
    if (MQTT_IsReady()) {
        if (!g_ariston_mqtt_was_ready) {
            // MQTT just became ready: republish full retained state so HA entities
            // don't stay Unknown after reconnect/discovery.
            g_ariston_mqtt_was_ready = 1;
            Ariston_PublishAllStatesNow();
        }
    } else {
        g_ariston_mqtt_was_ready = 0;
    }
#endif

    if (g_ariston_energy_loaded) {
        g_energy_save_seconds++;
        Ariston_SaveEnergyTotalIfNeeded(0);
    }

    if (g_seq != SEQ_RUNNING) return;

    Ariston_UpdateEnergyEstimate();
    if (Ariston_UpdateOnTimeRuntime()) {
        Ariston_PublishSensors();
    }
    Ariston_MaybeSendNtpClockSync();

    // Advance both timers
    g_wifi_report_timer++;

    if (g_wifi_report_timer >= 10) {
        g_wifi_report_timer = 0;
        Ariston_SendWifiState(WIFI_STATE_STA_CONNECTED);
    }
}

static int Ariston_HexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static ariston_identity_result_t Ariston_ParseMacString(const char *macStr, uint8_t out[6]) {
    char tmp[13];
    int n = 0;
    int i;

    if (!macStr || !out) {
        return ARISTON_ID_ERR_MAC_EMPTY;
    }
    if (macStr[0] == 0) {
        return ARISTON_ID_ERR_MAC_EMPTY;
    }
    for (i = 0; macStr[i] != 0; i++) {
        char c = macStr[i];
        if (c == ':' || c == '-' || c == ' ') {
            continue;
        }
        if (Ariston_HexNibble(c) < 0) {
            return ARISTON_ID_ERR_MAC_NON_HEX;
        }
        if (n >= 12) {
            return ARISTON_ID_ERR_MAC_LENGTH;
        }
        tmp[n++] = c;
    }
    if (n != 12) {
        return ARISTON_ID_ERR_MAC_LENGTH;
    }
    for (i = 0; i < 6; i++) {
        int hi = Ariston_HexNibble(tmp[i * 2 + 0]);
        int lo = Ariston_HexNibble(tmp[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            return ARISTON_ID_ERR_MAC_NON_HEX;
        }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return ARISTON_ID_OK;
}

static const char *Ariston_IdentityErrorToString(ariston_identity_result_t err) {
    switch (err) {
    case ARISTON_ID_OK:
        return "ok";
    case ARISTON_ID_ERR_MAC_EMPTY:
        return "MAC empty";
    case ARISTON_ID_ERR_MAC_LENGTH:
        return "MAC length";
    case ARISTON_ID_ERR_MAC_NON_HEX:
        return "MAC non-hex";
    case ARISTON_ID_ERR_SN_EMPTY:
        return "SN empty";
    case ARISTON_ID_ERR_SN_LENGTH:
        return "SN length";
    case ARISTON_ID_ERR_SN_NON_PRINTABLE:
        return "SN non-printable";
    default:
        return "unknown";
    }
}

static void Ariston_ClearIdentity(void) {
    memset(g_ariston_identity_mac, 0, sizeof(g_ariston_identity_mac));
    memset(g_ariston_identity_sn, 0, sizeof(g_ariston_identity_sn));
    g_ariston_identity_valid = 0;
}

bool Ariston_SetIdentityFromStrings(const char *macStr, const char *snStr) {
    ariston_identity_result_t res;
    uint8_t parsedMac[6];
    size_t snLen;
    int i;

    res = Ariston_ParseMacString(macStr, parsedMac);
    if (res != ARISTON_ID_OK) {
        Ariston_ClearIdentity();
        g_ariston_identity_warned = 0;
        addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
            "Ariston: identity validation failed (%s). MAC expected as 12 hex digits, separators [: - space] allowed",
            Ariston_IdentityErrorToString(res));
        return false;
    }
    if (!snStr || snStr[0] == 0) {
        Ariston_ClearIdentity();
        g_ariston_identity_warned = 0;
        addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Ariston: identity validation failed (%s)",
            Ariston_IdentityErrorToString(ARISTON_ID_ERR_SN_EMPTY));
        return false;
    }
    snLen = strlen(snStr);
    if (snLen != 12) {
        Ariston_ClearIdentity();
        g_ariston_identity_warned = 0;
        addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
            "Ariston: identity validation failed (%s=%u, expected 12)",
            Ariston_IdentityErrorToString(ARISTON_ID_ERR_SN_LENGTH), (unsigned)snLen);
        return false;
    }
    for (i = 0; i < 12; i++) {
        char c = snStr[i];
        if (c < 0x20 || c > 0x7E) {
            Ariston_ClearIdentity();
            g_ariston_identity_warned = 0;
            addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
                "Ariston: identity validation failed (%s at index %d)",
                Ariston_IdentityErrorToString(ARISTON_ID_ERR_SN_NON_PRINTABLE), i);
            return false;
        }
    }

    memcpy(g_ariston_identity_mac, parsedMac, sizeof(g_ariston_identity_mac));
    memcpy(g_ariston_identity_sn, snStr, 12);
    g_ariston_identity_sn[12] = 0;
    g_ariston_identity_valid = 1;
    g_ariston_identity_warned = 0;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
              "Ariston: identity set MAC=%02X:%02X:%02X:%02X:%02X:%02X SN=%s",
              g_ariston_identity_mac[0], g_ariston_identity_mac[1], g_ariston_identity_mac[2],
              g_ariston_identity_mac[3], g_ariston_identity_mac[4], g_ariston_identity_mac[5],
              g_ariston_identity_sn);
    return true;
}

// -------------------------------------------------------
// OpenBeken console commands
// -------------------------------------------------------

// ariston_setTemp 55  → set target temperature to 55°C
static commandResult_t Cmd_Ariston_SetTemp(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() < 1) {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Usage: ariston_setTemp [temp]");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    g_ctx.target_temp = Tokenizer_GetArgFloat(0);
    // Send immediately via CMD 33
    Ariston_SendSetTemp();
    Ariston_PublishSensors();
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Target temp set to %.1f°C", g_ctx.target_temp);
    return CMD_RES_OK;
}

// ariston_power 1|0  -> power ON or OFF (05 21)
static commandResult_t Cmd_Ariston_Power(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() < 1) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    int pwr = Tokenizer_GetArgInteger(0);
    Ariston_SendPowerState(pwr ? 0x01 : 0x00);
    return CMD_RES_OK;
}

// ariston_boost  → activate boost mode
static commandResult_t Cmd_Ariston_Boost(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() >= 1) {
        int en = Tokenizer_GetArgInteger(0);
        Ariston_SendPowerState(en ? 0x09 : 0x01);
        return CMD_RES_OK;
    }
    // Toggle behavior (dual-use button): ON->OFF, OFF->ON
    Ariston_SendPowerState((g_boost_state == 1) ? 0x01 : 0x09);
    return CMD_RES_OK;
}

// ariston_standby -> standby mode (distinct from power OFF)
static commandResult_t Cmd_Ariston_Standby(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() >= 1) {
        int en = Tokenizer_GetArgInteger(0);
        Ariston_SendPowerState(en ? 0x05 : 0x01);
        return CMD_RES_OK;
    }
    Ariston_SendPowerState(0x05);
    return CMD_RES_OK;
}

// ariston_mode accepts the HA-facing names plus a few compatibility aliases.
// For eco/manual selection we proactively clear boost first so the UI does not
// stay in a contradictory state until the next confirmation frame arrives.
static commandResult_t Cmd_Ariston_Mode(const void *context, const char *cmd, const char *args, int cmdFlags) {
    char normalized[40];
    int need_power_on;
    int need_clear_boost;
    int i;
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() < 1) {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Usage: ariston_mode [off|eco|electric|performance|boost|anti-legionella|manual|0|1]");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    const char *arg = Tokenizer_GetArg(0);
    strncpy(normalized, arg, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    for (i = 0; normalized[i]; i++) {
        if (normalized[i] >= 'A' && normalized[i] <= 'Z') {
            normalized[i] = (char)(normalized[i] - 'A' + 'a');
        } else if (normalized[i] == '_' || normalized[i] == ' ') {
            normalized[i] = '-';
        }
    }
    if ((normalized[0] == '"' || normalized[0] == '\'') && normalized[1]) {
        size_t len = strlen(normalized);
        if (normalized[len - 1] == normalized[0] && len > 2) {
            memmove(normalized, normalized + 1, len - 2);
            normalized[len - 2] = '\0';
        }
    }
    need_power_on = (g_power_state != ARISTON_PWR_ON && g_power_state != ARISTON_PWR_BOOST);
    need_clear_boost = (g_boost_state == 1 || g_power_state == ARISTON_PWR_BOOST);

    if (!strcmp(normalized, "eco") || !strcmp(normalized, "0")) {
        if (g_app_power_state != 1 || need_power_on || need_clear_boost) {
            Ariston_SendPowerState(0x01);
            rtos_delay_milliseconds(120);
        }
        Ariston_SendModeState(0x01);
        return CMD_RES_OK;
    }
    if (!strcmp(normalized, "manual") || !strcmp(normalized, "electric") || !strcmp(normalized, "1")) {
        if (g_app_power_state != 1 || need_power_on || need_clear_boost) {
            Ariston_SendPowerState(0x01);
            rtos_delay_milliseconds(120);
        }
        Ariston_SendModeState(0x00);
        return CMD_RES_OK;
    }
    if (!strcmp(normalized, "boost") || !strcmp(normalized, "performance") || !strcmp(normalized, "state-performance")) {
        if (g_app_power_state != 1 || need_power_on) {
            Ariston_SendPowerState(0x01);
            rtos_delay_milliseconds(120);
        }
        Ariston_SendPowerState(0x09);
        return CMD_RES_OK;
    }
    if (!strcmp(normalized, "anti-legionella") || !strcmp(normalized, "antilegionella")) {
        Ariston_SendAntiLegionella(1);
        return CMD_RES_OK;
    }
    if (!strcmp(normalized, "off")) {
        Ariston_SendPowerState(0x00);
        return CMD_RES_OK;
    }
    addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Ariston: Invalid mode '%s' (expected off/eco/electric/performance/boost/anti-legionella)", arg);
    return CMD_RES_BAD_ARGUMENT;
}

// ariston_antiLegionella 0|1
static commandResult_t Cmd_Ariston_AntiLegionella(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() < 1) {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Usage: ariston_antiLegionella [0|1]");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    int en = Tokenizer_GetArgInteger(0);
    Ariston_SendAntiLegionella(en ? 1 : 0);
    return CMD_RES_OK;
}

// ariston_energyReset -> clears persisted energy estimate total
static commandResult_t Cmd_Ariston_EnergyReset(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Ariston_ResetEnergyTotal();
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Energy totals reset to 0 Wh");
    return CMD_RES_OK;
}

// ariston_onTimeReset -> clear the derived runtime counter without touching
// the boiler clock or the persisted energy total.
static commandResult_t Cmd_Ariston_OnTimeReset(const void *context, const char *cmd, const char *args, int cmdFlags) {
    (void)context;
    (void)cmd;
    (void)args;
    (void)cmdFlags;
    g_ctx.on_time = 0;
    g_on_time_baseline_valid = 0;
    g_on_time_last_minute_key = 0;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: ON Time runtime reset");
    Ariston_PublishSensors();
    return CMD_RES_OK;
}

// ariston_uart [0|1|2] -> change the UART port used by the driver
static commandResult_t Cmd_Ariston_Uart(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() < 1) {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Current Ariston UART is %d", g_ariston_uart);
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    int port = Tokenizer_GetArgInteger(0);
    g_ariston_uart = port;
    Ariston_ReinitUart();
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston UART changed to %d", g_ariston_uart);
    return CMD_RES_OK;
}

// ariston_discovery -> manual HA discovery publish (Roomba style)
static commandResult_t Cmd_Ariston_Discovery(const void *context, const char *cmd, const char *args, int cmdFlags) {
    (void)context;
    (void)cmd;
    (void)args;
    (void)cmdFlags;
    g_ariston_discovery_sent = 0;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston: Manual HA discovery requested");
    Ariston_OnHassDiscovery("homeassistant");
    return CMD_RES_OK;
}

// -------------------------------------------------------
// Driver init
// -------------------------------------------------------
void Ariston_Init(void) {
    Ariston_ReinitUart();

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.rx_state = ARISTON_STATE_INIT;
    g_ctx.current_temp  = 22.5f;
    g_ctx.target_temp = 0.0f;  // 0 = unknown until boiler reports it
    g_ctx.heater_active = -1;
    g_ctx.clock_weekday_raw = -1;
    g_ctx.clock_status3_raw0 = -1;
    g_ctx.clock_year2 = -1;
    g_ctx.clock_month = -1;
    g_ctx.clock_day = -1;
    g_ctx.clock_hour = -1;
    g_ctx.clock_minute = -1;
    g_ctx.clock_has_status3 = 0;
    g_ctx.clock_has_minute = 0;
    g_ctx.clock_using_uptime_fallback = 1;

    g_seq = SEQ_IDLE;
    g_seq_timer = 0;
    g_param_poll_index = 0;
    g_startup_wifi_stage = 0;
    g_energy_day_source = ARISTON_CLOCK_SRC_UNKNOWN;
    g_ntp_sync_prev_synced = 0;
    g_ntp_sync_last_day_key = -1;
    g_ntp_sync_last_minute_of_day = -1;
    g_on_time_baseline_valid = 0;
    g_on_time_last_minute_key = 0;
    g_on_time_raw_companion = -1;
    Ariston_LoadEnergyTotal();
    g_ctx.energy_today_wh = g_energy_today_wh;
    g_energy_last_tick = xTaskGetTickCount();

    CMD_RegisterCommand("ariston_setTemp", Cmd_Ariston_SetTemp, NULL);
    CMD_RegisterCommand("ariston_power",   Cmd_Ariston_Power,   NULL);
    CMD_RegisterCommand("ariston_standby", Cmd_Ariston_Standby, NULL);
    CMD_RegisterCommand("ariston_boost",   Cmd_Ariston_Boost,   NULL);
    CMD_RegisterCommand("ariston_mode",    Cmd_Ariston_Mode,    NULL);
    CMD_RegisterCommand("ariston_antiLegionella", Cmd_Ariston_AntiLegionella, NULL);
    CMD_RegisterCommand("ariston_energyReset", Cmd_Ariston_EnergyReset, NULL);
    CMD_RegisterCommand("ariston_onTimeReset", Cmd_Ariston_OnTimeReset, NULL);
    CMD_RegisterCommand("ariston_uart",    Cmd_Ariston_Uart,    NULL);
    CMD_RegisterCommand("ariston_discovery", Cmd_Ariston_Discovery, NULL);

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Ariston driver initialized (ESP=master C3 41)");
}

// -------------------------------------------------------
// HTTP index page info
// -------------------------------------------------------
void Ariston_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
    if (bPreState) {
        return;
    }

    poststr(request, "<hr><h4>Ariston</h4>");
    poststr(request, "<div id='ariston_cmd_msg' style='margin:6px 0; font-size:12px;'></div>");
    poststr(request,
        "<script>"
        "function aristonSend(cmnd){"
        "var el=document.getElementById('ariston_cmd_msg');"
        "el.innerHTML='Sending: <b>'+cmnd+'</b> ...';"
        "fetch('/cm?cmnd='+encodeURIComponent(cmnd))"
        ".then(r=>r.text())"
        ".then(t=>{el.innerHTML='OK: <b>'+cmnd+'</b>';})"
        ".catch(e=>{el.innerHTML='Error sending <b>'+cmnd+'</b>';});"
        "return false;"
        "}"
        "function aristonSetTemp(){"
        "var v=document.getElementById('ari_temp_input').value;"
        "if(!v||isNaN(v)){return false;}"
        "if(v<40)v=40;if(v>80)v=80;"
        "document.getElementById('ari_temp_input').value=v;"
        "return aristonSend('ariston_setTemp '+v);"
        "}"
        "</script>"
    );

    const char *pwrCmd = (g_power_state == ARISTON_PWR_ON || g_power_state == ARISTON_PWR_BOOST) ? "ariston_power 0" : "ariston_power 1";
    const char *pwrLbl = (g_power_state == ARISTON_PWR_ON || g_power_state == ARISTON_PWR_BOOST) ? "Power OFF" : "Power ON";
    const char *modeCmd = (g_mode_state == 1) ? "ariston_mode eco" : "ariston_mode manual";
    const char *modeLbl = (g_mode_state == 1) ? "Set ECO" : "Set Manual";
    const char *antiCmd = (g_anti_legionella == 1) ? "ariston_antiLegionella 0" : "ariston_antiLegionella 1";
    const char *antiLbl = (g_anti_legionella == 1) ? "Anti-Leg OFF" : "Anti-Leg ON";
    const char *boostCmd = (g_boost_state == 1) ? "ariston_boost 0" : "ariston_boost 1";
    const char *boostLbl = (g_boost_state == 1) ? "Boost OFF" : "Boost ON";
    const char *powerStateStr;
    const char *modeStateStr;
    const char *boostStateStr;
    const char *heaterStateStr;
    const char *clockSrcStr = "unknown";
    const char *weekdayStr = "Unknown";
    ariston_clock_source_t clockSrc = ARISTON_CLOCK_SRC_UNKNOWN;
    char dateStr[20];
    char timeStr[12];
    uint32_t nowTick = xTaskGetTickCount();
    int hourFresh = Ariston_IsClockFresh(nowTick, g_ctx.clock_status3_tick);
    int minuteFresh = Ariston_IsClockFresh(nowTick, g_ctx.clock_minute_tick);
    int target_temp_ui = (int)(g_ctx.target_temp + 0.5f);
    clockSrc = Ariston_SelectClockSource(nowTick, 0);
    clockSrcStr = Ariston_ClockSourceName(clockSrc);
    snprintf(dateStr, sizeof(dateStr), "Unknown");
    snprintf(timeStr, sizeof(timeStr), "Unknown");
    if (target_temp_ui < 40) target_temp_ui = 40;
    if (target_temp_ui > 80) target_temp_ui = 80;

    powerStateStr = Ariston_GetPowerStateString();
    modeStateStr = Ariston_GetModeStateString();
    boostStateStr = Ariston_GetBoostStateString();
    heaterStateStr = Ariston_GetHeaterActivityString();
    if (g_ctx.clock_has_status3 && hourFresh &&
        g_ctx.clock_year2 >= 0 && g_ctx.clock_year2 <= 99 &&
        g_ctx.clock_month >= 1 && g_ctx.clock_month <= 12 &&
        g_ctx.clock_day >= 1 && g_ctx.clock_day <= 31) {
        snprintf(dateStr, sizeof(dateStr), "20%02d-%02d-%02d", g_ctx.clock_year2, g_ctx.clock_month, g_ctx.clock_day);
        weekdayStr = Ariston_WeekdayName(g_ctx.clock_weekday_raw);
    }
    if (g_ctx.clock_has_status3 && g_ctx.clock_has_minute && hourFresh && minuteFresh &&
        g_ctx.clock_hour >= 0 && g_ctx.clock_hour <= 23 &&
        g_ctx.clock_minute >= 0 && g_ctx.clock_minute <= 59) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", g_ctx.clock_hour, g_ctx.clock_minute);
    }

    poststr(request, "<table style='width:100%; border-collapse:separate; border-spacing:4px 6px'>");
    poststr(request, "<tr>");
    hprintf255(request, "<td style='width:33%%'><button type='button' class='btn btn-primary' style='width:100%%' onclick=\"return aristonSend('%s')\">%s</button></td>", pwrCmd, pwrLbl);
    poststr(request, "<td style='width:33%'><button type='button' class='btn' style='width:100%' onclick=\"return aristonSend('ariston_standby')\">Standby</button></td>");
    poststr(request, "<td style='width:34%; vertical-align:middle'><table style='width:100%; border-collapse:separate; border-spacing:3px 0'><tr>");
    hprintf255(request, "<td style='width:28%%; padding-top:3px'><input id='ari_temp_input' type='number' min='40' max='80' step='1' value='%d' style='width:100%%; height:34px; font-size:24px; text-align:center; padding:0'></td>", target_temp_ui);
    poststr(request, "<td style='width:72%; vertical-align:middle'><button type='button' class='btn' style='width:100%' onclick='return aristonSetTemp()'>Set Temp</button></td>");
    poststr(request, "</tr></table></td>");
    poststr(request, "</tr>");
    poststr(request, "<tr>");
    hprintf255(request, "<td style='width:33%%'><button type='button' class='btn' style='width:100%%' onclick=\"return aristonSend('%s')\">%s</button></td>", modeCmd, modeLbl);
    hprintf255(request, "<td style='width:33%%'><button type='button' class='btn' style='width:100%%' onclick=\"return aristonSend('%s')\">%s</button></td>", antiCmd, antiLbl);
    hprintf255(request, "<td style='width:34%%'><button type='button' class='btn' style='width:100%%' onclick=\"return aristonSend('%s')\">%s</button></td>", boostCmd, boostLbl);
    poststr(request, "</tr>");
    poststr(request, "</table><hr>");

    poststr(request, "<table style='width:100%'>");
    hprintf255(request, "<tr><td><b>Water</b></td><td style='text-align:right;'>%.1f&deg;C</td></tr>", g_ctx.water_temp);
    hprintf255(request, "<tr><td><b>Current</b></td><td style='text-align:right;'>%.1f&deg;C</td></tr>", g_ctx.current_temp);
    hprintf255(request, "<tr><td><b>Target</b></td><td style='text-align:right;'>%.1f&deg;C</td></tr>", g_ctx.target_temp);
    hprintf255(request, "<tr><td><b>Heater</b></td><td style='text-align:right;'>%d W</td></tr>", g_ctx.heater_power);
    hprintf255(request, "<tr><td><b>Heater Activity</b></td><td style='text-align:right;'>%s</td></tr>", heaterStateStr);
    hprintf255(request, "<tr><td><b>Showers</b></td><td style='text-align:right;'>%d</td></tr>", g_ctx.showers);
    hprintf255(request, "<tr><td><b>Time To Temp</b></td><td style='text-align:right;'>%d min</td></tr>", g_ctx.time_to_temp);
    hprintf255(request, "<tr><td><b>ON Time</b></td><td style='text-align:right;'>%d min</td></tr>", g_ctx.on_time);
    hprintf255(request, "<tr><td><b>Boiler Date</b></td><td style='text-align:right;'>%s</td></tr>", dateStr);
    hprintf255(request, "<tr><td><b>Boiler Time</b></td><td style='text-align:right;'>%s</td></tr>", timeStr);
    hprintf255(request, "<tr><td><b>Weekday</b></td><td style='text-align:right;'>%s (%d)</td></tr>", weekdayStr, g_ctx.clock_weekday_raw);
    hprintf255(request, "<tr><td><b>Clock Source</b></td><td style='text-align:right;'>%s</td></tr>", clockSrcStr);
    hprintf255(request, "<tr><td><b>Power State</b></td><td style='text-align:right;'>%s</td></tr>", powerStateStr);
    hprintf255(request, "<tr><td><b>Mode</b></td><td style='text-align:right;'>%s</td></tr>", modeStateStr);
    hprintf255(request, "<tr><td><b>Boost</b></td><td style='text-align:right;'>%s</td></tr>", boostStateStr);
    hprintf255(request, "<tr><td><b>Anti-Legionella</b></td><td style='text-align:right;'>%s</td></tr>", Ariston_GetAntiLegionellaString());
    hprintf255(request, "<tr><td><b>Energy Today</b></td><td style='text-align:right;'>%.2f Wh</td></tr>", g_energy_today_wh);
    hprintf255(request, "<tr><td><b>Energy Est (Session)</b></td><td style='text-align:right;'>%.2f Wh</td></tr>", g_energy_session_wh);
    hprintf255(request, "<tr><td><b>Energy Est (Total)</b></td><td style='text-align:right;'>%.2f Wh</td></tr>", g_energy_total_wh);
    poststr(request, "</table>");
}

// -------------------------------------------------------
// Home Assistant MQTT auto-discovery
// -------------------------------------------------------
static void Ariston_QueueDiscovery(const char *config_topic, const char *payload) {
#if ENABLE_MQTT
    const char *prefix = "homeassistant/";
    const int prefix_len = 14;
    if (!config_topic || !payload) {
        return;
    }
    if (!strncmp(config_topic, prefix, prefix_len)) {
        MQTT_QueuePublish("homeassistant", config_topic + prefix_len, payload, OBK_PUBLISH_FLAG_RETAIN);
    } else {
        MQTT_PublishMain_StringString(config_topic, payload, OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
    }
#else
    (void)config_topic;
    (void)payload;
#endif
}

void Ariston_OnHassDiscovery(const char *topic) {
#if ENABLE_MQTT
    (void)topic;
    if (!MQTT_IsReady()) return;
    if (g_ariston_discovery_sent) {
        addLogAdv(LOG_DEBUG, LOG_FEATURE_DRV, "Ariston: HA discovery already sent, skipping");
        return;
    }
    // Set guard immediately to prevent re-entry during reconnect/HA-online storms.
    g_ariston_discovery_sent = 1;

    const char *devName  = CFG_GetDeviceName();
    const char *clientId = CFG_GetMQTTClientId();
    static char unique_id[96];
    static char config_topic[192];
    static char payload[1536];
    int payload_len;

    // Water Temperature
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_water_temp", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Water Temperature\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/water_temp/get\","
        "\"device_class\":\"temperature\",\"unit_of_measurement\":\"°C\",\"state_class\":\"measurement\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Target Temperature (Number)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_target_temp", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/number/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Target Temperature\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/target_temp/get\","
        "\"command_topic\":\"cmnd/%s/ariston_setTemp\",\"min\":40,\"max\":80,\"step\":1,"
        "\"device_class\":\"temperature\",\"unit_of_measurement\":\"°C\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Water Heater main entity for HA control panel.
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_water_heater", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/water_heater/%s/config", unique_id);
    payload_len = snprintf(payload, sizeof(payload),
        "{"
        "\"name\":\"Ariston Water Heater\","
        "\"unique_id\":\"%s\","
        "\"temperature_state_topic\":\"%s/ariston/target_temp/get\","
        "\"temperature_command_topic\":\"cmnd/%s/ariston_setTemp\","
        "\"current_temperature_topic\":\"%s/ariston/current_temp/get\","
        "\"mode_state_topic\":\"%s/ariston/ha_mode/get\","
        "\"mode_command_topic\":\"cmnd/%s/ariston_mode\","
        "\"mode_command_template\":\"{{ value | lower }}\","
        "\"modes\":[\"off\",\"eco\",\"electric\",\"performance\"],"
        "\"power_command_topic\":\"cmnd/%s/ariston_power\","
        "\"payload_on\":\"1\",\"payload_off\":\"0\","
        "\"min_temp\":40,\"max_temp\":80,"
        "\"temperature_unit\":\"C\","
        "\"precision\":1.0,"
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}"
        "}",
        unique_id,
        clientId, clientId, clientId, clientId, clientId, clientId,
        devName, devName);
    if (payload_len < 0 || payload_len >= (int)sizeof(payload)) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_DRV, "Ariston: HA water_heater discovery payload truncated (len=%d, max=%d)",
            payload_len, (int)sizeof(payload) - 1);
    } else {
    Ariston_QueueDiscovery(config_topic, payload);
    }

    // Heater Power
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_heater_power", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Heater Power\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/heater_power/get\","
        "\"device_class\":\"power\",\"unit_of_measurement\":\"W\",\"state_class\":\"measurement\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Heater Activity from telemetry byte2 (01=OFF, 04=ON)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_heater_active", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/binary_sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Heater Active\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/heater_active/get\","
        "\"device_class\":\"heat\",\"payload_on\":\"1\",\"payload_off\":\"0\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Number of Showers
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_showers", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Showers\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/showers/get\","
        "\"icon\":\"mdi:shower\",\"state_class\":\"measurement\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Time To Temp
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_time_to_temp", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Time To Temp\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/time_to_temp/get\","
        "\"device_class\":\"duration\",\"unit_of_measurement\":\"min\",\"state_class\":\"measurement\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Power ON Time
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_on_time", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"ON Time\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/on_time/get\","
        "\"device_class\":\"duration\",\"unit_of_measurement\":\"min\",\"state_class\":\"measurement\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Mode text sensor (eco/manual)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_mode", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Mode\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/mode/get\","
        "\"icon\":\"mdi:tune\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Power state text sensor (off/on/standby/boost)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_power_state", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Power State\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/power_state/get\","
        "\"icon\":\"mdi:water-boiler\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_energy_est_kwh", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Energy Estimate kWh\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/energy_est_kwh/get\","
        "\"device_class\":\"energy\",\"unit_of_measurement\":\"kWh\",\"state_class\":\"measurement\","
        "\"icon\":\"mdi:lightning-bolt\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Estimated cumulative energy (kWh), persisted across OBK power cycles.
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_energy_total_kwh", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Energy Total\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/energy_total_kwh/get\","
        "\"device_class\":\"energy\",\"unit_of_measurement\":\"kWh\",\"state_class\":\"total_increasing\","
        "\"icon\":\"mdi:lightning-bolt\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_energy_today_kwh", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Energy Today\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/energy_today_kwh/get\","
        "\"device_class\":\"energy\",\"unit_of_measurement\":\"kWh\",\"state_class\":\"measurement\","
        "\"icon\":\"mdi:lightning-bolt\",\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Power Switch
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_power", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/switch/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Power\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/power/get\","
        "\"command_topic\":\"cmnd/%s/ariston_power\",\"payload_on\":\"1\",\"payload_off\":\"0\","
        "\"icon\":\"mdi:power\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Boost Switch (toggle ON/OFF)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_boost", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/switch/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Boost\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/boost/get\","
        "\"command_topic\":\"cmnd/%s/ariston_boost\",\"payload_on\":\"1\",\"payload_off\":\"0\","
        "\"icon\":\"mdi:fire\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Standby switch (ON=standby, OFF=normal ON)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_standby", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/switch/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Standby\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/power_state/get\","
        "\"value_template\":\"{{ '1' if value == 'Standby' else '0' }}\","
        "\"command_topic\":\"cmnd/%s/ariston_standby\",\"payload_on\":\"1\",\"payload_off\":\"0\","
        "\"icon\":\"mdi:sleep\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // ECO mode switch (ON=ECO, OFF=Manual)
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_mode_switch", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/switch/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"ECO Mode\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/mode/get\","
        "\"command_topic\":\"cmnd/%s/ariston_mode\","
        "\"payload_on\":\"eco\",\"payload_off\":\"manual\","
        "\"state_on\":\"ECO\",\"state_off\":\"Manual\","
        "\"icon\":\"mdi:leaf\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

    // Anti-legionella switch
    snprintf(unique_id,    sizeof(unique_id),    "%s_ariston_anti_legionella_switch", devName);
    snprintf(config_topic, sizeof(config_topic), "homeassistant/switch/%s/config", unique_id);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Anti-Legionella\",\"unique_id\":\"%s\",\"state_topic\":\"%s/ariston/anti_legionella/get\","
        "\"command_topic\":\"cmnd/%s/ariston_antiLegionella\",\"payload_on\":\"1\",\"payload_off\":\"0\","
        "\"icon\":\"mdi:shield-check\"," 
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"Ariston\",\"mdl\":\"Boiler\"}}",
        unique_id, clientId, clientId, devName, devName);
    Ariston_QueueDiscovery(config_topic, payload);

#endif
}

#endif // ENABLE_DRIVER_ARISTON

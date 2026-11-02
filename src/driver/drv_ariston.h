
#ifndef __DRV_ARISTON_H__
#define __DRV_ARISTON_H__

#include "../obk_config.h"

#if ENABLE_DRIVER_ARISTON

#include "../new_common.h"

// -------------------------------------------------------
// Receive state machine states
// We listen for boiler frames: 3C C3 14 [cmd] [len] [payload] [chk]
// -------------------------------------------------------
typedef enum {
    ARISTON_STATE_INIT       = 0,  // same as WAIT_SOF
    ARISTON_STATE_WAIT_SOF   = 1,  // waiting for 0x3C
    ARISTON_STATE_WAIT_HDR0  = 2,  // waiting for 0xC3
    ARISTON_STATE_WAIT_HDR1  = 3,  // waiting for 0x14
    ARISTON_STATE_WAIT_CMD   = 4,
    ARISTON_STATE_WAIT_LEN   = 5,
    ARISTON_STATE_WAIT_PAYLOAD = 6,
    ARISTON_STATE_WAIT_CHK   = 7
} ariston_state_t;

// State machine for the startup sequence with the boiler
typedef enum {
    SEQ_IDLE = 0,
    SEQ_SEND_INIT_1,            // Send MAC, Serial, Combined, and first Init
    SEQ_SEND_STARTUP,           // Send CMD 0x24 startup request
    SEQ_WAIT_STARTUP_ACK,       // Wait for 3C C3 14 24 01 00 FC
    SEQ_SEND_INIT_2,            // Send second Init (02 0B 00)
    SEQ_WAIT_INIT_ACK,          // Wait for 3C C3 14 52 04 1D FE 00 13
    SEQ_PARAM_POLL,
    SEQ_RUNNING
} ariston_seq_t;

// Enum to track which CMD 23 query was sent, to properly decode identical-length responses
typedef enum {
    ARISTON_QUERY_NONE = 0,
    ARISTON_QUERY_WATER_TEMP,
    ARISTON_QUERY_SETPOINT,
    ARISTON_QUERY_TELEMETRY,
    ARISTON_QUERY_POWER,
    ARISTON_QUERY_ON_TIME,
    ARISTON_QUERY_STATUS3
} ariston_query_t;

// Maximum payload size (largest observed in logs is ~20 bytes)
#define ARISTON_MAX_PAYLOAD 64

// Driver context
typedef struct {
    // Values we track / set
    float current_temp;  // current temperature reported by boiler LCD
    float target_temp;   // desired target setpoint
    float water_temp;    // water temperature reported by boiler
    int   heater_power;  // effective heater watts (activity-gated)
    int   heater_power_nominal; // nominal heater watts from power block
    int   heater_active; // telemetry activity flag: 0=off, 1=on, -1=unknown
    int   time_to_temp;  // time-to-temp minutes reported by boiler
    int   showers;       // number of showers reported by boiler
    int   on_time;       // derived heater-active runtime (minutes), clock-based
    float energy_today_wh; // day-bucket energy estimate (Wh)

    // Boiler clock decode (STATUS3 + ONTIME minute)
    int   clock_year2;      // 00..99
    int   clock_month;      // 1..12
    int   clock_day;        // 1..31
    int   clock_hour;       // 0..23
    int   clock_minute;     // 0..59
    int   clock_weekday_raw; // Sunday=0
    int   clock_status3_raw0; // STATUS3 byte0, unknown semantics
    int   clock_has_status3;
    int   clock_has_minute;
    int   clock_using_uptime_fallback;
    uint32_t clock_status3_tick;
    uint32_t clock_minute_tick;

    ariston_query_t last_temp_query; // tracks the type of the last CMD 23 sent

    // RX parser state
    ariston_state_t rx_state;
    int     rx_idx;
    uint8_t rx_cmd;
    uint8_t rx_len;
    uint8_t rx_payload[ARISTON_MAX_PAYLOAD];
    uint8_t current_checksum;
} ariston_ctx_t;

void Ariston_Init(void);
void Ariston_OnEverySecond(void);
void Ariston_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void Ariston_RunFrame(void);
void Ariston_OnHassDiscovery(const char *topic);
bool Ariston_SetIdentityFromStrings(const char *macStr, const char *snStr);

#endif // ENABLE_DRIVER_ARISTON

#endif /* __DRV_ARISTON_H__ */

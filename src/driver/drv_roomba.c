/*
	* Roomba Open Interface (OI) driver for OpenBeken.
	*
	* Scope:
	* - UART OI integration at 115200 8N1.
	* - Sensor polling with Group 100 as primary and Group 6 fallback.
	* - Optional IR Left/Right opcode polling (packets 52/53) in fallback mode.
	* - Parsing and publishing of core and extended telemetry (packets 7..58).
	*
	* Telemetry and integration:
	* - Per-sensor MQTT topics with change/heartbeat publish control.
	* - Vacuum state/attribute JSON topics for Home Assistant vacuum entity.
	* - Home Assistant discovery payloads for sensors, controls, and vacuum.
	* - HTTP index status panel with controls, activity, health, errors, and run time.
	*
	* Runtime behavior:
	* - State inference (idle, cleaning, paused, returning, docked, error).
	* - Keep-alive handling to preserve control during long idle windows.
	* - RX guardrails (stall/orphan/malformed/tail handling) with error counters.
	* - Combined runtime fault classifier from multiple sensor conditions.
	*
	* Control commands:
	* - High-level orders: start, pause, stop, dock, spot, locate.
	* - OI mode and motion: safe/full, drive, drive direct.
	* - Raw command interfaces: single opcode and multi-byte frame send.
	* - UI controls: LEDs, 7-segment digits (raw/ascii), and locate beep song.
	*
	* Reference:
	* - iRobot Roomba Open Interface (OI) specification.
	*/

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include <math.h>  // For fabs()
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"
#include "drv_deviceclock.h"
#include "drv_roomba.h"

// Explicit declarations to avoid implicit declaration warnings if headers are limited
#ifndef OBK_PUBLISH_FLAG_RAW_TOPIC_NAME
#define OBK_PUBLISH_FLAG_RAW_TOPIC_NAME 8
#endif
extern int MQTT_PublishMain_StringString(const char* sChannel, const char* valueStr, int flags);
extern int MQTT_Publish(const char* sTopic, const char* sChannel, const char* sVal, int flags);
extern void Main_ScheduleHomeAssistantDiscovery(int seconds);

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// UART port index (1 = UART1/TX1/RX1, 2 = UART2/TX2/RX2)
static int g_roomba_uart = 1;
static byte s_frame[128];
static int  s_frameLen = 0;
static int  s_rxExpectedLen = 80;
typedef enum {
	ROOMBA_RX_GROUP6 = 0,
	ROOMBA_RX_IR_LR  = 1
} roomba_rx_kind_t;
static roomba_rx_kind_t s_rxKind = ROOMBA_RX_GROUP6;
static int s_groupPacketId = PACKET_GROUP_100;
static int s_groupExpectedLen = 80;
static int s_group100FailCount = 0;
static int s_group100RetrySeconds = 0;

// --- helper functions ---------------------------------
static const char *Roomba_ChargingStateStr(int st) {
	switch (st) {
		case 0: return "Not charging";
		case 1: return "Reconditioning";
		case 2: return "Full charging";
		case 3: return "Trickle charging";
		case 4: return "Waiting";
		case 5: return "Charging fault";
		default: return "Unknown";
	}
}

static const char *Roomba_IrOmniStr(int ir) {
	switch (ir) {
		case 0:   return "None";
		case 161: return "Force Field";
		case 164: return "Green Buoy";
		case 165: return "Green+Force";
		case 168: return "Red Buoy";
		case 169: return "Red+Force";
		case 172: return "Red+Green";
		case 173: return "Red+Green+Force";
		case 242: return "Discovery Force Field";
		case 244: return "Discovery Green Buoy";
		case 246: return "Discovery Green+Force";
		case 248: return "Discovery Red Buoy";
		case 250: return "Discovery Red+Force";
		case 252: return "Discovery Red+Green";
		case 254: return "Discovery Red+Green+Force";
		default:  return "Other";
	}
}

static const char *Roomba_OiModeStr(int mode) {
	// OI mode values per Roomba OI spec:
	// 0=Off, 1=Passive, 2=Safe, 3=Full
	switch (mode) {
		case 0: return "Off";
		case 1: return "Passive";
		case 2: return "Safe";
		case 3: return "Full";
		default: return "Unknown";
	}
}

typedef enum {
	ROOMBA_DRV_ERR_NONE = 0,
	ROOMBA_DRV_ERR_RX_MALFORMED = 1,
	ROOMBA_DRV_ERR_RX_STALL_RESET = 2,
	ROOMBA_DRV_ERR_RX_ORPHAN_DROP = 3,
	ROOMBA_DRV_ERR_RX_UNEXPECTED_TAIL = 4,
	ROOMBA_DRV_ERR_GROUP100_FALLBACK = 5,
	ROOMBA_DRV_ERR_RUNTIME_COMBINED = 6
} roomba_drv_error_t;

static const char *Roomba_DrvErrorStr(roomba_drv_error_t err) {
	switch (err) {
		case ROOMBA_DRV_ERR_NONE: return "OK";
		case ROOMBA_DRV_ERR_RX_MALFORMED: return "RX malformed frame";
		case ROOMBA_DRV_ERR_RX_STALL_RESET: return "RX stall reset";
		case ROOMBA_DRV_ERR_RX_ORPHAN_DROP: return "RX orphan drop";
		case ROOMBA_DRV_ERR_RX_UNEXPECTED_TAIL: return "RX unexpected tail";
		case ROOMBA_DRV_ERR_GROUP100_FALLBACK: return "Group100 fallback";
		case ROOMBA_DRV_ERR_RUNTIME_COMBINED: return "Runtime combined fault";
		default: return "Unknown";
	}
}

typedef enum {
	ROOMBA_RUNTIME_ERR_NONE = 0,
	ROOMBA_RUNTIME_ERR_CLIFF = 1,
	ROOMBA_RUNTIME_ERR_WHEELDROP = 2,
	ROOMBA_RUNTIME_ERR_STASIS = 4,
	ROOMBA_RUNTIME_ERR_OVERCURRENT = 8
} roomba_runtime_error_t;

static const char *Roomba_RuntimeErrorStr(roomba_runtime_error_t err) {
	/* Cast to int: combined-flag cases (e.g. CLIFF|WHEELDROP) are valid int
	 * values but outside the enum range; strict GCC -Werror=switch rejects them
	 * when the switch expression has an enum type. */
	switch ((int)err) {
		case ROOMBA_RUNTIME_ERR_NONE: return "OK";
		case ROOMBA_RUNTIME_ERR_CLIFF: return "Cliff";
		case ROOMBA_RUNTIME_ERR_WHEELDROP: return "Wheeldrop";
		case ROOMBA_RUNTIME_ERR_STASIS: return "Stasis";
		case ROOMBA_RUNTIME_ERR_OVERCURRENT: return "Overcurrent";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_WHEELDROP): return "Cliff, Wheeldrop";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_STASIS): return "Cliff, Stasis";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_OVERCURRENT): return "Cliff, Overcurrent";
		case (ROOMBA_RUNTIME_ERR_WHEELDROP | ROOMBA_RUNTIME_ERR_STASIS): return "Wheeldrop, Stasis";
		case (ROOMBA_RUNTIME_ERR_WHEELDROP | ROOMBA_RUNTIME_ERR_OVERCURRENT): return "Wheeldrop, Overcurrent";
		case (ROOMBA_RUNTIME_ERR_STASIS | ROOMBA_RUNTIME_ERR_OVERCURRENT): return "Stasis, Overcurrent";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_WHEELDROP | ROOMBA_RUNTIME_ERR_STASIS): return "Cliff, Wheeldrop, Stasis";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_WHEELDROP | ROOMBA_RUNTIME_ERR_OVERCURRENT): return "Cliff, Wheeldrop, Overcurrent";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_STASIS | ROOMBA_RUNTIME_ERR_OVERCURRENT): return "Cliff, Stasis, Overcurrent";
		case (ROOMBA_RUNTIME_ERR_WHEELDROP | ROOMBA_RUNTIME_ERR_STASIS | ROOMBA_RUNTIME_ERR_OVERCURRENT): return "Wheeldrop, Stasis, Overcurrent";
		case (ROOMBA_RUNTIME_ERR_CLIFF | ROOMBA_RUNTIME_ERR_WHEELDROP | ROOMBA_RUNTIME_ERR_STASIS | ROOMBA_RUNTIME_ERR_OVERCURRENT):
			return "Cliff, Wheeldrop, Stasis, Overcurrent";
		default: return "Unknown";
	}
}

static void Roomba_LogPartialFrameSnapshot(const char *reason, int frameLen, uint32_t stalledTicks) {
	char hex[16 * 3 + 1];
	int n = frameLen;
	if (n > 16) {
		n = 16;
	}
	if (n < 1) {
		return;
	}
	int o = 0;
	for (int i = 0; i < n; i++) {
		o += snprintf(hex + o, sizeof(hex) - o, "%02X%s",
			(unsigned int)s_frame[i], (i == (n - 1)) ? "" : " ");
		if (o >= (int)sizeof(hex)) {
			break;
		}
	}
	addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
		"Roomba: %s partial frame len=%d exp=%d kind=%d stall=%dms first=%s",
		reason, frameLen, s_rxExpectedLen, (int)s_rxKind,
		(int)(stalledTicks * portTICK_PERIOD_MS), hex);
}

// Vacuum state for Home Assistant integration
static vacuum_state_t g_vacuum_state = VACUUM_STATE_IDLE;

// MQTT publishing control
static int stat_updatesSkipped = 0;
static int stat_updatesSent = 0;
static int changeSendAlwaysFrames = 60;  // Force publish every 60 seconds
static int changeDoNotSendMinFrames = 5; // Minimum 5 seconds between publishes

// Functions moved to end of file to ensure g_sensors is defined

// Polling state machine
static int g_ir_lr_poll_counter = 0;
static int g_rx_last_pending = 0;
static int g_rx_last_pending_uart = 0;
static int g_rx_last_pending_frame = 0;
static uint32_t g_rx_pending_since_tick = 0;
static uint32_t g_rx_last_skip_log_tick = 0;
static uint32_t g_rx_tail_drop_last_log_tick = 0;
static uint32_t g_rx_malformed_frames_total = 0;
static uint32_t g_rx_stall_resets_total = 0;
static uint32_t g_rx_orphan_drops_total = 0;
static uint32_t g_rx_unexpected_tail_drops_total = 0;
static uint32_t g_group100_fallback_total = 0;
static uint32_t g_runtime_combined_fault_total = 0;
static roomba_drv_error_t g_roomba_last_error = ROOMBA_DRV_ERR_NONE;
static uint32_t g_roomba_last_error_tick = 0;
static uint32_t g_roomba_error_dirty = 0;
static uint32_t g_roomba_last_error_report_pub_tick = 0;
static int g_group100_retry_pending = 0;
static int g_runtime_error_active = 0;
static roomba_runtime_error_t g_runtime_error_reason = ROOMBA_RUNTIME_ERR_NONE;
static uint32_t g_runtime_combo_since_tick = 0;
static uint32_t g_run_active_since_tick = 0;
static uint32_t g_run_current_duration_sec = 0;
static uint32_t g_run_last_duration_sec = 0;
static uint32_t g_run_active_since_epoch = 0;
static uint32_t g_run_last_start_epoch = 0;
static uint32_t g_run_last_end_epoch = 0;
static vacuum_state_t g_run_last_state = VACUUM_STATE_IDLE;
static int g_poll_pause_seconds = 0;
static uint32_t g_next_poll_tick = 0;
static int g_poll_interval_ticks = 0; // runtime poll cadence (2Hz target)
static uint32_t g_poll_block_until_tick = 0; // short quiet window after keepalive command burst

// Keep-awake: send OI opcode 128 (0x80) every 240 seconds
static int g_roomba_keepalive_sec = 0;
static int g_roomba_keepalive_interval_sec = 10;

// Last parsed OI Group 6 charging_state (0..5). Used for keepalive gating.
static int g_roomba_last_charging_state = -1;
static int g_roomba_last_charging_sources = -1; // Packet 34
static int g_roomba_last_oi_mode = -1;          // Packet 35
static int g_roomba_last_ir_omni = -1;
static int g_roomba_last_ir_left = -1;
static int g_roomba_last_ir_right = -1;
static int g_roomba_last_overcurrents = -1;    // Packet 14 bitmask
static int g_roomba_last_buttons = -1;          // Packet 18
static int g_roomba_last_encoder_left = 0;      // Packet 43
static int g_roomba_last_encoder_right = 0;     // Packet 44
static int g_roomba_last_light_bumper = -1;     // Packet 45
static int g_roomba_last_left_motor_current = 0;   // Packet 54
static int g_roomba_last_right_motor_current = 0;  // Packet 55
static int g_roomba_last_main_brush_current = 0;   // Packet 56
static int g_roomba_last_side_brush_current = 0;   // Packet 57
static int g_roomba_last_stasis = -1;           // Packet 58

// ============================================================================
// SENSOR REGISTRY
// ============================================================================

// Sensor definitions with auto-publishing metadata
// This array defines all sensors that will be automatically published to MQTT
// Sensor definitions with auto-publishing metadata
// This array defines all sensors that will be automatically published to MQTT
static roomba_sensor_t g_sensors[ROOMBA_SENSOR__COUNT] = {
	// name_friendly, name_mqtt, units, hass_dev_class, decimals, threshold, lastRead, lastSent, noChange, packet_id, data_size, is_binary
	{"Battery Voltage", "battery_voltage", "mV", "voltage", 0, 100.0f, 0, 0, 0, PACKET_VOLTAGE, 2, false},
	{"Current", "battery_current", "mA", "current", 0, 50.0f, 0, 0, 0, PACKET_CURRENT, 2, false},
	{"Temperature", "temperature", "deg C", "temperature", 1, 1.0f, 0, 0, 0, PACKET_TEMPERATURE, 1, false},
	{"Battery Charge", "battery_charge", "mAh", "", 0, 10.0f, 0, 0, 0, PACKET_CHARGE, 2, false},
	{"Battery Capacity", "battery_capacity", "mAh", "", 0, 10.0f, 0, 0, 0, PACKET_CAPACITY, 2, false},
	{"Charging State", "charging_state", "", "enum", 0, 0.5f, 0, 0, 0, PACKET_CHARGING_STATE, 1, false},
	{"Bump Right", "bump_right", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"Bump Left", "bump_left", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"Wheeldrop Right", "wheeldrop_right", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"Wheeldrop Left", "wheeldrop_left", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"Wall", "", "", "occupancy", 0, 0.5f, 0, 0, 0, PACKET_WALL, 1, true},
	{"Cliff Left", "cliff_left", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_LEFT, 1, true},
	{"Cliff Front Left", "cliff_front_left", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_FL, 1, true},
	{"Cliff Front Right", "cliff_front_right", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_FR, 1, true},
	{"Cliff Right", "cliff_right", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_RIGHT, 1, true},
	{"Virtual Wall", "virtual_wall", "", "occupancy", 0, 0.5f, 0, 0, 0, PACKET_VIRTUAL_WALL, 1, true},
	{"Overcurrent Side Brush", "", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_OVERCURRENTS, 1, true},
	{"Overcurrent Vacuum", "", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_OVERCURRENTS, 1, true},
	{"Overcurrent Main Brush", "", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_OVERCURRENTS, 1, true},
	{"Overcurrent Drive Right", "", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_OVERCURRENTS, 1, true},
	{"Overcurrent Drive Left", "", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_OVERCURRENTS, 1, true},
	{"Dirt Detect", "", "", "enum", 0, 1.0f, 0, 0, 0, PACKET_DIRT_DETECT, 1, false},
	{"Buttons", "", "", "", 0, 0.5f, 0, 0, 0, PACKET_BUTTONS, 1, false},
	{"Wall Signal", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_WALL_SIGNAL, 2, false},
	{"Cliff Left Signal", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_CLIFF_LEFT_SIGNAL, 2, false},
	{"Cliff Front Left Signal", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_CLIFF_FL_SIGNAL, 2, false},
	{"Cliff Front Right Signal", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_CLIFF_FR_SIGNAL, 2, false},
	{"Cliff Right Signal", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_CLIFF_RIGHT_SIGNAL, 2, false},
	{"Velocity", "", "mm/s", "", 0, 5.0f, 0, 0, 0, PACKET_REQ_VELOCITY, 2, false},
	{"Radius", "", "mm", "", 0, 5.0f, 0, 0, 0, PACKET_REQ_RADIUS, 2, false},
	{"Velocity Right", "", "mm/s", "", 0, 5.0f, 0, 0, 0, PACKET_REQ_RIGHT_VEL, 2, false},
	{"Velocity Left", "", "mm/s", "", 0, 5.0f, 0, 0, 0, PACKET_REQ_LEFT_VEL, 2, false},
	{"Distance", "", "mm", "distance", 0, 1.0f, 0, 0, 0, PACKET_DISTANCE, 2, false},
	{"Angle", "", "deg", "", 0, 1.0f, 0, 0, 0, PACKET_ANGLE, 2, false},
	{"Encoder Counts Left", "", "counts", "", 0, 10.0f, 0, 0, 0, PACKET_ENCODER_LEFT, 2, false},
	{"Encoder Counts Right", "", "counts", "", 0, 10.0f, 0, 0, 0, PACKET_ENCODER_RIGHT, 2, false},
	{"Light Bumper", "", "", "", 0, 1.0f, 0, 0, 0, PACKET_LIGHT_BUMPER, 1, false},
	{"Light Bump Left", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_LIGHT_BUMP_LEFT, 2, false},
	{"Light Bump Front Left", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_LIGHT_BUMP_FL, 2, false},
	{"Light Bump Center Left", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_LIGHT_BUMP_CL, 2, false},
	{"Light Bump Center Right", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_LIGHT_BUMP_CR, 2, false},
	{"Light Bump Front Right", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_LIGHT_BUMP_FR, 2, false},
	{"Light Bump Right", "", "raw", "", 0, 20.0f, 0, 0, 0, PACKET_LIGHT_BUMP_RIGHT, 2, false},
	{"IR Opcode Left", "", "", "", 0, 1.0f, 0, 0, 0, PACKET_IR_LEFT, 1, false},
	{"IR Opcode Right", "", "", "", 0, 1.0f, 0, 0, 0, PACKET_IR_RIGHT, 1, false},
	{"Left Motor Current", "", "mA", "current", 0, 30.0f, 0, 0, 0, PACKET_LEFT_MOTOR_CURRENT, 2, false},
	{"Right Motor Current", "", "mA", "current", 0, 30.0f, 0, 0, 0, PACKET_RIGHT_MOTOR_CURRENT, 2, false},
	{"Main Brush Current", "", "mA", "current", 0, 30.0f, 0, 0, 0, PACKET_MAIN_BRUSH_CURRENT, 2, false},
	{"Side Brush Current", "", "mA", "current", 0, 30.0f, 0, 0, 0, PACKET_SIDE_BRUSH_CURRENT, 2, false},
	{"Stasis", "stasis", "", "enum", 0, 1.0f, 0, 0, 0, PACKET_STASIS, 1, false},
};

// Packet 34 (Charging Sources Available) from Group 6:
// bit0=internal charger, bit1=home base on Create-family docs.
// For Roomba 600, source visibility can vary, so we use "any source bit" as strongest signal,
// then fallback to charging-state/current heuristic.
static int Roomba_IsDockedEffective(void) {
	int chg = (int)g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading;
	float current = g_sensors[ROOMBA_SENSOR_CURRENT].lastReading;
	int src = g_roomba_last_charging_sources;

	if (src > 0) {
		return 1;
	}

	// WAITING alone is not sufficient; it often persists after undock.
	if (chg == CHARGING_WAITING) {
		return 0;
	}

	// True charging modes generally imply dock if current isn't strongly negative.
	if ((chg == CHARGING_RECOND || chg == CHARGING_FULL || chg == CHARGING_TRICKLE || chg == CHARGING_FAULT)
		&& current > -120) {
		return 1;
	}

	return 0;
}

static int Roomba_GetEffectiveChargingState(void) {
	int chg = (int)g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading;
	if (chg == CHARGING_WAITING && !Roomba_IsDockedEffective()) {
		return CHARGING_NOT;
	}
	return chg;
}

static uint32_t g_roomba_lastVacStatePubMs = 0;

static uint32_t Roomba_GetErrorTotalCount(void) {
	return g_rx_malformed_frames_total +
		g_rx_stall_resets_total +
		g_rx_orphan_drops_total +
		g_rx_unexpected_tail_drops_total +
		g_group100_fallback_total +
		g_runtime_combined_fault_total;
}

static void Roomba_RecordDriverError(roomba_drv_error_t err) {
	switch (err) {
		case ROOMBA_DRV_ERR_RX_MALFORMED:
			g_rx_malformed_frames_total++;
			break;
		case ROOMBA_DRV_ERR_RX_STALL_RESET:
			g_rx_stall_resets_total++;
			break;
		case ROOMBA_DRV_ERR_RX_ORPHAN_DROP:
			g_rx_orphan_drops_total++;
			break;
		case ROOMBA_DRV_ERR_RX_UNEXPECTED_TAIL:
			g_rx_unexpected_tail_drops_total++;
			break;
		case ROOMBA_DRV_ERR_GROUP100_FALLBACK:
			g_group100_fallback_total++;
			break;
		case ROOMBA_DRV_ERR_RUNTIME_COMBINED:
			g_runtime_combined_fault_total++;
			break;
		default:
			break;
	}
	g_roomba_last_error = err;
	g_roomba_last_error_tick = xTaskGetTickCount();
	g_roomba_error_dirty = 1;
}

static roomba_runtime_error_t Roomba_GetRuntimeComboReason(
	int cliffHazard, int wheelDrop, int stasisNoProgress, int overcurrent) {
	int bits = 0;
	if (cliffHazard) {
		bits |= ROOMBA_RUNTIME_ERR_CLIFF;
	}
	if (wheelDrop) {
		bits |= ROOMBA_RUNTIME_ERR_WHEELDROP;
	}
	if (stasisNoProgress) {
		bits |= ROOMBA_RUNTIME_ERR_STASIS;
	}
	if (overcurrent) {
		bits |= ROOMBA_RUNTIME_ERR_OVERCURRENT;
	}
	// Must be at least 2 active signals to classify as runtime error.
	{
		int count = 0;
		count += cliffHazard ? 1 : 0;
		count += wheelDrop ? 1 : 0;
		count += stasisNoProgress ? 1 : 0;
		count += overcurrent ? 1 : 0;
		if (count < 2) {
			return ROOMBA_RUNTIME_ERR_NONE;
		}
	}
	return (roomba_runtime_error_t)bits;
}

static void Roomba_UpdateRuntimeErrorClassifier(uint32_t nowTick) {
	uint32_t triggerTicks = (uint32_t)(configTICK_RATE_HZ * 5); // require >5s persistence
	int movingIntent = (g_vacuum_state == VACUUM_STATE_CLEANING || g_vacuum_state == VACUUM_STATE_RETURNING) ? 1 : 0;
	// If the robot is Docked or effectively charging, it is safe.
	// We force-clear any runtime errors to prevent "Stasis+Wheeldrop" from triggering while sitting on the dock.
	int isDocked = (g_vacuum_state == VACUUM_STATE_DOCKED) || Roomba_IsDockedEffective();

	if (isDocked) {
		if (g_runtime_error_active) {
			g_runtime_error_active = 0;
			g_runtime_error_reason = ROOMBA_RUNTIME_ERR_NONE;
			g_roomba_error_dirty = 1;
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Runtime error cleared (Docked/Charging)");
		}
		g_runtime_combo_since_tick = 0;
		return;
	}

	int stasis = (int)g_sensors[ROOMBA_SENSOR_STASIS].lastReading;
	int cliffHazard = 0;
	int wheelDrop = 0;
	int stasisNoProgress = 0;
	int overcurrent = 0;
	int signalCount = 0;
	roomba_runtime_error_t reason = ROOMBA_RUNTIME_ERR_NONE;

	if (triggerTicks < 1) {
		triggerTicks = 1;
	}

	cliffHazard =
		(g_sensors[ROOMBA_SENSOR_CLIFF_LEFT].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_CLIFF_FL].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_CLIFF_FR].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_CLIFF_RIGHT].lastReading > 0.5f);
	wheelDrop =
		(g_sensors[ROOMBA_SENSOR_WHEELDROP_LEFT].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_WHEELDROP_RIGHT].lastReading > 0.5f);
	// Stasis is now detected regardless of movingIntent to allow latching errors when the robot stops
	stasisNoProgress = (stasis == 0 || stasis == 2) ? 1 : 0;
	overcurrent =
		(g_sensors[ROOMBA_SENSOR_OVERCURRENT_SIDE_BRUSH].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_OVERCURRENT_VACUUM].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_OVERCURRENT_MAIN_BRUSH].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_OVERCURRENT_DRIVE_RIGHT].lastReading > 0.5f) ||
		(g_sensors[ROOMBA_SENSOR_OVERCURRENT_DRIVE_LEFT].lastReading > 0.5f);

	signalCount = (cliffHazard ? 1 : 0) + (wheelDrop ? 1 : 0) + (stasisNoProgress ? 1 : 0) + (overcurrent ? 1 : 0);
	reason = Roomba_GetRuntimeComboReason(cliffHazard, wheelDrop, stasisNoProgress, overcurrent);

	if (signalCount >= 2 && reason != ROOMBA_RUNTIME_ERR_NONE) {
		if (g_runtime_combo_since_tick == 0) {
			g_runtime_combo_since_tick = nowTick;
		}
		if (!g_runtime_error_active && (nowTick - g_runtime_combo_since_tick) >= triggerTicks) {
			g_runtime_error_active = 1;
			g_runtime_error_reason = reason;
			Roomba_RecordDriverError(ROOMBA_DRV_ERR_RUNTIME_COMBINED);
			addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
				"Roomba: Runtime combined fault active: %s (cliff=%d wheeldrop=%d stasis=%d overcurrent=%d)",
				Roomba_RuntimeErrorStr(reason), cliffHazard, wheelDrop, stasisNoProgress, overcurrent);
		} else if (g_runtime_error_active && g_runtime_error_reason != reason) {
			g_runtime_error_reason = reason;
			g_roomba_error_dirty = 1;
		}
	} else {
		g_runtime_combo_since_tick = 0;
		if (g_runtime_error_active) {
			g_runtime_error_active = 0;
			g_runtime_error_reason = ROOMBA_RUNTIME_ERR_NONE;
			g_roomba_error_dirty = 1;
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Runtime combined fault cleared");
		}
	}
}

static int Roomba_IsWorkingState(vacuum_state_t st) {
	return (st == VACUUM_STATE_CLEANING || st == VACUUM_STATE_RETURNING) ? 1 : 0;
}

static void Roomba_UpdateRunDurationTracker(uint32_t nowTick) {
	int wasWorking = Roomba_IsWorkingState(g_run_last_state);
	int isWorking = Roomba_IsWorkingState(g_vacuum_state);
	uint32_t nowEpoch = TIME_IsTimeSynced() ? TIME_GetCurrentTimeWithoutOffset() : 0;

	if (!wasWorking && isWorking) {
		g_run_active_since_tick = nowTick;
		g_run_current_duration_sec = 0;
		g_run_active_since_epoch = nowEpoch;
		if (nowEpoch != 0) {
			g_run_last_start_epoch = nowEpoch;
		}
	}
	if (wasWorking && !isWorking) {
		if (g_run_active_since_tick != 0) {
			g_run_last_duration_sec = (nowTick - g_run_active_since_tick) * portTICK_PERIOD_MS / 1000;
		}
		if (nowEpoch != 0 && g_run_active_since_epoch != 0) {
			g_run_last_start_epoch = g_run_active_since_epoch;
			g_run_last_end_epoch = nowEpoch;
		}
		g_run_active_since_tick = 0;
		g_run_active_since_epoch = 0;
		g_run_current_duration_sec = 0;
	}
	if (isWorking && g_run_active_since_tick != 0) {
		g_run_current_duration_sec = (nowTick - g_run_active_since_tick) * portTICK_PERIOD_MS / 1000;
	}

	g_run_last_state = g_vacuum_state;
}

static void Roomba_FormatDurationHm(char *out, int outSize, uint32_t totalSec) {
	uint32_t minutes = totalSec / 60;
	uint32_t hours = minutes / 60;
	minutes = minutes % 60;
	snprintf(out, outSize, "%lu:%02lu", (unsigned long)hours, (unsigned long)minutes);
}

typedef enum {
	ROOMBA_HEALTH_OK = 0,
	ROOMBA_HEALTH_DEGRADED = 1,
	ROOMBA_HEALTH_ERROR = 2
} roomba_health_t;

static roomba_health_t Roomba_GetHealthCode(uint32_t nowTick) {
	uint32_t recentErrorWindowTicks = (uint32_t)(configTICK_RATE_HZ * 60);
	int fallbackActive = (s_groupPacketId != PACKET_GROUP_100);
	if (recentErrorWindowTicks < 1) {
		recentErrorWindowTicks = 1;
	}

	if (g_vacuum_state == VACUUM_STATE_ERROR) {
		return ROOMBA_HEALTH_ERROR;
	}
	if (g_runtime_error_active) {
		return ROOMBA_HEALTH_ERROR;
	}
	if (fallbackActive) {
		return ROOMBA_HEALTH_DEGRADED;
	}
	if (g_roomba_last_error_tick != 0 &&
		(nowTick - g_roomba_last_error_tick) < recentErrorWindowTicks) {
		return ROOMBA_HEALTH_DEGRADED;
	}
	return ROOMBA_HEALTH_OK;
}

static const char *Roomba_HealthStr(roomba_health_t health) {
	switch (health) {
		case ROOMBA_HEALTH_OK: return "Ok";
		case ROOMBA_HEALTH_DEGRADED: return "Degraded";
		case ROOMBA_HEALTH_ERROR: return "Error";
		default: return "Unknown";
	}
}

static const char *Roomba_GetSensorIcon(int sensorIndex) {
	switch (sensorIndex) {
		case ROOMBA_SENSOR_CHARGE:
			return "mdi:battery-high";
		case ROOMBA_SENSOR_CAPACITY:
			return "mdi:battery";
		case ROOMBA_SENSOR_BUTTONS:
			return "mdi:gesture-tap-button";
		case ROOMBA_SENSOR_DIRT_DETECT:
			return "mdi:shimmer";
		case ROOMBA_SENSOR_WALL_SIGNAL:
		case ROOMBA_SENSOR_CLIFF_LEFT_SIGNAL:
		case ROOMBA_SENSOR_CLIFF_FL_SIGNAL:
		case ROOMBA_SENSOR_CLIFF_FR_SIGNAL:
		case ROOMBA_SENSOR_CLIFF_RIGHT_SIGNAL:
			return "mdi:signal-distance-variant";
		case ROOMBA_SENSOR_REQ_VELOCITY:
		case ROOMBA_SENSOR_REQ_RIGHT_VEL:
		case ROOMBA_SENSOR_REQ_LEFT_VEL:
			return "mdi:speedometer";
		case ROOMBA_SENSOR_REQ_RADIUS:
			return "mdi:radius";
		case ROOMBA_SENSOR_ANGLE:
			return "mdi:rotate-right";
		case ROOMBA_SENSOR_ENCODER_LEFT:
		case ROOMBA_SENSOR_ENCODER_RIGHT:
			return "mdi:counter";
		case ROOMBA_SENSOR_LIGHT_BUMPER:
			return "mdi:car-emergency";
		case ROOMBA_SENSOR_LIGHT_BUMP_LEFT:
		case ROOMBA_SENSOR_LIGHT_BUMP_FL:
		case ROOMBA_SENSOR_LIGHT_BUMP_CL:
		case ROOMBA_SENSOR_LIGHT_BUMP_CR:
		case ROOMBA_SENSOR_LIGHT_BUMP_FR:
		case ROOMBA_SENSOR_LIGHT_BUMP_RIGHT:
			return "mdi:brightness-5";
		case ROOMBA_SENSOR_IR_LEFT:
		case ROOMBA_SENSOR_IR_RIGHT:
			return "mdi:remote";
		case ROOMBA_SENSOR_STASIS:
			return "mdi:motion-sensor";
		default:
			return NULL;
	}
}

static int Roomba_IsTextSensor(int sensorIndex) {
	switch (sensorIndex) {
		case ROOMBA_SENSOR_CHARGING_STATE:
		case ROOMBA_SENSOR_DIRT_DETECT:
		case ROOMBA_SENSOR_BUTTONS:
		case ROOMBA_SENSOR_LIGHT_BUMPER:
		case ROOMBA_SENSOR_IR_LEFT:
		case ROOMBA_SENSOR_IR_RIGHT:
		case ROOMBA_SENSOR_STASIS:
			return 1;
		default:
			return 0;
	}
}

static const char *Roomba_StasisStr(int stasis) {
	switch (stasis) {
		case 0: return "Not moving";
		case 1: return "Moving";
		case 2: return "Dirty sensor";
		case 3: return "Reserved";
		default: return "Unknown";
	}
}

static const char *Roomba_DirtDetectStr(int dirt) {
	return dirt > 0 ? "Detected" : "Clear";
}

static const char *Roomba_ButtonsStr(int buttons, char *out, int outSize) {
	static const char *names[8] = {
		"clean", "spot", "dock", "minute", "hour", "day", "schedule", "clock"
	};
	int first = 1;
	if (outSize < 1) {
		return "";
	}
	if ((buttons & 0xFF) == 0) {
		snprintf(out, outSize, "None");
		return out;
	}
	out[0] = 0;
	for (int bit = 0; bit < 8; bit++) {
		if ((buttons & (1 << bit)) == 0) {
			continue;
		}
		if (!first) {
			strncat(out, ",", outSize - strlen(out) - 1);
		}
		strncat(out, names[bit], outSize - strlen(out) - 1);
		first = 0;
	}
	return out;
}

static const char *Roomba_LightBumperStr(int bumperMask, char *out, int outSize) {
	static const char *names[6] = {
		"left", "front_left", "center_left", "center_right", "front_right", "right"
	};
	int first = 1;
	if (outSize < 1) {
		return "";
	}
	if ((bumperMask & 0x3F) == 0) {
		snprintf(out, outSize, "None");
		return out;
	}
	out[0] = 0;
	for (int bit = 0; bit < 6; bit++) {
		if ((bumperMask & (1 << bit)) == 0) {
			continue;
		}
		if (!first) {
			strncat(out, ", ", outSize - strlen(out) - 1);
		}
		// Manual sentence case with capitalization after comma
		int startIdx = strlen(out);
		strncat(out, names[bit], outSize - strlen(out) - 1);
		if (out[startIdx] >= 'a' && out[startIdx] <= 'z') {
			out[startIdx] -= 32;
		}
		// Replace underscores with spaces
		for (int i = startIdx; out[i]; i++) {
			if (out[i] == '_') out[i] = ' ';
		}
		first = 0;
	}
	return out;
}

static const char *Roomba_IrCodeStr(int ir, char *out, int outSize) {
	const char *name = Roomba_IrOmniStr(ir);
	if (strcmp(name, "Other") == 0) {
		snprintf(out, outSize, "Code %d", ir);
		return out;
	}
	return name;
}

static const char *Roomba_GetTextSensorValue(int sensorIndex, float currentReading, char *out, int outSize) {
	int v = (int)currentReading;
	switch (sensorIndex) {
		case ROOMBA_SENSOR_CHARGING_STATE:
			return Roomba_ChargingStateStr(Roomba_GetEffectiveChargingState());
		case ROOMBA_SENSOR_STASIS:
			return Roomba_StasisStr(v);
		case ROOMBA_SENSOR_DIRT_DETECT:
			return Roomba_DirtDetectStr(v);
		case ROOMBA_SENSOR_BUTTONS:
			return Roomba_ButtonsStr(v, out, outSize);
		case ROOMBA_SENSOR_LIGHT_BUMPER:
			return Roomba_LightBumperStr(v, out, outSize);
		case ROOMBA_SENSOR_IR_LEFT:
		case ROOMBA_SENSOR_IR_RIGHT:
			return Roomba_IrCodeStr(v, out, outSize);
		default:
			return NULL;
	}
}

static const char *Roomba_GetEnumOptionsForSensor(int sensorIndex) {
	switch (sensorIndex) {
		case ROOMBA_SENSOR_CHARGING_STATE:
			return "[\"Not charging\",\"Reconditioning\",\"Full charging\",\"Trickle charging\",\"Waiting\",\"Charging fault\",\"Unknown\"]";
		case ROOMBA_SENSOR_STASIS:
			return "[\"Not moving\",\"Moving\",\"Dirty sensor\",\"Reserved\",\"Unknown\"]";
		case ROOMBA_SENSOR_DIRT_DETECT:
			return "[\"Clear\",\"Detected\"]";
		default:
			return NULL;
	}
}

static const char *Roomba_VacuumStateStr(void) {
	switch (g_vacuum_state) {
		case VACUUM_STATE_DOCKED:    return "docked";
		case VACUUM_STATE_CLEANING:  return "cleaning";
		case VACUUM_STATE_RETURNING: return "returning";
		case VACUUM_STATE_PAUSED:    return "paused";
		case VACUUM_STATE_IDLE:      return "idle";
		case VACUUM_STATE_ERROR:     return "error";
		default:                     return "idle";
	}
}

static void Roomba_MQTT_PublishAvailability(int online) {
	MQTT_PublishMain_StringString("availability",
		online ? "online" : "offline",
		OBK_PUBLISH_FLAG_RETAIN);
}

static void Roomba_MQTT_PublishVacuumStateJSON(void) {
	char payload[512];
	uint32_t nowTick = xTaskGetTickCount();
	uint32_t nowEpoch = TIME_IsTimeSynced() ? TIME_GetCurrentTimeWithoutOffset() : 0;
	int timeSynced = (nowEpoch != 0) ? 1 : 0;

	const char *st = Roomba_VacuumStateStr();
	int chg = Roomba_GetEffectiveChargingState();
	int docked = Roomba_IsDockedEffective();
	const char *chg_str = Roomba_ChargingStateStr(chg);
	const char *error_str = "OK";
	if (g_runtime_error_active) {
		error_str = Roomba_RuntimeErrorStr(g_runtime_error_reason);
	} else if (chg == 5) {
		error_str = "Charging fault";
	}

	const char *comm_state = "OK";
	if (g_roomba_last_error != ROOMBA_DRV_ERR_NONE) {
		comm_state = Roomba_DrvErrorStr(g_roomba_last_error);
	} else if (s_groupPacketId != PACKET_GROUP_100) {
		comm_state = "Group100 fallback";
	}
	
	// Calculate battery percentage
	float batParams = 0.0f;
	if (g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading > 0) {
		batParams = (g_sensors[ROOMBA_SENSOR_CHARGE].lastReading / g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading * 100.0f);
	}

	// Keep stable JSON so HA attributes don’t thrash.
	// Includes battery_level and fan_speed for standard HA Vacuum entity support
	snprintf(payload, sizeof(payload),
		"{\"state\":\"%s\",\"battery_level\":%.0f,\"charging_state\":\"%s\",\"docked\":%s,\"error\":\"%s\",\"comm_state\":\"%s\",\"time_synced\":%s,\"timestamp_s\":%u,\"current_run_s\":%u,\"last_run_s\":%u,\"last_run_start_s\":%u,\"last_run_end_s\":%u}",
		st,
		batParams,
		chg_str,
		docked ? "true" : "false",
		error_str,
		comm_state,
		timeSynced ? "true" : "false",
		(unsigned int)nowEpoch,
		(unsigned int)g_run_current_duration_sec,
		(unsigned int)g_run_last_duration_sec,
		(unsigned int)g_run_last_start_epoch,
		(unsigned int)g_run_last_end_epoch
	);

	MQTT_PublishMain_StringString("vacuum/state", payload, 0);
	MQTT_PublishMain_StringString("vacuum/attr",  payload, 0);
}

static void Roomba_MQTT_PublishVacuumStateJSONIfNeeded(uint32_t nowMs) {
	static vacuum_state_t s_lastState = VACUUM_STATE_ERROR;
	static int s_lastBattery = -1;
	static int s_lastChargingState = -1;
	static int s_lastDocked = -1;
	static roomba_health_t s_lastHealth = ROOMBA_HEALTH_ERROR;
	// xTaskGetTickCount() is in RTOS ticks, so convert 15s to ticks.
	uint32_t heartbeatTicks = (uint32_t)(15000 / portTICK_PERIOD_MS);
	if (heartbeatTicks == 0) heartbeatTicks = 1;
	int reasonIsStateChange = 0;

	int effChg = Roomba_GetEffectiveChargingState();
	int docked = Roomba_IsDockedEffective();
	int battery = 0;
	if (g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading > 0) {
		battery = (int)((g_sensors[ROOMBA_SENSOR_CHARGE].lastReading * 100.0f) /
			g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading + 0.5f);
	}
	if (battery < 0) battery = 0;
	if (battery > 100) battery = 100;
	roomba_health_t health = Roomba_GetHealthCode(nowMs);

	if (s_lastState != g_vacuum_state ||
		s_lastBattery != battery ||
		s_lastChargingState != effChg ||
		s_lastDocked != docked ||
		s_lastHealth != health) {
		reasonIsStateChange = 1;
	}

	if (reasonIsStateChange || (nowMs - g_roomba_lastVacStatePubMs) >= heartbeatTicks) {
		g_roomba_lastVacStatePubMs = nowMs;
		Roomba_MQTT_PublishVacuumStateJSON();
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Vacuum MQTT: reason=%s",
			reasonIsStateChange ? "state_change" : "heartbeat");
		s_lastState = g_vacuum_state;
		s_lastBattery = battery;
		s_lastChargingState = effChg;
		s_lastDocked = docked;
		s_lastHealth = health;
	}
}

static void Roomba_MQTT_PublishErrorReportIfNeeded(uint32_t nowTick) {
	static int s_init = 0;
	static roomba_drv_error_t s_lastErr = ROOMBA_DRV_ERR_NONE;
	static uint32_t s_lastErrCount = 0;
	static int s_lastGroup = -1;
	static int s_lastFallback = -1;
	static int s_lastRuntimeActive = -1;
	static roomba_runtime_error_t s_lastRuntimeReason = ROOMBA_RUNTIME_ERR_NONE;
	uint32_t heartbeatTicks = (uint32_t)(configTICK_RATE_HZ * 60);
	uint32_t errCount = Roomba_GetErrorTotalCount();
	int groupMode = s_groupPacketId;
	int fallbackActive = (groupMode != PACKET_GROUP_100);
	int runtimeActive = g_runtime_error_active;
	roomba_runtime_error_t runtimeReason = g_runtime_error_reason;
	int changed = 0;

	if (heartbeatTicks < 1) {
		heartbeatTicks = 1;
	}
	if (!s_init ||
		s_lastErr != g_roomba_last_error ||
		s_lastErrCount != errCount ||
		s_lastGroup != groupMode ||
		s_lastFallback != fallbackActive ||
		s_lastRuntimeActive != runtimeActive ||
		s_lastRuntimeReason != runtimeReason ||
		g_roomba_error_dirty) {
		changed = 1;
	}

	if (changed ||
		g_roomba_last_error_report_pub_tick == 0 ||
		(nowTick - g_roomba_last_error_report_pub_tick) >= heartbeatTicks) {
		char payload[384];
		uint32_t nowEpoch = TIME_IsTimeSynced() ? TIME_GetCurrentTimeWithoutOffset() : 0;
		uint32_t deltaTicks = nowTick - g_roomba_last_error_tick;
		uint32_t deltaSec = (deltaTicks * portTICK_PERIOD_MS) / 1000;
		uint32_t lastErrEpoch = 0;
		uint32_t tsMs = g_roomba_last_error_tick * portTICK_PERIOD_MS;
		if (nowEpoch != 0 && g_roomba_last_error_tick != 0 && nowEpoch > deltaSec) {
			lastErrEpoch = nowEpoch - deltaSec;
		}
		snprintf(payload, sizeof(payload),
			"{\"last_error\":\"%s\",\"count\":%u,\"group_mode\":%d,\"fallback_active\":%s,\"runtime_error_active\":%s,\"runtime_error_reason\":\"%s\",\"timestamp_ms\":%u,\"time_synced\":%s,\"timestamp_s\":%u}",
			Roomba_DrvErrorStr(g_roomba_last_error),
			(unsigned int)errCount,
			groupMode,
			fallbackActive ? "true" : "false",
			runtimeActive ? "true" : "false",
			Roomba_RuntimeErrorStr(runtimeReason),
			(unsigned int)tsMs,
			(nowEpoch != 0) ? "true" : "false",
			(unsigned int)lastErrEpoch);
		MQTT_PublishMain_StringString("vacuum/error_report", payload, 0);
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Error report MQTT: %s", payload);

		g_roomba_last_error_report_pub_tick = nowTick;
		s_lastErr = g_roomba_last_error;
		s_lastErrCount = errCount;
		s_lastGroup = groupMode;
		s_lastFallback = fallbackActive;
		s_lastRuntimeActive = runtimeActive;
		s_lastRuntimeReason = runtimeReason;
		s_init = 1;
		g_roomba_error_dirty = 0;
	}
}



// ============================================================================
// UART COMMUNICATION FUNCTIONS
// ============================================================================

/**
	* Send a single byte to the Roomba via UART
	* @param b Byte to send
	*/
void Roomba_SendByte(byte b) {
	UART_SendByteEx(g_roomba_uart, b);
}

/**
	* Send a Roomba OI command (single opcode)
	* @param cmd Command opcode (see drv_roomba.h for definitions)
	*/
void Roomba_SendCmd(int cmd) {
	Roomba_SendByte((byte)cmd);
}

static void Roomba_PlayLocateBeep(void) {
	// Define song 0 (two short tones) and play it immediately.
	g_poll_pause_seconds = 2;
	Roomba_SendByte(CMD_SONG); // 140
	Roomba_SendByte(0);        // song number
	Roomba_SendByte(2);        // song length (notes)
	Roomba_SendByte(72);       // C5
	Roomba_SendByte(14);       // duration
	Roomba_SendByte(79);       // G5
	Roomba_SendByte(14);       // duration
	Roomba_SendByte(CMD_PLAY); // 141
	Roomba_SendByte(0);        // play song 0
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Locate beep played");
}

// ============================================================================
// CONSOLE COMMANDS
// ============================================================================

/**
	* Console command: Roomba_SendCmd <opcode>
	* Allows manual sending of Roomba OI opcodes for testing
	* Example: Roomba_SendCmd 135  (sends CLEAN command)
	*/
static commandResult_t CMD_Roomba_SendCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int op;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_SendCmd: requires 1 argument (opcode)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	op = Tokenizer_GetArgInteger(0);
	Roomba_SendCmd(op);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_SendCmd: sent opcode %i (0x%02X)", op, op);

	return CMD_RES_OK;
}

/**
	* Console command: Roomba_SendBytes <b0> <b1> ...
	* Sends a raw multi-byte frame without splitting bytes across separate requests.
	* Example (Drive Direct both wheels +120mm/s): Roomba_SendBytes 145 0 120 0 120
	*/
static commandResult_t CMD_Roomba_SendBytes(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte frame[64];
	int count, i;

	Tokenizer_TokenizeString(args, 0);
	count = Tokenizer_GetArgsCount();

	if (count < 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_SendBytes: requires at least 1 byte argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	if (count > (int)sizeof(frame)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_SendBytes: too many bytes (%d > %d)", count, (int)sizeof(frame));
		return CMD_RES_BAD_ARGUMENT;
	}

	// Validate all bytes first so we never send a partial malformed frame.
	for (i = 0; i < count; i++) {
		int v = Tokenizer_GetArgInteger(i);
		if (v < 0 || v > 255) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_SendBytes: arg %d out of range (0..255): %d", i, v);
			return CMD_RES_BAD_ARGUMENT;
		}
		frame[i] = (byte)v;
	}

	// Pause regular 1Hz polling briefly to avoid interleaving command bytes with sensor requests.
	g_poll_pause_seconds = 2;
	for (i = 0; i < count; i++) {
		Roomba_SendByte(frame[i]);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_SendBytes: sent %d bytes, poll pause=%ds", count, g_poll_pause_seconds);
	return CMD_RES_OK;
}

/**
	* Console command: Roomba_LEDs <bits> <color> <intensity>
	* bits: packet-139 LED bitmask (0-255), color: 0=green..255=red, intensity: 0-255
	*/
static commandResult_t CMD_Roomba_LEDs(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int bits, color, intensity;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 3) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_LEDs: requires 3 args: <bits> <color> <intensity>");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	bits = Tokenizer_GetArgInteger(0);
	color = Tokenizer_GetArgInteger(1);
	intensity = Tokenizer_GetArgInteger(2);
	if (bits < 0 || bits > 255 || color < 0 || color > 255 || intensity < 0 || intensity > 255) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_LEDs: all args must be in range 0..255");
		return CMD_RES_BAD_ARGUMENT;
	}

	g_poll_pause_seconds = 1;
	Roomba_SendByte(CMD_LEDS); // 139
	Roomba_SendByte((byte)bits);
	Roomba_SendByte((byte)color);
	Roomba_SendByte((byte)intensity);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_LEDs: bits=0x%02X color=%d intensity=%d", bits, color, intensity);
	return CMD_RES_OK;
}

/**
	* Console command: Roomba_DigitsRaw <d1> <d2> <d3> <d4>
	* Sends OI Digit LEDs Raw opcode 163 with four raw 7-segment bitmasks.
	*/
static commandResult_t CMD_Roomba_DigitsRaw(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int d1, d2, d3, d4;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 4) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsRaw: requires 4 args: <d1> <d2> <d3> <d4> (0..255)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	d1 = Tokenizer_GetArgInteger(0);
	d2 = Tokenizer_GetArgInteger(1);
	d3 = Tokenizer_GetArgInteger(2);
	d4 = Tokenizer_GetArgInteger(3);
	if (d1 < 0 || d1 > 255 || d2 < 0 || d2 > 255 || d3 < 0 || d3 > 255 || d4 < 0 || d4 > 255) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsRaw: all args must be in range 0..255");
		return CMD_RES_BAD_ARGUMENT;
	}

	g_poll_pause_seconds = 1;
	Roomba_SendByte(CMD_DIGIT_LEDS_RAW); // 163
	Roomba_SendByte((byte)d1);
	Roomba_SendByte((byte)d2);
	Roomba_SendByte((byte)d3);
	Roomba_SendByte((byte)d4);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsRaw: %d %d %d %d", d1, d2, d3, d4);
	return CMD_RES_OK;
}

/**
	* Console command: Roomba_DigitsASCII <text>
	* Sends OI Digit LEDs ASCII opcode 164 for up to 4 printable characters.
	* Shorter text is padded with spaces.
	*/
static commandResult_t CMD_Roomba_DigitsASCII(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char *txt;
	byte chars[4];
	int i;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsASCII: requires 1 arg: <text up to 4 chars>");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	txt = Tokenizer_GetArg(0);
	for (i = 0; i < 4; i++) {
		unsigned char c = (unsigned char)txt[i];
		if (c == 0) {
			chars[i] = ' ';
			continue;
		}
		if (c < 32 || c > 126) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsASCII: non-printable char at index %d", i);
			return CMD_RES_BAD_ARGUMENT;
		}
		chars[i] = (byte)c;
	}
	if (txt[4] != 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsASCII: text too long (max 4 chars)");
		return CMD_RES_BAD_ARGUMENT;
	}

	g_poll_pause_seconds = 1;
	Roomba_SendByte(CMD_DIGIT_LEDS_ASCII); // 164
	Roomba_SendByte(chars[0]);
	Roomba_SendByte(chars[1]);
	Roomba_SendByte(chars[2]);
	Roomba_SendByte(chars[3]);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DigitsASCII: '%c%c%c%c'",
		chars[0], chars[1], chars[2], chars[3]);
	return CMD_RES_OK;
}

/**
	* Console command: Roomba_Beep
	* Plays a short locate/beep sound using SONG/PLAY opcodes.
	*/
static commandResult_t CMD_Roomba_Beep(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_PlayLocateBeep();
	return CMD_RES_OK;
}

/**
	* Console command: Roomba_Drive <velocity_mm_s> <radius_mm>
	* Sends OI Drive opcode 137 with signed 16-bit velocity/radius.
	* This is the command that updates requested velocity/radius packets 39/40.
	*/
static commandResult_t CMD_Roomba_Drive(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int velocity;
	int radius;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_Drive: requires 2 args: <velocity_mm_s> <radius_mm>");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	velocity = Tokenizer_GetArgInteger(0);
	radius = Tokenizer_GetArgInteger(1);
	if (velocity < -500 || velocity > 500) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_Drive: velocity out of range (-500..500): %d", velocity);
		return CMD_RES_BAD_ARGUMENT;
	}
	if (radius < -2000 || radius > 2000) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_Drive: radius out of range (-2000..2000): %d", radius);
		return CMD_RES_BAD_ARGUMENT;
	}

	// Pause 1Hz polling briefly to avoid command/request stream interleaving.
	g_poll_pause_seconds = 2;
	Roomba_SendByte(CMD_DRIVE);
	Roomba_SendByte((byte)((velocity >> 8) & 0xFF));
	Roomba_SendByte((byte)(velocity & 0xFF));
	Roomba_SendByte((byte)((radius >> 8) & 0xFF));
	Roomba_SendByte((byte)(radius & 0xFF));
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
		"Roomba_Drive: v=%d r=%d sent, poll pause=%ds", velocity, radius, g_poll_pause_seconds);
	return CMD_RES_OK;
}

/**
	* Console command: Roomba_DriveDirect <right_mm_s> <left_mm_s>
	* Sends OI Drive Direct opcode 145 with signed 16-bit wheel velocities.
	* This is the command that updates requested right/left velocity packets 41/42.
	*/
static commandResult_t CMD_Roomba_DriveDirect(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int rightVel;
	int leftVel;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba_DriveDirect: requires 2 args: <right_mm_s> <left_mm_s>");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	rightVel = Tokenizer_GetArgInteger(0);
	leftVel = Tokenizer_GetArgInteger(1);
	if (rightVel < -500 || rightVel > 500 || leftVel < -500 || leftVel > 500) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
			"Roomba_DriveDirect: wheel velocity out of range (-500..500): right=%d left=%d",
			rightVel, leftVel);
		return CMD_RES_BAD_ARGUMENT;
	}

	// Pause 1Hz polling briefly to avoid command/request stream interleaving.
	g_poll_pause_seconds = 2;
	Roomba_SendByte(CMD_DRIVE_DIRECT);
	Roomba_SendByte((byte)((rightVel >> 8) & 0xFF));
	Roomba_SendByte((byte)(rightVel & 0xFF));
	Roomba_SendByte((byte)((leftVel >> 8) & 0xFF));
	Roomba_SendByte((byte)(leftVel & 0xFF));
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
		"Roomba_DriveDirect: right=%d left=%d sent, poll pause=%ds",
		rightVel, leftVel, g_poll_pause_seconds);
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Clean(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_CLEAN);
	g_vacuum_state = VACUUM_STATE_CLEANING;
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: CLEAN command sent");
	return CMD_RES_OK;
}

static commandResult_t CMD_Roomba_Max(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_MAX);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: MAX command sent");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Spot(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_SPOT);
	g_vacuum_state = VACUUM_STATE_CLEANING;
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: SPOT command sent");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Dock(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_DOCK);
	g_vacuum_state = VACUUM_STATE_RETURNING;
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: DOCK command sent");
	return CMD_RES_OK;
}

static commandResult_t CMD_Roomba_Safe(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_SAFE);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: SAFE command sent (Reset)");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Stop(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Stop command");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Discovery(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Manual HA discovery requested");
	Roomba_OnHassDiscovery("homeassistant");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_SetSafe(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Entering SAFE mode");
	Roomba_SendByte(CMD_START);  // 0x80
	rtos_delay_milliseconds(50);
	Roomba_SendByte(CMD_SAFE);   // 0x83
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_SetFull(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Entering FULL mode");
	Roomba_SendByte(CMD_START);  // 0x80
	rtos_delay_milliseconds(50);
	Roomba_SendByte(0x84);       // FULL mode
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_SetPassive(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Entering PASSIVE mode");
	Roomba_SendByte(CMD_START);  // 0x80 (START puts it in Passive mode)
	return CMD_RES_OK;
}

// Custom Command Handler for Home Assistant Vacuum Entity
// Payload: "clean", "dock", "stop", "return_to_base"
commandResult_t CMD_RoombaOrder(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// Home Assistant vacuum commands we accept:
	// start, pause, stop, return_to_base/return_home, clean_spot (plus legacy: clean, dock)
	if (args == 0 || *args == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Order received: <empty>");
		return CMD_RES_BAD_ARGUMENT;
	}

	// ------------------------------------------------------------------------
	// Command Handler: "clean" / "start"
	// ------------------------------------------------------------------------
	// Single-button behavior: 
	// - If Robot IS Cleaning -> Toggle to PAUSE (Stop motors)
	// - If Robot IS NOT Cleaning -> Start CLEANING
	// ------------------------------------------------------------------------
	if (stricmp(args, "start") == 0 || stricmp(args, "clean") == 0) {
		// Single-button behavior: If already cleaning, Stop/Pause. Else, Start.
		if (g_vacuum_state == VACUUM_STATE_CLEANING) {
			Roomba_SendCmd(CMD_SAFE); // Stop motors (Pause)
			g_vacuum_state = VACUUM_STATE_PAUSED;
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Toggling PAUSE");
		} else {
			Roomba_SendCmd(CMD_CLEAN);
			g_vacuum_state = VACUUM_STATE_CLEANING; 
		}

	} else if (stricmp(args, "clean_spot") == 0 || stricmp(args, "spot") == 0) {
		Roomba_SendCmd(CMD_SPOT);
		g_vacuum_state = VACUUM_STATE_CLEANING;

	} else if (stricmp(args, "return_to_base") == 0 || stricmp(args, "return_home") == 0 || stricmp(args, "dock") == 0) {
		Roomba_SendCmd(CMD_DOCK);
		g_vacuum_state = VACUUM_STATE_RETURNING;

	} else if (stricmp(args, "pause") == 0) {
		// No explicit PAUSE in OI; SAFE typically stops motion without powering off OI.
		Roomba_SendCmd(CMD_SAFE);
		g_vacuum_state = VACUUM_STATE_PAUSED;

	} else if (stricmp(args, "stop") == 0) {
		Roomba_SendCmd(CMD_SAFE);
		g_vacuum_state = VACUUM_STATE_IDLE;

	} else if (stricmp(args, "locate") == 0) {
		Roomba_PlayLocateBeep();
	} else {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Order received: %s (unhandled)", args);
		return CMD_RES_BAD_ARGUMENT;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Order received: %s", args);
	return CMD_RES_OK;
}


static void Roomba_UpdateState(void) {
	// ========================================================================
	// STATE INFERENCE ENGINE
	// ========================================================================
	// Determines the vacuum state (Docked, Cleaning, Idle, Returning)
	// based on sensor readings. Uses a latch to prevent flickering.
	// ========================================================================

	// State Inference Variables
	static int idle_latch_timer = 0; // Frames of low-current to confirm IDLE
	const int IDLE_CONFIRM_FRAMES = 5; // 5 seconds of low current -> IDLE
	
	// charging_state: 0=Not, 1=Recond, 2=Full, 3=Trickle, 4=Waiting, 5=Fault
	float current = g_sensors[ROOMBA_SENSOR_CURRENT].lastReading;
	bool isDocked = Roomba_IsDockedEffective();
	bool isSafeMode = (g_roomba_last_oi_mode == 2);

	// 0. ERROR Check (Runtime Faults)
	if (g_runtime_error_active) {
		g_vacuum_state = VACUUM_STATE_ERROR;
		idle_latch_timer = 0;
		return;
	}

	// 1. DOCKED Check
	// If Charging State is Non-Zero (1..5), we are effectively docked.
	// BUT subtract idle current (-200mA) vs charging current (>= 0).
	// Idle draw is ~200mA. Docked is > -50mA (usually positive).
	// Charging state 4 (Waiting) can persist briefly after undocking.
	if (isDocked) {
		g_vacuum_state = VACUUM_STATE_DOCKED;
		idle_latch_timer = 0; // Reset idle latch
	} 
	// 2. PAUSED Check
	// OI Safe mode indicates direct control with motors stopped by command.
	// If off-dock and not actively drawing cleaning current, report paused.
	else if (isSafeMode && current > -300 && g_vacuum_state != VACUUM_STATE_RETURNING) {
		g_vacuum_state = VACUUM_STATE_PAUSED;
		idle_latch_timer = 0;
	}
	// 3. CLEANING Check
	// Detects aggressive motor usage via current draw.
	// Threshold: < -300mA covers the sum of all motors + logic.
	else if (current < -300) {
		// Only switch to CLEANING if we aren't already RETURNING.
		// If we are RETURNING, motors are running but state is 'Docking'.
		if (g_vacuum_state != VACUUM_STATE_RETURNING) {
			g_vacuum_state = VACUUM_STATE_CLEANING;
		}
		idle_latch_timer = 0; // Reset idle latch
	}
	// 4. IDLE vs RETURNING Check (with Latch)
	// If current is low ( > -300mA):
	// - If we were Cleaning, wait for N frames to confirm we truly stopped (Idle).
	//   (Prevents flickering during turns/pauses).
	// - If we were Returning, keep Returning until Docked or Stopped.
	else {
		// We use a LATCH to prevent status flickering during turns/pauses.
		if (g_vacuum_state == VACUUM_STATE_CLEANING) {
			// currently marked as cleaning, but current is low.
			// Wait for N frames before dropping to IDLE.
			idle_latch_timer++;
			if (idle_latch_timer > IDLE_CONFIRM_FRAMES) {
				g_vacuum_state = VACUUM_STATE_IDLE;
			}
			// else: keep reporting cleaning for now
		} 
		else if (g_vacuum_state == VACUUM_STATE_RETURNING) {
			// If we were returning, we stay returning until we dock or user stops.
			// (No feedback for returning without visual sensors)
		}
		else if (g_vacuum_state != VACUUM_STATE_PAUSED) {
			// Already idle (and not paused)
			g_vacuum_state = VACUUM_STATE_IDLE;
		}
	}
}



/**
	* Publish Roomba sensor data via MQTT
	* Direct topic publishing with thresholds
	*/
void Roomba_PublishSensors(void) {
	if (!MQTT_IsReady()) {
		return;
	}

	// Iterate through all sensors and publish if changed
	for (int i = 0; i < ROOMBA_SENSOR__COUNT; i++) {
		// Skip sensors without MQTT names or device classes
		if (strlen(g_sensors[i].name_mqtt) == 0) {
			continue;
		}

		float currentReading = g_sensors[i].lastReading;
		float delta = fabs(currentReading - g_sensors[i].lastSent);

		// Publish if:
		// 1. Change exceeds threshold, OR
		// 2. Forced interval reached (changeSendAlwaysFrames)
		// AND minimum interval passed (changeDoNotSendMinFrames)
		bool shouldPublish = false;

		if (delta >= g_sensors[i].threshold) {
			// Significant change detected
			if (g_sensors[i].noChange >= changeDoNotSendMinFrames) {
				shouldPublish = true;
			}
		} else if (g_sensors[i].noChange >= changeSendAlwaysFrames) {
			// Force publish (timeout)
			shouldPublish = true;
		}

		if (shouldPublish) {
			// Publish via MQTT
			if (g_sensors[i].is_binary) {
				// Binary sensors: "ON" / "OFF"
				MQTT_PublishMain_StringString(g_sensors[i].name_mqtt, 
					currentReading ? "ON" : "OFF", 0);
			} else {
				if (Roomba_IsTextSensor(i)) {
					char textValue[96];
					const char *txt = Roomba_GetTextSensorValue(i, currentReading, textValue, sizeof(textValue));
					if (txt != NULL) {
						MQTT_PublishMain_StringString(g_sensors[i].name_mqtt, txt, 0);
					}
				} else {
					MQTT_PublishMain_StringFloat(g_sensors[i].name_mqtt,
						currentReading, g_sensors[i].decimals, 0);
				}
			}

			g_sensors[i].lastSent = currentReading;
			g_sensors[i].noChange = 0;
			stat_updatesSent++;
		} else {
			g_sensors[i].noChange++;
			stat_updatesSkipped++;
		}
	}

	// Publish computed Battery Level (not in g_sensors array)
	static float lastBatteryPct = -1;
	static int batteryNoChange = 0;
	float charge = g_sensors[ROOMBA_SENSOR_CHARGE].lastReading;
	float capacity = g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading;
	float pct = (capacity > 0) ? ((charge / capacity) * 100.0f) : 0;

	if (fabs(pct - lastBatteryPct) >= 1.0f || batteryNoChange >= changeSendAlwaysFrames) {
		// Publish computed battery level: <clientId>/battery_level/get
		MQTT_PublishMain_StringFloat("battery_level", pct, 0, 0);
		lastBatteryPct = pct;
		batteryNoChange = 0;
	} else {
		batteryNoChange++;
	}
}

// ============================================================================
// DRIVER INITIALIZATION
// ============================================================================

/**
	* Initialize the Roomba driver
	* Called once when the driver is started via "startDriver Roomba"
	* 
	* Initialization sequence:
	* 1. Configure UART (115200 baud, 8N1)
	* 2. Send START command (128) to wake up Roomba
	* 3. Send SAFE mode command (131) to enable sensor reading
	* 4. Register console commands
	*/
void Roomba_Init() {
	// Use the system's selected UART port
	g_roomba_uart = UART_GetSelectedPortIndex();

	// Initialize UART: 115200 baud, 8 data bits, no parity, 1 stop bit
	// This is the standard baud rate for Roomba OI (some older models use 57600)
	UART_InitUARTEx(g_roomba_uart, 115200, 0, 0);

	// Initialize the UART receive ring buffer AFTER initializing UART
	// Buffer size: 512 bytes (enough for sensor responses)
	UART_InitReceiveRingBufferEx(g_roomba_uart, 512);

	// Roomba OI initialization sequence
	// Send START (128) to wake up the Roomba and enter passive mode
	Roomba_SendByte(CMD_START);
	rtos_delay_milliseconds(20);  // Give Roomba time to process
	
	// Send SAFE mode command (131)
	// SAFE mode allows sensor reading and motor control
	// Roomba_SendByte(CMD_SAFE);

	// Register console commands for manual testing
	CMD_RegisterCommand("Roomba_SendCmd", CMD_Roomba_SendCmd, NULL);
	CMD_RegisterCommand("Roomba_SendBytes", CMD_Roomba_SendBytes, NULL);
	CMD_RegisterCommand("Roomba_LEDs", CMD_Roomba_LEDs, NULL);
	CMD_RegisterCommand("Roomba_DigitsRaw", CMD_Roomba_DigitsRaw, NULL);
	CMD_RegisterCommand("Roomba_DigitsASCII", CMD_Roomba_DigitsASCII, NULL);
	CMD_RegisterCommand("Roomba_Beep", CMD_Roomba_Beep, NULL);
	CMD_RegisterCommand("Roomba_Drive", CMD_Roomba_Drive, NULL);
	CMD_RegisterCommand("Roomba_DriveDirect", CMD_Roomba_DriveDirect, NULL);
	CMD_RegisterCommand("Roomba_Clean", CMD_Roomba_Clean, NULL);
	CMD_RegisterCommand("Roomba_Spot", CMD_Roomba_Spot, NULL);
	CMD_RegisterCommand("Roomba_Dock", CMD_Roomba_Dock, NULL);
	CMD_RegisterCommand("Roomba_Max", CMD_Roomba_Max, NULL);
	CMD_RegisterCommand("Roomba_Safe", CMD_Roomba_Safe, NULL);
	CMD_RegisterCommand("Roomba_Stop", CMD_Roomba_Stop, NULL);
	CMD_RegisterCommand("Roomba_Discovery", CMD_Roomba_Discovery, NULL);
	CMD_RegisterCommand("Roomba_SetSafe", CMD_Roomba_SetSafe, NULL);
	CMD_RegisterCommand("Roomba_SetFull", CMD_Roomba_SetFull, NULL);
	CMD_RegisterCommand("Roomba_SetPassive", CMD_Roomba_SetPassive, NULL);
	CMD_RegisterCommand("RoombaOrder", CMD_RoombaOrder, NULL); // Custom HA Command

	

	// Register vacuum entity MQTT command topics (HA standard)

	CMD_RegisterCommand("vacuum/command", CMD_RoombaOrder, NULL);

	CMD_RegisterCommand("vacuum/send_command", CMD_RoombaOrder, NULL);
	
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba driver initialized (UART%d @ 115200 baud, %d sensors)", 
		g_roomba_uart, ROOMBA_SENSOR__COUNT);

	// Start at 2Hz poll cadence.
	g_poll_interval_ticks = configTICK_RATE_HZ / 2;
	if (g_poll_interval_ticks < 1) {
		g_poll_interval_ticks = 1;
	}
	g_next_poll_tick = xTaskGetTickCount();
	
	// Note: HA discovery is triggered manually via Roomba_Discovery command
	// Not auto-triggered on startup (now handled by Roomba_RunEverySecond)
}

// ============================================================================
// SENSOR POLLING (CALLED EVERY SECOND)
// ============================================================================

/**
	* Called every second by the OpenBeken main loop
	* Implements 1Hz polling with optional IR side-query in Group 6 fallback mode.
	* 
	* Polling sequence:
	* - Primary: Sensor Group 100 (80 bytes, packets 7-58)
	* - Fallback: Sensor Group 6 (52 bytes, packets 7-42) if Group 100 is unstable
	* - IR Left/Right Query List (52/53): only in Group 6 fallback mode
	*/
static bool Roomba_TrySendSensorPoll(void) {
	uint32_t nowTick = xTaskGetTickCount();
	uint32_t stallLimitTicks = (uint32_t)(configTICK_RATE_HZ * 3); // 3s unchanged behavior target
	uint32_t shortFrameDropTicks = (uint32_t)(configTICK_RATE_HZ / 2); // 500ms orphan partial-frame guard
	uint32_t skipLogEveryTicks = (uint32_t)configTICK_RATE_HZ;
	uint32_t pollQuietTicks = (uint32_t)(configTICK_RATE_HZ / 4);
	if (stallLimitTicks < 1) {
		stallLimitTicks = 1;
	}
	if (shortFrameDropTicks < 1) {
		shortFrameDropTicks = 1;
	}
	if (skipLogEveryTicks < 1) {
		skipLogEveryTicks = 1;
	}
	if (pollQuietTicks < 1) {
		pollQuietTicks = 1;
	}

	// Avoid overlapping responses from multiple requests.
	int uartPending = UART_GetDataSizeEx(g_roomba_uart);
	int framePending = s_frameLen;
	int pending = uartPending + framePending;
	if (pending > 0) {
		uint32_t stalledTicks = 0;
		if (pending != g_rx_last_pending ||
			uartPending != g_rx_last_pending_uart ||
			framePending != g_rx_last_pending_frame ||
			g_rx_pending_since_tick == 0) {
			g_rx_last_pending = pending;
			g_rx_last_pending_uart = uartPending;
			g_rx_last_pending_frame = framePending;
			g_rx_pending_since_tick = nowTick;
		} else {
			stalledTicks = nowTick - g_rx_pending_since_tick;
		}

		// If only an incomplete local frame remains (no fresh UART bytes), drop it quickly.
		// This prevents sticky short-frame residue (e.g. 13 bytes) from blocking new polls.
		if (framePending > 0 && uartPending == 0 &&
			framePending < s_rxExpectedLen && stalledTicks >= shortFrameDropTicks) {
			Roomba_LogPartialFrameSnapshot("Dropping orphan", framePending, stalledTicks);
			Roomba_RecordDriverError(ROOMBA_DRV_ERR_RX_ORPHAN_DROP);
			s_frameLen = 0;
			g_rx_last_pending = 0;
			g_rx_last_pending_uart = 0;
			g_rx_last_pending_frame = 0;
			g_rx_pending_since_tick = 0;
			g_poll_block_until_tick = nowTick + pollQuietTicks;
			return false;
		}

		// Recovery: if pending RX count stays unchanged for a few real-time seconds,
		// flush stale bytes and reset parser state to avoid deadlock.
		if (stalledTicks >= stallLimitTicks) {
			if (uartPending > 0) {
				UART_ConsumeBytesEx(g_roomba_uart, uartPending);
			}
			if (framePending > 0) {
				Roomba_LogPartialFrameSnapshot("RX stalled", framePending, stalledTicks);
			}
			Roomba_RecordDriverError(ROOMBA_DRV_ERR_RX_STALL_RESET);
			s_frameLen = 0;
			s_rxKind = ROOMBA_RX_GROUP6;
			s_rxExpectedLen = s_groupExpectedLen;
			addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
				"Roomba: RX stalled (pending=%d uart=%d frame=%d exp=%d, %dms), flushed %d UART bytes and reset parser",
				pending, uartPending, framePending, s_rxExpectedLen,
				(int)(stalledTicks * portTICK_PERIOD_MS), uartPending);
			g_rx_last_pending = 0;
			g_rx_last_pending_uart = 0;
			g_rx_last_pending_frame = 0;
			g_rx_pending_since_tick = 0;
			// Avoid immediate command re-send right after a recovery reset.
			g_poll_block_until_tick = nowTick + pollQuietTicks;
		} else if ((g_rx_last_skip_log_tick == 0) ||
			((nowTick - g_rx_last_skip_log_tick) >= skipLogEveryTicks)) {
			addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
				"Roomba: Skip poll, pending=%d (uart=%d frame=%d exp=%d, stall %dms)",
				pending, uartPending, framePending, s_rxExpectedLen,
				(int)(stalledTicks * portTICK_PERIOD_MS));
			g_rx_last_skip_log_tick = nowTick;
		}
		return false;
	} else {
		g_rx_last_pending = 0;
		g_rx_last_pending_uart = 0;
		g_rx_last_pending_frame = 0;
		g_rx_pending_since_tick = 0;
	}

	// If Group 100 fallback is active, periodically retry Group 100.
	if (s_groupPacketId != PACKET_GROUP_100) {
		if (++s_group100RetrySeconds >= 120) {
			s_group100RetrySeconds = 0;
			s_groupPacketId = PACKET_GROUP_100;
			s_groupExpectedLen = 80;
			s_group100FailCount = 0;
			g_group100_retry_pending = 1;
			s_frameLen = 0;
			s_rxExpectedLen = s_groupExpectedLen;
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Retrying Sensor Group 100");
		}
	}

	// Group 100 already carries IR Left/Right (52/53), so only query them separately in Group 6 fallback mode.
	if (s_groupPacketId == PACKET_GROUP_6 && ++g_ir_lr_poll_counter >= 5) {
		g_ir_lr_poll_counter = 0;
		s_rxKind = ROOMBA_RX_IR_LR;
		s_rxExpectedLen = 2;
		s_frameLen = 0;
		Roomba_SendByte(CMD_QUERY_LIST); // 149
		Roomba_SendByte(2);              // two packet IDs follow
		Roomba_SendByte(PACKET_IR_LEFT); // IR Left
		Roomba_SendByte(PACKET_IR_RIGHT);// IR Right
	} else {
		s_rxKind = ROOMBA_RX_GROUP6;
		s_rxExpectedLen = s_groupExpectedLen;
		s_frameLen = 0;
		Roomba_SendByte(CMD_SENSORS);
		Roomba_SendByte(s_groupPacketId);
	}
	return true;
}

void Roomba_RunEverySecond() {
	uint32_t nowTick = xTaskGetTickCount();
	// Temporary poll hold for raw multi-byte command injection.
	// Used by Roomba_SendBytes to reduce UART stream interleaving.
	if (g_poll_pause_seconds > 0) {
		g_poll_pause_seconds--;
		Roomba_MQTT_PublishErrorReportIfNeeded(nowTick);
		return;
	}

	// ========================================================================
	// KEEP-ALIVE MECHANISM
	// ========================================================================
	// Roomba enters sleep mode after 5 minutes of inactivity on lack of commands.
	// We periodically "ping" the robot to keep it awake if it is NOT on the dock.
	// ========================================================================
	
	// Decide keepalive first, so we don't append to any other command stream
	int doKeepAlive = 0;
	// 240 seconds interval (OI sleep timeout is 5 minutes/300s)
	if (++g_roomba_keepalive_sec >= 240) {
		g_roomba_keepalive_sec = 0;
		doKeepAlive = 1;
	}

	// Keep-alive Logic:
	// - Trigger: Every 250 seconds
	// - Condition: Robot is IDLE or PAUSED (Not Cleaning/Returning) AND NOT DOCKED.
	// - Action: Send CMD_START (128) + CMD_SAFE (131).
	//   - 128 resets the sleep timer.
	//   - 131 ensures we are in Safe mode to accept commands.
	//   - do NOT send this during Cleaning because 131 stops the motors!
	if (doKeepAlive) {
		// use g_vacuum_state because it has the "latch" logic for cleaning.
		// If state is CLEANING or RETURNING, we MUST NOT send 131.
		bool isWorking = (g_vacuum_state == VACUUM_STATE_CLEANING || g_vacuum_state == VACUUM_STATE_RETURNING);
		bool isDocked  = Roomba_IsDockedEffective();
		int chg = Roomba_GetEffectiveChargingState();

		if (!isWorking && !isDocked) {
			// "Send the 128 131 only then" - User Request
			Roomba_SendByte(CMD_START); // 128
			Roomba_SendByte(CMD_SAFE);  // 131
			// Briefly block sensor polling after command burst.
			{
				uint32_t quietTicks = (uint32_t)(configTICK_RATE_HZ / 5);
				if (quietTicks < 1) {
					quietTicks = 1;
				}
				g_poll_block_until_tick = xTaskGetTickCount() + quietTicks;
			}
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
				"Roomba: Keep-Alive 128+131 sent (State: %d, Chg: %d)", g_vacuum_state, chg);
			Roomba_MQTT_PublishErrorReportIfNeeded(nowTick);
			return; // skip sensor poll to avoid traffic collision
		}
	}

	// Sensor polling is scheduled in Roomba_OnQuickTick.
	Roomba_MQTT_PublishErrorReportIfNeeded(nowTick);
}

// ============================================================================
// UART RESPONSE HANDLER (CALLED FREQUENTLY)
// ============================================================================

/**
	* Called frequently by OpenBeken main loop
	* Assembles and parses incoming sensor frames (Group 6 or Group 100)
	*/
void Roomba_OnQuickTick() {
	uint32_t nowTick = xTaskGetTickCount();

	// Scheduler: trigger sensor polling from quick-tick at 2Hz target.
	// Guards:
	// - honor temporary pause after outbound multi-byte commands
	// - honor short post-keepalive quiet window (128+131 burst)
	if (g_poll_interval_ticks <= 0) {
		g_poll_interval_ticks = configTICK_RATE_HZ / 2;
		if (g_poll_interval_ticks < 1) {
			g_poll_interval_ticks = 1;
		}
	}
	// Do not busy-loop retries on deferrals; use a short backoff.
	int retryTicks = g_poll_interval_ticks / 4;
	if (retryTicks < 2) {
		retryTicks = 2;
	}
	if ((int32_t)(nowTick - g_next_poll_tick) >= 0) {
		if (g_poll_pause_seconds <= 0 &&
			(int32_t)(nowTick - g_poll_block_until_tick) >= 0) {
			if (Roomba_TrySendSensorPoll()) {
				g_next_poll_tick = nowTick + (uint32_t)g_poll_interval_ticks;
			} else {
				// Retry soon after transient deferrals (pending RX, stall recovery).
				g_next_poll_tick = nowTick + (uint32_t)retryTicks;
			}
		} else {
			// Still blocked/pause active, retry with backoff.
			g_next_poll_tick = nowTick + (uint32_t)retryTicks;
		}
	}

	int avail = UART_GetDataSizeEx(g_roomba_uart);

	// Assemble expected response frame by consuming bytes as they arrive.
	while (avail > 0 && s_frameLen < s_rxExpectedLen) {
		s_frame[s_frameLen++] = UART_GetByteEx(g_roomba_uart, 0);
		UART_ConsumeBytesEx(g_roomba_uart, 1);
		avail--;
	}
	
	int totalPending = avail + s_frameLen;

	// Parse expected response when available
	if (s_frameLen >= s_rxExpectedLen) {
		byte buf[128];
		for (int i = 0; i < s_rxExpectedLen; i++) {
			buf[i] = s_frame[i];
		}

		// Parse Query List response for IR Left/Right (Packet 52/53).
		if (s_rxKind == ROOMBA_RX_IR_LR) {
			int irLeft = buf[0];
			int irRight = buf[1];
			g_sensors[ROOMBA_SENSOR_IR_LEFT].lastReading = irLeft;
			g_sensors[ROOMBA_SENSOR_IR_RIGHT].lastReading = irRight;

			if (irLeft != g_roomba_last_ir_left) {
				g_roomba_last_ir_left = irLeft;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: IR Left changed: %d (%s)",
					irLeft, Roomba_IrOmniStr(irLeft));
			}
			if (irRight != g_roomba_last_ir_right) {
				g_roomba_last_ir_right = irRight;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: IR Right changed: %d (%s)",
					irRight, Roomba_IrOmniStr(irRight));
			}

			s_frameLen = 0;
			return;
		}

		// Sanity Check: Charging State (Byte 16) must be 0-5
		if (buf[16] > 5) {
			Roomba_RecordDriverError(ROOMBA_DRV_ERR_RX_MALFORMED);
			addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
					"Roomba: Invalid Charging State (%d). Resync shift.", buf[16]);

			// Resync: drop 1 byte from assembled frame and try to realign on next ticks
			if (s_frameLen > 0) {
				for (int i = 1; i < s_frameLen; i++) {
					s_frame[i - 1] = s_frame[i];
				}
				s_frameLen--;
			} else {
				s_frameLen = 0;
			}

			// Group-100 guardrail: fallback to Group 6 after repeated malformed frames.
			if (s_groupPacketId == PACKET_GROUP_100) {
				s_group100FailCount++;
				if (s_group100FailCount >= 3) {
					s_groupPacketId = PACKET_GROUP_6;
					s_groupExpectedLen = 52;
					s_group100RetrySeconds = 0;
					s_frameLen = 0;
					s_rxExpectedLen = s_groupExpectedLen;
					Roomba_RecordDriverError(ROOMBA_DRV_ERR_GROUP100_FALLBACK);
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
						"Roomba: Group 100 malformed repeatedly, falling back to Group 6");
				}
			}
			return;
		}
		if (s_groupPacketId == PACKET_GROUP_100 && s_group100FailCount > 0) {
			s_group100FailCount = 0;
		}
		if (s_groupPacketId == PACKET_GROUP_100 && g_group100_retry_pending) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Group6->Group100 retry success");
			g_group100_retry_pending = 0;
		}

		// --- PARSE PACKET GROUP 6 (Packets 7-42) ---
		
		// Packet 7: Bumps & Wheel Drops (Byte 0)
		g_sensors[ROOMBA_SENSOR_BUMP_RIGHT].lastReading = (buf[0] & 0x01) ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_BUMP_LEFT].lastReading = (buf[0] & 0x02) ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_WHEELDROP_RIGHT].lastReading = (buf[0] & 0x04) ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_WHEELDROP_LEFT].lastReading = (buf[0] & 0x08) ? 1 : 0;
		
		// Packet 8: Wall (Byte 1)
		g_sensors[ROOMBA_SENSOR_WALL].lastReading = buf[1] ? 1 : 0;
		
		// Packet 9-12: Cliffs (Bytes 2-5)
		g_sensors[ROOMBA_SENSOR_CLIFF_LEFT].lastReading = buf[2] ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_CLIFF_FL].lastReading = buf[3] ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_CLIFF_FR].lastReading = buf[4] ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_CLIFF_RIGHT].lastReading = buf[5] ? 1 : 0;
		
		// Packet 13: Virtual Wall (Byte 6)
		g_sensors[ROOMBA_SENSOR_VIRTUAL_WALL].lastReading = buf[6] ? 1 : 0;

		// Packet 14: Overcurrents (Byte 7, bitmask)
		{
			int oc = buf[7];
			g_sensors[ROOMBA_SENSOR_OVERCURRENT_SIDE_BRUSH].lastReading = (oc & 0x01) ? 1 : 0;
			g_sensors[ROOMBA_SENSOR_OVERCURRENT_VACUUM].lastReading = (oc & 0x02) ? 1 : 0;
			g_sensors[ROOMBA_SENSOR_OVERCURRENT_MAIN_BRUSH].lastReading = (oc & 0x04) ? 1 : 0;
			g_sensors[ROOMBA_SENSOR_OVERCURRENT_DRIVE_RIGHT].lastReading = (oc & 0x08) ? 1 : 0;
			g_sensors[ROOMBA_SENSOR_OVERCURRENT_DRIVE_LEFT].lastReading = (oc & 0x10) ? 1 : 0;
			if (oc != g_roomba_last_overcurrents) {
				g_roomba_last_overcurrents = oc;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
					"Roomba: Overcurrents changed: 0x%02X (side=%d vac=%d main=%d right=%d left=%d)",
					oc,
					(oc & 0x01) ? 1 : 0,
					(oc & 0x02) ? 1 : 0,
					(oc & 0x04) ? 1 : 0,
					(oc & 0x08) ? 1 : 0,
					(oc & 0x10) ? 1 : 0);
			}
		}
		
		// Packet 15: Dirt Detect (Byte 8)
		g_sensors[ROOMBA_SENSOR_DIRT_DETECT].lastReading = buf[8];
		
		// Packet 17: IR Omni (Byte 10)
		int irOmni = buf[10];
		if (irOmni != g_roomba_last_ir_omni) {
			g_roomba_last_ir_omni = irOmni;
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: IR Omni changed: %d (%s)",
				irOmni, Roomba_IrOmniStr(irOmni));
		}

		// Packet 18: Buttons (Byte 11, bitmask)
		int buttons = buf[11];
		g_sensors[ROOMBA_SENSOR_BUTTONS].lastReading = buttons;
		if (buttons != g_roomba_last_buttons) {
			g_roomba_last_buttons = buttons;
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Buttons changed: 0x%02X", buttons);
		}

		// Packet 19: Distance (Bytes 12-13, mm, signed)
		g_sensors[ROOMBA_SENSOR_DISTANCE].lastReading = (short)((buf[12] << 8) | buf[13]);

		// Packet 20: Angle (Bytes 14-15, deg, signed)
		g_sensors[ROOMBA_SENSOR_ANGLE].lastReading = (short)((buf[14] << 8) | buf[15]);
				
		// Packet 21: Charging State (Byte 16)
		g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading = buf[16];
			g_roomba_last_charging_state = buf[16];

		// Packet 34: Charging Sources Available (Byte 39)
		{
			int src = buf[39];
			if (src != g_roomba_last_charging_sources) {
				g_roomba_last_charging_sources = src;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Charging Sources changed: 0x%02X", src);
			}
		}

		// Packet 35: OI Mode (Byte 40)
		{
			int mode = buf[40];
			if (mode != g_roomba_last_oi_mode) {
				g_roomba_last_oi_mode = mode;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: OI Mode changed: %d (%s)",
					mode, Roomba_OiModeStr(mode));
			}
		}
		
		// Packet 22: Voltage (Bytes 17-18, mV)
		g_sensors[ROOMBA_SENSOR_VOLTAGE].lastReading = (buf[17] << 8) | buf[18];
		
		// Packet 23: Current (Bytes 19-20, mA, signed)
		g_sensors[ROOMBA_SENSOR_CURRENT].lastReading = (short)((buf[19] << 8) | buf[20]);
		
		// Packet 24: Temperature (Byte 21, C, signed)
		g_sensors[ROOMBA_SENSOR_TEMPERATURE].lastReading = (signed char)buf[21];
		
		// Packet 25: Charge (Bytes 22-23, mAh)
		g_sensors[ROOMBA_SENSOR_CHARGE].lastReading = (buf[22] << 8) | buf[23];
		
		// Packet 26: Capacity (Bytes 24-25, mAh)
		g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading = (buf[24] << 8) | buf[25];

		// Packet 27-31: Wall/Cliff signal strengths (Bytes 26-35, unsigned)
		g_sensors[ROOMBA_SENSOR_WALL_SIGNAL].lastReading = (buf[26] << 8) | buf[27];
		g_sensors[ROOMBA_SENSOR_CLIFF_LEFT_SIGNAL].lastReading = (buf[28] << 8) | buf[29];
		g_sensors[ROOMBA_SENSOR_CLIFF_FL_SIGNAL].lastReading = (buf[30] << 8) | buf[31];
		g_sensors[ROOMBA_SENSOR_CLIFF_FR_SIGNAL].lastReading = (buf[32] << 8) | buf[33];
		g_sensors[ROOMBA_SENSOR_CLIFF_RIGHT_SIGNAL].lastReading = (buf[34] << 8) | buf[35];

		// Packet 39-42: Requested motion setpoints (Bytes 44-51, signed)
		g_sensors[ROOMBA_SENSOR_REQ_VELOCITY].lastReading = (short)((buf[44] << 8) | buf[45]);
		g_sensors[ROOMBA_SENSOR_REQ_RADIUS].lastReading = (short)((buf[46] << 8) | buf[47]);
		g_sensors[ROOMBA_SENSOR_REQ_RIGHT_VEL].lastReading = (short)((buf[48] << 8) | buf[49]);
		g_sensors[ROOMBA_SENSOR_REQ_LEFT_VEL].lastReading = (short)((buf[50] << 8) | buf[51]);

		// Group 100 extension (Packets 43-58)
		if (s_rxExpectedLen >= 80) {
			int encLeft = (short)((buf[52] << 8) | buf[53]);
			int encRight = (short)((buf[54] << 8) | buf[55]);
			int lightBumper = buf[56];
			int lightBumpLeft = (buf[57] << 8) | buf[58];
			int lightBumpFL = (buf[59] << 8) | buf[60];
			int lightBumpCL = (buf[61] << 8) | buf[62];
			int lightBumpCR = (buf[63] << 8) | buf[64];
			int lightBumpFR = (buf[65] << 8) | buf[66];
			int lightBumpRight = (buf[67] << 8) | buf[68];
			int irLeft = buf[69];
			int irRight = buf[70];
			int leftMotorCurrent = (short)((buf[71] << 8) | buf[72]);
			int rightMotorCurrent = (short)((buf[73] << 8) | buf[74]);
			int mainBrushCurrent = (short)((buf[75] << 8) | buf[76]);
			int sideBrushCurrent = (short)((buf[77] << 8) | buf[78]);
			int stasis = buf[79];
			bool extChanged = false;

			g_sensors[ROOMBA_SENSOR_ENCODER_LEFT].lastReading = encLeft;
			g_sensors[ROOMBA_SENSOR_ENCODER_RIGHT].lastReading = encRight;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMPER].lastReading = lightBumper;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMP_LEFT].lastReading = lightBumpLeft;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMP_FL].lastReading = lightBumpFL;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMP_CL].lastReading = lightBumpCL;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMP_CR].lastReading = lightBumpCR;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMP_FR].lastReading = lightBumpFR;
			g_sensors[ROOMBA_SENSOR_LIGHT_BUMP_RIGHT].lastReading = lightBumpRight;
			g_sensors[ROOMBA_SENSOR_IR_LEFT].lastReading = irLeft;
			g_sensors[ROOMBA_SENSOR_IR_RIGHT].lastReading = irRight;
			g_sensors[ROOMBA_SENSOR_LEFT_MOTOR_CURRENT].lastReading = leftMotorCurrent;
			g_sensors[ROOMBA_SENSOR_RIGHT_MOTOR_CURRENT].lastReading = rightMotorCurrent;
			g_sensors[ROOMBA_SENSOR_MAIN_BRUSH_CURRENT].lastReading = mainBrushCurrent;
			g_sensors[ROOMBA_SENSOR_SIDE_BRUSH_CURRENT].lastReading = sideBrushCurrent;
			g_sensors[ROOMBA_SENSOR_STASIS].lastReading = stasis;

			if (encLeft != g_roomba_last_encoder_left || encRight != g_roomba_last_encoder_right) {
				g_roomba_last_encoder_left = encLeft;
				g_roomba_last_encoder_right = encRight;
				extChanged = true;
			}
			if (lightBumper != g_roomba_last_light_bumper) {
				g_roomba_last_light_bumper = lightBumper;
				extChanged = true;
			}
			if (irLeft != g_roomba_last_ir_left) {
				g_roomba_last_ir_left = irLeft;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: IR Left changed: %d (%s)",
					irLeft, Roomba_IrOmniStr(irLeft));
			}
			if (irRight != g_roomba_last_ir_right) {
				g_roomba_last_ir_right = irRight;
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: IR Right changed: %d (%s)",
					irRight, Roomba_IrOmniStr(irRight));
			}
			if (leftMotorCurrent != g_roomba_last_left_motor_current ||
				rightMotorCurrent != g_roomba_last_right_motor_current ||
				mainBrushCurrent != g_roomba_last_main_brush_current ||
				sideBrushCurrent != g_roomba_last_side_brush_current ||
				stasis != g_roomba_last_stasis) {
				g_roomba_last_left_motor_current = leftMotorCurrent;
				g_roomba_last_right_motor_current = rightMotorCurrent;
				g_roomba_last_main_brush_current = mainBrushCurrent;
				g_roomba_last_side_brush_current = sideBrushCurrent;
				g_roomba_last_stasis = stasis;
				extChanged = true;
			}
			if (extChanged) {
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
					"Roomba: G100 ext Enc:%d/%d LB:0x%02X MC:%d/%d/%d/%d St:%d",
					encLeft, encRight, lightBumper,
					leftMotorCurrent, rightMotorCurrent, mainBrushCurrent, sideBrushCurrent, stasis);
			}
		}
		
		// Publish sensors via MQTT
		Roomba_PublishSensors();
		
		// Run State Inference (Decoupled from MQTT)
		Roomba_UpdateState();
		Roomba_UpdateRuntimeErrorClassifier(nowTick);
		Roomba_UpdateRunDurationTracker(nowTick);
		
		// Publish vacuum JSON only on meaningful change, else heartbeat every 15s.
		uint32_t now = xTaskGetTickCount();
		Roomba_MQTT_PublishVacuumStateJSONIfNeeded(now);

		// Drain any trailing bytes that arrived beyond the expected frame.
		// On some runs we observe a consistent 13-byte tail after valid Group 100,
		// which otherwise lingers and blocks subsequent polls.
		{
			int tail = UART_GetDataSizeEx(g_roomba_uart);
			if (tail > 0) {
				UART_ConsumeBytesEx(g_roomba_uart, tail);
				// For Roomba 780 we frequently observe a recurring 13-byte tail.
				// Keep logs quiet for the known recurring case, but warn on unexpected sizes.
				uint32_t tailInfoPeriodTicks = (uint32_t)(configTICK_RATE_HZ * 60);
				if (tailInfoPeriodTicks < 1) {
					tailInfoPeriodTicks = 1;
				}
				if (tail != 13) {
					Roomba_RecordDriverError(ROOMBA_DRV_ERR_RX_UNEXPECTED_TAIL);
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV,
						"Roomba: Dropped unexpected trailing RX bytes after frame parse: %d", tail);
					g_rx_tail_drop_last_log_tick = nowTick;
				} else if (g_rx_tail_drop_last_log_tick == 0 ||
					(nowTick - g_rx_tail_drop_last_log_tick) >= tailInfoPeriodTicks) {
					addLogAdv(LOG_INFO, LOG_FEATURE_DRV,
						"Roomba: Dropped recurring 13-byte trailing RX tail");
					g_rx_tail_drop_last_log_tick = nowTick;
				}
			}
			// Reset pending trackers so scheduler doesn't carry stale stall state.
			g_rx_last_pending = 0;
			g_rx_last_pending_uart = 0;
			g_rx_last_pending_frame = 0;
			g_rx_pending_since_tick = 0;
		}
		
		// Reset frame accumulator for next frame
		s_frameLen = 0;

	}

	else if (totalPending > (int)sizeof(s_frame)) {
		// Flush buffer if it's getting full (likely garbage)
		Roomba_RecordDriverError(ROOMBA_DRV_ERR_RX_MALFORMED);
		addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Roomba: Flushing overfull buffer (%d bytes)", avail);
		UART_ConsumeBytesEx(g_roomba_uart, avail);
		s_frameLen = 0;
	}
}

/**
	* Append Roomba sensor information to the HTTP index page
	* Formatted as a compact status table
	*/
static void Roomba_AddStatusToHTTPPage(http_request_t *request)
{
	uint32_t nowTick = xTaskGetTickCount();
	int isWorking = Roomba_IsWorkingState(g_vacuum_state);
	const char *mainCmd = isWorking ? "RoombaOrder pause" : "RoombaOrder start";
	const char *mainLabel = isWorking ? "Pause" : "Clean";

	// Command Buttons (top row) - call /cm?cmnd=... so we don't need http_fns.c hacks
	poststr(request, "<hr>");
	poststr(request, "<div id='roomba_cmd_msg' style='margin:6px 0; font-size:12px;'></div>");
	poststr(request,
		"<script>"
		"function roombaSend(cmnd){"
		"var el=document.getElementById('roomba_cmd_msg');"
		"el.innerHTML='Sending: <b>'+cmnd+'</b> ...';"
		"fetch('/cm?cmnd='+encodeURIComponent(cmnd))"
		".then(r=>r.text())"
		".then(t=>{el.innerHTML='OK: <b>'+cmnd+'</b>';})"
		".catch(e=>{el.innerHTML='Error sending <b>'+cmnd+'</b>';});"
		"return false;"
		"}"
		"</script>"
	);

	poststr(request, "<table style='width:100%'><tr>");
	hprintf255(request, "<td style='width:34%%'><button type='button' class='btn btn-primary' style='width:100%%' onclick=\"return roombaSend('%s')\">%s</button></td>",
		mainCmd, mainLabel);
	poststr(request, "<td style='width:33%'><button type='button' class='btn' style='width:100%' onclick=\"return roombaSend('Roomba_Spot')\">Spot</button></td>");
	poststr(request, "<td style='width:33%'><button type='button' class='btn' style='width:100%' onclick=\"return roombaSend('Roomba_Dock')\">Dock</button></td>");
	poststr(request, "</tr></table>");
		
	poststr(request, "<hr><h5>Roomba Sensors</h5><table style='width:100%'>");
	
	// Voltage
	poststr(request, "<tr><td><b>Batt Voltage</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.2f V</td></tr>", g_sensors[ROOMBA_SENSOR_VOLTAGE].lastReading / 1000.0f);
	
	// Current
	poststr(request, "<tr><td><b>Batt Current</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.2f A</td></tr>", g_sensors[ROOMBA_SENSOR_CURRENT].lastReading / 1000.0f);
	
	// Power (Calculated: V * A)
	float power_w = (g_sensors[ROOMBA_SENSOR_VOLTAGE].lastReading * g_sensors[ROOMBA_SENSOR_CURRENT].lastReading) / 1000000.0f;
	poststr(request, "<tr><td><b>Power</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.2f W</td></tr>", power_w);

	// Temperature
	poststr(request, "<tr><td><b>Batt Temp</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.0f C</td></tr>", g_sensors[ROOMBA_SENSOR_TEMPERATURE].lastReading);

	// Charge / Capacity
	poststr(request, "<tr><td><b>Charge/Cap</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.0f/%.0f mAh (%.0f%%)</td></tr>", 
		g_sensors[ROOMBA_SENSOR_CHARGE].lastReading, 
		g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading,
		(g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading > 0) ? 
			(g_sensors[ROOMBA_SENSOR_CHARGE].lastReading / g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading * 100.0f) : 0);

	// Charging State
	const char *charging_states[] = {"Not charging", "Reconditioning", "Full charging", "Trickle charging", "Waiting", "Fault"};
	int charging_state = g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading;
	const char *state_str = (charging_state >= 0 && charging_state <= 5) ? charging_states[charging_state] : "Unknown";
	poststr(request, "<tr><td><b>Charging</b></td><td style='text-align: right;'>");
	hprintf255(request, "%s</td></tr>", state_str);

	// Activity (based on the robust state inference)
	const char* actStr = "Idle";
	switch (g_vacuum_state) {
		case VACUUM_STATE_DOCKED:    actStr = "Docked"; break;
		case VACUUM_STATE_CLEANING:  actStr = "Cleaning"; break;
		case VACUUM_STATE_RETURNING: actStr = "Docking"; break;
		case VACUUM_STATE_PAUSED:    actStr = "Paused"; break;
		case VACUUM_STATE_IDLE:      actStr = "Idle"; break;
		case VACUUM_STATE_ERROR:     actStr = "Error"; break;
		default:                     actStr = "Unknown"; break;
	}
	poststr(request, "<tr><td><b>Activity</b></td><td style='text-align: right;'>");
	poststr(request, actStr);
	poststr(request, "</td></tr>");

	// Error (Robot Faults)
	{
		const char *errorStr = "OK";
		if (g_runtime_error_active) {
			errorStr = Roomba_RuntimeErrorStr(g_runtime_error_reason);
		} else if (charging_state == 5) {
			errorStr = "Charging fault";
		}
		poststr(request, "<tr><td><b>Error</b></td><td style='text-align: right;'>");
		poststr(request, errorStr);
		poststr(request, "</td></tr>");
	}


	// Run time tracker (hh:mm, NTP-independent)
	{
		char curRun[16];
		char lastRun[16];
		Roomba_FormatDurationHm(curRun, sizeof(curRun), g_run_current_duration_sec);
		Roomba_FormatDurationHm(lastRun, sizeof(lastRun), g_run_last_duration_sec);
		poststr(request, "<tr><td><b>Current Run</b></td><td style='text-align: right;'>");
		poststr(request, curRun);
		poststr(request, "</td></tr>");
		poststr(request, "<tr><td><b>Last Run</b></td><td style='text-align: right;'>");
		poststr(request, lastRun);
		poststr(request, "</td></tr>");
	}
	
	poststr(request, "</table>");	
	hprintf255(request, "<hr>");
}

void Roomba_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState)
		return;

	Roomba_AddStatusToHTTPPage(request);
}

/**
	* Handle Home Assistant Discovery
	* Called when HA discovery is requested (e.g. on startup or button press)
	* Publishes configuration payloads for all Roomba sensors
	*/
// Handle Home Assistant Discovery
// Handle Home Assistant Discovery
void Roomba_OnHassDiscovery(const char *topic) {
	if (!MQTT_IsReady()) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: MQTT not ready, skipping discovery.");
		return;
	}

	const char *devName  = CFG_GetDeviceName();
	const char *clientId = CFG_GetMQTTClientId();

	// Use static buffers to avoid stack overflow (user requested)
	static char unique_id[96];
	static char config_topic[192];
	static char payload[768];

	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Discovery start (devName=%s clientId=%s)", devName, clientId);

	// 1. Publish Sensors (Iterate g_sensors)
	for (int i = 0; i < ROOMBA_SENSOR__COUNT; i++) {
		if (strlen(g_sensors[i].name_mqtt) == 0) {
			continue;
		}
		if (strlen(g_sensors[i].hass_dev_class) == 0 && strlen(g_sensors[i].units) == 0 && !g_sensors[i].is_binary) {
			if (!Roomba_IsTextSensor(i))
				continue;
		}

		// unique id
		snprintf(unique_id, sizeof(unique_id), "%s_roomba_%s", devName, g_sensors[i].name_mqtt);

		const char *component = g_sensors[i].is_binary ? "binary_sensor" : "sensor";

		// homeassistant/<component>/<unique_id>/config
		snprintf(config_topic, sizeof(config_topic), "homeassistant/%s/%s/config", component, unique_id);

		// build JSON safely
		size_t n = 0;
		n += snprintf(payload + n, sizeof(payload) - n,
			"{\"name\":\"%s\",\"uniq_id\":\"%s\",",
			g_sensors[i].name_friendly, unique_id);

		// stat_t: topic <clientId>/<name_mqtt>/get
		// This matches MQTT_PublishMain_StringFloat() publishing
		n += snprintf(payload + n, sizeof(payload) - n,
			"\"stat_t\":\"%s/%s/get\",",
			clientId, g_sensors[i].name_mqtt);

		n += snprintf(payload + n, sizeof(payload) - n,
			"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"iRobot\",\"mdl\":\"Roomba\"},",
			devName, devName);

		if (strlen(g_sensors[i].hass_dev_class) > 0) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"dev_cla\":\"%s\",", g_sensors[i].hass_dev_class);
		}
		if (strlen(g_sensors[i].units) > 0) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"unit_of_meas\":\"%s\",", g_sensors[i].units);
		}
		{
			const char *icon = Roomba_GetSensorIcon(i);
			if (icon != NULL) {
				n += snprintf(payload + n, sizeof(payload) - n,
					"\"icon\":\"%s\",", icon);
			}
		}
		
		if (g_sensors[i].is_binary) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"pl_on\":\"ON\",\"pl_off\":\"OFF\",");
		}
		
		// Enum sensors should declare options in discovery.
		if (strcmp(g_sensors[i].hass_dev_class, "enum") == 0) {
			const char *options = Roomba_GetEnumOptionsForSensor(i);
			if (options != NULL) {
			n += snprintf(payload + n, sizeof(payload) - n,
					"\"options\":%s,", options);
			}
		}

		if (!g_sensors[i].is_binary &&
			strcmp(g_sensors[i].hass_dev_class, "enum") != 0 &&
			!Roomba_IsTextSensor(i)) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"stat_cla\":\"measurement\",");
		}

		// remove trailing comma if present
		if (n > 0 && payload[n-1] == ',') n--;
		payload[n++] = '}';
		payload[n] = 0;

		MQTT_PublishMain_StringString(config_topic, payload,
			OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
		rtos_delay_milliseconds(20);
	}

	// 1b. Publish Computed Battery Level (%)
	{
		snprintf(unique_id, sizeof(unique_id), "%s_roomba_battery_level", devName);
		snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);

		snprintf(payload, sizeof(payload),
			"{"
			"\"name\":\"Battery Level\","
			"\"uniq_id\":\"%s\","
			"\"stat_t\":\"%s/battery_level/get\","
			"\"unit_of_meas\":\"%%\","
			"\"dev_cla\":\"battery\","
			"\"icon\":\"mdi:battery\","
			"\"stat_cla\":\"measurement\","
			"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"iRobot\",\"mdl\":\"Roomba\"}"
			"}",
			unique_id, clientId, devName, devName);

		MQTT_PublishMain_StringString(config_topic, payload,
			OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
		rtos_delay_milliseconds(20);
	}

	// 2. Vacuum entity discovery is published later (Tasmota-style). Removed duplicate block.

	// 3. Publish Buttons
	const char* cmd_names[] = {"Clean", "Spot", "Dock"};
	const char* cmd_funcs[] = {"Roomba_Clean", "Roomba_Spot", "Roomba_Dock"}; 
	
	for (int i = 0; i < 3; i++) {
		snprintf(unique_id, sizeof(unique_id), "%s_roomba_btn_%s", devName, cmd_names[i]);
		snprintf(config_topic, sizeof(config_topic), "homeassistant/button/%s/config", unique_id);
		
		snprintf(payload, sizeof(payload), 
			"{\"name\":\"%s\",\"uniq_id\":\"%s\","
			"\"cmd_t\":\"cmnd/%s/%s\","
			"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\"}}",
			cmd_names[i], unique_id,
			clientId, cmd_funcs[i],
			devName, devName);
		
		MQTT_PublishMain_StringString(config_topic, payload,
			OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
		rtos_delay_milliseconds(20);
	}
	
	// ========================================================================
	// VACUUM ENTITY DISCOVERY
	// ========================================================================
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Publishing vacuum entity discovery...");
	
	snprintf(unique_id, sizeof(unique_id), "%s_vacuum", devName);
	// Tasmota/HA Standard: Topic based on ClientID
	snprintf(config_topic, sizeof(config_topic), "homeassistant/vacuum/%s/config", devName);
	
	// Get version and IP for device info
	extern const char *g_build_str;
	const char *ip = HAL_GetMyIPString();
	
	// Build vacuum discovery payload using Tasmota-style abbreviations (~)
	// and full device metadata to ensure HA accepts it.
	snprintf(payload, sizeof(payload),
		"{\"name\":\"Vacuum\",\"uniq_id\":\"%s\","
		"\"~\":\"%s\","
		"\"schema\":\"state\","
		"\"cmd_t\":\"cmnd/%s/RoombaOrder\","
		"\"stat_t\":\"~/vacuum/state/get\","
		"\"val_tpl\":\"{{ value_json.state }}\","
		"\"json_attr_t\":\"~/vacuum/attr/get\","
		"\"send_cmd_t\":\"cmnd/%s/RoombaOrder\","
		"\"supported_features\":[\"start\",\"pause\",\"stop\",\"return_home\",\"clean_spot\",\"locate\",\"send_command\"],"
		"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"sw\":\"%s\",\"mf\":\"iRobot\",\"mdl\":\"Roomba\",\"cu\":\"http://%s/index\"}}",
		unique_id,
		clientId,
		clientId, // For cmd_t
		clientId, // For send_cmd_t
		devName, // For dev.ids
		devName,
		g_build_str,
		ip);
	
	MQTT_PublishMain_StringString(config_topic, payload,
		OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
	rtos_delay_milliseconds(50);  // Delay to prevent MQTT overload
	Roomba_MQTT_PublishAvailability(1);
	
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: HA discovery complete!");
}




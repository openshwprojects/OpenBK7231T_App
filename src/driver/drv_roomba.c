/*
 * Roomba Open Interface (OI) Driver for OpenBeken
 * 
 * This driver implements basic sensor reading functionality for iRobot Roomba vacuum cleaners
 * using the Roomba Open Interface (OI) protocol over UART.
 * 
 * Supported Features:
 * - Battery voltage monitoring (Packet ID 22)
 * - Temperature monitoring (Packet ID 24)
 * - Automatic sensor polling every 5 seconds
 * - Channel-based value publishing (compatible with MQTT/Home Assistant)
 * 
 * Hardware Requirements:
 * - UART connection to Roomba's Mini-DIN connector
 * - Default: UART1 (TX2/RX2) at 115200 baud, 8N1
 * - Roomba models: 500/600/700/800/900 series (OI Spec compatible)
 * 
 * Channel Configuration:
 * - Channel 1: Battery Voltage (in mV)
 * - Channel 2: Temperature (in ??C ?? 10 for decimal precision)
 * 
 * Protocol Reference:
 * - Roomba Open Interface Specification
 * - Command format: [Opcode] [Data bytes...]
 * - Sensor request: 0x8E (142) [Packet ID]
 * - Response: Raw data bytes (big-endian for multi-byte values)
 * 
 * Author: OpenBeken Community
 * License: Same as OpenBeken project
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

// Vacuum state for Home Assistant integration
static vacuum_state_t g_vacuum_state = VACUUM_STATE_IDLE;

// MQTT Publishing Control (BL_shared style)
static int stat_updatesSkipped = 0;
static int stat_updatesSent = 0;
static int changeSendAlwaysFrames = 60;  // Force publish every 60 seconds
static int changeDoNotSendMinFrames = 5; // Minimum 5 seconds between publishes

// Functions moved to end of file to ensure g_sensors is defined

// Polling state machine
static int g_poll_counter = 0;
static int g_hass_discovery_timer = 0;    // Manual trigger only via Roomba_Discovery command
static bool g_has_auto_discovered = false;

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
	{"Battery Current", "battery_current", "mA", "current", 0, 50.0f, 0, 0, 0, PACKET_CURRENT, 2, false},
	{"Temperature", "temperature", "°C", "temperature", 1, 1.0f, 0, 0, 0, PACKET_TEMPERATURE, 1, false},
	{"Battery Charge", "battery_charge", "mAh", "battery", 0, 10.0f, 0, 0, 0, PACKET_CHARGE, 2, false},
	{"Battery Capacity", "battery_capacity", "mAh", "battery", 0, 10.0f, 0, 0, 0, PACKET_CAPACITY, 2, false},
	{"Charging State", "charging_state", "", "enum", 0, 0.5f, 0, 0, 0, PACKET_CHARGING_STATE, 1, false},
	
	// Phase 3: Group 6 Sensors
	{"Bump Right", "bump_right", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"Bump Left", "bump_left", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"WheelDrop Right", "wheeldrop_right", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"WheelDrop Left", "wheeldrop_left", "", "problem", 0, 0.5f, 0, 0, 0, PACKET_BUMPS_DROPS, 1, true},
	{"Wall Sensor", "wall", "", "occupancy", 0, 0.5f, 0, 0, 0, PACKET_WALL, 1, true},
	{"Cliff Left", "cliff_left", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_LEFT, 1, true},
	{"Cliff Front Left", "cliff_front_left", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_FL, 1, true},
	{"Cliff Front Right", "cliff_front_right", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_FR, 1, true},
	{"Cliff Right", "cliff_right", "", "safety", 0, 0.5f, 0, 0, 0, PACKET_CLIFF_RIGHT, 1, true},
	{"Virtual Wall", "virtual_wall", "", "occupancy", 0, 0.5f, 0, 0, 0, PACKET_VIRTUAL_WALL, 1, true},
	{"Dirt Detect", "dirt_detect", "", "occupancy", 0, 0.5f, 0, 0, 0, PACKET_DIRT_DETECT, 1, false},
	
	// Buttons (Parsed from Packet 18)
	{"Button Clean", "button_clean", "", "running", 0, 0.5f, 0, 0, 0, PACKET_BUTTONS, 1, true},
	{"Button Spot", "button_spot", "", "running", 0, 0.5f, 0, 0, 0, PACKET_BUTTONS, 1, true},
	{"Button Dock", "button_dock", "", "running", 0, 0.5f, 0, 0, 0, PACKET_BUTTONS, 1, true},
	{"Button Max", "button_max", "", "running", 0, 0.5f, 0, 0, 0, PACKET_BUTTONS, 1, true},
};

// ============================================================================
// UART COMMUNICATION FUNCTIONS
// ============================================================================

/**
 * Send a single byte to the Roomba via UART
 * @param b Byte to send
 */
void Roomba_SendByte(byte b) {
	UART_SendByteEx(g_roomba_uart, b);
	// Debug: Log every byte sent
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba TX: 0x%02X (%d)", b, b);
}

/**
 * Send a Roomba OI command (single opcode)
 * @param cmd Command opcode (see drv_roomba.h for definitions)
 */
void Roomba_SendCmd(int cmd) {
	Roomba_SendByte((byte)cmd);
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

commandResult_t CMD_Roomba_Clean(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_CLEAN);
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
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: SPOT command sent");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Dock(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_DOCK);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: DOCK command sent");
	return CMD_RES_OK;
}

static commandResult_t CMD_Roomba_Safe(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Roomba_SendCmd(CMD_SAFE);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: SAFE command sent (Reset)");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Stop(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Stop command (Debug)");
	return CMD_RES_OK;
}

commandResult_t CMD_Roomba_Discovery(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// Schedule discovery to run on main loop (safe stack)
	g_hass_discovery_timer = 1;
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
	if (stricmp(args, "clean") == 0) {
		Roomba_SendCmd(CMD_CLEAN);
		g_vacuum_state = VACUUM_STATE_CLEANING;
	} else if (stricmp(args, "dock") == 0 || stricmp(args, "return_to_base") == 0) {
		Roomba_SendCmd(CMD_DOCK);
		g_vacuum_state = VACUUM_STATE_RETURNING;
	} else if (stricmp(args, "stop") == 0) {
		// Stop means... Drive 0? or just Stop command? 
		// OI doesn't have explicit STOP in Cleaning mode except by switching mode.
		// Sending OFF (173)? Or just entering SAFE mode stops motors.
		Roomba_SendCmd(CMD_SAFE); // SAFE mode stops motors
		g_vacuum_state = VACUUM_STATE_IDLE;
	} else if (stricmp(args, "locate") == 0) {
		Roomba_SendCmd(CMD_PLAY); // Play a song?
	}
	
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Order received: %s", args);
	return CMD_RES_OK;
}

void Roomba_PublishVacuumState(void) {
	if (!MQTT_IsReady()) return;

	// Note: MQTT_Publish Main_StringString() automatically prepends clientId
	// So we just pass "roomba/state" and it becomes "<clientId>/roomba/state"
	const char* stateStr = "idle";
	
	// Infer state from sensors
	// Charging State: 0=Not, 1=Recond, 2=Full, 3=Trickle, 4=Waiting, 5=Fault
	int charging = g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading;
	float current = g_sensors[ROOMBA_SENSOR_CURRENT].lastReading;

	if (charging == 2 || charging == 3) {
		stateStr = "docked";
		g_vacuum_state = VACUUM_STATE_DOCKED;
	} else if (current < -400) { 
		// Significant negative current implies motors running -> cleaning
		stateStr = "cleaning";
		g_vacuum_state = VACUUM_STATE_CLEANING;
	} else if (g_vacuum_state == VACUUM_STATE_RETURNING) {
		stateStr = "returning";
	} else {
		stateStr = "idle";
		g_vacuum_state = VACUUM_STATE_IDLE;
	}

	// OLD: Publish sensor-style state topic for compatibility
	MQTT_PublishMain_StringString("state", stateStr, 0);
	
	// NEW: Publish vacuum-style JSON state for HA vacuum entity
	const char *clientId = CFG_GetMQTTClientId();
	char vacTopic[128];
	char vacPayload[256];
	const char *fanSpeed = "min";  // Roomba doesn't have variable speed
	
	// Handle paused state
	if (g_vacuum_state == VACUUM_STATE_PAUSED) {
		stateStr = "paused";
	}
	
	snprintf(vacTopic, sizeof(vacTopic), "%s/vacuum/state", clientId);
	snprintf(vacPayload, sizeof(vacPayload), "{\"state\":\"%s\",\"fan_speed\":\"%s\"}", 
		stateStr, fanSpeed);
	
	// Use low-level publish to avoid wrapper bugs/trailing slashes
	// MQTT_PublishMain_StringString(topic, payload, flags)
	extern int MQTT_PublishMain_StringString(const char* sChannel, const char* valueStr, int flags);
	MQTT_PublishMain_StringString(vacTopic, vacPayload, OBK_PUBLISH_FLAG_RAW_TOPIC_NAME);
}

/**
 * Publish Roomba sensor data via MQTT (BL_shared style)
 * Similar to BL09XX drivers - direct topic publishing with thresholds
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
			// Publish via MQTT (BL_shared style)
			if (g_sensors[i].is_binary) {
				// Binary sensors: "ON" / "OFF"
				MQTT_PublishMain_StringString(g_sensors[i].name_mqtt, 
					currentReading ? "ON" : "OFF", 0);
			} else {
				// Special-case: Charging State should publish TEXT, not a number
				if (i == ROOMBA_SENSOR_CHARGING_STATE) {
					MQTT_PublishMain_StringString(g_sensors[i].name_mqtt,
						Roomba_ChargingStateStr((int)currentReading), 0);
				} else {
					// Analog sensors: float value
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
		// Publish like BL drivers: <clientId>/battery_level/get
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
	Roomba_SendByte(CMD_SAFE);

	// Register console commands for manual testing
	CMD_RegisterCommand("Roomba_SendCmd", CMD_Roomba_SendCmd, NULL);
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
	
	// Note: HA discovery is triggered manually via Roomba_Discovery command
	// Not auto-triggered on startup (now handled by Roomba_RunEverySecond)
}

// ============================================================================
// SENSOR POLLING (CALLED EVERY SECOND)
// ============================================================================

/**
 * Called every second by the OpenBeken main loop
 * Implements a 5-second polling interval with alternating sensor requests
 * 
 * Polling sequence:
 * Requests Sensor Group 3 (Power) which returns 10 bytes of data
 * Includes: Charging State, Voltage, Current, Temperature, Charge, Capacity
 * Requests Sensor Group 6 every 5 seconds and handles HA discovery
 */
void Roomba_RunEverySecond() {
	// 1. Automatic Discovery on Startup / MQTT Reconnect
	// This fixes the "race condition" where discovery tried to run before MQTT was ready
	if (MQTT_IsReady()) {
		if (!g_has_auto_discovered) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Auto-triggering HA Discovery...");
			Roomba_OnHassDiscovery("homeassistant");
			g_has_auto_discovered = true;
		}
	} else {
		// Reset flag if MQTT disconnects to allow re-discovery on reconnection
		// g_has_auto_discovered = false; 
	}

	// 2. Handle HA Discovery Manual Trigger
	if (g_hass_discovery_timer > 0) {
		g_hass_discovery_timer--;
		if (g_hass_discovery_timer <= 0) {
			if (MQTT_IsReady()) {
				Roomba_OnHassDiscovery("homeassistant");
			}
			g_hass_discovery_timer = 0; // Always stop after one attempt
		}
	}
	
	// Request Sensor Group 6 every 5 seconds
	if (++g_poll_counter >= 5) {
		g_poll_counter = 0;
		Roomba_SendByte(CMD_SENSORS);
		Roomba_SendByte(PACKET_GROUP_6); // Group 6 (52 bytes)
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Requesting Sensor Group 6");
	}
	
	// Publish Vacuum State (State Machine / Inference)
	Roomba_PublishVacuumState();
}

// ============================================================================
// UART RESPONSE HANDLER (CALLED FREQUENTLY)
// ============================================================================

/**
 * Called frequently by OpenBeken main loop
 * Parses incoming 52-byte Group 6 response directly (no buffer checking)
 */
void Roomba_OnQuickTick() {
	int avail = UART_GetDataSizeEx(g_roomba_uart);
	
	// Parse Group 6 response (52 bytes) when available
	if (avail >= 52) {
		byte buf[52];
		for (int i = 0; i < 52; i++) {
			buf[i] = UART_GetByteEx(g_roomba_uart, i);
		}
		
		// Sanity Check: Charging State (Byte 16) must be 0-5
		if (buf[16] > 5) {
			addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Roomba: Invalid Charging State (%d). Flushing buffer.", buf[16]);
			UART_ConsumeBytesEx(g_roomba_uart, avail);
			return;
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
		
		// Packet 15: Dirt Detect (Byte 8)
		g_sensors[ROOMBA_SENSOR_DIRT_DETECT].lastReading = buf[8];
		
		// Packet 18: Buttons (Byte 11)
		g_sensors[ROOMBA_SENSOR_BUTTON_CLEAN].lastReading = (buf[11] & 0x01) ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_BUTTON_SPOT].lastReading = (buf[11] & 0x02) ? 1 : 0;
		g_sensors[ROOMBA_SENSOR_BUTTON_DOCK].lastReading = (buf[11] & 0x04) ? 1 : 0;
		
		// Packet 21: Charging State (Byte 16)
		g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading = buf[16];
		
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
		
		// Debug: Log parsed values
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: Parsed - V:%dmV I:%dmA T:%dC Charge:%d/%dmAh State:%d",
			(int)g_sensors[ROOMBA_SENSOR_VOLTAGE].lastReading,
			(int)g_sensors[ROOMBA_SENSOR_CURRENT].lastReading,
			(int)g_sensors[ROOMBA_SENSOR_TEMPERATURE].lastReading,
			(int)g_sensors[ROOMBA_SENSOR_CHARGE].lastReading,
			(int)g_sensors[ROOMBA_SENSOR_CAPACITY].lastReading,
			(int)g_sensors[ROOMBA_SENSOR_CHARGING_STATE].lastReading);
		
		// Publish sensors via MQTT (BL_shared style)
		Roomba_PublishSensors();
		
		// Consume the 52 bytes from buffer
		UART_ConsumeBytesEx(g_roomba_uart, 52);
	}
	else if (avail > 64) {
		// Flush buffer if it's getting full (likely garbage)
		addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Roomba: Flushing overfull buffer (%d bytes)", avail);
		UART_ConsumeBytesEx(g_roomba_uart, avail);
	}
}

/**
 * Append Roomba sensor information to the HTTP index page
 * Formatted like BL0937 driver
 */
void Roomba_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState)
		return;
		
	poststr(request, "<hr><h5>Roomba Sensors</h5><table style='width:100%'>");
	
	// Voltage
	poststr(request, "<tr><td><b>Voltage</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.2f V</td></tr>", g_sensors[ROOMBA_SENSOR_VOLTAGE].lastReading / 1000.0f);
	
	// Current
	poststr(request, "<tr><td><b>Current</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.2f A</td></tr>", g_sensors[ROOMBA_SENSOR_CURRENT].lastReading / 1000.0f);
	
	// Power (Calculated: V * A)
	float power_w = (g_sensors[ROOMBA_SENSOR_VOLTAGE].lastReading * g_sensors[ROOMBA_SENSOR_CURRENT].lastReading) / 1000000.0f;
	poststr(request, "<tr><td><b>Power</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.2f W</td></tr>", power_w);

	// Temperature
	poststr(request, "<tr><td><b>Temperature</b></td><td style='text-align: right;'>");
	hprintf255(request, "%.0f C</td></tr>", g_sensors[ROOMBA_SENSOR_TEMPERATURE].lastReading);

	// Battery
	poststr(request, "<tr><td><b>Battery</b></td><td style='text-align: right;'>");
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

	poststr(request, "</table>");
	
	// Binary Sensors Table
	// Binary Sensors Table (Generated from loop)
	poststr(request, "<hr><h5>Binary Sensors</h5><table style='width:100%'>");
	poststr(request, "<tr><th>Sensor</th><th style='text-align: right;'>Status</th></tr>");
	
	for (int i = 0; i < ROOMBA_SENSOR__COUNT; i++) {
		if (g_sensors[i].is_binary) {
			poststr(request, "<tr><td>");
			poststr(request, g_sensors[i].name_friendly);
			poststr(request, "</td><td style='text-align: right;'>");
			poststr(request, g_sensors[i].lastReading ? "YES" : "No");
			poststr(request, "</td></tr>");
		}
	}

	poststr(request, "</table>");
	
	// Command Buttons - 4 per row, full width
	poststr(request, "<hr><table style='width:100%'><tr>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_Clean' value='1' class='btn btn-primary' style='width:100%'>Clean</button></form></td>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_Spot' value='1' class='btn' style='width:100%'>Spot</button></form></td>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_Dock' value='1' class='btn' style='width:100%'>Dock</button></form></td>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_Max' value='1' class='btn' style='width:100%'>Max</button></form></td>");
	poststr(request, "</tr></table>");
	
	// Mode Toggles & Reset
	poststr(request, "<h5>OI Mode Control</h5>");
	poststr(request, "<table style='width:100%'><tr>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_SetPassive' value='1' class='btn' style='width:100%'>Passive</button></form></td>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_SetSafe' value='1' class='btn btn-success' style='width:100%'>Safe</button></form></td>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_SetFull' value='1' class='btn btn-warning' style='width:100%'>Full</button></form></td>");
	poststr(request, "<td style='width:25%'><form action='/index' method='get'><button name='Roomba_Safe' value='1' class='btn btn-danger' style='width:100%'>Reset</button></form></td>");
	poststr(request, "</tr></table>");
	
	hprintf255(request, "<hr>");
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
			"{\"name\":\"%s %s\",\"uniq_id\":\"%s\",",
			devName, g_sensors[i].name_friendly, unique_id);

		// stat_t: BL-style topic <clientId>/<name_mqtt>/get
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
		
		if (g_sensors[i].is_binary) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"pl_on\":\"ON\",\"pl_off\":\"OFF\",");
		}
		


		// Enum sensors (like Charging State) should declare options in discovery
		if (strcmp(g_sensors[i].hass_dev_class, "enum") == 0) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"options\":[\"not_charging\",\"reconditioning\",\"full_charging\",\"trickle_charging\",\"waiting\",\"charging_fault\",\"unknown\"],");
		}

		if (!g_sensors[i].is_binary && strcmp(g_sensors[i].hass_dev_class, "enum") != 0) {
			n += snprintf(payload + n, sizeof(payload) - n,
				"\"stat_cla\":\"measurement\",");
		}

		// remove trailing comma if present
		if (n > 0 && payload[n-1] == ',') payload[n-1] = 0;

		snprintf(payload + strlen(payload), sizeof(payload) - strlen(payload), "}");

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
			"\"name\":\"%s Battery Level\","
			"\"uniq_id\":\"%s\","
			"\"stat_t\":\"%s/battery_level/get\","
			"\"unit_of_meas\":\"%%\","
			"\"dev_cla\":\"battery\","
			"\"icon\":\"mdi:battery\","
			"\"stat_cla\":\"measurement\","
			"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"iRobot\",\"mdl\":\"Roomba\"}"
			"}",
			devName, unique_id, clientId, devName, devName);

		MQTT_PublishMain_StringString(config_topic, payload,
			OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
		rtos_delay_milliseconds(20);
	}

	// 2. Vacuum entity discovery is published later (Tasmota-style). Removed duplicate block.

	// 3. Publish Buttons
	const char* cmd_names[] = {"Clean", "Spot", "Dock", "Max", "SafeMode", "Passive", "Full", "Reset"};
	const char* cmd_funcs[] = {"Roomba_Clean", "Roomba_Spot", "Roomba_Dock", "Roomba_Max", "Roomba_SetSafe", "Roomba_SetPassive", "Roomba_SetFull", "Roomba_Safe"}; 
	
	for (int i = 0; i < 8; i++) {
		snprintf(unique_id, sizeof(unique_id), "%s_roomba_btn_%s", devName, cmd_names[i]);
		snprintf(config_topic, sizeof(config_topic), "homeassistant/button/%s/config", unique_id);
		
		snprintf(payload, sizeof(payload), 
			"{\"name\":\"%s %s\",\"uniq_id\":\"%s\","
			"\"cmd_t\":\"cmnd/%s/%s\","
			"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\"}}",
			devName, cmd_names[i], unique_id,
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
		"\"cmd_t\":\"~/vacuum/command\","
		"\"stat_t\":\"~/vacuum/state\","
		"\"send_cmd_t\":\"~/vacuum/send_command\","
		"\"sup_feat\":[\"start\",\"pause\",\"stop\",\"return_home\",\"clean_spot\"],"
		"\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"sw\":\"%s\",\"mf\":\"iRobot\",\"mdl\":\"Roomba\",\"cu\":\"http://%s/index\"}}",
		unique_id,
		clientId,
		devName, devName, g_build_str, ip);
	
	MQTT_PublishMain_StringString(config_topic, payload,
		OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);
	rtos_delay_milliseconds(50);  // Delay to prevent MQTT overload
	
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Roomba: HA discovery complete!");
}

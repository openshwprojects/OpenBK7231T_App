/*
 * Roomba Open Interface (OI) Driver - Header File
 * 
 * This header defines opcodes and packet IDs for the Roomba Open Interface protocol.
 * 
 * Protocol Overview:
 * - The Roomba OI uses a simple serial protocol over UART
 * - Commands are sent as single-byte opcodes, sometimes followed by data bytes
 * - Sensor data is requested using opcode 142 (SENSORS) + packet ID
 * - Responses are raw binary data (big-endian for multi-byte values)
 * 
 * Modes:
 * - OFF: Roomba is off (no commands accepted)
 * - PASSIVE: Can read sensors, cannot control motors (entered via START command)
 * - SAFE: Can read sensors and control motors, stops on safety events
 * - FULL: Full control, ignores safety sensors (use with caution!)
 * 
 * For complete protocol documentation, see:
 * "iRobot Roomba Open Interface (OI) Specification"
 * Available from iRobot's developer resources
 */

#ifndef __DRV_ROOMBA_H__
#define __DRV_ROOMBA_H__

#include "../cmnds/cmd_public.h"  // For commandResult_t
#include "../httpserver/http_fns.h"  // For http_request_t

// Roomba ROI Opcodes
#define CMD_START            128
#define CMD_BAUD             129
#define CMD_CONTROL          130
#define CMD_SAFE             131
#define CMD_FULL             132
#define CMD_POWER            133
#define CMD_SPOT             134
#define CMD_CLEAN            135
#define CMD_MAX              136
#define CMD_DRIVE            137
#define CMD_MOTORS           138
#define CMD_LEDS             139
#define CMD_SONG             140
#define CMD_PLAY             141
#define CMD_SENSORS          142
#define CMD_DOCK             143
#define CMD_PWM_MOTORS       144
#define CMD_DRIVE_DIRECT     145
#define CMD_DRIVE_PWM        146
#define CMD_STREAM           148
#define CMD_QUERY_LIST       149
#define CMD_DO_STREAM        150
#define CMD_SEND_IR          151
#define CMD_SCRIPT           152
#define CMD_PLAY_SCRIPT      153
#define CMD_SHOW_SCRIPT      154
#define CMD_WAIT_TIME        155
#define CMD_WAIT_DISTANCE    156
#define CMD_WAIT_ANGLE       157
#define CMD_WAIT_EVENT       158

// ============================================================================
// SENSOR PACKET IDs
// ============================================================================

#define PACKET_GROUP_6        6    // Group 6: All sensors (52 bytes) - Packets 7-42

// Individual sensor packets
#define PACKET_BUMPS_DROPS    7    // Bumps and wheel drops (1 byte, bitfield)
#define PACKET_WALL           8    // Wall sensor (1 byte, boolean)
#define PACKET_CLIFF_LEFT     9    // Cliff sensor left (1 byte, boolean)
#define PACKET_CLIFF_FL       10   // Cliff sensor front left (1 byte, boolean)
#define PACKET_CLIFF_FR       11   // Cliff sensor front right (1 byte, boolean)
#define PACKET_CLIFF_RIGHT    12   // Cliff sensor right (1 byte, boolean)
#define PACKET_VIRTUAL_WALL   13   // Virtual wall (1 byte, boolean)
#define PACKET_OVERCURRENTS   14   // Motor overcurrents (1 byte, bitfield)
#define PACKET_DIRT_DETECT    15   // Dirt detect (1 byte)
#define PACKET_IR_OPCODE      17   // IR opcode (1 byte)
#define PACKET_BUTTONS        18   // Buttons (1 byte, bitfield)
#define PACKET_DISTANCE       19   // Distance traveled (2 bytes, signed, mm)
#define PACKET_ANGLE          20   // Angle turned (2 bytes, signed, degrees)
#define PACKET_CHARGING_STATE 21   // Charging state (1 byte, enum)
#define PACKET_VOLTAGE        22   // Battery voltage (2 bytes, unsigned, mV)
#define PACKET_CURRENT        23   // Battery current (2 bytes, signed, mA)
#define PACKET_TEMPERATURE    24   // Battery temperature (1 byte, signed, °C)
#define PACKET_CHARGE         25   // Battery charge (2 bytes, unsigned, mAh)
#define PACKET_CAPACITY       26   // Battery capacity (2 bytes, unsigned, mAh)

// Charging states (for PACKET_CHARGING_STATE)
#define CHARGING_NOT          0    // Not charging
#define CHARGING_RECOND       1    // Reconditioning charging
#define CHARGING_FULL         2    // Full charging
#define CHARGING_TRICKLE      3    // Trickle charging
#define CHARGING_WAITING      4    // Waiting
#define CHARGING_FAULT        5    // Charging fault

// ============================================================================
// SENSOR STRUCTURE
// ============================================================================

// Sensor types for auto-publishing
typedef enum {
	ROOMBA_SENSOR_VOLTAGE = 0,
	ROOMBA_SENSOR_CURRENT,
	ROOMBA_SENSOR_TEMPERATURE,
	ROOMBA_SENSOR_CHARGE,
	ROOMBA_SENSOR_CAPACITY,
	ROOMBA_SENSOR_CHARGING_STATE,
	
	// Phase 3 Extensions (Group 6)
	ROOMBA_SENSOR_BUMP_RIGHT,
	ROOMBA_SENSOR_BUMP_LEFT,
	ROOMBA_SENSOR_WHEELDROP_RIGHT,
	ROOMBA_SENSOR_WHEELDROP_LEFT,
	ROOMBA_SENSOR_WALL,
	ROOMBA_SENSOR_CLIFF_LEFT,
	ROOMBA_SENSOR_CLIFF_FL,
	ROOMBA_SENSOR_CLIFF_FR,
	ROOMBA_SENSOR_CLIFF_RIGHT,
	ROOMBA_SENSOR_VIRTUAL_WALL,
	ROOMBA_SENSOR_DIRT_DETECT,
	
	// Buttons
	ROOMBA_SENSOR_BUTTON_CLEAN,
	ROOMBA_SENSOR_BUTTON_SPOT,
	ROOMBA_SENSOR_BUTTON_DOCK,
	ROOMBA_SENSOR_BUTTON_MAX,

	ROOMBA_SENSOR__COUNT  // Total number of sensors (21)
} roomba_sensor_type_t;

// Sensor metadata for auto-publishing
typedef struct {
	const char *name_friendly;   // "Battery Voltage"
	const char *name_mqtt;       // "battery_voltage"
	const char *units;           // "mV", "°C", etc.
	const char *hass_dev_class;  // "voltage", "temperature", "battery", etc.
	byte decimals;               // Decimal places for display
	float threshold;             // Minimum change to trigger MQTT publish
	float lastReading;           // Last value read from Roomba
	float lastSent;              // Last value published via MQTT
	int noChange;                // Frames since last MQTT publish
	byte packet_id;              // Roomba OI packet ID
	byte data_size;              // Response size in bytes
	bool is_binary;              // true for binary sensors (on/off)
} roomba_sensor_t;

// Vacuum state for Home Assistant
typedef enum {
    VACUUM_STATE_IDLE = 0,
    VACUUM_STATE_CLEANING,
    VACUUM_STATE_DOCKED,
    VACUUM_STATE_RETURNING,
    VACUUM_STATE_PAUSED,
    VACUUM_STATE_ERROR
} vacuum_state_t;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Driver lifecycle functions (called by OpenBeken framework)
void Roomba_Init();              // Initialize driver (called on startDriver)
void Roomba_RunEverySecond();    // Periodic polling (called every second)
void Roomba_OnQuickTick();       // Fast UART handler (called frequently)

// HTTP display function
void Roomba_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

// Home Assistant Discovery
void Roomba_OnHassDiscovery(const char *topic);

// Command handlers
commandResult_t CMD_Roomba_Clean(const void *context, const char *cmd, const char *args, int cmdFlags);
commandResult_t CMD_Roomba_Spot(const void *context, const char *cmd, const char *args, int cmdFlags);
commandResult_t CMD_Roomba_Dock(const void *context, const char *cmd, const char *args, int cmdFlags);
commandResult_t CMD_Roomba_Stop(const void *context, const char *cmd, const char *args, int cmdFlags);
commandResult_t CMD_Roomba_Discovery(const void *context, const char *cmd, const char *args, int cmdFlags);

#endif

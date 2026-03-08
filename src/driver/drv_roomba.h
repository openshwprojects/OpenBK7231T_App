/*
	* Roomba Open Interface (OI) header for OpenBeken.
	*
	* Defines:
	* - OI opcodes used by the driver and exposed commands.
	* - Sensor packet identifiers (core and extended groups).
	* - Driver-facing sensor index enum and sensor metadata structure.
	* - Vacuum state enum used by UI/MQTT state reporting.
	* - Public driver APIs for init, polling, HTTP status, and HA discovery.
	*
	* Coverage target:
	* - Group 100 primary polling with Group 6 fallback compatibility.
	* - Packet set spanning 7..58, including extended telemetry.
	*
	* Reference:
	* - iRobot Roomba Open Interface (OI) specification.
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
#define CMD_DIGIT_LEDS_RAW   163
#define CMD_DIGIT_LEDS_ASCII 164

// ============================================================================
// SENSOR PACKET IDs
// ============================================================================

#define PACKET_GROUP_6        6    // Group 6: All sensors (52 bytes) - Packets 7-42
#define PACKET_GROUP_100      100  // Group 100: All sensors (80 bytes) - Packets 7-58
#define PACKET_GROUP_101      101  // Group 101: Packets 43-58 (28 bytes)

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
#define PACKET_WALL_SIGNAL    27   // Wall signal (2 bytes, unsigned)
#define PACKET_CLIFF_LEFT_SIGNAL 28 // Cliff left signal (2 bytes, unsigned)
#define PACKET_CLIFF_FL_SIGNAL   29 // Cliff front-left signal (2 bytes, unsigned)
#define PACKET_CLIFF_FR_SIGNAL   30 // Cliff front-right signal (2 bytes, unsigned)
#define PACKET_CLIFF_RIGHT_SIGNAL 31 // Cliff right signal (2 bytes, unsigned)
#define PACKET_REQ_VELOCITY      39   // Requested velocity (2 bytes, signed, mm/s)
#define PACKET_REQ_RADIUS        40   // Requested radius (2 bytes, signed, mm)
#define PACKET_REQ_RIGHT_VEL     41   // Requested right velocity (2 bytes, signed, mm/s)
#define PACKET_REQ_LEFT_VEL      42   // Requested left velocity (2 bytes, signed, mm/s)
#define PACKET_ENCODER_LEFT      43   // Left encoder counts (2 bytes, signed)
#define PACKET_ENCODER_RIGHT     44   // Right encoder counts (2 bytes, signed)
#define PACKET_LIGHT_BUMPER      45   // Light bumper bitfield (1 byte, unsigned)
#define PACKET_LIGHT_BUMP_LEFT   46   // Light bump left signal (2 bytes, unsigned)
#define PACKET_LIGHT_BUMP_FL     47   // Light bump front-left signal (2 bytes, unsigned)
#define PACKET_LIGHT_BUMP_CL     48   // Light bump center-left signal (2 bytes, unsigned)
#define PACKET_LIGHT_BUMP_CR     49   // Light bump center-right signal (2 bytes, unsigned)
#define PACKET_LIGHT_BUMP_FR     50   // Light bump front-right signal (2 bytes, unsigned)
#define PACKET_LIGHT_BUMP_RIGHT  51   // Light bump right signal (2 bytes, unsigned)
#define PACKET_IR_LEFT           52   // IR character left receiver (1 byte)
#define PACKET_IR_RIGHT          53   // IR character right receiver (1 byte)
#define PACKET_LEFT_MOTOR_CURRENT 54  // Left motor current (2 bytes, signed, mA)
#define PACKET_RIGHT_MOTOR_CURRENT 55 // Right motor current (2 bytes, signed, mA)
#define PACKET_MAIN_BRUSH_CURRENT 56  // Main brush current (2 bytes, signed, mA)
#define PACKET_SIDE_BRUSH_CURRENT 57  // Side brush current (2 bytes, signed, mA)
#define PACKET_STASIS            58   // Stasis state (1 byte)

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
	
	// Extensions (Group 6)
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
	ROOMBA_SENSOR_OVERCURRENT_SIDE_BRUSH,
	ROOMBA_SENSOR_OVERCURRENT_VACUUM,
	ROOMBA_SENSOR_OVERCURRENT_MAIN_BRUSH,
	ROOMBA_SENSOR_OVERCURRENT_DRIVE_RIGHT,
	ROOMBA_SENSOR_OVERCURRENT_DRIVE_LEFT,
	ROOMBA_SENSOR_DIRT_DETECT,
	ROOMBA_SENSOR_BUTTONS,
	ROOMBA_SENSOR_WALL_SIGNAL,
	ROOMBA_SENSOR_CLIFF_LEFT_SIGNAL,
	ROOMBA_SENSOR_CLIFF_FL_SIGNAL,
	ROOMBA_SENSOR_CLIFF_FR_SIGNAL,
	ROOMBA_SENSOR_CLIFF_RIGHT_SIGNAL,
	ROOMBA_SENSOR_REQ_VELOCITY,
	ROOMBA_SENSOR_REQ_RADIUS,
	ROOMBA_SENSOR_REQ_RIGHT_VEL,
	ROOMBA_SENSOR_REQ_LEFT_VEL,
	ROOMBA_SENSOR_DISTANCE,
	ROOMBA_SENSOR_ANGLE,
	ROOMBA_SENSOR_ENCODER_LEFT,
	ROOMBA_SENSOR_ENCODER_RIGHT,
	ROOMBA_SENSOR_LIGHT_BUMPER,
	ROOMBA_SENSOR_LIGHT_BUMP_LEFT,
	ROOMBA_SENSOR_LIGHT_BUMP_FL,
	ROOMBA_SENSOR_LIGHT_BUMP_CL,
	ROOMBA_SENSOR_LIGHT_BUMP_CR,
	ROOMBA_SENSOR_LIGHT_BUMP_FR,
	ROOMBA_SENSOR_LIGHT_BUMP_RIGHT,
	ROOMBA_SENSOR_IR_LEFT,
	ROOMBA_SENSOR_IR_RIGHT,
	ROOMBA_SENSOR_LEFT_MOTOR_CURRENT,
	ROOMBA_SENSOR_RIGHT_MOTOR_CURRENT,
	ROOMBA_SENSOR_MAIN_BRUSH_CURRENT,
	ROOMBA_SENSOR_SIDE_BRUSH_CURRENT,
	ROOMBA_SENSOR_STASIS,

	ROOMBA_SENSOR__COUNT  // Total number of sensors
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


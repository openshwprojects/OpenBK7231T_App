#pragma once

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"


// some OneWire commands
#define SEARCH_ROM		0xF0	// to identify the ROM codes of all slave devices on the bus
#define ALARM_SEARCH		0xEC	// all devices with an alarm condition (as SEARCH_ROM except only slaves with a set alarm flag will respond)
#define READ_ROM		0x33	// Only single device! Read 64-bit ROM code without using the Search ROM procedure
#define SKIP_ROM		0xCC	// Ignore ROM adress / address all devices
#define CONVERT_T		0x44	// take temperature reading and copy it to scratchpad
#define READ_SCRATCHPAD		0xBE	// Read scratchpad
#define WRITE_SCRATCHPAD	0x4E	// Write scratchpad
#define SELECT_DEVICE		0x55	// (Match ROM) address a single devices on the bus
/*
//unused for now
#define COPY_SCRATCHPAD	0x48		// Copy scratchpad to EEPROM
#define RECALL_E2		0xB8	// Recall from EEPROM to scratchpad
#define READ_POWER_SUPPLY	0xB4	// Determine if device needs parasite power
*/

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8
// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

#define OWtimeA	6
#define OWtimeB	64
#define OWtimeC	60
#define OWtimeD	10
#define OWtimeE	9
#define OWtimeF	55
#define OWtimeG	0
#define OWtimeH	480
#define OWtimeI	70
#define OWtimeJ	410


#define DEVICE_DISCONNECTED_C -127


void init_TEST_US();

int OWReset(int Pin);
void OWWriteBit(int Pin, int bit);
int OWReadBit(int Pin);
int DS1820TConversionDone(int Pin);
void OWWriteByte(int Pin, int data);
int OWReadByte(int Pin);
int OWTouchByte(int Pin, int data);
uint8_t Crc8CQuick(uint8_t* Buffer, uint8_t Size);




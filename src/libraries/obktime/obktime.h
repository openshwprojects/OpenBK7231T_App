#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "../../new_common.h"

// Time components structure
typedef struct {
    uint8_t second;  // 0-59
    uint8_t minute;  // 0-59
    uint8_t hour;    // 0-23
    uint8_t day;     // 1-31
    uint8_t month;   // 1-12
    uint16_t year;   // 1970+
    uint8_t wday;    // 0-6 (Sunday=0)
} TimeComponents;

// Constants
#define SECS_PER_MIN  60
#define SECS_PER_HOUR 3600
#define SECS_PER_DAY  86400
#define EPOCH_YEAR    1970

// API Functions

// Conversion functions
TimeComponents calculateComponents(uint32_t timestamp);	// replacement for gmtime() to get (some) components of an epoch timestamp

// two simple replacements for mktime()
uint32_t dateToEpoch(uint16_t year, uint8_t month, uint8_t day, 
                    uint8_t hour, uint8_t minute, uint8_t second);
//uint32_t stringToEpoch(const char* datetime);

// Formatting functions
#define TIME_FORMAT_SHORT 	0, '-', 0		// "20231231-235959"
#define TIME_FORMAT_LONG 	'-', ' ', ':'		// "2023-12-31 23:59:59"
#define TIME_FORMAT_ISO_8601 	'-', 'T', ':'		// "2023-12-31T23:59:59"
#define TIME_FORMAT_DATEonly 	'-', 0, '\x1'		// "2023-12-31"
#define TIME_FORMAT_TIMEonly 	'\x1', 0, ':'		// "23:59:59"
/*
// examples with 
formatTime(ts, buf, TIME_FORMAT_LONG)			"2023-12-31 23:59:59"		"YYYY-MM-DD HH:MM:SS"	needs 20 chars including \0
formatTime(ts, buf, TIME_FORMAT_SHORT)			"20231231-235959"
No separators	formatTimeEx(ts, buf, 0, 0, 0)		"20231231235959"
File-safe	formatTimeEx(ts, buf, '-', '_', '-')	"2023-12-31_23-59-59"
*/

void formatTimestamp(uint32_t timestamp, char* buffer, char date_sep, char datetime_sep, char time_sep);
// same, but return pointer to internal char array. This is shared!!!!
char * TS2STR(uint32_t timestamp, char date_sep, char datetime_sep, char time_sep);

// Helper functions
bool isValidDate(uint16_t year, uint8_t month, uint8_t day);
bool isLeapYear(uint16_t year);

#endif // TIME_UTILS_H

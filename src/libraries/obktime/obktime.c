#include "obktime.h"
#include <string.h>
#include "../../logging/logging.h"

char timestring[20];
static const uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};

bool isLeapYear(uint16_t year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

bool isValidDate(uint16_t year, uint8_t month, uint8_t day) {
    if (year < EPOCH_YEAR || month < 1 || month > 12 || day < 1 || day > 31)
        return false;
    
    uint8_t dim = daysInMonth[month-1];
    if (month == 2 && isLeapYear(year)) dim++;
    return day <= dim;
}

TimeComponents calculateComponents(uint32_t timestamp) {
    TimeComponents tc = {0};
    if (timestamp == 0) return tc;
    
    // Calculate time
    tc.second = timestamp % SECS_PER_MIN;

    timestamp /= SECS_PER_MIN;
    tc.minute = timestamp % SECS_PER_MIN;

    timestamp /= SECS_PER_MIN;
    tc.hour = timestamp % 24;

    timestamp /= 24;    
     // Calculate weekday (0=Sunday)
    tc.wday = (timestamp + 4) % 7; // 1970-01-01 was Thursday (4)
    
    // Calculate date
    uint16_t days = timestamp;
    tc.year = EPOCH_YEAR;
    
    while (days >= (isLeapYear(tc.year) ? 366 : 365)) {
        days -= isLeapYear(tc.year) ? 366 : 365;
        tc.year++;
    }
    
    uint8_t month = 0;
    days++;
    for (month = 0; month < 12; month++) {
        uint8_t dim = daysInMonth[month];
        if (month == 1 && isLeapYear(tc.year)) dim++;
        if (days <= dim) break;
        days -= dim;
    }
    
    tc.month = month + 1;
    tc.day = days;

    return tc;
}

uint32_t dateToEpoch(uint16_t year, uint8_t month, uint8_t day, 
                    uint8_t hour, uint8_t minute, uint8_t second) {
    if (!isValidDate(year, month, day)) return 0;
    
    // Calculate days since epoch
    uint32_t days = 0;
    for (uint16_t y = EPOCH_YEAR; y < year; y++) {
        days += isLeapYear(y) ? 366 : 365;
    }
    
    // Add days in current year
    for (uint8_t m = 1; m < month; m++) {
        days += daysInMonth[m-1];
        if (m == 2 && isLeapYear(year)) days++;
    }
    days += day - 1;
    
    return ((days * 24 + hour) * 60 + minute) * 60 + second;
}

/*
uint32_t stringToEpoch(const char* datetime) {
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    
    if (strlen(datetime) == 19) {
        sscanf(datetime, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
              &year, &month, &day, &hour, &minute, &second);
    } else {
        sscanf(datetime, "%04hu%02hhu%02hhu-%02hhu%02hhu%02hhu",
              &year, &month, &day, &hour, &minute, &second);
    }
    
    return dateToEpoch(year, month, day, hour, minute, second);
}
*/

void formatTimestamp(uint32_t timestamp, char* buffer, char date_sep, char datetime_sep, char time_sep) {
    TimeComponents tc = calculateComponents(timestamp);
    char *p = buffer;

    // Year (always 4 digits)
    *p++ = '0' + (tc.year/1000)%10;
    *p++ = '0' + (tc.year/100)%10;
    *p++ = '0' + (tc.year/10)%10;
    *p++ = '0' + tc.year%10;

    // Date separator and month
    if (date_sep) *p++ = date_sep;
    *p++ = '0' + tc.month/10;
    *p++ = '0' + tc.month%10;

    // Date separator and day
    if (date_sep) *p++ = date_sep;
    *p++ = '0' + tc.day/10;
    *p++ = '0' + tc.day%10;

    // Date-time separator
    if (datetime_sep) *p++ = datetime_sep;

    // Hour
    *p++ = '0' + tc.hour/10;
    *p++ = '0' + tc.hour%10;

    // Time separator and minute
    if (time_sep) *p++ = time_sep;
    *p++ = '0' + tc.minute/10;
    *p++ = '0' + tc.minute%10;

    // Time separator and second
    if (time_sep) *p++ = time_sep;
    *p++ = '0' + tc.second/10;
    *p++ = '0' + tc.second%10;

    *p = '\0';
}

char * TS2STR(uint32_t timestamp, char date_sep, char datetime_sep, char time_sep) {
    TimeComponents tc = calculateComponents(timestamp);
    char *p = timestring;   
    timestring[19]='\0';
    if (date_sep != '\x1'){
	    // Year (always 4 digits)
	    *p++ = '0' + (tc.year/1000)%10;
	    *p++ = '0' + (tc.year/100)%10;
	    *p++ = '0' + (tc.year/10)%10;
	    *p++ = '0' + tc.year%10;

	    // Date separator and month
	    if (date_sep) *p++ = date_sep;
	    *p++ = '0' + tc.month/10;
	    *p++ = '0' + tc.month%10;

	    // Date separator and day
	    if (date_sep) *p++ = date_sep;
	    *p++ = '0' + tc.day/10;
	    *p++ = '0' + tc.day%10;
    }
    // Date-time separator
    if (datetime_sep) *p++ = datetime_sep;

    if (time_sep != '\x1'){
	    // Hour
	    *p++ = '0' + tc.hour/10;
	    *p++ = '0' + tc.hour%10;

	    // Time separator and minute
	    if (time_sep) *p++ = time_sep;
	    *p++ = '0' + tc.minute/10;
	    *p++ = '0' + tc.minute%10;

	    // Time separator and second
	    if (time_sep) *p++ = time_sep;
	    *p++ = '0' + tc.second/10;
	    *p++ = '0' + tc.second%10;
    }
    *p = '\0';
    
    return timestring;

}

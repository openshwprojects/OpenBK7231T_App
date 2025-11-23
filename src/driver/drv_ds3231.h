#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;	// 1-12
    uint8_t year;	// 00-99
} ds3231_time_t;

void DS3231_Init();
void DS3231_Stop();
bool DS3231_ReadTime(ds3231_time_t* time);
bool DS3231_SetTime(const ds3231_time_t* time);
bool DS3231_SetEpoch(const time_t st);
void DS3231_RegisterCommands();
void DS3231_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState);
void DS3231_OnEverySecond();


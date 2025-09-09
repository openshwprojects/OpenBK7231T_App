#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_ds3231.h"
#include <time.h>
#include "drv_deviceclock.h"


#define DS3231_I2C_ADDR (0x68 << 1)

static softI2C_t g_ds3231_softI2C;
static uint8_t sync2device = 0;			// 0 do nothing		1 set devicetime to RTC on startup	2 set devicetime to RTC regulary


static uint8_t bcd_to_dec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

// Epoch helpers
static time_t DS3231_TimeToEpoch(const ds3231_time_t* time)
{
    struct tm t;
    t.tm_sec  = time->seconds;
    t.tm_min  = time->minutes;
    t.tm_hour = time->hours;
    t.tm_mday = time->date;
    t.tm_mon  = time->month - 1; // tm_mon is 0-11
    t.tm_year = time->year + 100; // DS3231 year is 00-99, tm_year since 1900
    t.tm_isdst = -1;
    return mktime(&t);
}

static void DS3231_EpochToTime(time_t epoch, ds3231_time_t* time)
{
    struct tm* t = gmtime(&epoch);
    time->seconds = t->tm_sec;
    time->minutes = t->tm_min;
    time->hours   = t->tm_hour;
    time->date    = t->tm_mday;
    time->month   = t->tm_mon + 1;
    time->year    = t->tm_year - 100;
    time->day     = t->tm_wday;
}


bool DS3231_ReadTime(ds3231_time_t* time)
{
    uint8_t data[7];

    Soft_I2C_Start(&g_ds3231_softI2C, DS3231_I2C_ADDR);
    Soft_I2C_WriteByte(&g_ds3231_softI2C, 0x00);
    Soft_I2C_Stop(&g_ds3231_softI2C);

    Soft_I2C_Start(&g_ds3231_softI2C, DS3231_I2C_ADDR | 1);
    Soft_I2C_ReadBytes(&g_ds3231_softI2C, data, 7);
    Soft_I2C_Stop(&g_ds3231_softI2C);

    time->seconds = bcd_to_dec(data[0] & 0x7F);
    time->minutes = bcd_to_dec(data[1]);
    time->hours   = bcd_to_dec(data[2] & 0x3F);
    time->day     = bcd_to_dec(data[3]);
    time->date    = bcd_to_dec(data[4]);
    time->month   = bcd_to_dec(data[5] & 0x1F);
    time->year    = bcd_to_dec(data[6]);

//    ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231 Time: %02d/%02d/%02d %02d:%02d:%02d", 
//        time->date, time->month, time->year, time->hours, time->minutes, time->seconds);

    return true;
}

uint32_t DS3231_ReadEpoch()
{
    ds3231_time_t time;
    uint32_t retval = 0;
    if (DS3231_ReadTime(&time)) retval= (uint32_t)DS3231_TimeToEpoch(&time);
    return retval;
}


bool DS3231_SetEpoch(const time_t st)
{
    ds3231_time_t time;
    DS3231_EpochToTime(st, &time);
    return (DS3231_SetTime(&time));
}



bool DS3231_SetTime(const ds3231_time_t* time)
{
    uint8_t data[7];
    data[0] = dec_to_bcd(time->seconds);
    data[1] = dec_to_bcd(time->minutes);
    data[2] = dec_to_bcd(time->hours);
    data[3] = dec_to_bcd(time->day);
    data[4] = dec_to_bcd(time->date);
    data[5] = dec_to_bcd(time->month);
    data[6] = dec_to_bcd(time->year);

    Soft_I2C_Start(&g_ds3231_softI2C, DS3231_I2C_ADDR);
    Soft_I2C_WriteByte(&g_ds3231_softI2C, 0x00);
    for (int i = 0; i < 7; i++) {
        Soft_I2C_WriteByte(&g_ds3231_softI2C, data[i]);
    }
    Soft_I2C_Stop(&g_ds3231_softI2C);

    ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231 Time Set: %02d/%02d/%02d %02d:%02d:%02d", 
        time->date, time->month, time->year, time->hours, time->minutes, time->seconds);

    return true;
}

// Standard get/set commands
commandResult_t DS3231_GetTimeCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
    ds3231_time_t time;
    if (DS3231_ReadTime(&time)) {
        ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231_GetTimeCmd: %02d/%02d/%02d %02d:%02d:%02d", 
            time.date, time.month, time.year, time.hours, time.minutes, time.seconds);
        return CMD_RES_OK;
    }
    return CMD_RES_ERROR;
}

commandResult_t DS3231_SetTimeCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 6)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    ds3231_time_t time;
    time.hours   = Tokenizer_GetArgInteger(0);
    time.minutes = Tokenizer_GetArgInteger(1);
    time.seconds = Tokenizer_GetArgInteger(2);
    time.date    = Tokenizer_GetArgInteger(3);
    time.month   = Tokenizer_GetArgInteger(4);
    time.year    = Tokenizer_GetArgInteger(5);

    if (DS3231_SetTime(&time)) {
        return CMD_RES_OK;
    }
    return CMD_RES_ERROR;
}

// Epoch get/set commands
commandResult_t DS3231_GetEpochCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
    ds3231_time_t time;
    if (DS3231_ReadTime(&time)) {
        time_t epoch = DS3231_TimeToEpoch(&time);
        ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231_GetEpochCmd: Epoch: %lu", (unsigned long)epoch);
        // Optionally return epoch to HTTP or other interface
        return CMD_RES_OK;
    }
    return CMD_RES_ERROR;
}

commandResult_t DS3231_SetEpochCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    time_t epoch = (time_t)Tokenizer_GetArgInteger(0);
    if (DS3231_SetEpoch(epoch)) {
        ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231_SetEpochCmd: Set time to epoch %lu", (unsigned long)epoch);
        return CMD_RES_OK;
    }
    return CMD_RES_ERROR;
}

void DS3231_Init()
{
    if(Tokenizer_GetArgsCount() < 3){
    	 ADDLOG_INFO(LOG_FEATURE_CMD, "\"startdriver DS3213\" needs at least two arguments <CLK-Pin> and <DATA-Pin>, given ony %i" , Tokenizer_GetArgsCount() -1 );
//    	 Tokenizer_CheckArgsCountAndPrintWarning("startdriver DS3213" , 3);
    	 return;
    }
    g_ds3231_softI2C.pin_clk = Tokenizer_GetPin(1, 9);
    g_ds3231_softI2C.pin_data = Tokenizer_GetPin(2, 14);
    Soft_I2C_PreInit(&g_ds3231_softI2C);
    rtos_delay_milliseconds(100);
    ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231 RTC Initialized.");

    //cmddetail:{"name":"DS3231_GetTime,"args":"-",
    //cmddetail:"descr":"Gets time from RTC",
    //cmddetail:"fn":"DS3231_GetTimeCmd","file":"drv/drv_ds3231.c","requires":"",
    //cmddetail:"examples":""}
    CMD_RegisterCommand("DS3231_GetTime", DS3231_GetTimeCmd, NULL);
    //cmddetail:{"name":"DS3231_SetTime,"args":"<hours> <minutes> <seconds> <day> <month> <year>",
    //cmddetail:"descr":"Sets RTC time",
    //cmddetail:"fn":"DS3231_SetTimeCmd","file":"drv/drv_ds3231.c","requires":"",
    //cmddetail:"examples":"DS3231_SetTime 19 50 01 13 8 2025"}
    CMD_RegisterCommand("DS3231_SetTime", DS3231_SetTimeCmd, NULL);
    //cmddetail:{"name":"DS3231_GetTime as epoch,"args":"-",
    //cmddetail:"descr":"Gets time from RTC",
    //cmddetail:"fn":"DS3231_GetTimeCmd","file":"drv/drv_ds3231.c","requires":"",
    //cmddetail:"examples":""}
    CMD_RegisterCommand("DS3231_GetEpoch", DS3231_GetEpochCmd, NULL);
    //cmddetail:{"name":"DS3231_SetEpoch,"args":"<epoch>",
    //cmddetail:"descr":"Sets RTC time (to epoch)",
    //cmddetail:"fn":"DS3231_SetEpochCmd","file":"drv/drv_ds3231.c","requires":"",
    //cmddetail:"examples":"DS3231_SetTime 3269879540"}
    CMD_RegisterCommand("DS3231_SetEpoch", DS3231_SetEpochCmd, NULL);
    sync2device=Tokenizer_GetArgIntegerDefault(3, 0);
    if (sync2device > 0 ) {
	ADDLOG_INFO(LOG_FEATURE_RAW, "DS3231 set deviceclock to RTC time.");
    	time_t t = DS3231_ReadEpoch();
    	if ( t > g_secondsElapsed + 3600 ) CLOCK_setDeviceTime(t);
    }  
}

void DS3231_Stop(){
};

void DS3231_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState){
	if (bPreState){
		return;
	}
	if (! Clock_IsTimeSynced()){
	    ds3231_time_t time;
	    if (DS3231_ReadTime(&time)) {
		hprintf255(request, "<h5>DS3231 Time: %02d/%02d/%02d %02d:%02d:%02d</h5>", 
		    time.date, time.month, time.year, time.hours, time.minutes, time.seconds);
	    }
	}
};

void DS3231_OnEverySecond(){
	if ( sync2device > 1 && g_secondsElapsed % 60 == 2 )
		CLOCK_setDeviceTime(DS3231_ReadEpoch());
};

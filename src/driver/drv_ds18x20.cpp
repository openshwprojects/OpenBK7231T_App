#include "drv_ds18x20.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"  // Include hal_pins.h

#include "../libraries/OneWire/OneWire.h"
#include "../libraries/DallasTemperature/DallasTemperature.h"

#define ONE_WIRE_BUS_PIN 4

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);

static byte channel_temp = 0, g_ds_secondsUntilNextMeasurement = 1, g_ds_secondsBetweenMeasurements = 10;
static float g_temp = 0.0, g_caltemp = 0.0;

commandResult_t DS18x20_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    g_caltemp = Tokenizer_GetArgFloat(0);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, "Calibrate DS18x20: Calibration done temp %f", g_caltemp);
    return CMD_RES_OK;
}

void DS18x20_Init(int pin) {
    hal_set_pin_mode(pin, GPIO_MODE_INPUT_OUTPUT);  // Set pin mode using hal_pins.h
    oneWire.begin(pin);
    channel_temp = g_cfg.pins.channels[pin];
    sensors.setOneWire(&oneWire);
    sensors.begin();
}

void DS18x20_ReadTemperatures() {
    sensors.requestTemperatures();
    g_temp = sensors.getTempCByIndex(0);
    g_temp = (int)((g_temp + g_caltemp) * 10.0) / 10.0f;
    CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
    addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DS18x20_Measure: Temperature:%.1f°C", g_temp);
}

commandResult_t DS18x20_Measure(const void* context, const char* cmd, const char* args, int cmdFlags) {
    DS18x20_ReadTemperatures();
    return CMD_RES_OK;
}

commandResult_t DS_cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    g_ds_secondsBetweenMeasurements = Tokenizer_GetArgInteger(0);
    ADDLOG_INFO(LOG_FEATURE_CMD, "Measurement will run every %i seconds", g_ds_secondsBetweenMeasurements);
    return CMD_RES_OK;
}

void DS18x20_OnEverySecond() {
    if (g_ds_secondsUntilNextMeasurement <= 0) {
        DS18x20_ReadTemperatures();
        g_ds_secondsUntilNextMeasurement = g_ds_secondsBetweenMeasurements;
    }
    if (g_ds_secondsUntilNextMeasurement > 0) {
        g_ds_secondsUntilNextMeasurement--;
    }
}

void DS18x20_AppendInformationToHTTPIndexPage(http_request_t* request) {
    hprintf255(request, "<h2>DS18x20 Temperature=%.1f°C</h2>", g_temp);
    if (channel_temp == 0) {
        hprintf255(request, "WARNING: You don't have configured target channels for temp results, set the channel index in Pins!");
    }
}

void DS18x20_StopDriver() {
    addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DS18x20 : Stopping Driver");
}

void DS18x20_InitCommands() {
    CMD_RegisterCommand("DS_Cycle", DS_cycle, NULL);
    CMD_RegisterCommand("DS_Calibrate", DS18x20_Calibrate, NULL);
    CMD_RegisterCommand("DS_Measure", DS18x20_Measure, NULL);
}

void DS18x20_InitDriver() {
    int pin = ONE_WIRE_BUS_PIN; // Default pin

    // Check if a pin was specified in the command
    const char *args = CMD_GetArg(0);
    if (args && *args) {
        pin = atoi(args); // Convert argument to integer
    }

    DS18x20_Init(pin);
    DS18x20_InitCommands();
}

void DS18x20_ShutdownDriver() {
    DS18x20_StopDriver();
}

void DS18x20_OnEverySecondHook() {
    DS18x20_OnEverySecond();
}

void DS18x20_AppendInformationToHTTPIndexPageHook(http_request_t* request) {
    DS18x20_AppendInformationToHTTPIndexPage(request);
}

// Register the driver
void Register_DS18x20_Driver() {
    DRV_RegisterDriver("DS18x20", DS18x20_InitDriver, DS18x20_ShutdownDriver, DS18x20_OnEverySecondHook, DS18x20_AppendInformationToHTTPIndexPageHook);
}

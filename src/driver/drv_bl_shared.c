#include "drv_bl_shared.h"

#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cJSON/cJSON.h"
#include "../hal/hal_flashVars.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "../ota/ota.h"
#include "drv_local.h"
#include "drv_ntp.h"
#include "drv_public.h"
#include "drv_uart.h"

#include <math.h>
#include <time.h>

#define DAILY_STATS_LENGTH 4

int stat_updatesSkipped = 0;
int stat_updatesSent = 0;

// Current values
float lastReadings[OBK_NUM_MEASUREMENTS];
float lastReadingFrequency = NAN;
// precisions:
byte roundingPrecision[4] = {
	1, // OBK_VOLTAGE, // must match order in cmd_public.h
	3, // OBK_CURRENT,
	2, // OBK_POWER,
	3, // PRECISION_ENERGY
};
#define PRECISION_ENERGY 3

//
// Variables below are for optimization
// We can't send a full MQTT update every second.
// It's too much for Beken, and it's too much for LWIP 2 MQTT library,
// especially when actively browsing site and using JS app Log Viewer.
// It even fails to publish with -1 error (can't alloc next packet)
// So we publish when value changes from certain threshold or when a certain time passes.
//
// what are the last values we sent over the MQTT?
float lastSentValues[OBK_NUM_MEASUREMENTS];
// energyCounter in Wh
float energyCounter = 0.0f;
portTickType energyCounterStamp;

bool energyCounterStatsEnable = false;
int energyCounterSampleCount = 60;
int energyCounterSampleInterval = 60;
float *energyCounterMinutes = NULL;
portTickType energyCounterMinutesStamp;
long energyCounterMinutesIndex;
bool energyCounterStatsJSONEnable = false;

// how much update frames has passed without sending MQTT update of read values?
int noChangeFrames[OBK_NUM_MEASUREMENTS];
int noChangeFrameEnergyCounter;
float lastSentEnergyCounterValue = 0.0f; 
float changeSendThresholdEnergy = 0.1f;
float lastSentEnergyCounterLastHour = 0.0f;
float dailyStats[DAILY_STATS_LENGTH];
int actual_mday = -1;
float lastSavedEnergyCounterValue = 0.0f;
float changeSavedThresholdEnergy = 10.0f;
long ConsumptionSaveCounter = 0;
portTickType lastConsumptionSaveStamp;
time_t ConsumptionResetTime = 0;

// how much of value have to change in order to be send over MQTT again?
float changeSendThresholds[OBK_NUM_MEASUREMENTS] = {
    0.25f, // voltage - OBK_VOLTAGE
    0.002f, // current - OBK_CURRENT
    0.25f, // power - OBK_POWER
};


int changeSendAlwaysFrames = 60;
int changeDoNotSendMinFrames = 5;
float g_apparentPower = 0;
float g_powerFactor = 0;
float g_reactivePower = 0;

void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request)
{
    int i;
    const char *mode;
    struct tm *ltm;

    if(DRV_IsRunning("BL0937")) {
        mode = "BL0937";
    } else if(DRV_IsRunning("BL0942")) {
        mode = "BL0942";
    } else if (DRV_IsRunning("BL0942SPI")) {
        mode = "BL0942SPI";
    } else if(DRV_IsRunning("CSE7766")) {
        mode = "CSE7766";
    } else {
        mode = "PWR";
    }

    poststr(request, "<hr><table style='width:100%'>");

    if (!isnan(lastReadingFrequency)) {
        poststr(request,
                "<tr><td><b>Frequency</b></td><td style='text-align: right;'>");
        hprintf255(request, "%.2f</td><td>Hz</td>", lastReadingFrequency);
    }

    poststr(request,
            "<tr><td><b>Voltage</b></td><td style='text-align: right;'>");
    hprintf255(request, "%.1f</td><td>V</td>", lastReadings[OBK_VOLTAGE]);

    poststr(request,
            "<tr><td><b>Current</b></td><td style='text-align: right;'>");
    hprintf255(request, "%.3f</td><td>A</td>", lastReadings[OBK_CURRENT]);

    poststr(request,
            "<tr><td><b>Active Power</b></td><td style='text-align: right;'>");
    hprintf255(request, "%.1f</td><td>W</td>", lastReadings[OBK_POWER]);

    poststr(
        request,
        "<tr><td><b>Apparent Power</b></td><td style='text-align: right;'>");
    hprintf255(request, "%.1f</td><td>VA</td>", g_apparentPower);

    poststr(
        request,
        "<tr><td><b>Reactive Power</b></td><td style='text-align: right;'>");
    hprintf255(request, "%.1f</td><td>var</td>", g_reactivePower);

    poststr(request,
            "<tr><td><b>Power Factor</b></td><td style='text-align: right;'>");
    hprintf255(request, "%.2f</td><td></td>", g_powerFactor);

    if (NTP_IsTimeSynced()) {
        poststr(request, "<tr><td><b>Energy Today</b></td><td "
                         "style='text-align: right;'>");
        hprintf255(request, "%.1f</td><td>Wh</td>", dailyStats[0]);

        poststr(request, "<tr><td><b>Energy Yesterday</b></td><td "
                         "style='text-align: right;'>");
        hprintf255(request, "%.1f</td><td>Wh</td>", dailyStats[1]);
    }

    poststr(request,
            "<tr><td><b>Energy Total</b></td><td style='text-align: right;'>");
	// convert from Wh to kWh (thus / 1000.0f)
    hprintf255(request, "%.3f</td><td>kWh</td>", energyCounter / 1000.0f);

    poststr(request, "</table>");

    hprintf255(request, "(changes sent %i, skipped %i, saved %li) - %s<hr>",
               stat_updatesSent, stat_updatesSkipped, ConsumptionSaveCounter,
               mode);

    if (energyCounterStatsEnable == true)
    {
        /********************************************************************************************************************/
        hprintf255(request,"<h2>Periodic Statistics</h2><h5>Consumption (during this period): ");
        hprintf255(request,"%1.1f Wh<br>", DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
        hprintf255(request,"Sampling interval: %d sec<br>History length: ",energyCounterSampleInterval);
        hprintf255(request,"%d samples<br>History per samples:<br>",energyCounterSampleCount);
        if (energyCounterMinutes != NULL)
        {
            for(i=0; i<energyCounterSampleCount; i++)
            {
                if ((i%20)==0)
                {
                    hprintf255(request, "%1.1f", energyCounterMinutes[i]);
                } else {
                    hprintf255(request, ", %1.1f", energyCounterMinutes[i]);
                }
                if ((i%20)==19)
                {
                    hprintf255(request, "<br>");
                }
            }
			// energyCounterMinutesIndex is a long type, we need to use %ld instead of %d
            if ((i%20)!=0)
                hprintf255(request, "<br>");
            hprintf255(request, "History Index: %ld<br>JSON Stats: %s <br>", energyCounterMinutesIndex,
                    (energyCounterStatsJSONEnable == true) ? "enabled" : "disabled");
        }

        if(NTP_IsTimeSynced() == true)
        {
            hprintf255(request, "Today: %1.1f Wh DailyStats: [", dailyStats[0]);
            for(i = 1; i < DAILY_STATS_LENGTH; i++)
            {
                if (i==1)
                    hprintf255(request, "%1.1f", dailyStats[i]);
                else
                    hprintf255(request, ",%1.1f", dailyStats[i]);
            }
            hprintf255(request, "]<br>");
            ltm = localtime(&ConsumptionResetTime);
            hprintf255(request, "Consumption Reset Time: %04d/%02d/%02d %02d:%02d:%02d",
                       ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        } else {
            if(DRV_IsRunning("NTP")==false)
                hprintf255(request,"NTP driver is not started, daily stats disbled.");
            else
                hprintf255(request,"Daily stats require NTP driver to sync real time.");
        }
        hprintf255(request, "</h5>");
    } else {
        hprintf255(request,"<h5>Periodic Statistics disabled. Use startup command SetupEnergyStats to enable function.</h5>");
    }
    /********************************************************************************************************************/
}

void BL09XX_SaveEmeteringStatistics()
{
    ENERGY_METERING_DATA data;

    memset(&data, 0, sizeof(ENERGY_METERING_DATA));

    data.TotalConsumption = energyCounter;
    data.TodayConsumpion = dailyStats[0];
    data.YesterdayConsumption = dailyStats[1];
    data.actual_mday = actual_mday;
    data.ConsumptionHistory[0] = dailyStats[2];
    data.ConsumptionHistory[1] = dailyStats[3];
    data.ConsumptionResetTime = ConsumptionResetTime;
    ConsumptionSaveCounter++;
    data.save_counter = ConsumptionSaveCounter;

    HAL_SetEnergyMeterStatus(&data);
}

commandResult_t BL09XX_ResetEnergyCounter(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    float value;
    int i;

    if(args==0||*args==0) 
    {
        energyCounter = 0.0f;
        energyCounterStamp = xTaskGetTickCount();
        if (energyCounterStatsEnable == true)
        {
            if (energyCounterMinutes != NULL)
            {
                for(i = 0; i < energyCounterSampleCount; i++)
                {
                    energyCounterMinutes[i] = 0.0;
                }
            }
            energyCounterMinutesStamp = xTaskGetTickCount();
            energyCounterMinutesIndex = 0;
        }
        for(i = 0; i < DAILY_STATS_LENGTH; i++)
        {
            dailyStats[i] = 0.0;
        }
    } else {
        value = atof(args);
        energyCounter = value;
        energyCounterStamp = xTaskGetTickCount();
    }
    ConsumptionResetTime = (time_t)NTP_GetCurrentTime();
#if WINDOWS
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
    if (ota_progress()==-1)
#endif
    { 
        BL09XX_SaveEmeteringStatistics();
        lastConsumptionSaveStamp = xTaskGetTickCount();
    }
    return CMD_RES_OK;
}

commandResult_t BL09XX_SetupEnergyStatistic(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    // SetupEnergyStats enable sample_time sample_count
    int enable;
    int sample_time;
    int sample_count;
    int json_enable;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

    enable = Tokenizer_GetArgInteger(0);
    sample_time = Tokenizer_GetArgInteger(1);
    sample_count = Tokenizer_GetArgInteger(2);
    if (Tokenizer_GetArgsCount() >= 4)
        json_enable = Tokenizer_GetArgInteger(3);
    else
        json_enable = 0;

    /* Security limits for sample interval */
    if (sample_time <10)
        sample_time = 10;
    if (sample_time >900)
        sample_time = 900;

    /* Security limits for sample count */
    if (sample_count < 10)
        sample_count = 10;
    if (sample_count > 180)
        sample_count = 180;   

    /* process changes */
    if (enable != 0)
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Consumption History enabled");
        /* Enable function */
        energyCounterStatsEnable = true;
        if (energyCounterSampleCount != sample_count)
        {
            /* upgrade sample count, free memory */
            if (energyCounterMinutes != NULL)
                os_free(energyCounterMinutes);
            energyCounterMinutes = NULL;
            energyCounterSampleCount = sample_count;
        }
        addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Sample Count:    %d", energyCounterSampleCount);
        if (energyCounterSampleInterval != sample_time)
        {
            /* change sample time */            
            energyCounterSampleInterval = sample_time;
            if (energyCounterMinutes != NULL)
                memset(energyCounterMinutes, 0, energyCounterSampleCount*sizeof(float));
        }
        
        if (energyCounterMinutes == NULL)
        {
            /* allocate new memeory */
            energyCounterMinutes = (float*)os_malloc(sample_count*sizeof(float));
            if (energyCounterMinutes != NULL)
            {
                memset(energyCounterMinutes, 0, energyCounterSampleCount*sizeof(float));
            }
        }
        addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Sample Interval: %d", energyCounterSampleInterval);

        energyCounterMinutesStamp = xTaskGetTickCount();
        energyCounterMinutesIndex = 0;
    } else {
        /* Disable Consimption Nistory */
        addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Consumption History disabled");
        energyCounterStatsEnable = false;
        if (energyCounterMinutes != NULL)
        {
            os_free(energyCounterMinutes);
            energyCounterMinutes = NULL;
        }
        energyCounterSampleCount = sample_count;
        energyCounterSampleInterval = sample_time;
    }

    energyCounterStatsJSONEnable = (json_enable != 0) ? true : false; 

    return CMD_RES_OK;
}

commandResult_t BL09XX_VCPPublishIntervals(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	changeDoNotSendMinFrames = Tokenizer_GetArgInteger(0);
	changeSendAlwaysFrames = Tokenizer_GetArgInteger(1);

	return CMD_RES_OK;
}
commandResult_t BL09XX_VCPPrecision(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	int i;
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	for (i = 0; i < Tokenizer_GetArgsCount(); i++) {
		roundingPrecision[i] = Tokenizer_GetArgInteger(i);
	}

	return CMD_RES_OK;
}
commandResult_t BL09XX_VCPPublishThreshold(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	changeSendThresholds[OBK_VOLTAGE] = Tokenizer_GetArgFloat(0);
	changeSendThresholds[OBK_CURRENT] = Tokenizer_GetArgFloat(1);
	changeSendThresholds[OBK_POWER] = Tokenizer_GetArgFloat(2);
	if (Tokenizer_GetArgsCount() >= 4)
		changeSendThresholdEnergy = Tokenizer_GetArgFloat(3);

	return CMD_RES_OK;
}

commandResult_t BL09XX_SetupConsumptionThreshold(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    float threshold;
    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
    
    threshold = atof(Tokenizer_GetArg(0)); 

    if (threshold<1.0f)
        threshold = 1.0f;
    if (threshold>200.0f)
        threshold = 200.0f;
    changeSavedThresholdEnergy = threshold;
    addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ConsumptionThreshold: %1.1f", changeSavedThresholdEnergy);

    return CMD_RES_OK;
}

bool Channel_AreAllRelaysOpen() {
	int i, role, ch;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		role = g_cfg.pins.roles[i];
		ch = g_cfg.pins.channels[i];
		if (role == IOR_Relay) {
			// this channel is high = relay is set
			if (CHANNEL_Get(ch)) {
				return false;
			}
		}
		if (role == IOR_Relay_n) {
			// this channel is low = relay_n is set
			if (CHANNEL_Get(ch)==false) {
				return false;
			}
		}
		if (role == IOR_BridgeForward) {
			// this channel is high = relay is set
			if (CHANNEL_Get(ch)) {
				return false;
			}
		}
	}
	return true;
}

float BL_ChangeEnergyUnitIfNeeded(float Wh) {
	if (CFG_HasFlag(OBK_FLAG_MQTT_ENERGY_IN_KWH)) {
		return Wh * 0.001f;
	}
	return Wh;
}

void BL_ProcessUpdate(float voltage, float current, float power,
                      float frequency, float energyWh) {
    int i;
    int xPassedTicks;
    cJSON* root;
    cJSON* stats;
    char *msg;
    portTickType interval;
    time_t ntpTime;
    struct tm *ltm;
    char datetime[64];
	float diff;

	// I had reports that BL0942 sometimes gives 
	// a large, negative peak of current/power
	if (!CFG_HasFlag(OBK_FLAG_POWER_ALLOW_NEGATIVE)) 
    {
		if (power < 0.0f)
			power = 0.0f;
		if (voltage < 0.0f)
			voltage = 0.0f;
		if (current < 0.0f)
			current = 0.0f;
	}
	if (CFG_HasFlag(OBK_FLAG_POWER_FORCE_ZERO_IF_RELAYS_OPEN))
	{
		if (Channel_AreAllRelaysOpen()) {
			power = 0;
			current = 0;
		}
	}

    lastReadings[OBK_POWER] = power;
    lastReadings[OBK_VOLTAGE] = voltage;
    lastReadings[OBK_CURRENT] = current;
    lastReadingFrequency = frequency;

    g_apparentPower = lastReadings[OBK_VOLTAGE] * lastReadings[OBK_CURRENT];
    g_reactivePower = (g_apparentPower <= fabsf(lastReadings[OBK_POWER])
                           ? 0
                           : sqrtf(powf(g_apparentPower, 2) -
                                   powf(lastReadings[OBK_POWER], 2)));
    g_powerFactor =
        (g_apparentPower == 0 ? 1 : lastReadings[OBK_POWER] / g_apparentPower);

    float energy = 0;
    if (isnan(energyWh)) {
        xPassedTicks = (int)(xTaskGetTickCount() - energyCounterStamp);
        // FIXME: Wrong calculation if tick count overflows
        if (xPassedTicks <= 0)
            xPassedTicks = 1;
        energy = xPassedTicks * power / (3600000.0f / portTICK_PERIOD_MS);
    } else
        energy = energyWh;

    if (energy < 0)
        energy = 0.0;

    energyCounter += energy;
    energyCounterStamp = xTaskGetTickCount();
    HAL_FlashVars_SaveTotalConsumption(energyCounter);

    if (NTP_IsTimeSynced()) {
        ntpTime = (time_t)NTP_GetCurrentTime();
        ltm = localtime(&ntpTime);
        if (ConsumptionResetTime == 0)
            ConsumptionResetTime = (time_t)ntpTime;

        if (actual_mday == -1)
        {
            actual_mday = ltm->tm_mday;
        }
        if (actual_mday != ltm->tm_mday)
        {
            for (i = DAILY_STATS_LENGTH - 1; i > 0; i--)
                dailyStats[i] = dailyStats[i - 1];

            dailyStats[0] = 0.0;
            actual_mday = ltm->tm_mday;
            MQTT_PublishMain_StringFloat(counter_mqttNames[3], BL_ChangeEnergyUnitIfNeeded(dailyStats[1]), roundingPrecision[PRECISION_ENERGY]);
            stat_updatesSent++;
#if WINDOWS
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
            if (ota_progress()==-1)
#endif
            {
                BL09XX_SaveEmeteringStatistics();
                lastConsumptionSaveStamp = xTaskGetTickCount();
            }
            if (MQTT_IsReady() == true)
            {
                ltm = localtime(&ConsumptionResetTime);
                /* 2019-09-07T15:50-04:00 */
                if (NTP_GetTimesZoneOfsSeconds()>0)
                {
                    snprintf(datetime, sizeof(datetime), "%04i-%02i-%02iT%02i:%02i+%02i:%02i",
                            ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
                            NTP_GetTimesZoneOfsSeconds()/3600, (NTP_GetTimesZoneOfsSeconds()/60) % 60);
                } else {
                    snprintf(datetime, sizeof(datetime), "%04i-%02i-%02iT%02i:%02i-%02i:%02i",
                            ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
                            abs(NTP_GetTimesZoneOfsSeconds()/3600), (abs(NTP_GetTimesZoneOfsSeconds())/60) % 60);
                }
                MQTT_PublishMain_StringString(counter_mqttNames[5], datetime, 0);
                stat_updatesSent++;
            }
        }
    }

    dailyStats[0] += energy;

    if (energyCounterStatsEnable == true)
    {
        interval = energyCounterSampleInterval;
        interval *= (1000 / portTICK_PERIOD_MS); 
        if ((xTaskGetTickCount() - energyCounterMinutesStamp) >= interval)
        {
            if ((energyCounterStatsJSONEnable == true) && (MQTT_IsReady() == true))
            {
                root = cJSON_CreateObject();
                cJSON_AddNumberToObject(root, "uptime", g_secondsElapsed);
                cJSON_AddNumberToObject(root, "consumption_total", BL_ChangeEnergyUnitIfNeeded(energyCounter) );
                cJSON_AddNumberToObject(root, "consumption_last_hour",  DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
                cJSON_AddNumberToObject(root, "consumption_stat_index", BL_ChangeEnergyUnitIfNeeded(energyCounterMinutesIndex));
                cJSON_AddNumberToObject(root, "consumption_sample_count", energyCounterSampleCount);
                cJSON_AddNumberToObject(root, "consumption_sampling_period", energyCounterSampleInterval);
                if(NTP_IsTimeSynced() == true)
                {
                    cJSON_AddNumberToObject(root, "consumption_today", BL_ChangeEnergyUnitIfNeeded(dailyStats[0]));
                    cJSON_AddNumberToObject(root, "consumption_yesterday", BL_ChangeEnergyUnitIfNeeded(dailyStats[1]));
                    ltm = localtime(&ConsumptionResetTime);
                    if (NTP_GetTimesZoneOfsSeconds()>0)
                    {
                       snprintf(datetime,sizeof(datetime), "%04i-%02i-%02iT%02i:%02i+%02i:%02i",
                               ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
                               NTP_GetTimesZoneOfsSeconds()/3600, (NTP_GetTimesZoneOfsSeconds()/60) % 60);
                    } else {
                       snprintf(datetime, sizeof(datetime), "%04i-%02i-%02iT%02i:%02i-%02i:%02i",
                               ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
                               abs(NTP_GetTimesZoneOfsSeconds()/3600), (abs(NTP_GetTimesZoneOfsSeconds())/60) % 60);
                    }
                    cJSON_AddStringToObject(root, "consumption_clear_date", datetime);
                }

                if (energyCounterMinutes != NULL)
                {
                    stats = cJSON_CreateArray();
					// WARNING - it causes HA problems?
					// See: https://github.com/openshwprojects/OpenBK7231T_App/issues/870
					// Basically HA has 256 chars state limit?
					// Wait, no, it's over 256 even without samples?
                    for(i = 0; i < energyCounterSampleCount; i++)
                    {
                        cJSON_AddItemToArray(stats, cJSON_CreateNumber(energyCounterMinutes[i]));
                    }
                    cJSON_AddItemToObject(root, "consumption_samples", stats);
                }

                if(NTP_IsTimeSynced() == true)
                {
                    stats = cJSON_CreateArray();
                    for(i = 0; i < DAILY_STATS_LENGTH; i++)
                    {
                        cJSON_AddItemToArray(stats, cJSON_CreateNumber(dailyStats[i]));
                    }
                    cJSON_AddItemToObject(root, "consumption_daily", stats);
                }

                msg = cJSON_PrintUnformatted(root);
                cJSON_Delete(root);

               // addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "JSON Printed: %d bytes", strlen(msg));

                MQTT_PublishMain_StringString(counter_mqttNames[2], msg, 0);
                stat_updatesSent++;
                os_free(msg);
            }

            if (energyCounterMinutes != NULL)
            {
                for (i=energyCounterSampleCount-1;i>0;i--)
                {
                    if (energyCounterMinutes[i-1]>0.0)
                    {
                        energyCounterMinutes[i] = energyCounterMinutes[i-1];
                    } else {
                        energyCounterMinutes[i] = 0.0;
                    }
                }
                energyCounterMinutes[0] = 0.0;
            }
            energyCounterMinutesStamp = xTaskGetTickCount();
            energyCounterMinutesIndex++;

            if (MQTT_IsReady() == true)
            {
                MQTT_PublishMain_StringFloat(counter_mqttNames[1],
					BL_ChangeEnergyUnitIfNeeded(DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR)), roundingPrecision[PRECISION_ENERGY]);
                EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR, lastSentEnergyCounterLastHour, DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
                lastSentEnergyCounterLastHour = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);
                stat_updatesSent++;
            }
        }

        if (energyCounterMinutes != NULL)
            energyCounterMinutes[0] += energy;
    }

    for(i = 0; i < OBK_NUM_MEASUREMENTS; i++)
    {
        // send update only if there was a big change or if certain time has passed
        // Do not send message with every measurement. 
		diff = lastSentValues[i] - lastReadings[i];
		// get absolute value
		if (diff < 0)
			diff = -diff;
		// check for change
        if ( ((diff > changeSendThresholds[i]) &&
               (noChangeFrames[i] >= changeDoNotSendMinFrames)) ||
             (noChangeFrames[i] >= changeSendAlwaysFrames) )
        {
            noChangeFrames[i] = 0;
            if(i == OBK_CURRENT)
            {
                int prev_mA, now_mA;
                prev_mA = lastSentValues[i] * 1000;
                now_mA = lastReadings[i] * 1000;
                EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CURRENT, prev_mA,now_mA);
            } else {
                EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_VOLTAGE+i, lastSentValues[i], lastReadings[i]);
            }
            if (MQTT_IsReady() == true)
            {
                lastSentValues[i] = lastReadings[i];
                MQTT_PublishMain_StringFloat(sensor_mqttNames[i],lastReadings[i], roundingPrecision[i]);
                stat_updatesSent++;
            }
        } else {
            // no change frame
            noChangeFrames[i]++;
            stat_updatesSkipped++;
        }
    }

	// send update only if there was a big change or if certain time has passed
	// Do not send message with every measurement. 
	diff = energyCounter - lastSentEnergyCounterValue;
	// get absolute value
	if (diff < 0)
		diff = -diff;
	// check for change
    if ( (((diff) >= changeSendThresholdEnergy) &&
          (noChangeFrameEnergyCounter >= changeDoNotSendMinFrames)) || 
         (noChangeFrameEnergyCounter >= changeSendAlwaysFrames) )
    {
        if (MQTT_IsReady() == true)
        {
            MQTT_PublishMain_StringFloat(counter_mqttNames[0],
				BL_ChangeEnergyUnitIfNeeded(energyCounter), roundingPrecision[PRECISION_ENERGY]);

            EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_TOTAL, lastSentEnergyCounterValue, energyCounter);
            lastSentEnergyCounterValue = energyCounter;
            noChangeFrameEnergyCounter = 0;
            stat_updatesSent++;

            MQTT_PublishMain_StringFloat(counter_mqttNames[1], 
				BL_ChangeEnergyUnitIfNeeded(DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR)), roundingPrecision[PRECISION_ENERGY]);

            EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR, lastSentEnergyCounterLastHour, DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
            lastSentEnergyCounterLastHour = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);
            stat_updatesSent++;
            if(NTP_IsTimeSynced() == true)
            {
                MQTT_PublishMain_StringFloat(counter_mqttNames[3], 
					BL_ChangeEnergyUnitIfNeeded(dailyStats[1]), roundingPrecision[PRECISION_ENERGY]);
                stat_updatesSent++;
                MQTT_PublishMain_StringFloat(counter_mqttNames[4], 
					BL_ChangeEnergyUnitIfNeeded(dailyStats[0]), roundingPrecision[PRECISION_ENERGY]);
                stat_updatesSent++;
                ltm = localtime(&ConsumptionResetTime);
                snprintf(datetime,sizeof(datetime), "%04i-%02i-%02i %02i:%02i:%02i",
                        ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
                MQTT_PublishMain_StringString(counter_mqttNames[5], datetime, 0);
                stat_updatesSent++;
            }
        }
    } else {
        noChangeFrameEnergyCounter++;
        stat_updatesSkipped++;
    }
    if (((energyCounter - lastSavedEnergyCounterValue) >= changeSavedThresholdEnergy) ||
        ((xTaskGetTickCount() - lastConsumptionSaveStamp) >= (6 * 3600 * 1000 / portTICK_PERIOD_MS)))
    {
#if WINDOWS
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
        if (ota_progress() == -1)
#endif
        {
            lastSavedEnergyCounterValue = energyCounter;
            BL09XX_SaveEmeteringStatistics();
            lastConsumptionSaveStamp = xTaskGetTickCount();
        }
    }
}

void BL_Shared_Init(void)
{
    int i;
    ENERGY_METERING_DATA data;

    for(i = 0; i < OBK_NUM_MEASUREMENTS; i++)
    {
        noChangeFrames[i] = 0;
        lastReadings[i] = 0;
    }
    noChangeFrameEnergyCounter = 0;
    energyCounterStamp = xTaskGetTickCount(); 

    if (energyCounterStatsEnable == true)
    {
        if (energyCounterMinutes == NULL)
        {
            energyCounterMinutes = (float*)os_malloc(energyCounterSampleCount*sizeof(float));
        }
        if (energyCounterMinutes != NULL)
        {
            for(i = 0; i < energyCounterSampleCount; i++)
            {
                energyCounterMinutes[i] = 0.0;
            }   
        }
        energyCounterMinutesStamp = xTaskGetTickCount();
        energyCounterMinutesIndex = 0;
    }

    for(i = 0; i < DAILY_STATS_LENGTH; i++)
    {
        dailyStats[i] = 0;
    }

    addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Read ENERGYMETER status values. sizeof(ENERGY_METERING_DATA)=%d\n", sizeof(ENERGY_METERING_DATA));

    HAL_GetEnergyMeterStatus(&data);
    energyCounter = data.TotalConsumption;
    dailyStats[0] = data.TodayConsumpion;
    dailyStats[1] = data.YesterdayConsumption;
    actual_mday = data.actual_mday;    
    lastSavedEnergyCounterValue = energyCounter;
    dailyStats[2] = data.ConsumptionHistory[0];
    dailyStats[3] = data.ConsumptionHistory[1];
    ConsumptionResetTime = data.ConsumptionResetTime;
    ConsumptionSaveCounter = data.save_counter;
    lastConsumptionSaveStamp = xTaskGetTickCount();

    //int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA *data);

	//cmddetail:{"name":"EnergyCntReset","args":"",
	//cmddetail:"descr":"Resets the total Energy Counter, the one that is usually kept after device reboots. After this commands, the counter will start again from 0.",
	//cmddetail:"fn":"BL09XX_ResetEnergyCounter","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("EnergyCntReset", BL09XX_ResetEnergyCounter, NULL);
	//cmddetail:{"name":"SetupEnergyStats","args":"[Enable1or0][SampleTime][SampleCount][JSonEnable]",
	//cmddetail:"descr":"Setup Energy Statistic Parameters: [enable 0 or 1] [sample_time[10..90]] [sample_count[10..180]] [JsonEnable 0 or 1]. JSONEnable is optional.",
	//cmddetail:"fn":"BL09XX_SetupEnergyStatistic","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SetupEnergyStats", BL09XX_SetupEnergyStatistic, NULL);
	//cmddetail:{"name":"ConsumptionThreshold","args":"[FloatValue]",
	//cmddetail:"descr":"Setup value for automatic save of consumption data [1..100]",
	//cmddetail:"fn":"BL09XX_SetupConsumptionThreshold","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ConsumptionThreshold", BL09XX_SetupConsumptionThreshold, NULL);
	//cmddetail:{"name":"VCPPublishThreshold","args":"[VoltageDeltaVolts][CurrentDeltaAmpers][PowerDeltaWats][EnergyDeltaWh]",
	//cmddetail:"descr":"Sets the minimal change between previous reported value over MQTT and next reported value over MQTT. Very useful for BL0942, BL0937, etc. So, if you set, VCPPublishThreshold 0.5 0.001 0.5, it will only report voltage again if the delta from previous reported value is largen than 0.5V. Remember, that the device will also ALWAYS force-report values every N seconds (default 60)",
	//cmddetail:"fn":"BL09XX_VCPPublishThreshold","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("VCPPublishThreshold", BL09XX_VCPPublishThreshold, NULL);
	//cmddetail:{"name":"VCPPrecision","args":"[VoltageDigits][CurrentDigitsAmpers][PowerDigitsWats][EnergyDigitsWh]",
	//cmddetail:"descr":"Sets the number of digits after decimal point for power metering publishes. Default is BL09XX_VCPPrecision 1 3 2 3. This works for OBK-style publishes.",
	//cmddetail:"fn":"BL09XX_VCPPrecision","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("VCPPrecision", BL09XX_VCPPrecision, NULL);
	//cmddetail:{"name":"VCPPublishIntervals","args":"[MinDelayBetweenPublishes][ForcedPublishInterval]",
	//cmddetail:"descr":"First argument is minimal allowed interval in second between Voltage/Current/Power/Energy publishes (even if there is a large change), second value is an interval in which V/C/P/E is always published, even if there is no change",
	//cmddetail:"fn":"BL09XX_VCPPublishIntervals","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("VCPPublishIntervals", BL09XX_VCPPublishIntervals, NULL);
}

// OBK_POWER etc
float DRV_GetReading(int type) 
{
    int i;
    float hourly_sum = 0.0;
    switch (type)
    {
        case OBK_VOLTAGE: // must match order in cmd_public.h
        case OBK_CURRENT:
        case OBK_POWER:
            return lastReadings[type];
        case OBK_CONSUMPTION_TOTAL:
            return energyCounter;
        case OBK_CONSUMPTION_LAST_HOUR:
            if (energyCounterStatsEnable == true)
            {
                if (energyCounterMinutes != NULL)
                {
                    for(i=0;i<energyCounterSampleCount;i++)
                    {
                        hourly_sum += energyCounterMinutes[i];
                    }
                }
            }
            return hourly_sum;
        case OBK_CONSUMPTION_YESTERDAY:
            return dailyStats[1];
        case OBK_CONSUMPTION_TODAY:
            return dailyStats[0];
        default:
            break;
    }
    return 0.0f;
}


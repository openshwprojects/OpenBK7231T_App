#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../cJSON/cJSON.h"
#include <time.h>
#include "drv_ntp.h"
#include "../hal/hal_flashVars.h"
#include "../ota/ota.h"
#include <math.h>

#define DAILY_STATS_LENGTH 4

int stat_updatesSkipped = 0;
int stat_updatesSent = 0;

// Current values
float lastReadings[OBK_NUM_MEASUREMENTS];
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

void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request)
{
    int i;
    const char *mode;
    struct tm *ltm;

    if(DRV_IsRunning("BL0937")) {
        mode = "BL0937";
    } else if(DRV_IsRunning("BL0942")) {
        mode = "BL0942";
    } else if(DRV_IsRunning("CSE7766")) {
        mode = "CSE7766";
    } else {
        mode = "PWR";
    }
	
    hprintf255(request,"<h2>%s Voltage=%f, Current=%f, Power=%f",mode, lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT], lastReadings[OBK_POWER]);
    hprintf255(request,", Total Consumption=%1.1f Wh (changes sent %i, skipped %i, saved %li)</h2>",energyCounter, stat_updatesSent, stat_updatesSkipped, 
               ConsumptionSaveCounter);

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

void BL_ProcessUpdate(float voltage, float current, float power) 
{
    int i;
    float energy;    
    int xPassedTicks;
    cJSON* root;
    cJSON* stats;
    char *msg;
    portTickType interval;
    time_t g_time;
    struct tm *ltm;
    char datetime[64];

	// I had reports that BL0942 sometimes gives 
	// a large, negative peak of current/power
	if (CFG_HasFlag(OBK_FLAG_POWER_ALLOW_NEGATIVE)==false) 
    {
		if (power < 0.0f)
			power = 0.0f;
		if (voltage < 0.0f)
			voltage = 0.0f;
		if (current < 0.0f)
			current = 0.0f;
	}

    // those are final values, like 230V
    lastReadings[OBK_POWER] = power;
    lastReadings[OBK_VOLTAGE] = voltage;
    lastReadings[OBK_CURRENT] = current;
    
    xPassedTicks = (int)(xTaskGetTickCount() - energyCounterStamp);
    if (xPassedTicks <= 0)
        xPassedTicks = 1;
    energy = (float)xPassedTicks;
    energy *= power;
    energy /= (3600000.0f / (float)portTICK_PERIOD_MS);
    if (energy < 0)
    {
        energy = 0.0;
    }

    energyCounter += energy;
    energyCounterStamp = xTaskGetTickCount();
    HAL_FlashVars_SaveTotalConsumption(energyCounter);
    
    if(NTP_IsTimeSynced() == true) 
    {
        g_time = (time_t)NTP_GetCurrentTime();
        ltm = localtime(&g_time);
        if (ConsumptionResetTime == 0)
            ConsumptionResetTime = (time_t)g_time;

        if (actual_mday == -1)
        {
            actual_mday = ltm->tm_mday;
        }
        if (actual_mday != ltm->tm_mday)
        {
            for(i = 7; i > 0; i--)
            {
                dailyStats[i] = dailyStats[i - 1];
            } 
            dailyStats[0] = 0.0;
            actual_mday = ltm->tm_mday;
            MQTT_PublishMain_StringFloat(counter_mqttNames[3], dailyStats[1]);
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
                cJSON_AddNumberToObject(root, "uptime", Time_getUpTimeSeconds());
                cJSON_AddNumberToObject(root, "consumption_total", energyCounter );
                cJSON_AddNumberToObject(root, "consumption_last_hour",  DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
                cJSON_AddNumberToObject(root, "consumption_stat_index", energyCounterMinutesIndex);
                cJSON_AddNumberToObject(root, "consumption_sample_count", energyCounterSampleCount);
                cJSON_AddNumberToObject(root, "consumption_sampling_period", energyCounterSampleInterval);
                if(NTP_IsTimeSynced() == true)
                {
                    cJSON_AddNumberToObject(root, "consumption_today", dailyStats[0]);
                    cJSON_AddNumberToObject(root, "consumption_yesterday", dailyStats[1]);
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
                MQTT_PublishMain_StringFloat(counter_mqttNames[1], DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
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
		float diff = fabs(lastSentValues[i] - lastReadings[i]);
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
                MQTT_PublishMain_StringFloat(sensor_mqttNames[i],lastReadings[i]);
                stat_updatesSent++;
            }
        } else {
            // no change frame
            noChangeFrames[i]++;
            stat_updatesSkipped++;
        }
    }

    if ( (((energyCounter - lastSentEnergyCounterValue) >= changeSendThresholdEnergy) &&
          (noChangeFrameEnergyCounter >= changeDoNotSendMinFrames)) || 
         (noChangeFrameEnergyCounter >= changeSendAlwaysFrames) )
    {
        if (MQTT_IsReady() == true)
        {
            MQTT_PublishMain_StringFloat(counter_mqttNames[0], energyCounter);
            EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_TOTAL, lastSentEnergyCounterValue, energyCounter);
            lastSentEnergyCounterValue = energyCounter;
            noChangeFrameEnergyCounter = 0;
            stat_updatesSent++;
            MQTT_PublishMain_StringFloat(counter_mqttNames[1], DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
            EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR, lastSentEnergyCounterLastHour, DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
            lastSentEnergyCounterLastHour = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);
            stat_updatesSent++;
            if(NTP_IsTimeSynced() == true)
            {
                MQTT_PublishMain_StringFloat(counter_mqttNames[3], dailyStats[1]);
                stat_updatesSent++;
                MQTT_PublishMain_StringFloat(counter_mqttNames[4], dailyStats[0]);
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

void BL_Shared_Init()
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
	//cmddetail:"descr":"Reset Energy Counter",
	//cmddetail:"fn":"BL09XX_ResetEnergyCounter","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("EnergyCntReset", BL09XX_ResetEnergyCounter, NULL);
	//cmddetail:{"name":"SetupEnergyStats","args":"[Enable1or0][SampleTime][SampleCount]",
	//cmddetail:"descr":"Setup Energy Statistic Parameters: [enable<0|1>] [sample_time<10..900>] [sample_count<10..180>]",
	//cmddetail:"fn":"BL09XX_SetupEnergyStatistic","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SetupEnergyStats", BL09XX_SetupEnergyStatistic, NULL);
	//cmddetail:{"name":"ConsumptionThresold","args":"[FloatValue]",
	//cmddetail:"descr":"Setup value for automatic save of consumption data [1..100]",
	//cmddetail:"fn":"BL09XX_SetupConsumptionThreshold","file":"driver/drv_bl_shared.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ConsumptionThresold", BL09XX_SetupConsumptionThreshold, NULL);
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


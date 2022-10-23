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

// how much of value have to change in order to be send over MQTT again?
int changeSendThresholds[OBK_NUM_MEASUREMENTS] = {
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

    if(DRV_IsRunning("BL0937")) {
        mode = "BL0937";
    } else if(DRV_IsRunning("BL0942")) {
        mode = "BL0942";
    } else if(DRV_IsRunning("CSE7766")) {
        mode = "CSE7766";
    } else {
        mode = "PWR";
    }
	
    hprintf128(request,"<h2>%s Voltage=%f, Current=%f, Power=%f",mode, lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT], lastReadings[OBK_POWER]);
    hprintf128(request,", Total Consumption=%1.1f Wh (changes sent %i, skipped %i)</h2>",energyCounter, stat_updatesSent, stat_updatesSkipped);

    if (energyCounterStatsEnable == true)
    {
        /********************************************************************************************************************/
        hprintf128(request,"<h2>Periodic Statistics</h2><h5>Consumption (during this period): ");
        hprintf128(request,"%1.1f Wh<br>", DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
        hprintf128(request,"Sampling interval: %d sec<br>History length: ",energyCounterSampleInterval);
        hprintf128(request,"%d samples<br>History per samples:<br>",energyCounterSampleCount);
        if (energyCounterMinutes != NULL)
        {
            for(i=0; i<energyCounterSampleCount; i++)
            {
                if ((i%20)==0)
                {
                    hprintf128(request, "%1.1f", energyCounterMinutes[i]);
                } else {
                    hprintf128(request, ", %1.1f", energyCounterMinutes[i]);
                }
                if ((i%20)==19)
                {
                    hprintf128(request, "<br>");
                }
            }
			// energyCounterMinutesIndex is a long type, we need to use %ld instead of %d
            hprintf128(request, "<br>History Index: %ld<br>JSON Stats: %s </h5>", energyCounterMinutesIndex,
                    (energyCounterStatsJSONEnable == true) ? "enabled" : "disabled");
        }
    } else {
        hprintf128(request,"<h5>Periodic Statistics disabled. Use startup command SetupEnergyStats to enable function.</h5>");
    }
    /********************************************************************************************************************/
}

int BL09XX_ResetEnergyCounter(const void *context, const char *cmd, const char *args, int cmdFlags)
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
    } else {
        value = atof(args);
        energyCounter = value;
        energyCounterStamp = xTaskGetTickCount();
    }
    return 0;
}

int BL09XX_SetupEnergyStatistic(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    // SetupEnergyStats enable sample_time sample_count
    int enable;
    int sample_time;
    int sample_count;
    int json_enable;

    Tokenizer_TokenizeString(args);

    if(Tokenizer_GetArgsCount() < 3) 
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"BL09XX_SetupEnergyStatistic: requires 3 arguments (enable, sample_time, sample_count)\n");
        return -1;
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
        addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"Consumption History enabled\n");
        /* Enable function */
        energyCounterStatsEnable = true;
        if (energyCounterSampleCount != sample_count)
        {
            /* upgrade sample count, free memory */
            if (energyCounterMinutes != NULL)
                free(energyCounterMinutes);
            energyCounterMinutes = NULL;
            energyCounterSampleCount = sample_count;
        }
        addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"Sample Count:    %d\n", energyCounterSampleCount);
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
            energyCounterMinutes = (float*)malloc(sample_count*sizeof(float));
            if (energyCounterMinutes != NULL)
            {
                memset(energyCounterMinutes, 0, energyCounterSampleCount*sizeof(float));
            }
        }
        addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"Sample Interval: %d\n", energyCounterSampleInterval);

        energyCounterMinutesStamp = xTaskGetTickCount();
        energyCounterMinutesIndex = 0;
    } else {
        /* Disable Consimption Nistory */
        addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"Consumption History disabled\n");
        energyCounterStatsEnable = false;
        if (energyCounterMinutes != NULL)
        {
            free(energyCounterMinutes);
            energyCounterMinutes = NULL;
        }
        energyCounterSampleCount = sample_count;
        energyCounterSampleInterval = sample_time;
    }

    energyCounterStatsJSONEnable = (json_enable != 0) ? true : false; 

    return 0;
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

    energyCounter += energy;
    energyCounterStamp = xTaskGetTickCount();

    if (energyCounterStatsEnable == true)
    {
        interval = energyCounterSampleInterval;
        interval *= (1000 / portTICK_PERIOD_MS); 
        if ((xTaskGetTickCount() - energyCounterMinutesStamp) >= interval)
        {
            if (energyCounterStatsJSONEnable == true)
            {
                root = cJSON_CreateObject();
                cJSON_AddNumberToObject(root, "uptime", Time_getUpTimeSeconds());
                cJSON_AddNumberToObject(root, "consumption_total", energyCounter );
                cJSON_AddNumberToObject(root, "consumption_last_hour",  DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
                cJSON_AddNumberToObject(root, "consumption_stat_index", energyCounterMinutesIndex);
                cJSON_AddNumberToObject(root, "consumption_sample_count", energyCounterSampleCount);
                cJSON_AddNumberToObject(root, "consumption_sampling_period", energyCounterSampleInterval);

                if (energyCounterMinutes != NULL)
                {
                    stats = cJSON_CreateArray();
                    for(i = 0; i < energyCounterSampleCount; i++)
                    {
                        cJSON_AddItemToArray(stats, cJSON_CreateNumber(energyCounterMinutes[i]));
                    }
                    cJSON_AddItemToObject(root, "consumption_samples", stats);
                }

                msg = cJSON_Print(root);
                cJSON_Delete(root);

                MQTT_PublishMain_StringString(counter_mqttNames[2], msg, 0);
                stat_updatesSent++;
                free(msg);
            }

            if (energyCounterMinutes != NULL)
            {
                for (i=energyCounterSampleCount-1;i>0;i--)
                {
                    energyCounterMinutes[i] = energyCounterMinutes[i-1];
                }
                energyCounterMinutes[0] = 0.0;
            }
            energyCounterMinutesStamp = xTaskGetTickCount();
            energyCounterMinutesIndex++;

            MQTT_PublishMain_StringFloat(counter_mqttNames[1], DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
            EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR, lastSentEnergyCounterLastHour, DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
            lastSentEnergyCounterLastHour = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);
            stat_updatesSent++;
        }

        if (energyCounterMinutes != NULL)
            energyCounterMinutes[0] += energy;
    }

    for(i = 0; i < OBK_NUM_MEASUREMENTS; i++)
    {
        // send update only if there was a big change or if certain time has passed
        // Do not send message with every measurement. 
        if ( ((abs(lastSentValues[i]-lastReadings[i]) > changeSendThresholds[i]) &&
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
            lastSentValues[i] = lastReadings[i];
            MQTT_PublishMain_StringFloat(sensor_mqttNames[i],lastReadings[i]);
            stat_updatesSent++;
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
        MQTT_PublishMain_StringFloat(counter_mqttNames[0], energyCounter);
        EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_TOTAL, lastSentEnergyCounterValue, energyCounter);
        lastSentEnergyCounterValue = energyCounter;
        noChangeFrameEnergyCounter = 0;
        stat_updatesSent++;
        MQTT_PublishMain_StringFloat(counter_mqttNames[1], DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
        EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR, lastSentEnergyCounterLastHour, DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
        lastSentEnergyCounterLastHour = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);
        stat_updatesSent++;
    } else {
        noChangeFrameEnergyCounter++;
        stat_updatesSkipped++;
    }
}

void BL_Shared_Init()
{
    int i;

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
            energyCounterMinutes = (float*)malloc(energyCounterSampleCount*sizeof(float));
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

    CMD_RegisterCommand("EnergyCntReset", "", BL09XX_ResetEnergyCounter, "Reset Energy Counter", NULL);
    CMD_RegisterCommand("SetupEnergyStats", "", BL09XX_SetupEnergyStatistic, "Setup Energy Statistic Parameters: [enable<0|1>] [sample_time<10..900>] [sample_count<10..180>]", NULL);
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
        default:
            break;
    }
    return 0.0f;
}


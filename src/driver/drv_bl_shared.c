#include "drv_bl_shared.h"
#include "../obk_config.h"

#if ENABLE_BL_SHARED

#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cJSON/cJSON.h"
#include "../hal/hal_flashVars.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "../hal/hal_ota.h"
#include "drv_local.h"
#include "drv_ntp.h"
#include "drv_public.h"
#include "drv_uart.h"
#include "../cmnds/cmd_public.h" //for enum EventCode
#include <math.h>
#include <time.h>

#if ENABLE_BL_TWIN
#define BL_SENSDATASETS_COUNT 2
#else
#define BL_SENSDATASETS_COUNT 1
#endif
#if ENABLE_BL_TWIN
const int OBK_CONSUMPTION_STORED_LAST[2] = { OBK_CONSUMPTION_YESTERDAY,OBK_CONSUMPTION_TODAY };
#else
const int OBK_CONSUMPTION_STORED_LAST[1] = { OBK_CONSUMPTION__DAILY_LAST };
#endif

#if ENABLE_BL_TWIN
int stat_updatesSkipped[BL_SENSDATASETS_COUNT] = { 0 ,0 };
int stat_updatesSent[BL_SENSDATASETS_COUNT] = { 0,0 };
bool sensors_reciveddata[BL_SENSDATASETS_COUNT] = { 0,0 };  //1 if data received
float lastSavedEnergyCounterValue[BL_SENSDATASETS_COUNT] = { 0.0f, 0.0f };
int actual_mday[BL_SENSDATASETS_COUNT] = { -1 ,-1 };
#else
float lastSavedEnergyCounterValue[BL_SENSDATASETS_COUNT] = { 0.0f };
int stat_updatesSkipped[BL_SENSDATASETS_COUNT] = { 0 };
int stat_updatesSent[BL_SENSDATASETS_COUNT] = { 0 };
bool sensors_reciveddata[BL_SENSDATASETS_COUNT] = { 0 };  //1 if data received
int actual_mday[BL_SENSDATASETS_COUNT] = { -1 };
#endif

// Order corrsponds to enums OBK_VOLTAGE - OBK__LAST
// note that Wh/kWh units are overridden in hass_init_energy_sensor_device_info()
const char UNIT_WH[] = "Wh";
struct energysensor {
  energySensorNames_t names;
	byte rounding_decimals;
	// Variables below are for optimization
	// We can't send a full MQTT update every second.
	// It's too much for Beken, and it's too much for LWIP 2 MQTT library,
	// especially when actively browsing site and using JS app Log Viewer.
	// It even fails to publish with -1 error (can't alloc next packet)
	// So we publish when value changes from certain threshold or when a certain time passes.
	float changeSendThreshold;
	double lastReading; //double only needed for energycounter i.e. OBK_CONSUMPTION_TOTAL to avoid rounding issues as value becomes high
	double lastSentValue; // what are the last values we sent over the MQTT?
	int noChangeFrame; // how much update frames has passed without sending MQTT update of read values?
};

typedef struct energysensor energysensor_t;
typedef struct {
  energysensor_t sensors[OBK__NUM_SENSORS];
} energysensdataset_t;

energysensdataset_t datasetlist[BL_SENSDATASETS_COUNT] = {
  { {
  //.hass_dev_class, 	.units,		.name_friendly,			.name_mqtt,		 .hass_uniq_id_suffix, .rounding_decimals, .changeSendThreshold		
	  {{"voltage",		"V",		"Voltage",				"voltage",					"0",		},  1,			0.25,		},	// OBK_VOLTAGE
	  {{"current",		"A",		"Current",				"current",					"1",		},	3,			0.002,		},	// OBK_CURRENT
	  {{"power",			"W",		"Power",				"power",					"2",		},	2,			0.25,		},	// OBK_POWER
	  {{"apparent_power",	"VA",		"Apparent Power",		"power_apparent",			"9",		},	2,			0.25,		},	// OBK_POWER_APPARENT
	  {{"reactive_power",	"var",		"Reactive Power",		"power_reactive",			"10",		},	2,			0.25,		},	// OBK_POWER_REACTIVE
	  {{"power_factor",	"",			"Power Factor",			"power_factor",				"11",		},	2,			0.05,		},	// OBK_POWER_FACTOR
	  {{"energy",			UNIT_WH,	"Energy Total",			"energycounter",			"3",		},	3,			0.1,		},	// OBK_CONSUMPTION_TOTAL
	  {{"energy",			UNIT_WH,	"Energy Last Hour",		"energycounter_last_hour",	"4",		},	3,			0.1,		},	// OBK_CONSUMPTION_LAST_HOUR
	  //{{"",				"",			"Consumption Stats",	"consumption_stats",		"5",		},	0,			0,			},	// OBK_CONSUMPTION_STATS
	  {{"energy",			UNIT_WH,	"Energy Today",			"energycounter_today",		"7",		},	3,			0.1,		},	// OBK_CONSUMPTION_TODAY
	  {{"energy",			UNIT_WH,	"Energy Yesterday",		"energycounter_yesterday",	"6",		},	3,			0.1,		},	// OBK_CONSUMPTION_YESTERDAY
	  {{"energy",			UNIT_WH,	"Energy 2 Days Ago",	"energycounter_2_days_ago",	"12",		},	3,			0.1,		},	// OBK_CONSUMPTION_2_DAYS_AGO
	  {{"energy",			UNIT_WH,	"Energy 3 Days Ago",	"energycounter_3_days_ago",	"13",		},	3,			0.1,		},	// OBK_CONSUMPTION_3_DAYS_AGO
	  {{"timestamp",		"",			"Energy Clear Date",	"energycounter_clear_date",	"8",		},	0,			86400,		},	// OBK_CONSUMPTION_CLEAR_DATE	
    } }
#if BL_SENSDATASETS_COUNT==2
,
   { {
       //.hass_dev_class, 	.units,		.name_friendly,			.name_mqtt,		 .hass_uniq_id_suffix, .rounding_decimals, .changeSendThreshold		
    {{"voltage",		"V",		"Voltage B",				"voltage_b",					"b_0",		},  1,			0.25,		},	// OBK_VOLTAGE
    {{"current",		"A",		"Current B",				"current_b",					"b_1",		},	3,			0.002,		},	// OBK_CURRENT
    {{"power",			"W",		"Power B",				"power_b",					"b_2",		},	2,			0.25,		},	// OBK_POWER
    {{"apparent_power",	"VA",		"Apparent Power B",		"power_apparent_b",			"b_9",		},	2,			0.25,		},	// OBK_POWER_APPARENT
    {{"reactive_power",	"var",		"Reactive Power B",		"power_reactive_b",			"b_10",		},	2,			0.25,		},	// OBK_POWER_REACTIVE
    {{"power_factor",	"",			"Power Factor B",			"power_factor_b",				"b_11",		},	2,			0.05,		},	// OBK_POWER_FACTOR
    {{"energy",			UNIT_WH,	"Energy Total B",			"energycounter_b",			"b_3",		},	3,			0.1,		},	// OBK_CONSUMPTION_TOTAL
    {{"energy",			UNIT_WH,	"Energy Last Hour B",		"energycounter_last_hour_b",	"b_4",		},	3,			0.1,		},	// OBK_CONSUMPTION_LAST_HOUR
    //{{"",				"",			"Consumption Stats B",	"consumption_stats_b",		"b_5",		},	0,			0,			},	// OBK_CONSUMPTION_STATS
    {{"energy",			UNIT_WH,	"Energy Today B",			"energycounter_today_b",		"b_7",		},	3,			0.1,		},	// OBK_CONSUMPTION_TODAY
    {{"energy",			UNIT_WH,	"Energy Yesterday B",		"energycounter_yesterday_b",	"b_6",		},	3,			0.1,		},	// OBK_CONSUMPTION_YESTERDAY
    {{"energy",			UNIT_WH,	"Energy 2 Days Ago B",	"energycounter_2_days_ago_b",	"b_12",		},	3,			0.1,		},	// OBK_CONSUMPTION_2_DAYS_AGO
    {{"energy",			UNIT_WH,	"Energy 3 Days Ago B",	"energycounter_3_days_ago_b",	"b_13",		},	3,			0.1,		},	// OBK_CONSUMPTION_3_DAYS_AGO
    {{"timestamp",		"",			"Energy Clear Date B",	"energycounter_clear_date_b",	"b_8",		},	0,			86400,		},	// OBK_CONSUMPTION_CLEAR_DATE	
     } }
#endif
};

float lastReadingFrequency = NAN;

//static double energyCounter = 0.0;
portTickType energyCounterStamp[BL_SENSDATASETS_COUNT];
bool energyCounterStatsEnable = false;
int energyCounterSampleCount = 60;
int energyCounterSampleInterval = 60;
float *energyCounterMinutes = NULL;
portTickType energyCounterMinutesStamp;
long energyCounterMinutesIndex;
bool energyCounterStatsJSONEnable = false;

float changeSavedThresholdEnergy = 10.0f;
long ConsumptionSaveCounter = 0;
portTickType lastConsumptionSaveStamp;
time_t ConsumptionResetTime = 0;

int changeSendAlwaysFrames = 60;
int changeDoNotSendMinFrames = 5;

void BL_ResetRecivedDataBool() {
  for (int i = 0; i < BL_SENSDATASETS_COUNT; i++) sensors_reciveddata[i] = 0;
}

#if ENABLE_BL_TWIN
void BL09XX_AppendInformationToHTTPIndexPageEx(int asensdatasetix, http_request_t *request)
{
  if ((asensdatasetix < 0) || (asensdatasetix >= BL_SENSDATASETS_COUNT)) return;  //to avoid bad index on data[BL_SENSDATASETS_COUNT]
#else
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t * request, int bPreState)
{
	if (bPreState)
		return;
  int asensdatasetix = BL_SENSORS_IX_0;
#endif
  energysensdataset_t* sensdataset = &datasetlist[asensdatasetix];
    
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
    } else if(DRV_IsRunning("RN8209")) {
        mode = "RN8209";
    } else {
        mode = "PWR";
    }

    poststr(request, "<hr><table style='width:100%'>");

    if (asensdatasetix == BL_SENSORS_IX_0) 
    {
      if (!isnan(lastReadingFrequency)) {
        poststr(request,
          "<tr><td><b>Frequency</b></td><td style='text-align: right;'>");
        hprintf255(request, "%.2f</td><td>Hz</td>", lastReadingFrequency);
      }
    }

	for (int i = OBK__FIRST; i <= OBK_CONSUMPTION__DAILY_LAST; i++) {
#if ENABLE_BL_TWIN
    //in twin mode, for ix1 is possible to skip OBK_VOLTAGE, dont skip for now
    //if ((asensdatasetix > BL_SENSORS_IX_0) && (i == OBK_VOLTAGE)) continue;
    //in twin mode, for ix0 is last OBK_CONSUMPTION_YESTERDAY, for ix1 ,OBK_CONSUMPTION_TODAY
    if (i > OBK_CONSUMPTION_STORED_LAST[asensdatasetix]) continue;
#endif
    if ((energyCounterMinutes == NULL) && (i == OBK_CONSUMPTION_LAST_HOUR)) {
      continue;
    }
    if (i <= OBK__NUM_MEASUREMENTS || NTP_IsTimeSynced()) {
			poststr(request, "<tr><td><b>");
			poststr(request, sensdataset->sensors[i].names.name_friendly);
			poststr(request, "</b></td><td style='text-align: right;'>");
			hprintf255(request, "%.*f</td><td>%s</td>", sensdataset->sensors[i].rounding_decimals,
					(i == OBK_CONSUMPTION_TOTAL ? 0.001 : 1) * sensdataset->sensors[i].lastReading, //always display OBK_CONSUMPTION_TOTAL in kwh
					i == OBK_CONSUMPTION_TOTAL ? "kWh": sensdataset->sensors[i].names.units);
		}
	};

    poststr(request, "</table>");

    hprintf255(request, "(changes sent %i, skipped %i, saved %li) - %s<hr>",
        stat_updatesSent[asensdatasetix], stat_updatesSkipped[asensdatasetix], ConsumptionSaveCounter,
      mode);

    if (asensdatasetix == BL_SENSORS_IX_0)
    {
      poststr(request, "<h5>Energy Clear Date: ");
      if (ConsumptionResetTime) {
        ltm = gmtime(&ConsumptionResetTime);
        hprintf255(request, "%04d-%02d-%02d %02d:%02d:%02d",
          ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
      } else {
        poststr(request, "(not set)");
      }

      hprintf255(request, "<br>");
      if(DRV_IsRunning("NTP")==false) {
        hprintf255(request,"NTP driver is not started, daily energy stats disabled.");
      } else if (!NTP_IsTimeSynced()) {
        hprintf255(request,"Daily energy stats awaiting NTP driver to sync real time...");
      }
      hprintf255(request, "</h5>");

      if (energyCounterStatsEnable == true)
      {
        /********************************************************************************************************************/
        hprintf255(request,"<h2>Periodic Statistics</h2><h5>Consumption (during this period): ");
        hprintf255(request,"%1.*f Wh<br>", sensdataset->sensors[OBK_CONSUMPTION_LAST_HOUR].rounding_decimals, DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR));
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

        hprintf255(request, "</h5>");
      } else {
        hprintf255(request,"<h5>Periodic Statistics disabled. Use startup command SetupEnergyStats to enable function.</h5>");
      }
      /********************************************************************************************************************/
    }
}

#if ENABLE_BL_TWIN
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState)
		return;
  if (sensors_reciveddata[BL_SENSORS_IX_0]) {
    BL09XX_AppendInformationToHTTPIndexPageEx(BL_SENSORS_IX_0, request);
  }
  if (sensors_reciveddata[BL_SENSORS_IX_1]) {
    BL09XX_AppendInformationToHTTPIndexPageEx(BL_SENSORS_IX_1, request);
  }
}
#endif

void BL09XX_SaveEmeteringStatistics()
{
  energysensdataset_t* sensdataset = &datasetlist[BL_SENSORS_IX_0];
  #if ENABLE_BL_TWIN
  energysensdataset_t* sensdataset1 = &datasetlist[BL_SENSORS_IX_1];
  #endif

  ENERGY_METERING_DATA data;

    memset(&data, 0, sizeof(ENERGY_METERING_DATA));

    data.TotalConsumption = (float)sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading;
    data.TodayConsumpion = (float)sensdataset->sensors[OBK_CONSUMPTION_TODAY].lastReading;
    data.YesterdayConsumption = (float)sensdataset->sensors[OBK_CONSUMPTION_YESTERDAY].lastReading;
    data.actual_mday = actual_mday[BL_SENSORS_IX_0];//one in flashvars is enough, I assume that both channels are synchronized
#if ENABLE_BL_TWIN
    data.TotalConsumption_b = (float)sensdataset1->sensors[OBK_CONSUMPTION_TOTAL].lastReading;
    data.TodayConsumpion_b = (float)sensdataset1->sensors[OBK_CONSUMPTION_TODAY].lastReading;
#else
    data.ConsumptionHistory[0] = (float)sensdataset->sensors[OBK_CONSUMPTION_2_DAYS_AGO].lastReading;
    data.ConsumptionHistory[1] = (float)sensdataset->sensors[OBK_CONSUMPTION_3_DAYS_AGO].lastReading;
#endif
    data.ConsumptionResetTime = ConsumptionResetTime;
    ConsumptionSaveCounter++;
    data.save_counter = ConsumptionSaveCounter;

    HAL_SetEnergyMeterStatus(&data);
}

commandResult_t BL09XX_ResetEnergyCounterEx(int asensdatasetix, float* pvalue)
{
  if ((asensdatasetix < 0) || (asensdatasetix >= BL_SENSDATASETS_COUNT)) return CMD_RES_ERROR;  //to avoid bad index on data[BL_SENSDATASETS_COUNT]
  energysensdataset_t* sensdataset = &datasetlist[asensdatasetix];

    int i;

    if (!pvalue) {
    //addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ResEC %i", asensdatasetix);
      lastSavedEnergyCounterValue[asensdatasetix] = 0.0; //20250203 reset lastSavedEnergyCounterValue, otherwise the values will not be saved until restart BL
      sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading = 0.0;
      energyCounterStamp[asensdatasetix] = xTaskGetTickCount();
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
        for(i = OBK_CONSUMPTION__DAILY_FIRST; i <= OBK_CONSUMPTION__DAILY_LAST; i++)
        {
          sensdataset->sensors[i].lastReading = 0.0;
        }
    } else {
      //addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "ResEC %i t=%f", asensdatasetix,avalue);
      sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading = *pvalue;
      energyCounterStamp[asensdatasetix] = xTaskGetTickCount();
    }
    ConsumptionResetTime = (time_t)NTP_GetCurrentTime();
    if (OTA_GetProgress()==-1)
    { 
      BL09XX_SaveEmeteringStatistics();
      lastConsumptionSaveStamp = xTaskGetTickCount();
    }
    return CMD_RES_OK;
}

commandResult_t BL09XX_ResetEnergyCounter(const void* context, const char* cmd, const char* args, int cmdFlags)
{
  Tokenizer_TokenizeString(args, 0);
  int acnt = Tokenizer_GetArgsCount();
  if (acnt == 0) {    
#if ENABLE_BL_TWIN
    BL09XX_ResetEnergyCounterEx(BL_SENSORS_IX_1, NULL);
#endif
    return BL09XX_ResetEnergyCounterEx(BL_SENSORS_IX_0, NULL);
  } else {
    float fvalue = Tokenizer_GetArgFloat(0);
    int fsix = BL_SENSORS_IX_0;
#if ENABLE_BL_TWIN
    if (acnt>=2) fsix = Tokenizer_GetArgInteger(1);
#endif
    return BL09XX_ResetEnergyCounterEx(fsix, &fvalue);
  }
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

#if ENABLE_BL_TWIN
commandResult_t BL09XX_VCPPrecisionEx(int asensdatasetix, const void *context, const char *cmd, const char *args, int cmdFlags)
{
  if ((asensdatasetix < 0) || (asensdatasetix >= BL_SENSDATASETS_COUNT)) return CMD_RES_ERROR;  //to avoid bad index on data[BL_SENSDATASETS_COUNT]
#else
commandResult_t BL09XX_VCPPrecision(const void* context, const char* cmd, const char* args, int cmdFlags) {
  int asensdatasetix = BL_SENSORS_IX_0;
#endif
  energysensdataset_t* sensdataset = &datasetlist[asensdatasetix];

  int i;
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

  for (i = 0; i < Tokenizer_GetArgsCount(); i++) {
    int val = Tokenizer_GetArgInteger(i);
    switch(i) {
    case 0: // voltage
      sensdataset->sensors[OBK_VOLTAGE].rounding_decimals = val;
      break;
    case 1: // current
      sensdataset->sensors[OBK_CURRENT].rounding_decimals = val;
      break;
    case 2: // power
      sensdataset->sensors[OBK_POWER].rounding_decimals = val;
      sensdataset->sensors[OBK_POWER_APPARENT].rounding_decimals = val;
      sensdataset->sensors[OBK_POWER_REACTIVE].rounding_decimals = val;
      break;
    case 3: // energy
      for (int j = OBK_CONSUMPTION__DAILY_FIRST; j <= OBK_CONSUMPTION__DAILY_LAST; j++) {
        sensdataset->sensors[j].rounding_decimals = val;
      };

    };
  }
	return CMD_RES_OK;
}

#if ENABLE_BL_TWIN
commandResult_t BL09XX_VCPPrecision(const void* context, const char* cmd, const char* args, int cmdFlags) {
  return BL09XX_VCPPrecisionEx(BL_SENSORS_IX_0, context, cmd, args, cmdFlags);
}
#endif

#if ENABLE_BL_TWIN
commandResult_t BL09XX_VCPPublishThresholdEx(int asensdatasetix, const void* context, const char* cmd, const char* args, int cmdFlags)
{
  if ((asensdatasetix < 0) || (asensdatasetix >= BL_SENSDATASETS_COUNT)) return CMD_RES_ERROR;  //to avoid bad index on data[BL_SENSDATASETS_COUNT]
#else
commandResult_t BL09XX_VCPPublishThreshold(const void* context, const char* cmd, const char* args, int cmdFlags)
{
  int asensdatasetix = BL_SENSORS_IX_0;
#endif
  energysensdataset_t* sensdataset = &datasetlist[asensdatasetix];

  Tokenizer_TokenizeString(args, 0);
  // following check must be done after 'Tokenizer_TokenizeString',
  // so we know arguments count in Tokenizer. 'cmd' argument is
  // only for warning display
  if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  }

  sensdataset->sensors[OBK_VOLTAGE].changeSendThreshold = Tokenizer_GetArgFloat(0);
  sensdataset->sensors[OBK_CURRENT].changeSendThreshold = Tokenizer_GetArgFloat(1);
  sensdataset->sensors[OBK_POWER].changeSendThreshold = Tokenizer_GetArgFloat(2);
  sensdataset->sensors[OBK_POWER_APPARENT].changeSendThreshold = Tokenizer_GetArgFloat(2);
  sensdataset->sensors[OBK_POWER_REACTIVE].changeSendThreshold = Tokenizer_GetArgFloat(2);
  //sensdataset->sensors[OBK_POWER_FACTOR].changeSendThreshold = Tokenizer_GetArgFloat(TODO);

  if (Tokenizer_GetArgsCount() >= 4) {
    for (int i = OBK_CONSUMPTION_LAST_HOUR; i <= OBK_CONSUMPTION__DAILY_LAST; i++) {
      sensdataset->sensors[i].changeSendThreshold = Tokenizer_GetArgFloat(3);
    }
  }
  return CMD_RES_OK;
}

#if ENABLE_BL_TWIN
commandResult_t BL09XX_VCPPublishThreshold(const void* context, const char* cmd, const char* args, int cmdFlags)
{
  BL09XX_VCPPublishThresholdEx(BL_SENSORS_IX_1, context, cmd, args, cmdFlags);
  return BL09XX_VCPPublishThresholdEx(BL_SENSORS_IX_0, context, cmd, args, cmdFlags);
}
#endif

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
    
    threshold = (float)atof(Tokenizer_GetArg(0));

    if (threshold<1.0f)
        threshold = 1.0f;
    if (threshold>5000.0f)
        threshold = 5000.0f;
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

#if ENABLE_BL_TWIN
void BL_ProcessUpdateEx(int asensdatasetix, float voltage, float current, float power,
  float frequency, float energyWh) {
  if ((asensdatasetix < 0) || (asensdatasetix >= BL_SENSDATASETS_COUNT)) return;  //to avoid bad index on data[BL_SENSDATASETS_COUNT]
#else
void BL_ProcessUpdate(float voltage, float current, float power,
  float frequency, float energyWh) {
  int asensdatasetix = BL_SENSORS_IX_0;
#endif
  energysensdataset_t* sensdataset = &datasetlist[asensdatasetix];

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

#ifdef ENABLE_BL_MOVINGAVG
  power = XJ_MovingAverage_float((float)sensdataset->sensors[OBK_POWER].lastReading, power);
  current = XJ_MovingAverage_float((float)sensdataset->sensors[OBK_CURRENT].lastReading, current);
#endif

  sensdataset->sensors[OBK_VOLTAGE].lastReading = voltage;
  sensdataset->sensors[OBK_CURRENT].lastReading = current;
  sensdataset->sensors[OBK_POWER].lastReading = power;
  sensdataset->sensors[OBK_POWER_APPARENT].lastReading = sensdataset->sensors[OBK_VOLTAGE].lastReading * sensdataset->sensors[OBK_CURRENT].lastReading;
  sensdataset->sensors[OBK_POWER_REACTIVE].lastReading = (sensdataset->sensors[OBK_POWER_APPARENT].lastReading <= fabsf((float)sensdataset->sensors[OBK_POWER].lastReading)
    ? 0
    : sqrtf(powf((float)sensdataset->sensors[OBK_POWER_APPARENT].lastReading, 2) -
      powf((float)sensdataset->sensors[OBK_POWER].lastReading, 2)));
  sensdataset->sensors[OBK_POWER_FACTOR].lastReading =
    (sensdataset->sensors[OBK_POWER_APPARENT].lastReading == 0 ? 1 : sensdataset->sensors[OBK_POWER].lastReading / sensdataset->sensors[OBK_POWER_APPARENT].lastReading);

  lastReadingFrequency = frequency;

  sensors_reciveddata[asensdatasetix] = 1;
  {
    float energy = 0;
    if (isnan(energyWh)) {
      xPassedTicks = (int)(xTaskGetTickCount() - energyCounterStamp[asensdatasetix]);
      // FIXME: Wrong calculation if tick count overflows
      if (xPassedTicks <= 0)
        xPassedTicks = 1;
      energy = xPassedTicks * power / (3600000.0f / portTICK_PERIOD_MS);
    } else
      energy = energyWh;

    if (energy < 0)
      energy = 0.0;

    sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading += (double)energy;
    energyCounterStamp[asensdatasetix] = xTaskGetTickCount();
    #if ENABLE_BL_TWIN
    if (asensdatasetix == BL_SENSORS_IX_0) {
      //update only IX0, IX1 will be saved later in BL09XX_SaveEmeteringStatistics()
      HAL_FlashVars_SaveTotalConsumption((float)sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading);
    }
    #else
    HAL_FlashVars_SaveTotalConsumption((float)sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading);
    #endif
    sensdataset->sensors[OBK_CONSUMPTION_TODAY].lastReading += energy;

    if (NTP_IsTimeSynced()) {
      ntpTime = (time_t)NTP_GetCurrentTime();
      ltm = gmtime(&ntpTime);
      if (ConsumptionResetTime == 0)
        ConsumptionResetTime = (time_t)ntpTime;

      if (actual_mday[asensdatasetix] == -1)
      {
        actual_mday[asensdatasetix] = ltm->tm_mday;
      }
      if (actual_mday[asensdatasetix] != ltm->tm_mday)
      {
        for (i = OBK_CONSUMPTION__DAILY_LAST; i >= OBK_CONSUMPTION__DAILY_FIRST; i--) {
          sensdataset->sensors[i].lastReading = sensdataset->sensors[i - 1].lastReading;
        }
        sensdataset->sensors[OBK_CONSUMPTION_TODAY].lastReading = 0.0;
        actual_mday[asensdatasetix] = ltm->tm_mday;

        //MQTT_PublishMain_StringFloat(sensdataset->sensors[OBK_CONSUMPTION_YESTERDAY].names.name_mqtt, BL_ChangeEnergyUnitIfNeeded(sensors[OBK_CONSUMPTION_YESTERDAY].lastReading ),
        //							sensdataset->sensors[OBK_CONSUMPTION_YESTERDAY].rounding_decimals, 0);
        //stat_updatesSent++;
        if (OTA_GetProgress()==-1)
        {
          BL09XX_SaveEmeteringStatistics();
          lastConsumptionSaveStamp = xTaskGetTickCount();
        }

      }
    }

    if ((energyCounterStatsEnable == true) && (asensdatasetix==BL_SENSORS_IX_0))
    {
      interval = energyCounterSampleInterval;
      interval *= (1000 / portTICK_PERIOD_MS);
      if ((xTaskGetTickCount() - energyCounterMinutesStamp) >= interval)
      {
        if (energyCounterMinutes != NULL) {
          sensdataset->sensors[OBK_CONSUMPTION_LAST_HOUR].lastReading = 0;
          for (int i = 0; i < energyCounterSampleCount; i++) {
            sensdataset->sensors[OBK_CONSUMPTION_LAST_HOUR].lastReading += energyCounterMinutes[i];
          }
        }
#if ENABLE_MQTT
        if ((energyCounterStatsJSONEnable == true) && (MQTT_IsReady() == true))
        {
          root = cJSON_CreateObject();
          cJSON_AddNumberToObject(root, "uptime", g_secondsElapsed);
          cJSON_AddNumberToObject(root, "consumption_total", BL_ChangeEnergyUnitIfNeeded(DRV_GetReading(OBK_CONSUMPTION_TOTAL)));
          cJSON_AddNumberToObject(root, "consumption_last_hour", BL_ChangeEnergyUnitIfNeeded(DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR)));
          cJSON_AddNumberToObject(root, "consumption_stat_index", energyCounterMinutesIndex);
          cJSON_AddNumberToObject(root, "consumption_sample_count", energyCounterSampleCount);
          cJSON_AddNumberToObject(root, "consumption_sampling_period", energyCounterSampleInterval);
          if(NTP_IsTimeSynced() == true)
          {
            cJSON_AddNumberToObject(root, "consumption_today", BL_ChangeEnergyUnitIfNeeded(DRV_GetReading(OBK_CONSUMPTION_TODAY)));
            cJSON_AddNumberToObject(root, "consumption_yesterday", BL_ChangeEnergyUnitIfNeeded(DRV_GetReading(OBK_CONSUMPTION_YESTERDAY)));
            ltm = gmtime(&ConsumptionResetTime);
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
            for(i = OBK_CONSUMPTION__DAILY_FIRST; i <= OBK_CONSUMPTION__DAILY_LAST; i++)
            {
              cJSON_AddItemToArray(stats, cJSON_CreateNumber(DRV_GetReading(i)));
            }
            cJSON_AddItemToObject(root, "consumption_daily", stats);
          }

          msg = cJSON_PrintUnformatted(root);
          cJSON_Delete(root);

          // addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "JSON Printed: %d bytes", strlen(msg));

          MQTT_PublishMain_StringString("consumption_stats", msg, 0);
          stat_updatesSent[asensdatasetix]++;
          os_free(msg);
        }
#endif

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

      }

      if (energyCounterMinutes != NULL)
        energyCounterMinutes[0] += energy;
    }
  }
  for (i = OBK__FIRST; i <= OBK__LAST; i++)
  {
#ifdef ENABLE_BL_TWIN
    //in twin mode, for ix1 is possible to skip OBK_VOLTAGE, dont skip for now
    //if ((asensdatasetix > 0) && (i== OBK_VOLTAGE)) continue;
    //in twin mode, for ix0 is last OBK_CONSUMPTION_YESTERDAY, for ix1 ,OBK_CONSUMPTION_TODAY
    if ((i > OBK_CONSUMPTION_STORED_LAST[asensdatasetix]) && (i <= OBK_CONSUMPTION__DAILY_LAST)) continue;
#endif
      // send update only if there was a big change or if certain time has passed
    // Do not send message with every measurement. 
    diff = (float)sensdataset->sensors[i].lastSentValue - (float)sensdataset->sensors[i].lastReading;
    // check for change
    if (((fabsf(diff) > sensdataset->sensors[i].changeSendThreshold) &&
      (sensdataset->sensors[i].noChangeFrame >= changeDoNotSendMinFrames)) ||
      (sensdataset->sensors[i].noChangeFrame >= changeSendAlwaysFrames))
    {
      sensdataset->sensors[i].noChangeFrame = 0;

      enum EventCode eventChangeCode;
      switch (i) {
      case OBK_VOLTAGE:				eventChangeCode = CMD_EVENT_CHANGE_VOLTAGE;	break;
      case OBK_CURRENT:				eventChangeCode = CMD_EVENT_CHANGE_CURRENT;	break;
      case OBK_POWER:					eventChangeCode = CMD_EVENT_CHANGE_POWER; break;
      case OBK_CONSUMPTION_TOTAL:		eventChangeCode = CMD_EVENT_CHANGE_CONSUMPTION_TOTAL; break;
      case OBK_CONSUMPTION_LAST_HOUR:	eventChangeCode = CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR; break;
      default:						eventChangeCode = CMD_EVENT_NONE; break;
      }
      switch (eventChangeCode) {
      case CMD_EVENT_NONE:
        break;
      case CMD_EVENT_CHANGE_CURRENT:;
        int prev_mA = (int)(sensdataset->sensors[i].lastSentValue * 1000);
        int now_mA = (int)(sensdataset->sensors[i].lastReading * 1000);
        EventHandlers_ProcessVariableChange_Integer(eventChangeCode, prev_mA, now_mA);
        break;
      default:
        EventHandlers_ProcessVariableChange_Integer(eventChangeCode, (int)sensdataset->sensors[i].lastSentValue, (int)sensdataset->sensors[i].lastReading);
        break;
      }

      sensdataset->sensors[i].lastSentValue = sensdataset->sensors[i].lastReading;
#if ENABLE_MQTT
      if (MQTT_IsReady() == true)
      {
        if (i == OBK_CONSUMPTION_CLEAR_DATE) {
          {
            sensdataset->sensors[i].lastReading = ConsumptionResetTime; //Only to make the 'nochangeframe' mechanism work here
            ltm = gmtime(&ConsumptionResetTime);
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
            MQTT_PublishMain_StringString(sensdataset->sensors[i].names.name_mqtt, datetime, 0);
          }
        } else { //all other sensors
          float val = (float)sensdataset->sensors[i].lastReading;
          if (sensdataset->sensors[i].names.units == UNIT_WH) val = BL_ChangeEnergyUnitIfNeeded(val);
          MQTT_PublishMain_StringFloat(sensdataset->sensors[i].names.name_mqtt, val, sensdataset->sensors[i].rounding_decimals, OBK_PUBLISH_FLAG_QOS_ZERO);
        }
        if ((asensdatasetix==BL_SENSORS_IX_0) && (i == OBK_VOLTAGE)) {
          //20250319 XJIKKA to simplify and save space in flash frequency together with voltage
          if (!isnan(lastReadingFrequency)) {
            char schannel[4];
            snprintf(schannel, sizeof(schannel), "%i", SPECIAL_CHANNEL_OBK_FREQUENCY);
            MQTT_PublishMain_StringFloat(schannel, lastReadingFrequency, 2, OBK_PUBLISH_FLAG_QOS_ZERO);
          }
        }
        stat_updatesSent[asensdatasetix]++;
      }
#endif
    } else {
      // no change frame
      sensdataset->sensors[i].noChangeFrame++;
      stat_updatesSkipped[asensdatasetix]++;
    }
  }

  {
      if (((sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading - lastSavedEnergyCounterValue[asensdatasetix]) >= changeSavedThresholdEnergy) ||
      ((xTaskGetTickCount() - lastConsumptionSaveStamp) >= (6 * 3600 * 1000 / portTICK_PERIOD_MS)))
    {
      if (OTA_GetProgress() == -1)
      {
        lastSavedEnergyCounterValue[asensdatasetix] = (float)sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading;
        BL09XX_SaveEmeteringStatistics();
        lastConsumptionSaveStamp = xTaskGetTickCount();
      }
    }
  }
}

#if ENABLE_BL_TWIN
void BL_ProcessUpdate(float voltage, float current, float power,
  float frequency, float energyWh) {
  BL_ProcessUpdateEx(BL_SENSORS_IX_0, voltage, current, power, frequency, energyWh);
}
#endif

void BL_Shared_Init(void) {
  energysensdataset_t* sensdataset = &datasetlist[BL_SENSORS_IX_0];
#if ENABLE_BL_TWIN
  energysensdataset_t* sensdataset1 = &datasetlist[BL_SENSORS_IX_1];
#endif

  int i;
    ENERGY_METERING_DATA data;

    for(i = OBK__FIRST; i <= OBK__LAST; i++)
    {
      sensdataset->sensors[i].noChangeFrame = 0;
      sensdataset->sensors[i].lastReading = 0;
    }
    {

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

      addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Read ENERGYMETER values sz=%d\n", sizeof(ENERGY_METERING_DATA));

      HAL_GetEnergyMeterStatus(&data);
      sensdataset->sensors[OBK_CONSUMPTION_TOTAL].lastReading = data.TotalConsumption;
      sensdataset->sensors[OBK_CONSUMPTION_TODAY].lastReading = data.TodayConsumpion;
      sensdataset->sensors[OBK_CONSUMPTION_YESTERDAY].lastReading = data.YesterdayConsumption;
      actual_mday[BL_SENSORS_IX_0] = data.actual_mday;//one in flashvars is enough, I assume that both channels are synchronized
#if ENABLE_BL_TWIN
      sensdataset1->sensors[OBK_CONSUMPTION_TOTAL].lastReading = data.TotalConsumption_b;
      sensdataset1->sensors[OBK_CONSUMPTION_TODAY].lastReading = data.TodayConsumpion_b;
      lastSavedEnergyCounterValue[BL_SENSORS_IX_0] = data.TotalConsumption;
      lastSavedEnergyCounterValue[BL_SENSORS_IX_1] = data.TotalConsumption_b;
      energyCounterStamp[BL_SENSORS_IX_0] = xTaskGetTickCount();
      energyCounterStamp[BL_SENSORS_IX_1] = xTaskGetTickCount();
      actual_mday[BL_SENSORS_IX_1] = data.actual_mday;//one in flashvars is enough, I assume that both channels are synchronized
#else
      lastSavedEnergyCounterValue[BL_SENSORS_IX_0] = data.TotalConsumption;
      sensdataset->sensors[OBK_CONSUMPTION_2_DAYS_AGO].lastReading = data.ConsumptionHistory[0];
      sensdataset->sensors[OBK_CONSUMPTION_3_DAYS_AGO].lastReading = data.ConsumptionHistory[1];
      energyCounterStamp[BL_SENSORS_IX_0] = xTaskGetTickCount();
#endif
      ConsumptionResetTime = data.ConsumptionResetTime;
      ConsumptionSaveCounter = data.save_counter;
      lastConsumptionSaveStamp = xTaskGetTickCount();

      //int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA *data);
    }

	//cmddetail:{"name":"EnergyCntReset","args":"[OptionalNewValue][sensorix]",
	//cmddetail:"descr":"Resets the total Energy Counter, the one that is usually kept after device reboots. After this commands, the counter will start again from 0 (or from the value you specified). sensorix is used in ENABLE_BL_TWIN",
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
float DRV_GetReading(energySensor_t type) 
{
	return (float)datasetlist[BL_SENSORS_IX_0].sensors[type].lastReading;
}

#if ENABLE_BL_TWIN
int BL_IsMeteringDeviceIndexActive(int asensdatasetix){
  if (asensdatasetix < 0) return false;
  if (asensdatasetix >= BL_SENSDATASETS_COUNT) return false;
  return sensors_reciveddata[asensdatasetix];
}
#endif

energySensorNames_t* DRV_GetEnergySensorNames(energySensor_t type)
{
	return &datasetlist[BL_SENSORS_IX_0].sensors[type].names;
}

/// @param asensdatasetix dataset index when using two energy sensors
/// @param type energySensor_t
energySensorNames_t* DRV_GetEnergySensorNamesEx(int asensdatasetix, energySensor_t type)
{
  if ((asensdatasetix < 0) || (asensdatasetix >= BL_SENSDATASETS_COUNT)) asensdatasetix= BL_SENSORS_IX_0;//here default 0 - hass.c not checking result
  return &datasetlist[asensdatasetix].sensors[type].names;
}

#endif

int sync = 0;
int check_time = 0;
int dump_load_hysteresis = 2;	// This is shortest time the relay will turn on or off. Recommended 1/4 of the netmetering period. Never use less than 1min as this stresses the relay/load.
int min_production = -120;	// The minimun instantaneous solar production that will trigger the dump load.
int dump_load_on = -2;		// The ammount of 'excess' energy stored over the period. Above this, the dump load will be turned on.
int dump_load_off = 0;		// The minimun 'excess' energy stored over the period. Below this, the dump load will be turned off.
int dump_load_relay = 0;
//Command to turn remote plug on/off
//const char* rem_relay_on = "http://<ip>/cm?cmnd=Power%20on";
//const char* rem_relay_off = "http://<ip>/cm?cmnd=Power%20off";
//-----------------------------------------------------------
	
// Order corrsponds to enums OBK_VOLTAGE - OBK__LAST
// note that Wh/kWh units are overridden in hass_init_energy_sensor_device_info()
const char UNIT_WH[] = "Wh";
struct {
	energySensorNames_t names;
	byte rounding_decimals;
	// Variables below are for optimization
	// We can't send a full MQTT update every second.
	// It's too much for Beken, and it's too much for LWIP 2 MQTT library,
	// especially when actively browsing site and using JS app Log Viewer.
	// It even fails to publish with -1 error (can't alloc next packet)
	// So we publish when value changes from certain threshold or when a certain time passes.
	float changeSendThreshold;
	double lastReading; //double only needed for energycounter i.e. OBK_CONSUMPTION_TOTAL to avoid rounding errors as value becomes high
	double lastSentValue; // what are the last values we sent over the MQTT?
	int noChangeFrame; // how much update frames has passed without sending MQTT update of read values?
} sensors[OBK__NUM_SENSORS] = { 
	//.hass_dev_class, 	.units,		.name_friendly,			.name_mqtt,		 .hass_uniq_id_suffix, .rounding_decimals, .changeSendThreshold		
	{{"voltage",		"V",		"Voltage",			"voltage",			"0",		},  	1,			0.25,		},	// OBK_VOLTAGE
	{{"current",		"A",		"Current",			"current",			"1",		},	3,			0.002,		},	// OBK_CURRENT
	{{"power",		"W",		"Power",			"power",			"2",		},	2,			0.25,		},	// OBK_POWER
	{{"apparent_power",	"VA",		"Apparent Power",		"power_apparent",		"9",		},	2,			0.25,		},	// OBK_POWER_APPARENT
	{{"reactive_power",	"var",		"Reactive Power",		"power_reactive",		"10",		},	2,			0.25,		},	// OBK_POWER_REACTIVE
	{{"power_factor",	"",		"Power Factor",			"power_factor",			"11",		},	2,			0.05,		},	// OBK_POWER_FACTOR
	{{"energy",		UNIT_WH,	"Total Consumption",		"energycounter",		"3",		},	3,			0.1,		},	// OBK_CONSUMPTION_TOTAL
	{{"energy",		UNIT_WH,	"Total Generation",		"energycounter_generation",	"14",		},	3,			0.1,		},	// OBK_GENERATION_TOTAL	
	{{"energy",		UNIT_WH,	"Energy Last Hour",		"energycounter_last_hour",	"4",		},	3,			0.1,		},	// OBK_CONSUMPTION_LAST_HOUR
	//{{"",			"",		"Consumption Stats",		"consumption_stats",		"5",		},	0,			0,		},	// OBK_CONSUMPTION_STATS
	{{"energy",		UNIT_WH,	"Energy Today",			"energycounter_today",		"7",		},	3,			0.1,		},	// OBK_CONSUMPTION_TODAY
	{{"energy",		UNIT_WH,	"Energy Yesterday",		"energycounter_yesterday",	"6",		},	3,			0.1,		},	// OBK_CONSUMPTION_YESTERDAY
	{{"energy",		UNIT_WH,	"Energy 2 Days Ago",		"energycounter_2_days_ago",	"12",		},	3,			0.1,		},	// OBK_CONSUMPTION_2_DAYS_AGO
	{{"energy",		UNIT_WH,	"Energy 3 Days Ago",		"energycounter_3_days_ago",	"13",		},	3,			0.1,		},	// OBK_CONSUMPTION_3_DAYS_AGO
	{{"timestamp",		"",		"Energy Clear Date",		"energycounter_clear_date",	"8",		},	0,			86400,		},	// OBK_CONSUMPTION_CLEAR_DATE	
}; 
float lastReadingFrequency = NAN;
portTickType energyCounterStamp;
bool energyCounterStatsEnable = false;
int energyCounterSampleCount = 60;
int energyCounterSampleInterval = 60;
float *energyCounterMinutes = NULL;
portTickType energyCounterMinutesStamp;
long energyCounterMinutesIndex;
bool energyCounterStatsJSONEnable = false;
int actual_mday = -1;
float lastSavedEnergyCounterValue = 0.0f;
float lastSavedGenerationCounterValue = 0.0f;
float changeSavedThresholdEnergy = 100.0f;
long ConsumptionSaveCounter = 0;
portTickType lastConsumptionSaveStamp;
time_t ConsumptionResetTime = 0;
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
    if (!isnan(lastReadingFrequency)) {
        poststr(request,
                "<tr><td><b>Frequency</b></td><td style='text-align: right;'>");
        hprintf255(request, "%.2f</td><td>Hz</td>", lastReadingFrequency);
    }
	for (int i = (OBK__FIRST); i <= (OBK_CONSUMPTION__DAILY_LAST); i++) {
		if (i == OBK_GENERATION_TOTAL && (!CFG_HasFlag(OBK_FLAG_POWER_ALLOW_NEGATIVE))){i++;}
		//if (i == 7){i++;}
		if (i <= OBK__NUM_MEASUREMENTS || NTP_IsTimeSynced()) {
			poststr(request, "<tr><td><b>");
			poststr(request, sensors[i].names.name_friendly);
			poststr(request, "</b></td><td style='text-align: right;'>");
			if ((i == OBK_CONSUMPTION_TOTAL) || (i == OBK_GENERATION_TOTAL))
			{
				hprintf255(request, "%.*f</td><td>kWh</td>", sensors[i].rounding_decimals, (0.001*sensors[i].lastReading));
			}

   			lastsync = check_time;
			//CMD_ExecuteCommand("SendGet http://192.168.8.164/cm?cmnd=Power%20TOGGLE", 0);
			// Are we exporting enough? If so, turn the relay on
			if ((min_production>(sensors[OBK_POWER].lastReading)&&(net_energy>dump_load_on)))
			{
				dump_load_relay = 1;
				CMD_ExecuteCommand("SendGet http://192.168.8.164/cm?cmnd=Power%20on", 0);

#include "../obk_config.h"

#if (ENABLE_DRIVER_DS1820)

#include "drv_ds1820_simple.h"
#include "drv_ds1820_common.h"

static uint8_t dsread = 0;
static int Pin = -1;
static int t = -12700;		//t is 100 * temperature (so we can represent 2 fractional digits) 
static int errcount = 0;
static int lastconv; // secondsElapsed on last successfull reading
static uint8_t ds18_family = 0;
static int ds18_conversionPeriod = 0;

static int DS1820_DiscoverFamily();


#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820[%i] - " fmt, Pin, ##__VA_ARGS__)



int DS1820_getTemp()
{
	return t;
}

static commandResult_t Cmd_SetResolution(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int arg = Tokenizer_GetArgInteger(0);
	if (arg > 12 || arg < 9)
		return CMD_RES_BAD_ARGUMENT;

	if (ds18_family != 0x28) {
		DS1820_LOG(ERROR, "DS1820_SetResolution not supported by sensor");
		return CMD_RES_UNKNOWN_COMMAND;
	}
	// arg can be 9 10 11 12
	// the cfg for resolution shall be 9:0x1F 10:0x3F 11:0x5F 12:0x7F
	// ignoring the "0x0F" an lookig just at the first digit, 
	// we can easily see this is 1+(arg-9)*2
	// so this can be written as 
	uint8_t cfg = 0x1F | (arg-9)<<5 ;

	// use simple temperature setting, always write, ignoring actual scratchpad
	if(OWReset(Pin) == 0)
	{
		DS1820_LOG(ERROR, "WriteScratchpad Reset failed");
		return CMD_RES_ERROR;
	}

	OWWriteByte(Pin, SKIP_ROM);
	OWWriteByte(Pin, WRITE_SCRATCHPAD); //Write Scratchpad command
	OWWriteByte(Pin, 0x7F); //TH
	OWWriteByte(Pin, 0x80); //TL
	OWWriteByte(Pin, cfg);  //CFG

	//temperature conversion was interrupted
	dsread = 0;

	return CMD_RES_OK;
}

// startDriver DS1820 [conversionPeriod (seconds) - default 15]
void DS1820_driver_Init()
{
	ds18_conversionPeriod = Tokenizer_GetArgIntegerDefault(1, 15);
	lastconv = 0;
	dsread = 0;
	ds18_family = 0;

	//cmddetail:{"name":"DS1820_SetResolution","args":"[int]",
	//cmddetail:"descr":"Sets resolution for connected DS1820 sensor (9/10/11/12 bits)",
	//cmddetail:"fn":"Cmd_SetResolution","file":"driver/drv_ds1820_simple.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DS1820_SetResolution", Cmd_SetResolution, NULL);
	//Find PIN and check device so DS1820_SetResolution could be used in autoexec.bat
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, -1);
	if (Pin >= 0)
		DS1820_DiscoverFamily();
		
#if ENABLE_DS1820_TEST_US	
	init_TEST_US();
#endif

};

void DS1820_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState){
		return;
	}
	hprintf255(request, "<h5>DS1820 Temperature: %.2f C (read %i secs ago)</h5>", (float)t / 100, g_secondsElapsed - lastconv);
}

static int DS1820_DiscoverFamily()
{
	if(!OWReset(Pin))
	{
//		DS1820_LOG(DEBUG, "Discover Reset failed");
		return 0;
	}

	// Read ROM
	uint8_t ROM[8];
	OWWriteByte(Pin, READ_ROM);
	for(int i = 0; i < 8; i++)
	{
		ROM[i] = OWReadByte(Pin);
	}

	// Check CRC
	uint8_t crc = Crc8CQuick(ROM, 7);
	if(crc != ROM[7])
	{
		// This might mean bad signal integrity or multiple 1-wire devices on the bus
		DS1820_LOG(DEBUG, "Discover CRC failed (CRC=%x != calculated:%x)", ROM[7], crc);
		return 0;
	}

	// Check family
	uint8_t family = ROM[0];
	if(family == 0x10 || family == 0x28)
	{
		ds18_family = family;
		DS1820_LOG(INFO, "Discoverd Family: %x", family);
		return 1;
	}
	else
	{
		DS1820_LOG(DEBUG, "Discovered Family %x not supported", family);
		return 0;
	}
}



void DS1820_OnEverySecond()
{
	uint8_t scratchpad[9], crc;
	int16_t raw;


	// for now just find the pin used
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, -1);

	// only if pin is set
	if(Pin < 0) {
		DS1820_LOG(INFO, "No Pin found\r\n");
		return;
	}
	//Temperature measurement is done in two repeatable steps. 
	// Step 1 - dsread = 0. Sensor requested to do temperature conversion.
	//          That requires some time - 15-100-750ms, depending on sensor family/vendor.
	//          However, time between steps is always one second.
	// Step 2 - dsread = 1. Sensor finished conversion, requesting conversion result.

	// request temp if conversion was requested two seconds after request
	// if (dsread == 1 && g_secondsElapsed % 5 == 2) {
	// better if we don't use parasitic power, we can check if conversion is ready
	if(dsread == 1)
	{
		if (!DS1820TConversionDone(Pin))
			return;

		if(OWReset(Pin) == 0)
		{
			DS1820_LOG(ERROR, "Read Reset failed");
			return;
		}

		OWWriteByte(Pin, SKIP_ROM);
		OWWriteByte(Pin, READ_SCRATCHPAD);

		for(int i = 0; i < 9; i++)
		{
			scratchpad[i] = OWReadByte(Pin);
		}

		crc = Crc8CQuick(scratchpad, 8);
		if(crc != scratchpad[8])
		{
			DS1820_LOG(ERROR, "Read CRC=%x != calculated:%x (errcount=%i)", scratchpad[8], crc, errcount);
			DS1820_LOG(ERROR, "Scratchpad Data Read: %x %x %x %x %x %x %x %x %x",
				scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3], scratchpad[4],
				scratchpad[5], scratchpad[6], scratchpad[7], scratchpad[8]);

			errcount++;
			if(errcount > 5) dsread = 0; // retry afer 5 failures
			
			return;
		}

		raw = (scratchpad[1] << 8) | scratchpad[0];

		if(ds18_family == 0x10)
		{
			// DS18S20 or old DS1820
			int16_t dT = 128 * (scratchpad[7] - scratchpad[6]);
			dT /= scratchpad[7];
			raw = 64 * (raw & 0xFFFE) - 32 + dT;
			DS1820_LOG(DEBUG, "family=%x, raw=%i, count_remain=%i, count_per_c=%i, dT=%i", ds18_family, raw, scratchpad[6], scratchpad[7], dT);
		}
		else
		{ // DS18B20
			uint8_t cfg = scratchpad[4] & 0x60;
			if(cfg == 0x00) raw = raw & ~7;      // 9 bit resolution, 93.75 ms
			else if(cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
			else if(cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
			raw = raw << 3; // multiply by 8
			DS1820_LOG(DEBUG, "family=%x, raw=%i, cfg=%x (%i bit resolution)", ds18_family, raw, cfg, 9 + (cfg) / 32);
		}

		// Raw is t * 128
/*
		t = (raw / 128) * 100; // Whole degrees
		int frac = (raw % 128) * 100 / 128; // Fractional degrees
		t += t > 0 ? frac : -frac;
*/
		t = (raw * 100) / 128;	// no need to calc fractional, if we can simply get 100 * temp in one line
		dsread = 0;
		lastconv = g_secondsElapsed;
		CHANNEL_Set(g_cfg.pins.channels[Pin], t, CHANNEL_SET_FLAG_SILENT);
		DS1820_LOG(INFO, "Temp=%i.%02i", (int)t / 100, (int)t % 100);
		
		return;
	}
	
	if(g_secondsElapsed % ds18_conversionPeriod == 0 || lastconv == 0) //dsread == 0
	{
		if(ds18_family == 0)
		{
			int discovered = DS1820_DiscoverFamily();
			if(!discovered)
			{
				lastconv = -1; // reset lastconv to avoid immediate retry
				DS1820_LOG(ERROR, "Family not discovered");
				return;
			}
		}
		if(OWReset(Pin) == 0)
		{
			lastconv = -1; // reset lastconv to avoid immediate retry
			DS1820_LOG(ERROR, "Reset failed");
#if DS1820_DEBUG
			// if device is not found, maybe "usleep" is not working as expected
			// lets do usleepds() with numbers 50.000 and 100.00
			// if all is well, it should take 50ms and 100ms
			// if not, we need to "calibrate" the loop
			int tempsleep = 5000;
			portTickType actTick = portTICK_RATE_MS * xTaskGetTickCount();
			usleepds(tempsleep);
			int duration = (int)(portTICK_RATE_MS * xTaskGetTickCount() - actTick);

			DS1820_LOG(DEBUG, "usleepds(%i) took %i ms ", tempsleep, duration);

			tempsleep = 100000;
			actTick = portTICK_RATE_MS * xTaskGetTickCount();
			usleepds(tempsleep);
			duration = (int)(portTICK_RATE_MS * xTaskGetTickCount() - actTick);

			DS1820_LOG(DEBUG, "usleepds(%i) took %i ms ", tempsleep, duration);

			if(duration < 95 || duration > 105)
			{
				// calc a new factor for usleepds
				DS1820_LOG(ERROR, "usleepds duration divergates - proposed factor to adjust usleepds %f ", (float)100 / duration);
			}
#endif	// #if DS1820_DEBUG
			return;
		}

		DS1820_LOG(INFO, "Starting conversion");
		OWWriteByte(Pin, SKIP_ROM);
		OWWriteByte(Pin, CONVERT_T);
		errcount = 0;
		dsread = 1;
	}
}

#endif // to #if (ENABLE_DRIVER_DS1820)

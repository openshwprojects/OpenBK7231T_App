#include "../obk_config.h"
#if (ENABLE_DRIVER_DS1820)

#include "drv_ds1820_simple.h"
#include "../hal/hal_generic.h"
#include "OneWire_common.h"

/*
#if PLATFORM_ESPIDF
#include "freertos/task.h"
#define noInterrupts() portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;taskENTER_CRITICAL(&mux)
#define interrupts() taskEXIT_CRITICAL(&mux)
#else
#include <task.h>
#define noInterrupts() taskENTER_CRITICAL()
#define interrupts() taskEXIT_CRITICAL()
#endif
*/

static uint8_t dsread = 0;
static int Pin;
static int t = -127;
static int errcount = 0;
static int lastconv; // secondsElapsed on last successfull reading
static uint8_t ds18_family = 0;
static int ds18_conversionPeriod = 0;

#define DS1820_LOG(x, fmt, ...) addLogAdv(LOG_##x, LOG_FEATURE_SENSOR, "DS1820[%i] - " fmt, Pin, ##__VA_ARGS__)

int DS1820_getTemp()
{
	return t;
}

// startDriver DS1820 [conversionPeriod (seconds) - default 15]
void DS1820_driver_Init()
{
	ds18_conversionPeriod = Tokenizer_GetArgIntegerDefault(1, 15);
	lastconv = 0;
};

void DS1820_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h5>DS1820 Temperature: %.2f C (read %i secs ago)</h5>", (float)t / 100, g_secondsElapsed - lastconv);
}

int DS1820_DiscoverFamily()
{
	if(!OWReset(Pin))
	{
		DS1820_LOG(DEBUG, "Discover Reset failed");
		return 0;
	}

	// Read ROM
	uint8_t ROM[8];
	OWWriteByte(Pin, 0x33);
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
		DS1820_LOG(INFO, "Discover Family - discovered %x", family);
		return 1;
	}
	else
	{
		DS1820_LOG(DEBUG, "Discover Family %x not supported", family);
		return 0;
	}
}

void DS1820_OnEverySecond()
{
	// for now just find the pin used
	Pin = PIN_FindPinIndexForRole(IOR_DS1820_IO, 99);
	uint8_t scratchpad[9], crc;
	if(Pin != 99)
	{
		// only if pin is set
		// request temp if conversion was requested two seconds after request
		// if (dsread == 1 && g_secondsElapsed % 5 == 2) {
		// better if we don't use parasitic power, we can check if conversion is ready		
		if(dsread == 1 && DS1820TConversionDone(Pin) == 1)
		{
			if(OWReset(Pin) == 0)
			{
				DS1820_LOG(ERROR, "Read Reset failed");
				return;
			}
			OWWriteByte(Pin, 0xCC);
			OWWriteByte(Pin, 0xBE);

			for(int i = 0; i < 9; i++)
			{
				scratchpad[i] = OWReadByte(Pin); //read Scratchpad Memory of DS
			}
			crc = Crc8CQuick(scratchpad, 8);
			if(crc != scratchpad[8])
			{
				errcount++;
				DS1820_LOG(ERROR, "Read CRC=%x != calculated:%x (errcount=%i)", scratchpad[8], crc, errcount);
				DS1820_LOG(ERROR, "Scratchpad Data Read: %x %x %x %x %x %x %x %x %x",
					scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3], scratchpad[4],
					scratchpad[5], scratchpad[6], scratchpad[7], scratchpad[8]);

				if(errcount > 5) dsread = 0; // retry afer 5 failures		
			}
			else
			{
				int16_t raw = (scratchpad[1] << 8) | scratchpad[0];

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
				t = (raw / 128) * 100; // Whole degrees
				int frac = (raw % 128) * 100 / 128; // Fractional degrees
				t += t > 0 ? frac : -frac;

				dsread = 0;
				lastconv = g_secondsElapsed;
				CHANNEL_Set(g_cfg.pins.channels[Pin], t, CHANNEL_SET_FLAG_SILENT);
				DS1820_LOG(INFO, "Temp=%i.%02i", (int)t / 100, (int)t % 100);
			}
		}
		else if(dsread == 0 && (g_secondsElapsed % ds18_conversionPeriod == 0 || lastconv == 0))
		{
			if(OWReset(Pin) == 0)
			{
				lastconv = -1; // reset lastconv to avoid immediate retry
				DS1820_LOG(ERROR, "Reset failed");
#if DS1820_DEBUG
				// if device is not found, maybe "usleep" is not working as expected
				// lets do OWusleep with numbers 50.000 and 100.00
				// if all is well, it should take 50ms and 100ms
				// if not, we need to "calibrate" the loop
				int tempsleep = 5000;
				portTickType actTick = portTICK_RATE_MS * xTaskGetTickCount();
				OWusleep(tempsleep);
				int duration = (int)(portTICK_RATE_MS * xTaskGetTickCount() - actTick);

				DS1820_LOG(DEBUG, "OWusleep(%i) took %i ms ", tempsleep, duration);

				tempsleep = 100000;
				actTick = portTICK_RATE_MS * xTaskGetTickCount();
				OWusleep(tempsleep);
				duration = (int)(portTICK_RATE_MS * xTaskGetTickCount() - actTick);

				DS1820_LOG(DEBUG, "OWusleep(%i) took %i ms ", tempsleep, duration);

				if(duration < 95 || duration > 105)
				{
					// calc a new factor for OWusleep
					DS1820_LOG(ERROR, "OWusleep duration divergates - proposed factor to adjust OWusleep %f ", (float)100 / duration);
				}
#endif
			}
			else
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

				OWWriteByte(Pin, 0xCC);
				OWWriteByte(Pin, 0x44);
				DS1820_LOG(INFO, "Starting conversion");
				dsread = 1;
				errcount = 0;
			}
		}
	}
}
#endif

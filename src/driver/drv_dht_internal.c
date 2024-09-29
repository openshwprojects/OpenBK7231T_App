/*
// drv_dht_internal.c
This is a backend for DHT sensors. This is a ported Adafruit library.
It was translated to C and slightly adjusted for our needs.
The following file only contains the DHT code itself,
our DHT sensors wrapper is in drv_dht.c
*/
/*!
 *  @file DHT.h
 *
 *  This is a library for DHT series of low cost temperature/humidity sensors.
 *
 *  You must have Adafruit Unified Sensor Library library installed to use this
 * class.
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit andopen-source hardware by purchasing products
 *  from Adafruit!
 *
 *  Written by Adafruit Industries.
 *
 *  MIT license, all text above must be included in any redistribution
 */
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
 // Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_dht_internal.h"
#include <math.h>

#define TIMEOUT UINT32_MAX

void usleep2(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#ifdef WIN32
	// not possible on Windows port
#elif PLATFORM_BL602 
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#elif PLATFORM_W600
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#elif PLATFORM_ESPIDF
	usleep(r);
#else
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#endif
}

#define delay(x) usleep2(x*1000);

float DHT_computeHeatIndex(dht_t *dht, bool isFahrenheit);
void DHT_begin(dht_t *dht, uint8_t usec);
float DHT_computeHeatIndexInternal(dht_t *dht, float temperature, float percentHumidity,
	bool isFahrenheit);
bool DHT_read(dht_t *dht, bool force);
uint32_t DHT_expectPulse(dht_t *dht, bool level);


/*!
 *  @brief  Converts Celcius to Fahrenheit
 *  @param  c
 *					value in Celcius
 *	@return float value in Fahrenheit
 */
float convertCtoF(float c) { return c * 1.8 + 32; }

/*!
 *  @brief  Converts Fahrenheit to Celcius
 *  @param  f
 *					value in Fahrenheit
 *	@return float value in Celcius
 */
float convertFtoC(float f) { return (f - 32) * 0.55555; }

dht_t *DHT_Create(byte pin, byte type) {
	dht_t *ret;

	ret = (dht_t*)malloc(sizeof(dht_t));
	if (ret == 0)
		return 0;
	memset(ret, 0, sizeof(dht_t));

	ret->_pin = pin;
	ret->_type = type;
	ret->_maxcycles = 10000;

	DHT_begin(ret, 55);

	return ret;
}
/*!
 *  @brief  Read temperature
 *  @param  S
 *          Scale. Boolean value:
 *					- true = Fahrenheit
 *					- false = Celcius
 *  @param  force
 *          true if in force mode
 *	@return Temperature value in selected scale
 */
float DHT_readTemperature(dht_t *dht, bool S, bool force) {
	float f = 0;
	byte *data;
	data = dht->data;

	if (DHT_read(dht,force)) {
		switch (dht->_type) {
		case DHT11:
			f = data[2];
			if (data[3] & 0x80) {
				f = -1 - f;
			}
			f += (data[3] & 0x0f) * 0.1;
			if (S) {
				f = convertCtoF(f);
			}
			break;
		case DHT12:
			f = data[2];
			f += (data[3] & 0x0f) * 0.1;
			if (data[2] & 0x80) {
				f *= -1;
			}
			if (S) {
				f = convertCtoF(f);
			}
			break;
		case DHT22:
		case DHT21:
			f = ((unsigned short)(data[2] & 0x7F)) << 8 | data[3];
			f *= 0.1;
			if (data[2] & 0x80) {
				f *= -1;
			}
			if (S) {
				f = convertCtoF(f);
			}
			break;
		}
	}
	return f;
}

/*!
 *  @brief  Setup sensor pins and set pull timings
 *  @param  usec
 *          Optionally pass pull-up time (in microseconds) before DHT reading
 *starts. Default is 55 (see function declaration in DHT.h).
 */
void DHT_begin(dht_t *dht, uint8_t usec) {
	// set up the pins!
	HAL_PIN_Setup_Input_Pullup(dht->_pin);
	// Using this value makes sure that millis() - lastreadtime will be
	// >= MIN_INTERVAL right away. Note that this assignment wraps around,
	// but so will the subtraction.
	//_lastreadtime = millis() - MIN_INTERVAL;
	//DEBUG_PRINT("DHT max clock cycles: ");
	//DEBUG_PRINTLN(_maxcycles, DEC);
	dht->pullTime = usec;
}

/*!
 *  @brief  Read Humidity
 *  @param  force
 *					force read mode
 *	@return float value - humidity in percent
 */
float DHT_readHumidity(dht_t *dht, bool force) {
	float f = 0;
	if (DHT_read(dht, force)) {
		switch (dht->_type) {
		case DHT11:
		case DHT12:
			f = dht->data[0] + dht->data[1] * 0.1;
			break;
		case DHT22:
		case DHT21:
			f = ((unsigned short)dht->data[0]) << 8 | dht->data[1];
			f *= 0.1;
			break;
		}
	}
	return f;
}


/*!
 *  @brief  Compute Heat Index
 *          Simplified version that reads temp and humidity from sensor
 *  @param  isFahrenheit
 * 					true if fahrenheit, false if celcius
 *(default true)
 *	@return float heat index
 */
float DHT_computeHeatIndex(dht_t *dht, bool isFahrenheit) {
	float hi = DHT_computeHeatIndexInternal(dht,DHT_readTemperature(dht,isFahrenheit,false), DHT_readHumidity(dht,false),
		isFahrenheit);
	return hi;
}

/*!
 *  @brief  Compute Heat Index
 *  				Using both Rothfusz and Steadman's equations
 *					(http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml)
 *  @param  temperature
 *          temperature in selected scale
 *  @param  percentHumidity
 *          humidity in percent
 *  @param  isFahrenheit
 * 					true if fahrenheit, false if celcius
 *	@return float heat index
 */
float DHT_computeHeatIndexInternal(dht_t *dht, float temperature, float percentHumidity,
	bool isFahrenheit) {
	float hi;

	if (!isFahrenheit)
		temperature = convertCtoF(temperature);

	hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) +
		(percentHumidity * 0.094));

	if (hi > 79) {
		hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity +
			-0.22475541 * temperature * percentHumidity +
			-0.00683783 * pow(temperature, 2) +
			-0.05481717 * pow(percentHumidity, 2) +
			0.00122874 * pow(temperature, 2) * percentHumidity +
			0.00085282 * temperature * pow(percentHumidity, 2) +
			-0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

		if ((percentHumidity < 13) && (temperature >= 80.0) &&
			(temperature <= 112.0))
			hi -= ((13.0 - percentHumidity) * 0.25) *
			sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

		else if ((percentHumidity > 85.0) && (temperature >= 80.0) &&
			(temperature <= 87.0))
			hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
	}

	return isFahrenheit ? hi : convertFtoC(hi);
}
/*!
 *  @brief  Read value from sensor or return last one from less than two
 *seconds.
 *  @param  force
 *          true if using force mode
 *	@return float value
 */	
bool DHT_read(dht_t *dht, bool force) {
	byte *data = dht->data;
	// Check if sensor was read less than two seconds ago and return early
	// to use last reading.
	uint32_t currenttime = g_secondsElapsed;
	if (!force && ((currenttime - dht->_lastreadtime) < 3)) {
		return dht->_lastresult; // return last correct measurement
	}
	dht->_lastreadtime = currenttime;

#if WINDOWS
	bool SIM_ReadDHT11(int pin, byte *data);

	dht->_lastresult = true;

	return SIM_ReadDHT11(dht->_pin, data);
#endif
	// Reset 40 bits of received data to zero.
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;


	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DHT start, pin is %i",(int)dht->_pin);
  // Send start signal.  See DHT datasheet for full signal diagram:
  //   http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

  // Go into high impedence state to let pull-up raise data line level and
	// start the reading process.
	HAL_PIN_Setup_Input_Pullup(dht->_pin);
	delay(1);

	// First set data line low for a period according to sensor type
	HAL_PIN_Setup_Output(dht->_pin);
	HAL_PIN_SetOutputValue(dht->_pin, false);
	switch (dht->_type) {
	case DHT22:
	case DHT21:
		usleep2(1100); // data sheet says "at least 1ms"
		break;
	case DHT11:
	default:
		delay(20); // data sheet says at least 18ms, 20ms just to be safe
		break;
	}

	uint32_t cycles[80];
#ifdef PLATFORM_BEKEN
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
#endif
	{
		// End the start signal by setting data line high for 40 microseconds.
		HAL_PIN_Setup_Input_Pullup(dht->_pin);

		// Delay a moment to let sensor pull data line low.
		usleep2(dht->pullTime);

		// Now start reading the data line to get the value from the DHT sensor.

		// Turn off interrupts temporarily because the next sections
		// are timing critical and we don't want any interruptions.

		// First expect a low signal for ~80 microseconds followed by a high signal
		// for ~80 microseconds again.
		if (DHT_expectPulse(dht,false) == TIMEOUT) {
#ifdef PLATFORM_BEKEN
			GLOBAL_INT_RESTORE();
#endif
			addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DHT timeout waiting for start signal low pulse.");
			dht->_lastresult = false;
			return dht->_lastresult;
		}
		if (DHT_expectPulse(dht, true) == TIMEOUT) {
#ifdef PLATFORM_BEKEN
			GLOBAL_INT_RESTORE();
#endif
			addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DHT timeout waiting for start signal high pulse.");
			dht->_lastresult = false;
			return dht->_lastresult;
		}

		// Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
		// microsecond low pulse followed by a variable length high pulse.  If the
		// high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
		// then it's a 1.  We measure the cycle count of the initial 50us low pulse
		// and use that to compare to the cycle count of the high pulse to determine
		// if the bit is a 0 (high state cycle count < low state cycle count), or a
		// 1 (high state cycle count > low state cycle count). Note that for speed
		// all the pulses are read into a array and then examined in a later step.
		for (int i = 0; i < 80; i += 2) {
			cycles[i] = DHT_expectPulse(dht, false);
			cycles[i + 1] = DHT_expectPulse(dht, true);
		}
	} // Timing critical code is now complete.

#ifdef PLATFORM_BEKEN
	GLOBAL_INT_RESTORE();
#endif
	// Inspect pulses and determine which ones are 0 (high state cycle count < low
	// state cycle count), or 1 (high state cycle count > low state cycle count).
	for (int i = 0; i < 40; ++i) {
		uint32_t lowCycles = cycles[2 * i];
		uint32_t highCycles = cycles[2 * i + 1];
		if ((lowCycles == TIMEOUT) || (highCycles == TIMEOUT)) {
			addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DHT timeout waiting for pulse.");
			dht->_lastresult = false;
			return dht->_lastresult;
		}
		data[i / 8] <<= 1;
		// Now compare the low and high cycle times to see if the bit is a 0 or 1.
		if (highCycles > lowCycles) {
			// High cycles are greater than 50us low cycle count, must be a 1.
			data[i / 8] |= 1;
		}
		// Else high cycles are less than (or equal to, a weird case) the 50us low
		// cycle count so this must be a zero.  Nothing needs to be changed in the
		// stored data.
	}

	//DEBUG_PRINTLN(F("Received from DHT:"));
	//DEBUG_PRINT(data[0], HEX);
	//DEBUG_PRINT(F(", "));
	//DEBUG_PRINT(data[1], HEX);
	//DEBUG_PRINT(F(", "));
	//DEBUG_PRINT(data[2], HEX);
	//DEBUG_PRINT(F(", "));
	//DEBUG_PRINT(data[3], HEX);
	//DEBUG_PRINT(F(", "));
	//DEBUG_PRINT(data[4], HEX);
	//DEBUG_PRINT(F(" =? "));
	//DEBUG_PRINTLN((data[0] + data[1] + data[2] + data[3]) & 0xFF, HEX);

	// Check we read 40 bits and that the checksum matches.
	if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
		dht->_lastresult = true;
		return dht->_lastresult;
	}
	else {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DHT checksum failure!");
		dht->_lastresult = false;
		return dht->_lastresult;
	}
}

// Expect the signal line to be at the specified level for a period of time and
// return a count of loop cycles spent at that level (this cycle count can be
// used to compare the relative time of two pulses).  If more than a millisecond
// ellapses without the level changing then the call fails with a 0 response.
// This is adapted from Arduino's pulseInLong function (which is only available
// in the very latest IDE versions):
//   https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/wiring_pulse.c
uint32_t DHT_expectPulse(dht_t *dht, bool level) {
	uint32_t count = 0;
	while (HAL_PIN_ReadDigitalInput(dht->_pin) == level) {
		if (count++ >= dht->_maxcycles) {
			return TIMEOUT; // Exceeded timeout, fail.
		}
	}
	return count;
}



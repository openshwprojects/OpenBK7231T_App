#ifndef DallasTemperature_h
#define DallasTemperature_h

#define DALLASTEMPLIBVERSION "3.8.1" // To be deprecated -> TODO remove in 4.0.0

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// set to true to include code for new and delete operators
#ifndef REQUIRESNEW
#define REQUIRESNEW false
#endif

// set to true to include code implementing alarm search functions
#ifndef REQUIRESALARMS
#define REQUIRESALARMS true
#endif

#include <inttypes.h>
#ifdef __STM32F1__
#include <OneWireSTM.h>
#else
#include <OneWire.h>
#endif

// Model IDs
#define DS18S20MODEL 0x10  // also DS1820
#define DS18B20MODEL 0x28  // also MAX31820
#define DS1822MODEL  0x22
#define DS1825MODEL  0x3B  // also MAX31850
#define DS28EA00MODEL 0x42

// Error Codes
// See https://github.com/milesburton/Arduino-Temperature-Control-Library/commit/ac1eb7f56e3894e855edc3353be4bde4aa838d41#commitcomment-75490966 for the 16bit implementation. Reverted due to microcontroller resource constraints.
#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_F -196.6
#define DEVICE_DISCONNECTED_RAW -7040

#define DEVICE_FAULT_OPEN_C -254
#define DEVICE_FAULT_OPEN_F -425.199982
#define DEVICE_FAULT_OPEN_RAW -32512

#define DEVICE_FAULT_SHORTGND_C -253
#define DEVICE_FAULT_SHORTGND_F -423.399994
#define DEVICE_FAULT_SHORTGND_RAW -32384

#define DEVICE_FAULT_SHORTVDD_C -252
#define DEVICE_FAULT_SHORTVDD_F -421.599976
#define DEVICE_FAULT_SHORTVDD_RAW -32256

// For readPowerSupply on oneWire bus
// definition of nullptr for C++ < 11, using official workaround:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2431.pdf
#if __cplusplus < 201103L
const class
{
public:
	template <class T>
	operator T *() const {
		return 0;
	}

	template <class C, class T>
	operator T C::*() const {
		return 0;
	}

private:
	void operator&() const;
} nullptr = {};
#endif

typedef uint8_t DeviceAddress[8];

class DallasTemperature {
public:

	DallasTemperature();
	DallasTemperature(OneWire*);
	DallasTemperature(OneWire*, uint8_t);

	void setOneWire(OneWire*);

	void setPullupPin(uint8_t);

	// initialise bus
	void begin(void);

	// returns the number of devices found on the bus
	uint8_t getDeviceCount(void);

	// returns the number of DS18xxx Family devices on bus
	uint8_t getDS18Count(void);

	// returns true if address is valid
	bool validAddress(const uint8_t*);

	// returns true if address is of the family of sensors the lib supports.
	bool validFamily(const uint8_t* deviceAddress);

	// finds an address at a given index on the bus
	bool getAddress(uint8_t*, uint8_t);

	// attempt to determine if the device at the given address is connected to the bus
	bool isConnected(const uint8_t*);

	// attempt to determine if the device at the given address is connected to the bus
	// also allows for updating the read scratchpad
	bool isConnected(const uint8_t*, uint8_t*);

	// read device's scratchpad
	bool readScratchPad(const uint8_t*, uint8_t*);

	// write device's scratchpad
	void writeScratchPad(const uint8_t*, const uint8_t*);

	// read device's power requirements
	bool readPowerSupply(const uint8_t* deviceAddress = nullptr);

	// get global resolution
	uint8_t getResolution();

	// set global resolution to 9, 10, 11, or 12 bits
	void setResolution(uint8_t);

	// returns the device resolution: 9, 10, 11, or 12 bits
	uint8_t getResolution(const uint8_t*);

	// set resolution of a device to 9, 10, 11, or 12 bits
	bool setResolution(const uint8_t*, uint8_t,
	                   bool skipGlobalBitResolutionCalculation = false);

	// sets/gets the waitForConversion flag
	void setWaitForConversion(bool);
	bool getWaitForConversion(void);

	// sets/gets the checkForConversion flag
	void setCheckForConversion(bool);
	bool getCheckForConversion(void);

	struct request_t {
		bool result;
		unsigned long timestamp;

		operator bool() {
			return result;
		}
	};

	// sends command for all devices on the bus to perform a temperature conversion
	request_t requestTemperatures(void);

	// sends command for one device to perform a temperature conversion by address
	request_t requestTemperaturesByAddress(const uint8_t*);

	// sends command for one device to perform a temperature conversion by index
	request_t requestTemperaturesByIndex(uint8_t);

	// returns temperature raw value (12 bit integer of 1/128 degrees C)
	int32_t getTemp(const uint8_t*);

	// returns temperature in degrees C
	float getTempC(const uint8_t*);

	// returns temperature in degrees F
	float getTempF(const uint8_t*);

	// Get temperature for device index (slow)
	float getTempCByIndex(uint8_t);

	// Get temperature for device index (slow)
	float getTempFByIndex(uint8_t);

	// returns true if the bus requires parasite power
	bool isParasitePowerMode(void);

	// Is a conversion complete on the wire? Only applies to the first sensor on the wire.
	bool isConversionComplete(void);

	static uint16_t millisToWaitForConversion(uint8_t);

	uint16_t millisToWaitForConversion();

	// Sends command to one device to save values from scratchpad to EEPROM by index
	// Returns true if no errors were encountered, false indicates failure
	bool saveScratchPadByIndex(uint8_t);

	// Sends command to one or more devices to save values from scratchpad to EEPROM
	// Returns true if no errors were encountered, false indicates failure
	bool saveScratchPad(const uint8_t* = nullptr);

	// Sends command to one device to recall values from EEPROM to scratchpad by index
	// Returns true if no errors were encountered, false indicates failure
	bool recallScratchPadByIndex(uint8_t);

	// Sends command to one or more devices to recall values from EEPROM to scratchpad
	// Returns true if no errors were encountered, false indicates failure
	bool recallScratchPad(const uint8_t* = nullptr);

	// Sets the autoSaveScratchPad flag
	void setAutoSaveScratchPad(bool);

	// Gets the autoSaveScratchPad flag
	bool getAutoSaveScratchPad(void);

#if REQUIRESALARMS

	typedef void AlarmHandler(const uint8_t*);

	// sets the high alarm temperature for a device
	// accepts a int8_t.  valid range is -55C - 125C
	void setHighAlarmTemp(const uint8_t*, int8_t);

	// sets the low alarm temperature for a device
	// accepts a int8_t.  valid range is -55C - 125C
	void setLowAlarmTemp(const uint8_t*, int8_t);

	// returns a int8_t with the current high alarm temperature for a device
	// in the range -55C - 125C
	int8_t getHighAlarmTemp(const uint8_t*);

	// returns a int8_t with the current low alarm temperature for a device
	// in the range -55C - 125C
	int8_t getLowAlarmTemp(const uint8_t*);

	// resets internal variables used for the alarm search
	void resetAlarmSearch(void);

	// search the wire for devices with active alarms
	bool alarmSearch(uint8_t*);

	// returns true if ia specific device has an alarm
	bool hasAlarm(const uint8_t*);

	// returns true if any device is reporting an alarm on the bus
	bool hasAlarm(void);

	// runs the alarm handler for all devices returned by alarmSearch()
	void processAlarms(void);

	// sets the alarm handler
	void setAlarmHandler(const AlarmHandler *);

	// returns true if an AlarmHandler has been set
	bool hasAlarmHandler();

#endif

	// if no alarm handler is used the two bytes can be used as user data
	// example of such usage is an ID.
	// note if device is not connected it will fail writing the data.
	// note if address cannot be found no error will be reported.
	// in short use carefully
	void setUserData(const uint8_t*, int16_t);
	void setUserDataByIndex(uint8_t, int16_t);
	int16_t getUserData(const uint8_t*);
	int16_t getUserDataByIndex(uint8_t);

	// convert from Celsius to Fahrenheit
	static float toFahrenheit(float);

	// convert from Fahrenheit to Celsius
	static float toCelsius(float);

	// convert from raw to Celsius
	static float rawToCelsius(int32_t);

	// convert from Celsius to raw
	static int16_t celsiusToRaw(float);

	// convert from raw to Fahrenheit
	static float rawToFahrenheit(int32_t);

#if REQUIRESNEW

	// initialize memory area
	void* operator new(unsigned int);

	// delete memory reference
	void operator delete(void*);

#endif

	void blockTillConversionComplete(uint8_t);
	void blockTillConversionComplete(uint8_t, unsigned long);
	void blockTillConversionComplete(uint8_t, request_t);

private:
	typedef uint8_t ScratchPad[9];

	// parasite power on or off
	bool parasite;

	// external pullup
	bool useExternalPullup;
	uint8_t pullupPin;

	// used to determine the delay amount needed to allow for the
	// temperature conversion to take place
	uint8_t globalBitResolution;

	// used to requestTemperature with or without delay
	bool waitForConversion;

	// used to requestTemperature to dynamically check if a conversion is complete
	bool checkForConversion;

	// used to determine if values will be saved from scratchpad to EEPROM on every scratchpad write
	bool autoSaveScratchPad;

	// count of devices on the bus
	uint8_t devices;

	// count of DS18xxx Family devices on bus
	uint8_t ds18Count;

	// Take a pointer to one wire instance
	OneWire* _wire;

	// reads scratchpad and returns the raw temperature
	int32_t calculateTemperature(const uint8_t*, uint8_t*);


	// Returns true if all bytes of scratchPad are '\0'
	bool isAllZeros(const uint8_t* const scratchPad, const size_t length = 9);

	// External pullup control
	void activateExternalPullup(void);
	void deactivateExternalPullup(void);

#if REQUIRESALARMS

	// required for alarmSearch
	uint8_t alarmSearchAddress[8];
	int8_t alarmSearchJunction;
	uint8_t alarmSearchExhausted;

	// the alarm handler function pointer
	AlarmHandler *_AlarmHandler;

#endif

};
#endif

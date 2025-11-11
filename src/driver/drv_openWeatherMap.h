#ifndef __DRV_OPENWEATHERMAP_H__
#define __DRV_OPENWEATHERMAP_H__

typedef struct weatherData_s {
	double lon;
	double lat;
	char main_weather[32];
	char description[32];
	double temp;
	int pressure;
	int humidity;
	int timezone;
	int sunrise;
	int sunset;
} weatherData_t;

typedef struct weatherChannels_s {
	byte bInitialized;
	char temperature;
	char humidity;
	char pressure;
} weatherChannels_t;

weatherData_t *Weather_GetData();

#endif // __DRV_OPENWEATHERMAP_H__

//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

//Start with all driver flags undefined

//ENABLE_NTP - Enable support for Test drivers, NTP and HttpButton
//ENABLE_DRIVER_LED - Enable support for all LED drivers
//ENABLE_I2C - Enable support for I2C
//ENABLE_DRIVER_BL0937 - Enable support for BL0937
//ENABLE_DRIVER_BL0942 - Enable support for BL0942
//ENABLE_DRIVER_CSE7766 - Enable support for CSE7766
//ENABLE_DRIVER_TUYAMCU - Enable support for TuyaMCU and tmSensor


#if PLATFORM_XR809

#define OBK_DISABLE_ALL_DRIVERS       1

#elif PLATFORM_W600

// Some limited drivers are supported on W600, OBK_DISABLE_ALL_DRIVERS is not defined
#define ENABLE_NTP				1
#define ENABLE_DRIVER_BL0937    1

#elif PLATFORM_W800

#define OBK_DISABLE_ALL_DRIVERS 1

#elif WINDOWS

#define ENABLE_LITTLEFS			1
#define ENABLE_NTP				1
#define ENABLE_DRIVER_LED       1
#define ENABLE_DRIVER_BL0937    1
#define ENABLE_DRIVER_BL0942    1
#define ENABLE_DRIVER_BL0942SPI 1
#define ENABLE_DRIVER_CSE7766   1
#define ENABLE_DRIVER_HT16K33   1
#define ENABLE_DRIVER_MAX72XX	1
#define ENABLE_DRIVER_TUYAMCU   1
#define ENABLE_TEST_COMMANDS	1
#define ENABLE_CALENDAR_EVENTS	1
#define ENABLE_TEST_DRIVERS		1
#define ENABLE_DRIVER_BRIDGE	1
#define ENABLE_HTTPBUTTONS		1
#define ENABLE_ADVANCED_CHANNELTYPES_DISCOVERY 1
#define ENABLE_DRIVER_WEMO		1
#define ENABLE_DRIVER_HUE		1
#define ENABLE_DRIVER_CHARGINGLIMIT		1
#define ENABLE_DRIVER_BATTERY	1

#elif PLATFORM_BL602

// I have enabled drivers on BL602
#define ENABLE_NTP    1
#define ENABLE_DRIVER_LED       1
#define ENABLE_DRIVER_BL0937    1
#define ENABLE_DRIVER_BL0942    1
#define ENABLE_DRIVER_CSE7766   1
#define ENABLE_DRIVER_WEMO		1

#elif PLATFORM_BEKEN

#define ENABLE_LITTLEFS			1
#define ENABLE_NTP    1
#define ENABLE_DRIVER_LED       1
#define ENABLE_DRIVER_BL0937    1
#define ENABLE_DRIVER_BL0942    1
#define ENABLE_DRIVER_BL0942SPI 1
#define ENABLE_DRIVER_CSE7766   1
#define ENABLE_DRIVER_TUYAMCU   1
//#define ENABLE_DRIVER_HT16K33   1
//#define ENABLE_DRIVER_MAX72XX	  1
#define ENABLE_I2C			    1
#define ENABLE_TEST_COMMANDS	1
#define ENABLE_CALENDAR_EVENTS	1
#define ENABLE_DRIVER_BRIDGE	1
#define ENABLE_HTTPBUTTONS		1
#define ENABLE_ADVANCED_CHANNELTYPES_DISCOVERY 1
#define ENABLE_DRIVER_WEMO		1
#define ENABLE_DRIVER_HUE		1
#define ENABLE_DRIVER_CHARGINGLIMIT		1
#define ENABLE_DRIVER_BATTERY	1

#else

#error "Platform not defined"

#endif



// closing OBK_CONFIG_H
#endif

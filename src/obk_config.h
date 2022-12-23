//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

//Start with all driver flags undefined

//ENABLE_BASIC_DRIVERS - Enable support for Test drivers, NTP and HttpButton
//ENABLE_DRIVER_LED - Enable support for all LED drivers
//ENABLE_I2C - Enable support for I2C
//ENABLE_DRIVER_BL0937 - Enable support for BL0937
//ENABLE_DRIVER_BL0942 - Enable support for BL0942
//ENABLE_DRIVER_CSE7766 - Enable support for CSE7766
//ENABLE_DRIVER_TUYAMCU - Enable support for TuyaMCU and tmSensor


#if PLATFORM_XR809
	///#define DEBUG_USE_SIMPLE_LOGGER
#define OBK_DISABLE_ALL_DRIVERS       1

#elif PLATFORM_W600

//Some limited drivers are supported on W600, OBK_DISABLE_ALL_DRIVERS is not defined
#define ENABLE_BASIC_DRIVERS    1
#define ENABLE_DRIVER_BL0937    1

#elif PLATFORM_W800

#define OBK_DISABLE_ALL_DRIVERS      1

#elif WINDOWS

// comment out to remove littlefs
#define BK_LITTLEFS

#define ENABLE_BASIC_DRIVERS    1
#define ENABLE_DRIVER_LED       1
#define ENABLE_DRIVER_BL0937    1
#define ENABLE_DRIVER_BL0942    1
#define ENABLE_DRIVER_CSE7766   1
#define ENABLE_DRIVER_TUYAMCU   1


#elif PLATFORM_BL602

// I have enabled drivers on BL602
#define ENABLE_BASIC_DRIVERS    1
#define ENABLE_DRIVER_LED       1
#define ENABLE_DRIVER_BL0937    1
#define ENABLE_DRIVER_BL0942    1
#define ENABLE_DRIVER_CSE7766   1
#define ENABLE_DRIVER_TUYAMCU   1

#else

	// comment out to remove component
#undef OBK_DISABLE_ALL_DRIVERS
// comment out to remove littlefs
#define BK_LITTLEFS

// add further app wide defined here, and used them to control build inclusion.

#define ENABLE_BASIC_DRIVERS    1
#define ENABLE_DRIVER_LED       1
#define ENABLE_DRIVER_BL0937    1
#define ENABLE_DRIVER_BL0942    1
#define ENABLE_DRIVER_CSE7766   1
#define ENABLE_DRIVER_TUYAMCU   1


#endif

#endif

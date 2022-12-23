//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

//Start with all driver flags undefined

#undef BASIC_DRIVER_SUPPORT    //This includes Test drivers, NTP and HttpButton
#undef LED_DRIVER_SUPPORT      //This includes all LED driver variants
#undef I2C_SUPPORT
#undef POWER_BL0937_SUPPORT
#undef POWER_BL0942_SUPPORT
#undef POWER_CSE7766_SUPPORT
#undef TUYA_SUPPORT            //This includes TuyaMCU and tmSensor


#if PLATFORM_XR809
	///#define DEBUG_USE_SIMPLE_LOGGER
#define OBK_DISABLE_ALL_DRIVERS       1

#elif PLATFORM_W600

//Some limited drivers are supported on W600, OBK_DISABLE_ALL_DRIVERS is not defined
#define BASIC_DRIVER_SUPPORT    1
#define POWER_BL0937_SUPPORT    1

#elif PLATFORM_W800

#define OBK_DISABLE_ALL_DRIVERS      1

#elif WINDOWS

// comment out to remove littlefs
#define BK_LITTLEFS

#define BASIC_DRIVER_SUPPORT    1
#define LED_DRIVER_SUPPORT      1
#define POWER_BL0937_SUPPORT    1
#define POWER_BL0942_SUPPORT    1
#define POWER_CSE7766_SUPPORT   1
#define TUYA_SUPPORT            1


#elif PLATFORM_BL602

// I have enabled drivers on BL602
#define BASIC_DRIVER_SUPPORT    1
#define LED_DRIVER_SUPPORT      1
#define POWER_BL0937_SUPPORT    1
#define POWER_BL0942_SUPPORT    1
#define POWER_CSE7766_SUPPORT   1
#define TUYA_SUPPORT            1

#else

	// comment out to remove component
#undef OBK_DISABLE_ALL_DRIVERS
// comment out to remove littlefs
#define BK_LITTLEFS

// add further app wide defined here, and used them to control build inclusion.

#define BASIC_DRIVER_SUPPORT    1
#define LED_DRIVER_SUPPORT      1
#define POWER_BL0937_SUPPORT    1
#define POWER_BL0942_SUPPORT    1
#define POWER_CSE7766_SUPPORT   1
#define TUYA_SUPPORT            1


#endif

#endif

//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

//Discard any previous declarations
#undef DRIVER_ENABLE_TUYAMCU


#if PLATFORM_XR809
	///#define DEBUG_USE_SIMPLE_LOGGER
#define OBK_DISABLE_ALL_DRIVERS 1

#elif PLATFORM_W600

//Some basic drivers are allowed on W600, OBK_DISABLE_ALL_DRIVERS is not defined

#elif PLATFORM_W800

#define OBK_DISABLE_ALL_DRIVERS 1

#elif WINDOWS

// comment out to remove littlefs
#define BK_LITTLEFS

#elif PLATFORM_BL602

// I have enabled drivers on BL602


#else

	// comment out to remove component
#undef OBK_DISABLE_ALL_DRIVERS
// comment out to remove littlefs
#define BK_LITTLEFS

#define DRIVER_ENABLE_TUYAMCU	1
#define DRIVER_DEVICE_GROUPS_ENABLED	1

// add further app wide defined here, and used them to control build inclusion.

#endif

#endif

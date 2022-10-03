//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

	#if PLATFORM_XR809
		///#define DEBUG_USE_SIMPLE_LOGGER
		#define OBK_DISABLE_ALL_DRIVERS 1

	#elif PLATFORM_W600 || PLATFORM_W800

		#define OBK_DISABLE_ALL_DRIVERS 1
		
	#elif WINDOWS

	#elif PLATFORM_BL602
		///#define DEBUG_USE_SIMPLE_LOGGER
		#define OBK_DISABLE_ALL_DRIVERS 1


	#else

		// comment out to remove component
        #undef OBK_DISABLE_ALL_DRIVERS
		// comment out to remove littlefs
		#define BK_LITTLEFS

		// add further app wide defined here, and used them to control build inclusion.

	#endif
#endif

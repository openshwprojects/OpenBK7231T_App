//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

#if PLATFORM_XR809


#else

// comment out to remove component

// comment out to remove littlefs
#define BK_LITTLEFS

// add further app wide defined here, and used them to control build inclusion.

#endif


#endif
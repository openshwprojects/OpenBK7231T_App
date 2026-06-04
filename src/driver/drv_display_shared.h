#ifndef __DRV_DISPLAY_SHARED_H__
#define __DRV_DISPLAY_SHARED_H__

#ifdef __cplusplus
extern "C" {
#endif

bool Display_PreInit();
void Display_PostInit();
void Display_Shutdown();
extern bool g_display_ready;

#ifdef __cplusplus
}
#endif

#endif // __DRV_DISPLAY_SHARED_H__

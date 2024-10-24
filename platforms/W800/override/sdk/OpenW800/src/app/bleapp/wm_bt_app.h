#ifndef _WM_BT_APP_H
#define _WM_BT_APP_H

/*****************************************************************************
**
**  Name:           wm_bt_app.h
**
**  Description:    This file contains the sample functions for bluetooth application
**
*****************************************************************************/
#include "wm_bt.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    WM_BT_SYSTEM_ACTION_IDLE,
    WM_BT_SYSTEM_ACTION_ENABLING,
    WM_BT_SYSTEM_ACTION_DISABLING
} bt_system_action_t;

extern volatile tls_bt_state_t bt_adapter_state;
extern volatile bt_system_action_t bt_system_action;

#define CHECK_SYSTEM_READY()    \
    do{                         \
        if(bt_adapter_state == WM_BT_STATE_OFF || bt_system_action != WM_BT_SYSTEM_ACTION_IDLE) \
        {                                                                                       \
            TLS_BT_APPL_TRACE_ERROR("%s failed rc=%s\r\n", __FUNCTION__, tls_bt_rc_2_str(BLE_HS_EDISABLED));    \
            return BLE_HS_EDISABLED;                                                            \
        }                                                                                       \
      }while(0)

#ifdef __cplusplus
        }
#endif

        

#endif

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

extern void wm_bt_init_report_list();
extern tls_bt_status_t wm_bt_register_report_evt(tls_bt_host_evt_t rpt_evt,
        tls_bt_host_callback_t rpt_callback);
extern tls_bt_status_t wm_bt_deregister_report_evt(tls_bt_host_evt_t rpt_evt,
        tls_bt_host_callback_t rpt_callback);


#endif

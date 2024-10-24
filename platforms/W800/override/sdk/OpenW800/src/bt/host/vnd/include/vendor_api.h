/****************************************************************************
**
** Name:         vendor_api.h
**
** Description:  Vendor specific BTE API function external definitions.
**
** Copyright (c) 2009-2011, BROADCOM Inc., All Rights Reserved.
** Broadcom Bluetooth Core. Proprietary and confidential.
**
******************************************************************************/
#ifndef VENDOR_API_H
#define VENDOR_API_H

#include "bt_types.h"
#include "btm_api.h"

/****************************************************************************
**  Resolvable private address offload VSC specific definitions
******************************************************************************/

enum {
    BTM_BLE_PRIVACY_ENABLE,
    BTM_BLE_PRIVACY_DISABLE
};


/****************************************************************************
**  Advertising packet filter VSC specific definitions
******************************************************************************/



#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
**              VENDOR SPECIFIC BLE FEATURE FUNCTIONS
******************************************************************************/
#if BLE_ANDROID_CONTROLLER_SCAN_FILTER == TRUE

#endif

#ifdef __cplusplus
}
#endif

#endif

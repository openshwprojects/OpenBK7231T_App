/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2012 The CyanogenMod Project <http://www.cyanogenmod.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __BDROID_BUILDCFG_H__
#define __BDROID_BUILDCFG_H__

#include "wm_bt_config.h"

#ifndef OS_MEMCPY
#define wm_memcpy(dst, src, len)     memcpy(dst, src, len)
#endif

#ifndef OS_MEMSET
#define wm_memset(s, c, n)          memset(s, c, n)
#endif

#define BTM_DEF_LOCAL_NAME        "WM"
#define BTA_DM_COD {0x20, 0x04, 0x14} // Loudspeaker
//#define BTA_DM_COD {0x04, 0x06, 0x80} //Printer

#define BTM_LOCAL_IO_CAPS         BTM_IO_CAP_OUT

#define BTA_EIR_CANNED_UUID_LIST TRUE


#ifdef SIZE_MAX
#undef SIZE_MAX
#define SIZE_MAX 16
#else
#define SIZE_MAX 16
#endif

#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON)
#define BTA_INCLUDED            TRUE
#else
#define BTA_INCLUDED            FALSE
#endif

#if (WM_BT_INCLUDED == CFG_ON)
#define SDP_INCLUDED            TRUE
#else
#define SDP_INCLUDED            FALSE
#endif

#if (WM_BLE_INCLUDED == CFG_ON)
#define BLE_INCLUDED              TRUE
#define BTA_GATT_INCLUDED         TRUE
#else
#define BLE_INCLUDED              FALSE
#define BTA_GATT_INCLUDED         FALSE
#endif

#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)
#define BTA_AV_SINK_INCLUDED        TRUE
#define BTA_AR_INCLUDED             TRUE
#define BTA_AV_INCLUDED             TRUE
#define AVRC_METADATA_INCLUDED      TRUE
#define AVRC_ADV_CTRL_INCLUDED      TRUE
#define AVRC_CTLR_INCLUDED          TRUE
#else
#define BTA_AV_SINK_INCLUDED        FALSE
#define BTA_AR_INCLUDED             FALSE
#define BTA_AV_INCLUDED             FALSE
#define AVRC_METADATA_INCLUDED      FALSE
#define AVRC_ADV_CTRL_INCLUDED      FALSE
#define AVRC_CTLR_INCLUDED          FALSE
#endif

#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)
#define BTA_HFP_HSP_INCLUDED    TRUE
#define BTM_SCO_INCLUDED        TRUE
#define BTM_SCO_HCI_INCLUDED    TRUE   /* TRUE includes SCO over HCI code */
#else
#define BTA_HFP_HSP_INCLUDED    FALSE
#define BTM_SCO_INCLUDED        FALSE
#define BTM_SCO_HCI_INCLUDED    FALSE
#endif


#if (WM_BTA_SPPS_INCLUDED == CFG_ON || WM_BTA_SPPC_INCLUDED == CFG_ON)
#define BTA_SPP_INCLUDED        TRUE
#define BTA_JV_INCLUDED         TRUE
#else
#define BTA_SPP_INCLUDED       FALSE
#define BTA_JV_INCLUDED        FALSE
#endif

#if (WM_BTA_SPPC_INCLUDED == CFG_ON)
#define SDP_CLIENT_ENABLED        TRUE
#else
#define SDP_CLIENT_ENABLED        FALSE
#endif


/*user application will control advertisement*/
#define ENABLE_ADV_AUTO_MODE 0

/*It means only ble bonding keys are saved for master role; otherwise all properties will be written into nvram*/
#define NO_NVRAM_FOR_BLE_PROP

#define GKI_USE_DYNAMIC_BUFFERS     FALSE
#define L2C_DYNAMIC_MEMORY        TRUE
#define SDP_DYNAMIC_MEMORY        TRUE
#define BTM_DYNAMIC_MEMORY        TRUE
#define GATT_DYNAMIC_MEMORY       TRUE
#define BTA_DYNAMIC_MEMORY        TRUE

/******************************************/

#define SRVC_INCLUDED             FALSE
#define BTA_GATTC_CO_CACHE        FALSE
#define BTA_HH_LE_INCLUDED        FALSE
#define HL_INCLUDED               FALSE
#define ANDROID_APP_INCLUDED      FALSE
#define BTA_PAN_INCLUDED          FALSE
#define BTA_FS_INCLUDED           FALSE
#define BTA_HH_INCLUDED           FALSE

#define GATT_MAX_PHY_CHANNEL      9               /*support max remote devices connections*/
#define SDP_MAX_DISC_SERVER_RECS  9
#define GAP_MAX_CONNECTIONS       9


#define BTM_BLE_GAP_DISC_SCAN_INT      64         /* Interval(scan_int) = 11.25 ms= 0x0010 * 0.625 ms */
#define BTM_BLE_GAP_DISC_SCAN_WIN      64         /* scan_window = 11.25 ms= 0x0010 * 0.625 ms */



#define BTM_BLE_SCAN_FAST_INT     0x60
#define BTM_BLE_SCAN_FAST_WIN     0x20
#define BTM_BLE_CONN_INT_MIN_DEF  20
#define BTM_BLE_CONN_INT_MAX_DEF  20
#define BTM_BLE_CONN_TIMEOUT_DEF  500
#define GATT_DATA_BUF_SIZE        660

#define SDP_MAX_CONNECTIONS       3
#define SDP_MAX_RECORDS           8

#define MAX_RFC_PORTS             5
#define MAX_ACL_CONNECTIONS       9
#define MAX_L2CAP_CHANNELS        9
#define MAX_L2CAP_CLIENTS         6         /*sdp, rfcomm,*/
#define BTM_SEC_MAX_SERVICE_RECORDS 9
#define BTA_DM_SDP_DB_SIZE        2048
#define SDP_MAX_LIST_BYTE_COUNT   2048
#define BT_DEFAULT_BUFFER_SIZE   (2048 + 16)


#if ((WM_BLE_PERIPHERAL_INCLUDED == CFG_ON)&&(WM_BLE_CENTRAL_INCLUDED == CFG_ON))
#define GATT_MAX_SR_PROFILES      8
#define GATT_MAX_APPS             9   /* MAX is 32 note: 2 apps used internally GATT and GAP */
#define GATT_MAX_SERVER_APPS      2
#define GATT_MAX_CLIENT_APPS      (GATT_MAX_APPS - GATT_MAX_SERVER_APPS -2)
#define GATT_MAX_BG_CONN_DEV      2

#define BTM_SEC_MAX_DEVICE_RECORDS 9
#define BTA_GATTC_CL_MAX          8
#define BTA_GATTC_KNOWN_SR_MAX    8

#define GATT_CL_MAX_LCB           8
#define BLE_MAX_L2CAP_CLIENTS     8

#else

#if (WM_BLE_CENTRAL_INCLUDED == CFG_ON)
#define GATT_MAX_SR_PROFILES      0
#define GATT_MAX_APPS             9   /* MAX is 32 note: 2 apps used internally GATT and GAP */
#define GATT_MAX_SERVER_APPS      0
#define GATT_MAX_CLIENT_APPS      (GATT_MAX_APPS - GATT_MAX_SERVER_APPS -2)
#define GATT_MAX_BG_CONN_DEV      2

#define BTM_SEC_MAX_DEVICE_RECORDS 9
#define BTA_GATTC_CL_MAX          8
#define BTA_GATTC_KNOWN_SR_MAX    8

#define GATT_CL_MAX_LCB           8
#define BLE_MAX_L2CAP_CLIENTS     8

#else
#define GATT_MAX_SR_PROFILES      8
#define GATT_MAX_APPS             8   /* MAX is 32 note: 2 apps used internally GATT and GAP */
#define GATT_MAX_SERVER_APPS      8
#define GATT_MAX_CLIENT_APPS      (GATT_MAX_APPS - GATT_MAX_SERVER_APPS -2)
#define GATT_MAX_BG_CONN_DEV      1

#define BTM_SEC_MAX_DEVICE_RECORDS 2
#define BTA_GATTC_CL_MAX          1
#define BTA_GATTC_KNOWN_SR_MAX    1

#define GATT_CL_MAX_LCB           1
#define BLE_MAX_L2CAP_CLIENTS     1

#endif

#endif

#define BT_USE_TRACES             FALSE
#define BT_TRACE_STORAGE          FALSE
#define BT_TRACE_BTIF             FALSE
#define BT_TRACE_BTM              FALSE

#define BT_TRACE_L2CAP            FALSE
#define BT_TRACE_AVDT             FALSE
#define BTM_INITIAL_TRACE_LEVEL  BT_TRACE_LEVEL_DEBUG
//#define BT_TRACE_VERBOSE  TRUE


#define HCI_DBG                   FALSE
#define BTHC_DBG                  FALSE
#define BTVND_DBG                 FALSE
#define USERIAL_DBG               FALSE

#define BTM_OOB_INCLUDED        FALSE

#define GKI_DEBUG                FALSE
#define BT_TRACE_GKI             FALSE
#define BT_TRACE_GATT            FALSE


#define GATT_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_VERBOSE
#define HCI_USE_VARIABLE_SIZE_CMD_BUF TRUE

/***************SMP DEBUG**********************/
#define BT_TRACE_SMP             FALSE
#define SMP_DEBUG_WM         0
#define SMP_ACTION_MSG_DEBUG 0
#define SMP_OPCODE_DEBUG     0
#define DEBUG_SMP_FUNCTION   0


/***********************************************/


/****************************************/

#define BTIF_DM_OOB_TEST            FALSE
#define BTM_PERIODIC_INQ_INCLUDED   FALSE
#define PAN_INCLUDED                FALSE
#define BNEP_INCLUDED               FALSE
#define FTP_CLIENT_INCLUDED         FALSE
#define FTP_SERVER_INCLUDED         FALSE
#define HID_HOST_INCLUDED           FALSE
#define BNEP_INCLUDED               FALSE
#define MCA_INCLUDED                FALSE

/********************************************/
#define OBX_INCLUDED                FALSE

#if defined(OBX_INCLUDED) && (OBX_INCLUDED == FALSE)
#define OBX_NUM_SERVERS 0
#define OBX_NUM_CLIENTS 0
#endif



#endif

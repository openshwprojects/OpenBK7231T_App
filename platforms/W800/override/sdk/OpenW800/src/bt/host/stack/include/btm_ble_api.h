/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains the Bluetooth Manager (BTM) API function external
 *  definitions.
 *
 ******************************************************************************/
#ifndef BTM_BLE_API_H
#define BTM_BLE_API_H

#include "btm_api.h"
#include "bt_common.h"
#include "gki.h"
#include "bt_common_types.h"

#define CHNL_MAP_LEN    5
typedef uint8_t tBTM_BLE_CHNL_MAP[CHNL_MAP_LEN];

/* 0x00-0x04 only used for set advertising parameter command */
#define BTM_BLE_CONNECT_EVT     0x00   /* 0x00-0x04 only used for set advertising
                                            parameter command */
#define BTM_BLE_CONNECT_DIR_EVT 0x01   /* Connectable directed advertising */
#define BTM_BLE_DISCOVER_EVT    0x02  /* Scannable undirected advertising */
#define BTM_BLE_NON_CONNECT_EVT 0x03  /* Non connectable undirected advertising */
#define BTM_BLE_CONNECT_LO_DUTY_DIR_EVT 0x04        /* Connectable low duty
                                                       cycle directed advertising  */
/* 0x00 - 0x05 can be received on adv event type */
#define BTM_BLE_SCAN_RSP_EVT    0x04
#define BTM_BLE_SCAN_REQ_EVT    0x05
#define BTM_BLE_UNKNOWN_EVT     0xff

#define BTM_BLE_UNKNOWN_EVT     0xff

typedef uint8_t tBTM_BLE_EVT;
typedef uint8_t tBTM_BLE_CONN_MODE;

typedef uint32_t tBTM_BLE_REF_VALUE;

#define BTM_BLE_SCAN_MODE_PASS      0
#define BTM_BLE_SCAN_MODE_ACTI      1
#define BTM_BLE_SCAN_MODE_NONE      0xff
typedef uint8_t tBLE_SCAN_MODE;

#define BTM_BLE_BATCH_SCAN_MODE_DISABLE 0
#define BTM_BLE_BATCH_SCAN_MODE_PASS  1
#define BTM_BLE_BATCH_SCAN_MODE_ACTI  2
#define BTM_BLE_BATCH_SCAN_MODE_PASS_ACTI 3

typedef uint8_t tBTM_BLE_BATCH_SCAN_MODE;

/* advertising channel map */
#define BTM_BLE_ADV_CHNL_37    (0x01 << 0)
#define BTM_BLE_ADV_CHNL_38    (0x01 << 1)
#define BTM_BLE_ADV_CHNL_39    (0x01 << 2)
typedef uint8_t tBTM_BLE_ADV_CHNL_MAP;

/*d efault advertising channel map */
#ifndef BTM_BLE_DEFAULT_ADV_CHNL_MAP
#define BTM_BLE_DEFAULT_ADV_CHNL_MAP   (BTM_BLE_ADV_CHNL_37| BTM_BLE_ADV_CHNL_38| BTM_BLE_ADV_CHNL_39)
#endif

/* advertising filter policy */
#define AP_SCAN_CONN_ALL           0x00        /* default */
#define AP_SCAN_WL_CONN_ALL        0x01
#define AP_SCAN_ALL_CONN_WL        0x02
#define AP_SCAN_CONN_WL            0x03
#define AP_SCAN_CONN_POLICY_MAX    0x04
typedef uint8_t   tBTM_BLE_AFP;

/* default advertising filter policy */
#ifndef BTM_BLE_DEFAULT_AFP
#define BTM_BLE_DEFAULT_AFP   AP_SCAN_CONN_ALL
#endif

/* scanning filter policy */
#define SP_ADV_ALL     0x00     /* 0: accept adv packet from all, directed adv pkt not directed */
/* to local device is ignored */
#define SP_ADV_WL      0x01     /* 1: accept adv packet from device in white list, directed adv */
/* packet not directed to local device is ignored */
#define SP_ADV_ALL_RPA_DIR_ADV 0x02  /* 2: accept adv packet from all, directed adv pkt */
/* not directed to me is ignored except direct adv with RPA */
#define SP_ADV_WL_RPA_DIR_ADV  0x03  /* 3: accept adv packet from device in white list, directed */
/* adv pkt not directed to me is ignored except direct adv */
/* with RPA */
typedef uint8_t   tBTM_BLE_SFP;

#ifndef BTM_BLE_DEFAULT_SFP
#define BTM_BLE_DEFAULT_SFP   SP_ADV_ALL
#endif

/* adv parameter boundary values */
#define BTM_BLE_ADV_INT_MIN            0x0020
#define BTM_BLE_ADV_INT_MAX            0x4000

/* Full scan boundary values */
#define BTM_BLE_ADV_SCAN_FULL_MIN      0x00
#define BTM_BLE_ADV_SCAN_FULL_MAX      0x64

/* Partial scan boundary values */
#define BTM_BLE_ADV_SCAN_TRUNC_MIN      BTM_BLE_ADV_SCAN_FULL_MIN
#define BTM_BLE_ADV_SCAN_TRUNC_MAX      BTM_BLE_ADV_SCAN_FULL_MAX

/* Threshold values */
#define BTM_BLE_ADV_SCAN_THR_MIN        BTM_BLE_ADV_SCAN_FULL_MIN
#define BTM_BLE_ADV_SCAN_THR_MAX        BTM_BLE_ADV_SCAN_FULL_MAX

/* connection parameter boundary values */
#define BTM_BLE_SCAN_INT_MIN            0x0004
#define BTM_BLE_SCAN_INT_MAX            0x4000
#define BTM_BLE_SCAN_WIN_MIN            0x0004
#define BTM_BLE_SCAN_WIN_MAX            0x4000
#define BTM_BLE_EXT_SCAN_INT_MAX        0x00FFFFFF
#define BTM_BLE_EXT_SCAN_WIN_MAX        0xFFFF
#define BTM_BLE_CONN_INT_MIN            0x0006
#define BTM_BLE_CONN_INT_MAX            0x0C80
#define BTM_BLE_CONN_LATENCY_MAX        500
#define BTM_BLE_CONN_SUP_TOUT_MIN       0x000A
#define BTM_BLE_CONN_SUP_TOUT_MAX       0x0C80
#define BTM_BLE_CONN_EVT_MAX            512
#define BTM_BLE_CONN_EVT_MIN            0
#define BTM_BLE_CONN_PARAM_UNDEF        0xffff      /* use this value when a specific value not to be overwritten */
#define BTM_BLE_SCAN_PARAM_UNDEF        0xffffffff

/* default connection parameters if not configured, use GAP recommend value for auto/selective connection */
/* default scan interval */
#ifndef BTM_BLE_SCAN_FAST_INT
#define BTM_BLE_SCAN_FAST_INT    96    /* 30 ~ 60 ms (use 60)  = 96 *0.625 */
#endif
/* default scan window for background connection, applicable for auto connection or selective conenction */
#ifndef BTM_BLE_SCAN_FAST_WIN
#define BTM_BLE_SCAN_FAST_WIN   48      /* 30 ms = 48 *0.625 */
#endif

/* default scan paramter used in reduced power cycle (background scanning) */
#ifndef BTM_BLE_SCAN_SLOW_INT_1
#define BTM_BLE_SCAN_SLOW_INT_1    2048    /* 1.28 s   = 2048 *0.625 */
#endif
#ifndef BTM_BLE_SCAN_SLOW_WIN_1
#define BTM_BLE_SCAN_SLOW_WIN_1   48      /* 30 ms = 48 *0.625 */
#endif

/* default scan paramter used in reduced power cycle (background scanning) */
#ifndef BTM_BLE_SCAN_SLOW_INT_2
#define BTM_BLE_SCAN_SLOW_INT_2    4096    /* 2.56 s   = 4096 *0.625 */
#endif
#ifndef BTM_BLE_SCAN_SLOW_WIN_2
#define BTM_BLE_SCAN_SLOW_WIN_2   36      /* 22.5 ms = 36 *0.625 */
#endif

/* default connection interval min */
#ifndef BTM_BLE_CONN_INT_MIN_DEF
#define BTM_BLE_CONN_INT_MIN_DEF     24      /* recommended min: 30ms  = 24 * 1.25 */
#endif

/* default connectino interval max */
#ifndef BTM_BLE_CONN_INT_MAX_DEF
#define BTM_BLE_CONN_INT_MAX_DEF     40      /* recommended max: 50 ms = 56 * 1.25 */
#endif

/* default slave latency */
#ifndef BTM_BLE_CONN_SLAVE_LATENCY_DEF
#define BTM_BLE_CONN_SLAVE_LATENCY_DEF  0      /* 0 */
#endif

/* default supervision timeout */
#ifndef BTM_BLE_CONN_TIMEOUT_DEF
#define BTM_BLE_CONN_TIMEOUT_DEF    2000
#endif

/* minimum supervision timeout */
#ifndef BTM_BLE_CONN_TIMEOUT_MIN_DEF
#define BTM_BLE_CONN_TIMEOUT_MIN_DEF    100
#endif

/* minimum acceptable connection interval */
#ifndef BTM_BLE_CONN_INT_MIN_LIMIT
#define BTM_BLE_CONN_INT_MIN_LIMIT     0x0009
#endif

#define BTM_BLE_DIR_CONN_FALLBACK_UNDIR         1
#define BTM_BLE_DIR_CONN_FALLBACK_NO_ADV        2

#ifndef BTM_BLE_DIR_CONN_FALLBACK
#define BTM_BLE_DIR_CONN_FALLBACK   BTM_BLE_DIR_CONN_FALLBACK_UNDIR
#endif

#define BTM_CMAC_TLEN_SIZE          8                   /* 64 bits */
#define BTM_BLE_AUTH_SIGN_LEN       12                   /* BLE data signature length 8 Bytes + 4 bytes counter*/
typedef uint8_t BLE_SIGNATURE[BTM_BLE_AUTH_SIGN_LEN];         /* Device address */

#ifndef BTM_BLE_HOST_SUPPORT
#define BTM_BLE_HOST_SUPPORT        0x01
#endif

#ifndef BTM_BLE_SIMULTANEOUS_HOST
#define BTM_BLE_SIMULTANEOUS_HOST   0x01
#endif

/* Appearance Values Reported with BTM_BLE_AD_TYPE_APPEARANCE */
#define BTM_BLE_APPEARANCE_UKNOWN                  0x0000
#define BTM_BLE_APPEARANCE_GENERIC_PHONE           0x0040
#define BTM_BLE_APPEARANCE_GENERIC_COMPUTER        0x0080
#define BTM_BLE_APPEARANCE_GENERIC_WATCH           0x00C0
#define BTM_BLE_APPEARANCE_SPORTS_WATCH            0x00C1
#define BTM_BLE_APPEARANCE_GENERIC_CLOCK           0x0100
#define BTM_BLE_APPEARANCE_GENERIC_DISPLAY         0x0140
#define BTM_BLE_APPEARANCE_GENERIC_REMOTE          0x0180
#define BTM_BLE_APPEARANCE_GENERIC_EYEGLASSES      0x01C0
#define BTM_BLE_APPEARANCE_GENERIC_TAG             0x0200
#define BTM_BLE_APPEARANCE_GENERIC_KEYRING         0x0240
#define BTM_BLE_APPEARANCE_GENERIC_MEDIA_PLAYER    0x0280
#define BTM_BLE_APPEARANCE_GENERIC_BARCODE_SCANNER 0x02C0
#define BTM_BLE_APPEARANCE_GENERIC_THERMOMETER     0x0300
#define BTM_BLE_APPEARANCE_THERMOMETER_EAR         0x0301
#define BTM_BLE_APPEARANCE_GENERIC_HEART_RATE      0x0340
#define BTM_BLE_APPEARANCE_HEART_RATE_BELT         0x0341
#define BTM_BLE_APPEARANCE_GENERIC_BLOOD_PRESSURE  0x0380
#define BTM_BLE_APPEARANCE_BLOOD_PRESSURE_ARM      0x0381
#define BTM_BLE_APPEARANCE_BLOOD_PRESSURE_WRIST    0x0382
#define BTM_BLE_APPEARANCE_GENERIC_HID             0x03C0
#define BTM_BLE_APPEARANCE_HID_KEYBOARD            0x03C1
#define BTM_BLE_APPEARANCE_HID_MOUSE               0x03C2
#define BTM_BLE_APPEARANCE_HID_JOYSTICK            0x03C3
#define BTM_BLE_APPEARANCE_HID_GAMEPAD             0x03C4
#define BTM_BLE_APPEARANCE_HID_DIGITIZER_TABLET    0x03C5
#define BTM_BLE_APPEARANCE_HID_CARD_READER         0x03C6
#define BTM_BLE_APPEARANCE_HID_DIGITAL_PEN         0x03C7
#define BTM_BLE_APPEARANCE_HID_BARCODE_SCANNER     0x03C8
#define BTM_BLE_APPEARANCE_GENERIC_GLUCOSE         0x0400
#define BTM_BLE_APPEARANCE_GENERIC_WALKING         0x0440
#define BTM_BLE_APPEARANCE_WALKING_IN_SHOE         0x0441
#define BTM_BLE_APPEARANCE_WALKING_ON_SHOE         0x0442
#define BTM_BLE_APPEARANCE_WALKING_ON_HIP          0x0443
#define BTM_BLE_APPEARANCE_GENERIC_CYCLING         0x0480
#define BTM_BLE_APPEARANCE_CYCLING_COMPUTER        0x0481
#define BTM_BLE_APPEARANCE_CYCLING_SPEED           0x0482
#define BTM_BLE_APPEARANCE_CYCLING_CADENCE         0x0483
#define BTM_BLE_APPEARANCE_CYCLING_POWER           0x0484
#define BTM_BLE_APPEARANCE_CYCLING_SPEED_CADENCE   0x0485
#define BTM_BLE_APPEARANCE_GENERIC_PULSE_OXIMETER  0x0C40
#define BTM_BLE_APPEARANCE_PULSE_OXIMETER_FINGERTIP 0x0C41
#define BTM_BLE_APPEARANCE_PULSE_OXIMETER_WRIST    0x0C42
#define BTM_BLE_APPEARANCE_GENERIC_WEIGHT          0x0C80
#define BTM_BLE_APPEARANCE_GENERIC_OUTDOOR_SPORTS  0x1440
#define BTM_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION 0x1441
#define BTM_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION_AND_NAV     0x1442
#define BTM_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION_POD         0x1443
#define BTM_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION_POD_AND_NAV 0x1444


/* Structure returned with Rand/Encrypt complete callback */
typedef struct {
    uint8_t   status;
    uint8_t   param_len;
    uint16_t  opcode;
    uint8_t   param_buf[BT_OCTET16_LEN];
} tBTM_RAND_ENC;

/* General callback function for notifying an application that a synchronous
** BTM function is complete. The pointer contains the address of any returned data.
*/
typedef void (tBTM_RAND_ENC_CB)(tBTM_RAND_ENC *p1);

#define BTM_BLE_FILTER_TARGET_SCANNER       0x01
#define BTM_BLE_FILTER_TARGET_ADVR          0x00

#define BTM_BLE_POLICY_BLACK_ALL            0x00    /* relevant to both */
#define BTM_BLE_POLICY_ALLOW_SCAN           0x01    /* relevant to advertiser */
#define BTM_BLE_POLICY_ALLOW_CONN           0x02    /* relevant to advertiser */
#define BTM_BLE_POLICY_WHITE_ALL            0x03    /* relevant to both */

/* ADV data flag bit definition used for BTM_BLE_AD_TYPE_FLAG */
#define BTM_BLE_LIMIT_DISC_FLAG         (0x01 << 0)
#define BTM_BLE_GEN_DISC_FLAG           (0x01 << 1)
#define BTM_BLE_BREDR_NOT_SPT           (0x01 << 2)
/* 4.1 spec adv flag for simultaneous BR/EDR+LE connection support */
#define BTM_BLE_DMT_CONTROLLER_SPT      (0x01 << 3)
#define BTM_BLE_DMT_HOST_SPT            (0x01 << 4)
#define BTM_BLE_NON_LIMIT_DISC_FLAG     (0x00 )         /* lowest bit unset */
#define BTM_BLE_ADV_FLAG_MASK           (BTM_BLE_LIMIT_DISC_FLAG | BTM_BLE_BREDR_NOT_SPT | BTM_BLE_GEN_DISC_FLAG)
#define BTM_BLE_LIMIT_DISC_MASK         (BTM_BLE_LIMIT_DISC_FLAG )

#define BTM_BLE_AD_BIT_DEV_NAME        (0x00000001 << 0)
#define BTM_BLE_AD_BIT_FLAGS           (0x00000001 << 1)
#define BTM_BLE_AD_BIT_MANU            (0x00000001 << 2)
#define BTM_BLE_AD_BIT_TX_PWR          (0x00000001 << 3)
#define BTM_BLE_AD_BIT_INT_RANGE       (0x00000001 << 5)
#define BTM_BLE_AD_BIT_SERVICE         (0x00000001 << 6)
#define BTM_BLE_AD_BIT_SERVICE_SOL     (0x00000001 << 7)
#define BTM_BLE_AD_BIT_SERVICE_DATA    (0x00000001 << 8)
#define BTM_BLE_AD_BIT_SIGN_DATA       (0x00000001 << 9)
#define BTM_BLE_AD_BIT_SERVICE_128SOL  (0x00000001 << 10)
#define BTM_BLE_AD_BIT_APPEARANCE      (0x00000001 << 11)
#define BTM_BLE_AD_BIT_PUBLIC_ADDR      (0x00000001 << 12)
#define BTM_BLE_AD_BIT_RANDOM_ADDR       (0x00000001 << 13)
#define BTM_BLE_AD_BIT_SERVICE_32        (0x00000001 << 4)
#define BTM_BLE_AD_BIT_SERVICE_32SOL     (0x00000001 << 14)
#define BTM_BLE_AD_BIT_PROPRIETARY     (0x00000001 << 15)
#define BTM_BLE_AD_BIT_SERVICE_128      (0x00000001 << 16)      /*128-bit Service UUIDs*/
#define BTM_BLE_AD_BIT_PURE_DATA        (0x00000001 << 31)      /*user prepare the formated adv data, note the payload saved in manfu buffer*/

typedef  uint32_t  tBTM_BLE_AD_MASK;

#define BTM_BLE_AD_TYPE_FLAG            HCI_EIR_FLAGS_TYPE                  /* 0x01 */
#define BTM_BLE_AD_TYPE_16SRV_PART      HCI_EIR_MORE_16BITS_UUID_TYPE       /* 0x02 */
#define BTM_BLE_AD_TYPE_16SRV_CMPL      HCI_EIR_COMPLETE_16BITS_UUID_TYPE   /* 0x03 */
#define BTM_BLE_AD_TYPE_32SRV_PART      HCI_EIR_MORE_32BITS_UUID_TYPE       /* 0x04 */
#define BTM_BLE_AD_TYPE_32SRV_CMPL      HCI_EIR_COMPLETE_32BITS_UUID_TYPE   /* 0x05 */
#define BTM_BLE_AD_TYPE_128SRV_PART     HCI_EIR_MORE_128BITS_UUID_TYPE       /* 0x06 */
#define BTM_BLE_AD_TYPE_128SRV_CMPL     HCI_EIR_COMPLETE_128BITS_UUID_TYPE   /* 0x07 */
#define BTM_BLE_AD_TYPE_NAME_SHORT      HCI_EIR_SHORTENED_LOCAL_NAME_TYPE       /* 0x08 */
#define BTM_BLE_AD_TYPE_NAME_CMPL       HCI_EIR_COMPLETE_LOCAL_NAME_TYPE        /* 0x09 */
#define BTM_BLE_AD_TYPE_TX_PWR          HCI_EIR_TX_POWER_LEVEL_TYPE             /* 0x0A */
#define BTM_BLE_AD_TYPE_DEV_CLASS       0x0D
#define BTM_BLE_AD_TYPE_SM_TK           0x10
#define BTM_BLE_AD_TYPE_SM_OOB_FLAG     0x11
#define BTM_BLE_AD_TYPE_INT_RANGE       0x12
#define BTM_BLE_AD_TYPE_SOL_SRV_UUID    0x14
#define BTM_BLE_AD_TYPE_128SOL_SRV_UUID 0x15
#define BTM_BLE_AD_TYPE_SERVICE_DATA    0x16
#define BTM_BLE_AD_TYPE_PUBLIC_TARGET   0x17
#define BTM_BLE_AD_TYPE_RANDOM_TARGET   0x18
#define BTM_BLE_AD_TYPE_APPEARANCE      0x19
#define BTM_BLE_AD_TYPE_ADV_INT         0x1a
#define BTM_BLE_AD_TYPE_32SOL_SRV_UUID  0x1b
#define BTM_BLE_AD_TYPE_32SERVICE_DATA  0x1c
#define BTM_BLE_AD_TYPE_128SERVICE_DATA 0x1d

#define BTM_BLE_AD_TYPE_MANU            HCI_EIR_MANUFACTURER_SPECIFIC_TYPE      /* 0xff */
typedef uint8_t   tBTM_BLE_AD_TYPE;

/*  Security settings used with L2CAP LE COC */
#define BTM_SEC_LE_LINK_ENCRYPTED           0x01
#define BTM_SEC_LE_LINK_PAIRED_WITHOUT_MITM 0x02
#define BTM_SEC_LE_LINK_PAIRED_WITH_MITM    0x04

/*  Min/max Preferred  number of payload octets that the local Controller
    should include in a single Link Layer Data Channel PDU. */
#define BTM_BLE_DATA_SIZE_MAX     0x00fb
#define BTM_BLE_DATA_SIZE_MIN     0x001b

/*  Preferred maximum number of microseconds that the local Controller
    should use to transmit a single Link Layer Data Channel PDU. */
#define BTM_BLE_DATA_TX_TIME_MIN     0x0148
#define BTM_BLE_DATA_TX_TIME_MAX     0x0848

/* adv tx power level */
#define BTM_BLE_ADV_TX_POWER_MIN        0           /* minimum tx power */
#define BTM_BLE_ADV_TX_POWER_LOW        1           /* low tx power     */
#define BTM_BLE_ADV_TX_POWER_MID        2           /* middle tx power  */
#define BTM_BLE_ADV_TX_POWER_UPPER      3           /* upper tx power   */
#define BTM_BLE_ADV_TX_POWER_MAX        4           /* maximum tx power */
typedef uint8_t tBTM_BLE_ADV_TX_POWER;

/* adv tx power in dBm */
typedef struct {
    uint8_t adv_inst_max;         /* max adv instance supported in controller */
    uint8_t rpa_offloading;
    uint16_t tot_scan_results_strg;
    uint8_t max_irk_list_sz;
    uint8_t filter_support;
    uint8_t max_filter;
    uint8_t energy_support;
    uint8_t values_read;
    uint16_t version_supported;
    uint16_t total_trackable_advertisers;
    uint8_t extended_scan_support;
    uint8_t debug_logging_supported;
} tBTM_BLE_VSC_CB;

/* slave preferred connection interval range */
typedef struct {
    uint16_t  low;
    uint16_t  hi;

} tBTM_BLE_INT_RANGE;

/* Service tag supported in the device */
#define MAX_16BIT_SERVICES 16
typedef struct {
    uint8_t       num_service;
    uint8_t     list_cmpl;
    uint16_t      uuid[MAX_16BIT_SERVICES];
} tBTM_BLE_SERVICE;

/* 32 bits Service supported in the device */
#define MAX_32BIT_SERVICES 4
typedef struct {
    uint8_t       num_service;
    uint8_t     list_cmpl;
    uint32_t      uuid[MAX_32BIT_SERVICES];
} tBTM_BLE_32SERVICE;

/* 128 bits Service supported in the device */
typedef struct {
    uint8_t       num_service;
    uint8_t     list_cmpl;
    uint8_t       uuid128[MAX_UUID_SIZE];
} tBTM_BLE_128SERVICE;

#define MAX_SIZE_MANUFACTURER_DATA 32
typedef struct {
    uint8_t len;
    uint8_t val[MAX_SIZE_MANUFACTURER_DATA];
} tBTM_BLE_MANU;

#define MAX_SIZE_SERVICE_DATA 32
typedef struct {
    tBT_UUID    service_uuid;
    uint8_t       len;
    uint8_t       val[MAX_SIZE_SERVICE_DATA];
} tBTM_BLE_SERVICE_DATA;

#define MAX_SIZE_PROPRIETARY_ELEMENT 32
typedef struct {
    uint8_t       adv_type;
    uint8_t       len;
    uint8_t       val[MAX_SIZE_PROPRIETARY_ELEMENT];     /* number of len byte */
} tBTM_BLE_PROP_ELEM;

#define MAX_PROPRIETARY_ELEMENTS 4
typedef struct {
    uint8_t                   num_elem;
    tBTM_BLE_PROP_ELEM      elem[MAX_PROPRIETARY_ELEMENTS];
} tBTM_BLE_PROPRIETARY;

typedef struct {
    tBTM_BLE_INT_RANGE      int_range;      /* slave prefered conn interval range */
    tBTM_BLE_MANU           manu;           /* manufactuer data */
    tBTM_BLE_SERVICE        services;       /* services */
    tBTM_BLE_128SERVICE     services_128b;  /* 128 bits service */
    tBTM_BLE_32SERVICE      service_32b;     /* 32 bits Service UUID */
    tBTM_BLE_SERVICE        sol_services;    /* 16 bits services Solicitation UUIDs */
    tBTM_BLE_32SERVICE      sol_service_32b;    /* List of 32 bit Service Solicitation UUIDs */
    tBTM_BLE_128SERVICE     sol_service_128b;    /* List of 128 bit Service Solicitation UUIDs */
    tBTM_BLE_PROPRIETARY    proprietary;
    tBTM_BLE_SERVICE_DATA   service_data;    /* service data */
    uint16_t                  appearance;
    uint8_t                   flag;
    uint8_t                   tx_power;
} tBTM_BLE_ADV_DATA;

#ifndef BTM_BLE_MULTI_ADV_MAX
#define BTM_BLE_MULTI_ADV_MAX   16 /* controller returned adv_inst_max should be less
    than this number */
#endif

#define BTM_BLE_MULTI_ADV_INVALID   0

#define BTM_BLE_MULTI_ADV_ENB_EVT           1
#define BTM_BLE_MULTI_ADV_DISABLE_EVT       2
#define BTM_BLE_MULTI_ADV_PARAM_EVT         3
#define BTM_BLE_MULTI_ADV_DATA_EVT          4
typedef uint8_t tBTM_BLE_MULTI_ADV_EVT;

#define BTM_BLE_MULTI_ADV_DEFAULT_STD 0

typedef struct {
    uint16_t          adv_int_min;
    uint16_t          adv_int_max;
    uint8_t           adv_type;
    tBTM_BLE_ADV_CHNL_MAP channel_map;
    tBTM_BLE_AFP    adv_filter_policy;
    tBTM_BLE_ADV_TX_POWER tx_power;
} tBTM_BLE_ADV_PARAMS;

typedef struct {
    uint8_t   *p_sub_code; /* dynamic array to store sub code */
    uint8_t   *p_inst_id;  /* dynamic array to store instance id */
    uint8_t   pending_idx;
    uint8_t   next_idx;
} tBTM_BLE_MULTI_ADV_OPQ;

typedef void (tBTM_BLE_MULTI_ADV_CBACK)(tBTM_BLE_MULTI_ADV_EVT evt, uint8_t inst_id,
                                        void *p_ref, tBTM_STATUS status);

typedef struct {
    uint8_t                       inst_id;
    uint8_t                     in_use;
    uint8_t                       adv_evt;
    BD_ADDR                     rpa;
    TIMER_LIST_ENT adv_raddr_timer;
    tBTM_BLE_MULTI_ADV_CBACK    *p_cback;
    void                        *p_ref;
    uint8_t                       index;
} tBTM_BLE_MULTI_ADV_INST;

typedef struct {
    uint8_t inst_index_queue[BTM_BLE_MULTI_ADV_MAX];
    int front;
    int rear;
} tBTM_BLE_MULTI_ADV_INST_IDX_Q;

typedef struct {
    tBTM_BLE_MULTI_ADV_INST *p_adv_inst; /* dynamic array to store adv instance */
    tBTM_BLE_MULTI_ADV_OPQ  op_q;
} tBTM_BLE_MULTI_ADV_CB;

typedef uint8_t tGATT_IF;

typedef void (tBTM_BLE_SCAN_THRESHOLD_CBACK)(tBTM_BLE_REF_VALUE ref_value);
typedef void (tBTM_BLE_SCAN_REP_CBACK)(tBTM_BLE_REF_VALUE ref_value, uint8_t report_format,
                                       uint8_t num_records, uint16_t total_len,
                                       uint8_t *p_rep_data, uint8_t status);
typedef void (tBTM_BLE_SCAN_SETUP_CBACK)(uint8_t evt, tBTM_BLE_REF_VALUE ref_value, uint8_t status);

#ifndef BTM_BLE_BATCH_SCAN_MAX
#define BTM_BLE_BATCH_SCAN_MAX   5
#endif

#ifndef BTM_BLE_BATCH_REP_MAIN_Q_SIZE
#define BTM_BLE_BATCH_REP_MAIN_Q_SIZE  2
#endif

typedef enum {
    BTM_BLE_SCAN_INVALID_STATE = 0,
    BTM_BLE_SCAN_ENABLE_CALLED = 1,
    BTM_BLE_SCAN_ENABLED_STATE = 2,
    BTM_BLE_SCAN_DISABLE_CALLED = 3,
    BTM_BLE_SCAN_DISABLED_STATE = 4
} tBTM_BLE_BATCH_SCAN_STATE;

enum {
    BTM_BLE_DISCARD_OLD_ITEMS,
    BTM_BLE_DISCARD_LOWER_RSSI_ITEMS
};
typedef uint8_t tBTM_BLE_DISCARD_RULE;

typedef struct {
    uint8_t   sub_code[BTM_BLE_BATCH_SCAN_MAX];
    tBTM_BLE_BATCH_SCAN_STATE cur_state[BTM_BLE_BATCH_SCAN_MAX];
    tBTM_BLE_REF_VALUE        ref_value[BTM_BLE_BATCH_SCAN_MAX];
    uint8_t   pending_idx;
    uint8_t   next_idx;
} tBTM_BLE_BATCH_SCAN_OPQ;

typedef struct {
    uint8_t   rep_mode[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    tBTM_BLE_REF_VALUE  ref_value[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    uint8_t   num_records[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    uint16_t  data_len[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    uint8_t   *p_data[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    uint8_t   pending_idx;
    uint8_t   next_idx;
} tBTM_BLE_BATCH_SCAN_REP_Q;

typedef struct {
    tBTM_BLE_BATCH_SCAN_STATE      cur_state;
    tBTM_BLE_BATCH_SCAN_MODE scan_mode;
    uint32_t                  scan_interval;
    uint32_t                  scan_window;
    tBLE_ADDR_TYPE          addr_type;
    tBTM_BLE_DISCARD_RULE   discard_rule;
    tBTM_BLE_BATCH_SCAN_OPQ  op_q;
    tBTM_BLE_BATCH_SCAN_REP_Q main_rep_q;
    tBTM_BLE_SCAN_SETUP_CBACK     *p_setup_cback;
    tBTM_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback;
    tBTM_BLE_SCAN_REP_CBACK       *p_scan_rep_cback;
    tBTM_BLE_REF_VALUE             ref_value;
} tBTM_BLE_BATCH_SCAN_CB;

/* filter selection bit index  */
#define BTM_BLE_PF_ADDR_FILTER          0
#define BTM_BLE_PF_SRVC_DATA            1
#define BTM_BLE_PF_SRVC_UUID            2
#define BTM_BLE_PF_SRVC_SOL_UUID        3
#define BTM_BLE_PF_LOCAL_NAME           4
#define BTM_BLE_PF_MANU_DATA            5
#define BTM_BLE_PF_SRVC_DATA_PATTERN    6
#define BTM_BLE_PF_TYPE_ALL             7  /* when passed in payload filter type all, only clear action is applicable */
#define BTM_BLE_PF_TYPE_MAX             8

/* max number of filter spot for different filter type */
#ifndef BTM_BLE_MAX_UUID_FILTER
#define BTM_BLE_MAX_UUID_FILTER     8
#endif
#ifndef BTM_BLE_MAX_ADDR_FILTER
#define BTM_BLE_MAX_ADDR_FILTER     8
#endif
#ifndef BTM_BLE_PF_STR_COND_MAX
#define BTM_BLE_PF_STR_COND_MAX     4   /* apply to manu data , or local name */
#endif
#ifndef BTM_BLE_PF_STR_LEN_MAX
#define BTM_BLE_PF_STR_LEN_MAX      29  /* match for first 29 bytes */
#endif

typedef uint8_t   tBTM_BLE_PF_COND_TYPE;

#define BTM_BLE_PF_LOGIC_OR              0
#define BTM_BLE_PF_LOGIC_AND             1
typedef uint8_t tBTM_BLE_PF_LOGIC_TYPE;

#define BTM_BLE_PF_ENABLE       1
#define BTM_BLE_PF_CONFIG       2
typedef uint8_t tBTM_BLE_PF_ACTION;

typedef uint8_t tBTM_BLE_PF_FILT_INDEX;

typedef uint8_t tBTM_BLE_PF_AVBL_SPACE;

#define BTM_BLE_PF_BRDCAST_ADDR_FILT  1
#define BTM_BLE_PF_SERV_DATA_CHG_FILT 2
#define BTM_BLE_PF_SERV_UUID          4
#define BTM_BLE_PF_SERV_SOLC_UUID     8
#define BTM_BLE_PF_LOC_NAME_CHECK    16
#define BTM_BLE_PF_MANUF_NAME_CHECK  32
#define BTM_BLE_PF_SERV_DATA_CHECK   64
typedef uint16_t tBTM_BLE_PF_FEAT_SEL;

#define BTM_BLE_PF_LIST_LOGIC_OR   1
#define BTM_BLE_PF_LIST_LOGIC_AND  2
typedef uint16_t tBTM_BLE_PF_LIST_LOGIC_TYPE;

#define BTM_BLE_PF_FILT_LOGIC_OR   0
#define BTM_BLE_PF_FILT_LOGIC_AND  1
typedef uint16_t tBTM_BLE_PF_FILT_LOGIC_TYPE;

typedef uint8_t  tBTM_BLE_PF_RSSI_THRESHOLD;
typedef uint8_t  tBTM_BLE_PF_DELIVERY_MODE;
typedef uint16_t tBTM_BLE_PF_TIMEOUT;
typedef uint8_t  tBTM_BLE_PF_TIMEOUT_CNT;
typedef uint16_t tBTM_BLE_PF_ADV_TRACK_ENTRIES;

typedef struct {
    tBTM_BLE_PF_FEAT_SEL feat_seln;
    tBTM_BLE_PF_LIST_LOGIC_TYPE logic_type;
    tBTM_BLE_PF_FILT_LOGIC_TYPE filt_logic_type;
    tBTM_BLE_PF_RSSI_THRESHOLD  rssi_high_thres;
    tBTM_BLE_PF_RSSI_THRESHOLD  rssi_low_thres;
    tBTM_BLE_PF_DELIVERY_MODE dely_mode;
    tBTM_BLE_PF_TIMEOUT found_timeout;
    tBTM_BLE_PF_TIMEOUT lost_timeout;
    tBTM_BLE_PF_TIMEOUT_CNT found_timeout_cnt;
    tBTM_BLE_PF_ADV_TRACK_ENTRIES num_of_tracking_entries;
} tBTM_BLE_PF_FILT_PARAMS;

enum {
    BTM_BLE_SCAN_COND_ADD,
    BTM_BLE_SCAN_COND_DELETE,
    BTM_BLE_SCAN_COND_CLEAR = 2
};
typedef uint8_t tBTM_BLE_SCAN_COND_OP;

enum {
    BTM_BLE_FILT_ENABLE_DISABLE = 1,
    BTM_BLE_FILT_CFG            = 2,
    BTM_BLE_FILT_ADV_PARAM      = 3
};

typedef uint8_t tBTM_BLE_FILT_CB_EVT;

/* BLE adv payload filtering config complete callback */
typedef void (tBTM_BLE_PF_CFG_CBACK)(tBTM_BLE_PF_ACTION action, tBTM_BLE_SCAN_COND_OP cfg_op,
                                     tBTM_BLE_PF_AVBL_SPACE avbl_space, tBTM_STATUS status,
                                     tBTM_BLE_REF_VALUE ref_value);

typedef void (tBTM_BLE_PF_CMPL_CBACK)(tBTM_BLE_PF_CFG_CBACK);

/* BLE adv payload filtering status setup complete callback */
typedef void (tBTM_BLE_PF_STATUS_CBACK)(uint8_t action, tBTM_STATUS status,
                                        tBTM_BLE_REF_VALUE ref_value);

/* BLE adv payload filtering param setup complete callback */
typedef void (tBTM_BLE_PF_PARAM_CBACK)(tBTM_BLE_PF_ACTION action_type,
                                       tBTM_BLE_PF_AVBL_SPACE avbl_space,
                                       tBTM_BLE_REF_VALUE ref_value, tBTM_STATUS status);

typedef union {
    uint16_t              uuid16_mask;
    uint32_t              uuid32_mask;
    uint8_t               uuid128_mask[LEN_UUID_128];
} tBTM_BLE_PF_COND_MASK;

typedef struct {
    tBLE_BD_ADDR            *p_target_addr;     /* target address, if NULL, generic UUID filter */
    tBT_UUID                uuid;           /* UUID condition */
    tBTM_BLE_PF_LOGIC_TYPE  cond_logic;    /* AND/OR */
    tBTM_BLE_PF_COND_MASK   *p_uuid_mask;           /* UUID mask */
} tBTM_BLE_PF_UUID_COND;

typedef struct {
    uint8_t                   data_len;       /* <= 20 bytes */
    uint8_t                   *p_data;
} tBTM_BLE_PF_LOCAL_NAME_COND;

typedef struct {
    uint16_t                  company_id;     /* company ID */
    uint8_t                   data_len;       /* <= 20 bytes */
    uint8_t                   *p_pattern;
    uint16_t                  company_id_mask; /* UUID value mask */
    uint8_t                   *p_pattern_mask; /* Manufacturer data matching mask,
                                                same length as data pattern,
                                                set to all 0xff, match exact data */
} tBTM_BLE_PF_MANU_COND;

typedef struct {
    uint16_t                  uuid;     /* service ID */
    uint8_t                   data_len;       /* <= 20 bytes */
    uint8_t                   *p_pattern;
    uint8_t
    *p_pattern_mask; /* Service data matching mask, same length as data pattern,
                                                set to all 0xff, match exact data */
} tBTM_BLE_PF_SRVC_PATTERN_COND;


typedef union {
    tBLE_BD_ADDR                            target_addr;
    tBTM_BLE_PF_LOCAL_NAME_COND             local_name; /* lcoal name filtering */
    tBTM_BLE_PF_MANU_COND                   manu_data;  /* manufactuer data filtering */
    tBTM_BLE_PF_UUID_COND                   srvc_uuid;  /* service UUID filtering */
    tBTM_BLE_PF_UUID_COND                   solicitate_uuid;   /* solicitated service UUID filtering */
    tBTM_BLE_PF_SRVC_PATTERN_COND           srvc_data;      /* service data pattern */
} tBTM_BLE_PF_COND_PARAM;

typedef struct {
    uint8_t   action_ocf[BTM_BLE_PF_TYPE_MAX];
    tBTM_BLE_REF_VALUE  ref_value[BTM_BLE_PF_TYPE_MAX];
    tBTM_BLE_PF_PARAM_CBACK  *p_filt_param_cback[BTM_BLE_PF_TYPE_MAX];
    tBTM_BLE_PF_CFG_CBACK *p_scan_cfg_cback[BTM_BLE_PF_TYPE_MAX];
    uint8_t   cb_evt[BTM_BLE_PF_TYPE_MAX];
    uint8_t   pending_idx;
    uint8_t   next_idx;
} tBTM_BLE_ADV_FILTER_ADV_OPQ;

#define BTM_BLE_MAX_FILTER_COUNTER  (BTM_BLE_MAX_ADDR_FILTER + 1) /* per device filter + one generic filter indexed by 0 */

#ifndef BTM_CS_IRK_LIST_MAX
#define BTM_CS_IRK_LIST_MAX 0x20
#endif

typedef struct {
    uint8_t    in_use;
    BD_ADDR    bd_addr;
    uint8_t
    pf_counter[BTM_BLE_PF_TYPE_MAX]; /* number of filter indexed by tBTM_BLE_PF_COND_TYPE */
} tBTM_BLE_PF_COUNT;

typedef struct {
    uint8_t             enable;
    uint8_t               op_type;
    tBTM_BLE_PF_COUNT   *p_addr_filter_count; /* per BDA filter array */
    tBLE_BD_ADDR        cur_filter_target;
    tBTM_BLE_PF_STATUS_CBACK *p_filt_stat_cback;
    tBTM_BLE_ADV_FILTER_ADV_OPQ  op_q;
} tBTM_BLE_ADV_FILTER_CB;

/* Sub codes */
#define BTM_BLE_META_PF_ENABLE          0x00
#define BTM_BLE_META_PF_FEAT_SEL        0x01
#define BTM_BLE_META_PF_ADDR            0x02
#define BTM_BLE_META_PF_UUID            0x03
#define BTM_BLE_META_PF_SOL_UUID        0x04
#define BTM_BLE_META_PF_LOCAL_NAME      0x05
#define BTM_BLE_META_PF_MANU_DATA       0x06
#define BTM_BLE_META_PF_SRVC_DATA       0x07
#define BTM_BLE_META_PF_ALL             0x08

typedef uint8_t BTM_BLE_ADV_STATE;
typedef uint8_t BTM_BLE_ADV_INFO_PRESENT;
typedef uint8_t BTM_BLE_RSSI_VALUE;
typedef uint16_t BTM_BLE_ADV_INFO_TIMESTAMP;

/* These are the fields returned in each device adv packet.  It
** is returned in the results callback if registered.
*/
typedef struct {
    uint8_t               conn_mode;
    tBTM_BLE_AD_MASK    ad_mask;        /* mask of the valid adv data field */
    uint8_t               flag;
    uint8_t               tx_power_level;
    uint8_t               remote_name_len;
    uint8_t               *p_remote_name;
    tBTM_BLE_SERVICE    service;
} tBTM_BLE_INQ_DATA;

enum {
    BTM_BLE_CONN_NONE,
    BTM_BLE_CONN_AUTO,
    BTM_BLE_CONN_SELECTIVE
};
typedef uint8_t   tBTM_BLE_CONN_TYPE;

#define ADV_INFO_PRESENT        0x00
#define NO_ADV_INFO_PRESENT     0x01

typedef btgatt_track_adv_info_t tBTM_BLE_TRACK_ADV_DATA;

typedef void (tBTM_BLE_TRACK_ADV_CBACK)(tBTM_BLE_TRACK_ADV_DATA *p_track_adv_data);

typedef uint8_t tBTM_BLE_TRACK_ADV_EVT;

typedef struct {
    tBTM_BLE_REF_VALUE             ref_value;
    tBTM_BLE_TRACK_ADV_CBACK *p_track_cback;
} tBTM_BLE_ADV_TRACK_CB;

enum {
    BTM_BLE_TRACK_ADV_ADD,
    BTM_BLE_TRACK_ADV_REMOVE
};

typedef uint8_t tBTM_BLE_TRACK_ADV_ACTION;

#define BTM_BLE_MULTI_ADV_INVALID   0

#define BTM_BLE_BATCH_SCAN_ENABLE_EVT     1
#define BTM_BLE_BATCH_SCAN_CFG_STRG_EVT   2
#define BTM_BLE_BATCH_SCAN_READ_REPTS_EVT 3
#define BTM_BLE_BATCH_SCAN_THR_EVT        4
#define BTM_BLE_BATCH_SCAN_PARAM_EVT      5
#define BTM_BLE_BATCH_SCAN_DISABLE_EVT    6

typedef uint8_t tBTM_BLE_BATCH_SCAN_EVT;

typedef uint32_t tBTM_BLE_TX_TIME_MS;
typedef uint32_t tBTM_BLE_RX_TIME_MS;
typedef uint32_t tBTM_BLE_IDLE_TIME_MS;
typedef uint32_t tBTM_BLE_ENERGY_USED;

typedef void (tBTM_BLE_ENERGY_INFO_CBACK)(tBTM_BLE_TX_TIME_MS tx_time, tBTM_BLE_RX_TIME_MS rx_time,
        tBTM_BLE_IDLE_TIME_MS idle_time,
        tBTM_BLE_ENERGY_USED  energy_used,
        tBTM_STATUS status);

typedef struct {
    tBTM_BLE_ENERGY_INFO_CBACK *p_ener_cback;
} tBTM_BLE_ENERGY_INFO_CB;

typedef uint8_t (tBTM_BLE_SEL_CBACK)(BD_ADDR random_bda,     uint8_t *p_remote_name);
typedef void (tBTM_BLE_CTRL_FEATURES_CBACK)(tBTM_STATUS status);

/* callback function for SMP signing algorithm, signed data in little endian order with tlen bits long */
typedef void (tBTM_BLE_SIGN_CBACK)(void *p_ref_data, uint8_t *p_signing_data);
typedef void (tBTM_BLE_VERIFY_CBACK)(void *p_ref_data, uint8_t match);
/* random address set complete callback */
typedef void (tBTM_BLE_RANDOM_SET_CBACK)(BD_ADDR random_bda);

typedef void (tBTM_BLE_SCAN_REQ_CBACK)(BD_ADDR remote_bda, tBLE_ADDR_TYPE addr_type,
                                       uint8_t adv_evt);
typedef void (*tBLE_SCAN_PARAM_SETUP_CBACK)(tGATT_IF client_if, tBTM_STATUS status);

tBTM_BLE_SCAN_SETUP_CBACK bta_ble_scan_setup_cb;

/*****************************************************************************
**  EXTERNAL FUNCTION DECLARATIONS
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/*******************************************************************************
**
** Function         BTM_SecAddBleDevice
**
** Description      Add/modify device.  This function will be normally called
**                  during host startup to restore all required information
**                  for a LE device stored in the NVRAM.
**
** Parameters:      bd_addr          - BD address of the peer
**                  bd_name          - Name of the peer device.  NULL if unknown.
**                  dev_type         - Remote device's device type.
**                  addr_type        - LE device address type.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
extern uint8_t BTM_SecAddBleDevice(BD_ADDR bd_addr, BD_NAME bd_name,
                                   tBT_DEVICE_TYPE dev_type, tBLE_ADDR_TYPE addr_type);

/*******************************************************************************
**
** Function         BTM_SecAddBleKey
**
** Description      Add/modify LE device information.  This function will be
**                  normally called during host startup to restore all required
**                  information stored in the NVRAM.
**
** Parameters:      bd_addr          - BD address of the peer
**                  p_le_key         - LE key values.
**                  key_type         - LE SMP key type.
*
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
extern uint8_t BTM_SecAddBleKey(BD_ADDR bd_addr, tBTM_LE_KEY_VALUE *p_le_key,
                                tBTM_LE_KEY_TYPE key_type);

/*******************************************************************************
**
** Function         BTM_BleSetAdvExtParams
**
** Description      This function is called to set advertising parameters.
**
** Parameters:       None.
**
** Returns          void
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleSetAdvExtParams(uint16_t adv_int_min, uint16_t adv_int_max,
        uint8_t adv_type, uint8_t own_addr_type,
        tBTM_BLE_ADV_CHNL_MAP chnl_map, uint8_t afp, uint8_t peer_addr_type, tBLE_BD_ADDR *p_dir_bda);

/*******************************************************************************
**
** Function         BTM_BleSetAdvParams
**
** Description      This function is called to set extended advertising parameters.
**
** Parameters:       None.
**
** Returns          void
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleSetAdvParams(uint16_t adv_int_min, uint16_t adv_int_max,
                                       tBLE_BD_ADDR *p_dir_bda, tBTM_BLE_ADV_CHNL_MAP chnl_map);


/*******************************************************************************
**
** Function         BTM_BleWriteAdvData
**
** Description      This function is called to write advertising data.
**
** Parameters:       None.
**
** Returns          void
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleWriteAdvData(tBTM_BLE_AD_MASK  data_mask,
                                       tBTM_BLE_ADV_DATA *p_data);

/*******************************************************************************
**
** Function         BTM_BleSetAdvParams
**
** Description      This function is called to set advertising parameters.
**
** Parameters       adv_int_min: minimum advertising interval
**                  adv_int_max: maximum advertising interval
**                  p_dir_bda: connectable direct initiator's LE device address
**                  chnl_map: advertising channel map.
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleReadAdvParams(uint16_t *adv_int_min, uint16_t *adv_int_max,
                                 tBLE_BD_ADDR *p_dir_bda, tBTM_BLE_ADV_CHNL_MAP *p_chnl_map);

/*******************************************************************************
**
** Function         BTM_BleObtainVendorCapabilities
**
** Description      This function is called to obatin vendor capabilties
**
** Parameters       p_cmn_vsc_cb - Returns the vednor capabilities
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleObtainVendorCapabilities(tBTM_BLE_VSC_CB *p_cmn_vsc_cb);

/*******************************************************************************
**
** Function         BTM_BleSetScanParams
**
** Description      This function is called to set Scan parameters.
**
** Parameters       client_if - Client IF value
**                  scan_interval - Scan interval
**                  scan_window - Scan window
**                  scan_type - Scan type
**                  scan_setup_status_cback - Scan setup status callback
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleSetScanParams(tGATT_IF client_if, uint32_t scan_interval,
                                 uint32_t scan_window, tBLE_SCAN_MODE scan_type,
                                 tBLE_SCAN_PARAM_SETUP_CBACK scan_setup_status_cback);

/*******************************************************************************
**
** Function         BTM_BleGetVendorCapabilities
**
** Description      This function reads local LE features
**
** Parameters       p_cmn_vsc_cb : Locala LE capability structure
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleGetVendorCapabilities(tBTM_BLE_VSC_CB *p_cmn_vsc_cb);
/*******************************************************************************
**
** Function         BTM_BleSetStorageConfig
**
** Description      This function is called to setup storage configuration and setup callbacks.
**
** Parameters       uint8_t batch_scan_full_max -Batch scan full maximum
                    uint8_t batch_scan_trunc_max - Batch scan truncated value maximum
                    uint8_t batch_scan_notify_threshold - Threshold value
                    tBTM_BLE_SCAN_SETUP_CBACK *p_setup_cback - Setup callback
                    tBTM_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback -Threshold callback
                    void *p_ref - Reference value
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleSetStorageConfig(uint8_t batch_scan_full_max,
        uint8_t batch_scan_trunc_max,
        uint8_t batch_scan_notify_threshold,
        tBTM_BLE_SCAN_SETUP_CBACK *p_setup_cback,
        tBTM_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback,
        tBTM_BLE_SCAN_REP_CBACK *p_cback,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleEnableBatchScan
**
** Description      This function is called to enable batch scan
**
** Parameters       tBTM_BLE_BATCH_SCAN_MODE scan_mode - Batch scan mode
                    uint32_t scan_interval -Scan interval
                    uint32_t scan_window - Scan window value
                    tBLE_ADDR_TYPE addr_type - Address type
                    tBTM_BLE_DISCARD_RULE discard_rule - Data discard rules
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleEnableBatchScan(tBTM_BLE_BATCH_SCAN_MODE scan_mode,
        uint32_t scan_interval, uint32_t scan_window,
        tBTM_BLE_DISCARD_RULE discard_rule,
        tBLE_ADDR_TYPE addr_type,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleDisableBatchScan
**
** Description      This function is called to disable batch scanning
**
** Parameters       void
**
** Returns          void
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleDisableBatchScan(tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleReadScanReports
**
** Description      This function is called to read batch scan reports
**
** Parameters       tBLE_SCAN_MODE scan_mode - Scan mode report to be read out
                    tBTM_BLE_SCAN_REP_CBACK* p_cback - Reports callback
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleReadScanReports(tBLE_SCAN_MODE scan_mode,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleTrackAdvertiser
**
** Description      This function is called to read batch scan reports
**
** Parameters       p_track_cback - Tracking callback
**                  ref_value - Reference value
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleTrackAdvertiser(tBTM_BLE_TRACK_ADV_CBACK *p_track_cback,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleWriteScanRsp
**
** Description      This function is called to write LE scan response.
**
** Parameters:      p_scan_rsp: scan response.
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleWriteScanRsp(tBTM_BLE_AD_MASK data_mask,
                                       tBTM_BLE_ADV_DATA *p_data);

/*******************************************************************************
**
** Function         BTM_BleObserve
**
** Description      This procedure keep the device listening for advertising
**                  events from a broadcast device.
**
** Parameters       start: start or stop observe.
**
** Returns          void
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleObserve(uint8_t start, uint8_t duration,
                                  tBTM_INQ_RESULTS_CB *p_results_cb, tBTM_CMPL_CB *p_cmpl_cb);


/*******************************************************************************
**
** Function         BTM_GetDeviceIDRoot
**
** Description      This function is called to read the local device identity
**                  root.
**
** Returns          void
**                  the local device ER is copied into er
**
*******************************************************************************/
extern void BTM_GetDeviceIDRoot(BT_OCTET16 ir);

/*******************************************************************************
**
** Function         BTM_GetDeviceEncRoot
**
** Description      This function is called to read the local device encryption
**                  root.
**
** Returns          void
**                  the local device ER is copied into er
**
*******************************************************************************/
extern void BTM_GetDeviceEncRoot(BT_OCTET16 er);

/*******************************************************************************
**
** Function         BTM_GetDeviceDHK
**
** Description      This function is called to read the local device DHK.
**
** Returns          void
**                  the local device DHK is copied into dhk
**
*******************************************************************************/
extern void BTM_GetDeviceDHK(BT_OCTET16 dhk);

/*******************************************************************************
**
** Function         BTM_SecurityGrant
**
** Description      This function is called to grant security process.
**
** Parameters       bd_addr - peer device bd address.
**                  res     - result of the operation BTM_SUCCESS if success.
**                            Otherwise, BTM_REPEATED_ATTEMPTS is too many attempts.
**
** Returns          None
**
*******************************************************************************/
extern void BTM_SecurityGrant(BD_ADDR bd_addr, uint8_t res);

/*******************************************************************************
**
** Function         BTM_BlePasskeyReply
**
** Description      This function is called after Security Manager submitted
**                  passkey request to the application.
**
** Parameters:      bd_addr      - Address of the device for which passkey was requested
**                  res          - result of the operation SMP_SUCCESS if success
**                  passkey - numeric value in the range of
**                  BTM_MIN_PASSKEY_VAL(0) - BTM_MAX_PASSKEY_VAL(999999(0xF423F)).
**
*******************************************************************************/
extern void BTM_BlePasskeyReply(BD_ADDR bd_addr, uint8_t res, uint32_t passkey);

/*******************************************************************************
**
** Function         BTM_BleConfirmReply
**
** Description      This function is called after Security Manager submitted
**                  numeric comparison request to the application.
**
** Parameters:      bd_addr      - Address of the device with which numeric
**                                 comparison was requested
**                  res          - comparison result BTM_SUCCESS if success
**
*******************************************************************************/
extern void BTM_BleConfirmReply(BD_ADDR bd_addr, uint8_t res);

/*******************************************************************************
**
** Function         BTM_LeOobDataReply
**
** Description      This function is called to provide the OOB data for
**                  SMP in response to BTM_LE_OOB_REQ_EVT
**
** Parameters:      bd_addr     - Address of the peer device
**                  res         - result of the operation SMP_SUCCESS if success
**                  p_data      - simple pairing Randomizer  C.
**
*******************************************************************************/
extern void BTM_BleOobDataReply(BD_ADDR bd_addr, uint8_t res, uint8_t len, uint8_t *p_data);


/*******************************************************************************
**
** Function         BTM_BleDataSignature
**
** Description      This function is called to sign the data using AES128 CMAC
**                  algorith.
**
** Parameter        bd_addr: target device the data to be signed for.
**                  p_text: singing data
**                  len: length of the signing data
**                  signature: output parameter where data signature is going to
**                             be stored.
**
** Returns          TRUE if signing sucessul, otherwise FALSE.
**
*******************************************************************************/
extern uint8_t BTM_BleDataSignature(BD_ADDR bd_addr, uint8_t *p_text, uint16_t len,
                                    BLE_SIGNATURE signature);

/*******************************************************************************
**
** Function         BTM_BleVerifySignature
**
** Description      This function is called to verify the data signature
**
** Parameter        bd_addr: target device the data to be signed for.
**                  p_orig:  original data before signature.
**                  len: length of the signing data
**                  counter: counter used when doing data signing
**                  p_comp: signature to be compared against.

** Returns          TRUE if signature verified correctly; otherwise FALSE.
**
*******************************************************************************/
extern uint8_t BTM_BleVerifySignature(BD_ADDR bd_addr, uint8_t *p_orig,
                                      uint16_t len, uint32_t counter,
                                      uint8_t *p_comp);

/*******************************************************************************
**
** Function         BTM_ReadConnectionAddr
**
** Description      This function is called to set the local device random address
**                  .
**
** Returns          void
**
*******************************************************************************/
extern void BTM_ReadConnectionAddr(BD_ADDR remote_bda, BD_ADDR local_conn_addr,
                                   tBLE_ADDR_TYPE *p_addr_type);



/*******************************************************************************
**
** Function         BTM_ReadRemoteConnectionAddr
**
** Description      This function is read the remote device address currently used
**                  .
**
** Returns          void
**
*******************************************************************************/
extern uint8_t BTM_ReadRemoteConnectionAddr(BD_ADDR pseudo_addr,
        BD_ADDR conn_addr,
        tBLE_ADDR_TYPE *p_addr_type);

/*******************************************************************************
**
** Function         BTM_BleLoadLocalKeys
**
** Description      Local local identity key, encryption root or sign counter.
**
** Parameters:      key_type: type of key, can be BTM_BLE_KEY_TYPE_ID, BTM_BLE_KEY_TYPE_ER
**                            or BTM_BLE_KEY_TYPE_COUNTER.
**                  p_key: pointer to the key.
*
** Returns          non2.
**
*******************************************************************************/
extern void BTM_BleLoadLocalKeys(uint8_t key_type, tBTM_BLE_LOCAL_KEYS *p_key);


/*******************************************************************************
**
** Function         BTM_BleSetBgConnType
**
** Description      This function is called to set BLE background connection
**                  procedure type. It can be auto connection, or selective connection.
**
** Parameters       conn_type: it can be auto connection, or selective connection.
**                  p_select_cback: callback function when selective connection procedure
**                              is being used.
**
** Returns          void
**
*******************************************************************************/
extern uint8_t BTM_BleSetBgConnType(tBTM_BLE_CONN_TYPE   conn_type,
                                    tBTM_BLE_SEL_CBACK   *p_select_cback);

/*******************************************************************************
**
** Function         BTM_BleUpdateBgConnDev
**
** Description      This function is called to add or remove a device into/from
**                  background connection procedure. The background connection
*                   procedure is decided by the background connection type, it can be
*                   auto connection, or selective connection.
**
** Parameters       add_remove: TRUE to add; FALSE to remove.
**                  remote_bda: device address to add/remove.
**
** Returns          void
**
*******************************************************************************/
extern uint8_t BTM_BleUpdateBgConnDev(uint8_t add_remove, BD_ADDR   remote_bda);

/*******************************************************************************
**
** Function         BTM_BleClearBgConnDev
**
** Description      This function is called to clear the whitelist,
**                  end any pending whitelist connections,
*                   and reset the local bg device list.
**
** Parameters       void
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleClearBgConnDev(void);

/********************************************************
**
** Function         BTM_BleSetPrefConnParams
**
** Description      Set a peripheral's preferred connection parameters. When
**                  any of the value does not want to be updated while others
**                  do, use BTM_BLE_CONN_PARAM_UNDEF for the ones want to
**                  leave untouched.
**
** Parameters:      bd_addr          - BD address of the peripheral
**                  min_conn_int     - minimum preferred connection interval
**                  max_conn_int     - maximum preferred connection interval
**                  slave_latency    - preferred slave latency
**                  supervision_tout - preferred supervision timeout
**
** Returns          void
**
*******************************************************************************/
extern  void BTM_BleSetPrefConnParams(BD_ADDR bd_addr,
                                      uint16_t min_conn_int,  uint16_t max_conn_int,
                                      uint16_t slave_latency, uint16_t supervision_tout);

/******************************************************************************
**
** Function         BTM_BleSetConnScanParams
**
** Description      Set scan parameters used in BLE connection request
**
** Parameters:      scan_interval    - scan interval
**                  scan_window      - scan window
**
** Returns          void
**
*******************************************************************************/
extern  void BTM_BleSetConnScanParams(uint32_t scan_interval, uint32_t scan_window);

/******************************************************************************
**
** Function         BTM_BleReadControllerFeatures
**
** Description      Reads BLE specific controller features
**
** Parameters:      tBTM_BLE_CTRL_FEATURES_CBACK : Callback to notify when features are read
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleReadControllerFeatures(tBTM_BLE_CTRL_FEATURES_CBACK  *p_vsc_cback);

/*******************************************************************************
**
** Function         BTM_CheckAdvData
**
** Description      This function is called to get ADV data for a specific type.
**
** Parameters       p_adv - pointer of ADV data
**                  type   - finding ADV data type
**                  p_length - return the length of ADV data not including type
**
** Returns          pointer of ADV data
**
*******************************************************************************/
extern  uint8_t *BTM_CheckAdvData(uint8_t *p_adv, uint8_t type, uint8_t *p_length);

/*******************************************************************************
**
** Function         BTM__BLEReadDiscoverability
**
** Description      This function is called to read the current LE discoverability
**                  mode of the device.
**
** Returns          BTM_BLE_NON_DISCOVERABLE ,BTM_BLE_LIMITED_DISCOVERABLE or
**                     BTM_BLE_GENRAL_DISCOVERABLE
**
*******************************************************************************/
uint16_t BTM_BleReadDiscoverability();

/*******************************************************************************
**
** Function         BTM__BLEReadConnectability
**
** Description      This function is called to read the current LE connectibility
**                  mode of the device.
**
** Returns          BTM_BLE_NON_CONNECTABLE or BTM_BLE_CONNECTABLE
**
*******************************************************************************/
extern uint16_t BTM_BleReadConnectability();

/*******************************************************************************
**
** Function         BTM_ReadDevInfo
**
** Description      This function is called to read the device/address type
**                  of BD address.
**
** Parameter        remote_bda: remote device address
**                  p_dev_type: output parameter to read the device type.
**                  p_addr_type: output parameter to read the address type.
**
*******************************************************************************/
extern void BTM_ReadDevInfo(BD_ADDR remote_bda, tBT_DEVICE_TYPE *p_dev_type,
                            tBLE_ADDR_TYPE *p_addr_type);


/*******************************************************************************
**
** Function         BTM_ReadConnectedTransportAddress
**
** Description      This function is called to read the paired device/address type of other device paired
**                  corresponding to the BD_address
**
** Parameter        remote_bda: remote device address, carry out the transport address
**                  transport: active transport
**
** Return           TRUE if an active link is identified; FALSE otherwise
**
*******************************************************************************/
extern uint8_t BTM_ReadConnectedTransportAddress(BD_ADDR remote_bda,
        tBT_TRANSPORT transport);

/*******************************************************************************
**
** Function         BTM_BleBroadcast
**
** Description      This function is to start or stop broadcasting.
**
** Parameters       start: start or stop broadcasting.
**                         duration: 0 for infinite. other, duration seconds for broadcast
**
** Returns          status.
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleBroadcast(uint8_t start, uint32_t duration);

/*******************************************************************************
**
** Function         BTM_BleConfigPrivacy
**
** Description      This function is called to enable or disable the privacy in
**                  the local device.
**
** Parameters       enable: TRUE to enable it; FALSE to disable it.
**
** Returns          uint8_t privacy mode set success; otherwise failed.
**
*******************************************************************************/
extern uint8_t BTM_BleConfigPrivacy(uint8_t enable);

/*******************************************************************************
**
** Function         BTM_BleLocalPrivacyEnabled
**
** Description        Checks if local device supports private address
**
** Returns          Return TRUE if local privacy is enabled else FALSE
**
*******************************************************************************/
extern uint8_t BTM_BleLocalPrivacyEnabled(void);

/*******************************************************************************
**
** Function         BTM_BleEnableMixedPrivacyMode
**
** Description      This function is called to enabled Mixed mode if privacy 1.2
**                  is applicable in controller.
**
** Parameters       mixed_on:  mixed mode to be used or not.
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleEnableMixedPrivacyMode(uint8_t mixed_on);

/*******************************************************************************
**
** Function          BTM_BleMaxMultiAdvInstanceCount
**
** Description        Returns max number of multi adv instances  supported by controller
**
** Returns          Max multi adv instance count
**
*******************************************************************************/
extern uint8_t  BTM_BleMaxMultiAdvInstanceCount();

/*******************************************************************************
**
** Function         BTM_BleSetConnectableMode
**
** Description      This function is called to set BLE connectable mode for a
**                  peripheral device.
**
** Parameters       connectable_mode:  directed connectable mode, or non-directed.It can
**                              be BTM_BLE_CONNECT_EVT, BTM_BLE_CONNECT_DIR_EVT or
**                              BTM_BLE_CONNECT_LO_DUTY_DIR_EVT
**
** Returns          BTM_ILLEGAL_VALUE if controller does not support BLE.
**                  BTM_SUCCESS is status set successfully; otherwise failure.
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleSetConnectableMode(tBTM_BLE_CONN_MODE connectable_mode);

/*******************************************************************************
**
** Function         BTM_BleTurnOnPrivacyOnRemote
**
** Description      This function is called to enable or disable the privacy on the
**                  remote device.
**
** Parameters       bd_addr: remote device address.
**                  privacy_on: TRUE to enable it; FALSE to disable it.
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleTurnOnPrivacyOnRemote(BD_ADDR bd_addr,
        uint8_t privacy_on);

/*******************************************************************************
**
** Function         BTM_BleUpdateAdvWhitelist
**
** Description      Add or remove device from advertising white list
**
** Returns          void
**
*******************************************************************************/
extern uint8_t BTM_BleUpdateAdvWhitelist(uint8_t add_remove, BD_ADDR emote_bda);

/*******************************************************************************
**
** Function         BTM_BleUpdateAdvFilterPolicy
**
** Description      This function update the filter policy of advertiser.
**
** Parameter        adv_policy: advertising filter policy
**
** Return           void
*******************************************************************************/
extern void BTM_BleUpdateAdvFilterPolicy(tBTM_BLE_AFP adv_policy);

/*******************************************************************************
**
** Function         BTM_BleReceiverTest
**
** Description      This function is called to start the LE Receiver test
**
** Parameter       rx_freq - Frequency Range
**               p_cmd_cmpl_cback - Command Complete callback
**
*******************************************************************************/
void BTM_BleReceiverTest(uint8_t rx_freq, tBTM_CMPL_CB *p_cmd_cmpl_cback);


/*******************************************************************************
**
** Function         BTM_BleTransmitterTest
**
** Description      This function is called to start the LE Transmitter test
**
** Parameter       tx_freq - Frequency Range
**                       test_data_len - Length in bytes of payload data in each packet
**                       packet_payload - Pattern to use in the payload
**                       p_cmd_cmpl_cback - Command Complete callback
**
*******************************************************************************/
void BTM_BleTransmitterTest(uint8_t tx_freq, uint8_t test_data_len,
                            uint8_t packet_payload, tBTM_CMPL_CB *p_cmd_cmpl_cback);

/*******************************************************************************
**
** Function         BTM_BleTestEnd
**
** Description      This function is called to stop the in-progress TX or RX test
**
** Parameter       p_cmd_cmpl_cback - Command complete callback
**
*******************************************************************************/
void BTM_BleTestEnd(tBTM_CMPL_CB *p_cmd_cmpl_cback);

/*******************************************************************************
**
** Function         BTM_UseLeLink
**
** Description      This function is to select the underneath physical link to use.
**
** Returns          TRUE to use LE, FALSE use BR/EDR.
**
*******************************************************************************/
extern uint8_t BTM_UseLeLink(BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         BTM_BleStackEnable
**
** Description      Enable/Disable BLE functionality on stack regarless controller
**                  capability.
**
** Parameters:      enable: TRUE to enable, FALSE to disable.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleStackEnable(uint8_t enable);

/*******************************************************************************
**
** Function         BTM_GetLeSecurityState
**
** Description      This function is called to get security mode 1 flags and
**                  encryption key size for LE peer.
**
** Returns          uint8_t TRUE if LE device is found, FALSE otherwise.
**
*******************************************************************************/
extern uint8_t BTM_GetLeSecurityState(BD_ADDR bd_addr,
                                      uint8_t *p_le_dev_sec_flags,
                                      uint8_t *p_le_key_size);

/*******************************************************************************
**
** Function         BTM_BleSecurityProcedureIsRunning
**
** Description      This function indicates if LE security procedure is
**                  currently running with the peer.
**
** Returns          uint8_t TRUE if security procedure is running, FALSE otherwise.
**
*******************************************************************************/
extern uint8_t BTM_BleSecurityProcedureIsRunning(BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         BTM_BleGetSupportedKeySize
**
** Description      This function gets the maximum encryption key size in bytes
**                  the local device can suport.
**                  record.
**
** Returns          the key size or 0 if the size can't be retrieved.
**
*******************************************************************************/
extern uint8_t BTM_BleGetSupportedKeySize(BD_ADDR bd_addr);

/*******************************************************************************/
/*                          Multi ADV API                                      */
/*******************************************************************************
**
** Function         BTM_BleEnableAdvInstance
**
** Description      This function enable a Multi-ADV instance with the specified
**                  adv parameters
**
** Parameters       p_params: pointer to the adv parameter structure, set as default
**                            adv parameter when the instance is enabled.
**                  p_cback: callback function for the adv instance.
**                  p_ref:  reference data attach to the adv instance to be enabled.
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleEnableAdvInstance(tBTM_BLE_ADV_PARAMS *p_params,
        tBTM_BLE_MULTI_ADV_CBACK *p_cback,
        void *p_ref);

/*******************************************************************************
**
** Function         BTM_BleUpdateAdvInstParam
**
** Description      This function update a Multi-ADV instance with the specififed
**                  adv parameters.
**
** Parameters       inst_id: adv instance ID
**                  p_params: pointer to the adv parameter structure.
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleUpdateAdvInstParam(uint8_t inst_id, tBTM_BLE_ADV_PARAMS *p_params);

/*******************************************************************************
**
** Function         BTM_BleCfgAdvInstData
**
** Description      This function configure a Multi-ADV instance with the specified
**                  adv data or scan response data.
**
** Parameters       inst_id: adv instance ID
**                  is_scan_rsp: is this scacn response, if no set as adv data.
**                  data_mask: adv data mask.
**                  p_data: pointer to the adv data structure.
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleCfgAdvInstData(uint8_t inst_id, uint8_t is_scan_rsp,
        tBTM_BLE_AD_MASK data_mask,
        tBTM_BLE_ADV_DATA *p_data);

/*******************************************************************************
**
** Function         BTM_BleDisableAdvInstance
**
** Description      This function disable a Multi-ADV instance.
**
** Parameters       inst_id: adv instance ID
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleDisableAdvInstance(uint8_t inst_id);

/*******************************************************************************
**
** Function         BTM_BleAdvFilterParamSetup
**
** Description      This function is called to setup the adv data payload filter
**                  condition.
**
** Parameters       p_target: enabble the filter condition on a target device; if NULL
**                            enable the generic scan condition.
**                  enable: enable or disable the filter condition
**
** Returns          void
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleAdvFilterParamSetup(int action,
        tBTM_BLE_PF_FILT_INDEX filt_index,
        tBTM_BLE_PF_FILT_PARAMS *p_filt_params,
        tBLE_BD_ADDR *p_target, tBTM_BLE_PF_PARAM_CBACK *p_cmpl_cback,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleCfgFilterCondition
**
** Description      This function is called to configure the adv data payload filter
**                  condition.
**
** Parameters       action: to read/write/clear
**                  cond_type: filter condition type.
**                  p_cond: filter condition paramter
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleCfgFilterCondition(tBTM_BLE_SCAN_COND_OP action,
        tBTM_BLE_PF_COND_TYPE cond_type,
        tBTM_BLE_PF_FILT_INDEX filt_index,
        tBTM_BLE_PF_COND_PARAM *p_cond,
        tBTM_BLE_PF_CFG_CBACK *p_cmpl_cback,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleEnableDisableFilterFeature
**
** Description      This function is called to enable or disable the APCF feature
**
** Parameters       enable - TRUE - enables the APCF, FALSE - disables the APCF
**                       ref_value - Ref value
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleEnableDisableFilterFeature(uint8_t enable,
        tBTM_BLE_PF_STATUS_CBACK *p_stat_cback,
        tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleGetEnergyInfo
**
** Description      This function obtains the energy info
**
** Parameters       p_ener_cback - Callback pointer
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleGetEnergyInfo(tBTM_BLE_ENERGY_INFO_CBACK *p_ener_cback);

/*******************************************************************************
**
** Function         BTM_SetBleDataLength
**
** Description      This function is called to set maximum BLE transmission packet size
**
** Returns          BTM_SUCCESS if success; otherwise failed.
**
*******************************************************************************/
extern tBTM_STATUS BTM_SetBleDataLength(BD_ADDR bd_addr, uint16_t tx_pdu_length);

#ifdef __cplusplus
}
#endif

#endif

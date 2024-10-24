/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

#ifndef BT_HCI_LIB_H
#define BT_HCI_LIB_H

#include "data_types.h"

/** Struct types */


/** Typedefs and defines */

/* Generic purpose transac returned upon request complete */
typedef void *TRANSAC;

/** Bluetooth Power Control States */
typedef enum {
    BT_HC_CHIP_PWR_OFF,
    BT_HC_CHIP_PWR_ON,
}  bt_hc_chip_power_state_t;

/** Bluetooth Low Power Mode */
typedef enum {
    BT_HC_LPM_DISABLE,
    BT_HC_LPM_ENABLE,
    BT_HC_LPM_WAKE_ASSERT,
    BT_HC_LPM_WAKE_DEASSERT,
} bt_hc_low_power_event_t;

/** Receive flow control */
typedef enum {
    BT_RXFLOW_OFF, /* add transport device fd to select set */
    BT_RXFLOW_ON,  /* remove transport device to from select set */
} bt_rx_flow_state_t;

/** HCI logging control */
typedef enum {
    BT_HC_LOGGING_OFF,
    BT_HC_LOGGING_ON,
} bt_hc_logging_state_t;

/* commands to be used in LSB with MSG_CTRL_TO_HC_CMD */
typedef enum {
    BT_HC_AUDIO_STATE = 0,
    BT_HC_CMD_MAX
} bt_hc_tx_cmd_t;
/** Result of write request */
typedef enum {
    BT_HC_TX_SUCCESS,  /* a buffer is fully processed and can be released */
    BT_HC_TX_FAIL,     /* transmit fail */
    BT_HC_TX_FRAGMENT, /* send split ACL pkt back to stack to reprocess */
} bt_hc_transmit_result_t;

/** Result of preload initialization */
typedef enum {
    BT_HC_PRELOAD_SUCCESS,
    BT_HC_PRELOAD_FAIL,
} bt_hc_preload_result_t;

/** Result of postload initialization */
typedef enum {
    BT_HC_POSTLOAD_SUCCESS,
    BT_HC_POSTLOAD_FAIL,
} bt_hc_postload_result_t;

/** Result of low power enable/disable request */
typedef enum {
    BT_HC_LPM_DISABLED,
    BT_HC_LPM_ENABLED,
} bt_hc_lpm_request_result_t;

/** Host/Controller Library Return Status */
typedef enum {
    BT_HC_STATUS_SUCCESS,
    BT_HC_STATUS_FAIL,
    BT_HC_STATUS_NOT_READY,
    BT_HC_STATUS_NOMEM,
    BT_HC_STATUS_BUSY,
    BT_HC_STATUS_CORRUPTED_BUFFER
} bt_hc_status_t;


/* Section comment */

/*
 * Bluetooth Host/Controller callback structure.
 */

/* called upon bt host wake signal */
typedef void (*hostwake_ind_cb)(bt_hc_low_power_event_t event);

/* preload initialization callback */
typedef void (*preload_result_cb)(TRANSAC transac, bt_hc_preload_result_t result);

/* postload initialization callback */
typedef void (*postload_result_cb)(TRANSAC transac, bt_hc_postload_result_t result);

/* lpm enable/disable callback */
typedef void (*lpm_result_cb)(bt_hc_lpm_request_result_t result);

/* datapath buffer allocation callback (callout) */
typedef char *(*alloc_mem_cb)(int size);

typedef char *(*alloc_mem_pool_cb)(int pool_id);

/* datapath buffer deallocation callback (callout) */
typedef void (*dealloc_mem_cb)(TRANSAC transac);

/* transmit result callback */
typedef int (*tx_result_cb)(TRANSAC transac, char *p_buf, bt_hc_transmit_result_t result);

/* a previously setup buffer is read and available for processing
   buffer is deallocated in stack when processed */
typedef int (*data_ind_cb)(TRANSAC transac, char *p_buf, int len);

typedef struct {
    /** set to sizeof(bt_hc_callbacks_t) */
    unsigned int         size;

    /* notifies caller result of preload request */
    preload_result_cb  preload_cb;

    /* notifies caller result of postload request */
    postload_result_cb  postload_cb;

    /* notifies caller result of lpm enable/disable */
    lpm_result_cb  lpm_cb;

    /* notifies hardware on host wake state */
    hostwake_ind_cb       hostwake_ind;

    /* buffer allocation request */
    alloc_mem_cb   alloc;

    /*buffer allocation from pool request*/
    alloc_mem_pool_cb pool_alloc;

    /* buffer deallocation request */
    dealloc_mem_cb dealloc;

    /* notifies stack data is available */
    data_ind_cb data_ind;

    /* notifies caller when a buffer is transmitted (or failed) */
    tx_result_cb  tx_result;
} bt_hc_callbacks_t;

/*
 * Bluetooth Host/Controller Interface
 */
typedef struct {
    /** Set to sizeof(bt_hc_interface_t) */
    unsigned int          size;

    /**
     * Opens the interface and provides the callback routines
     * to the implemenation of this interface.
     */
    int (*init)(const bt_hc_callbacks_t *p_cb, unsigned char *local_bdaddr);

    /** Chip power control */
    void (*set_power)(bt_hc_chip_power_state_t state);

    /** Set low power mode wake */
    int (*lpm)(bt_hc_low_power_event_t event);

    /** Called prior to stack initialization */
    void (*preload)(TRANSAC transac);

    /** Called post stack initialization */
    void (*postload)(TRANSAC transac);

    /** Transmit buffer */
    int (*transmit_buf)(TRANSAC transac, char *p_buf, int len);

    /** Controls HCI logging on/off */
    int (*logging)(bt_hc_logging_state_t state, char *p_path, int save_existing);

    /** Closes the interface */
    void (*cleanup)(void);

    /** sends commands to hc layer (e.g. SCO state) */
    int (*tx_cmd)(TRANSAC transac, char *p_buf, int len);
} bt_hc_interface_t;


/*
 * External shared lib functions
 */

extern const bt_hc_interface_t *bt_hc_get_interface(void);

#endif /* BT_HCI_LIB_H */


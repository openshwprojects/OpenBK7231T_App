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

/*******************************************************************************
 *
 *  Filename:      btif_av.h
 *
 *  Description:   Main API header file for all BTIF AV functions accessed
 *                 from internal stack.
 *
 *******************************************************************************/

#ifndef BTIF_AV_H
#define BTIF_AV_H

#include "btif_common.h"
#include "btif_sm.h"
#include "bta_av_api.h"


/*******************************************************************************
**  Type definitions for callback functions
********************************************************************************/

typedef enum {
    /* Reuse BTA_AV_XXX_EVT - No need to redefine them here */
    BTIF_AV_CONNECT_REQ_EVT = BTA_AV_MAX_EVT,
    BTIF_AV_DISCONNECT_REQ_EVT,
    BTIF_AV_START_STREAM_REQ_EVT,
    BTIF_AV_STOP_STREAM_REQ_EVT,
    BTIF_AV_SUSPEND_STREAM_REQ_EVT,
    BTIF_AV_SINK_CONFIG_REQ_EVT,
    BTIF_AV_OFFLOAD_START_REQ_EVT,
    BTIF_AV_SINK_FOCUS_REQ_EVT,
    BTIF_AV_CLEANUP_REQ_EVT,
} btif_av_sm_event_t;


/*******************************************************************************
**  BTIF AV API
********************************************************************************/

/*******************************************************************************
**
** Function         btif_av_get_sm_handle
**
** Description      Fetches current av SM handle
**
** Returns          None
**
*******************************************************************************/

btif_sm_handle_t btif_av_get_sm_handle(void);

/*******************************************************************************
**
** Function         btif_av_get_addr
**
** Description      Fetches current AV BD address
**
** Returns          BD address
**
*******************************************************************************/

tls_bt_addr_t btif_av_get_addr(void);

/*******************************************************************************
** Function         btif_av_is_sink_enabled
**
** Description      Checks if A2DP Sink is enabled or not
**
** Returns          TRUE if A2DP Sink is enabled, false otherwise
**
*******************************************************************************/

uint8_t btif_av_is_sink_enabled(void);

/*******************************************************************************
**
** Function         btif_av_stream_ready
**
** Description      Checks whether AV is ready for starting a stream
**
** Returns          None
**
*******************************************************************************/

uint8_t btif_av_stream_ready(void);

/*******************************************************************************
**
** Function         btif_av_stream_started_ready
**
** Description      Checks whether AV ready for media start in streaming state
**
** Returns          None
**
*******************************************************************************/

uint8_t btif_av_stream_started_ready(void);

/*******************************************************************************
**
** Function         btif_dispatch_sm_event
**
** Description      Send event to AV statemachine
**
** Returns          None
**
*******************************************************************************/

/* used to pass events to AV statemachine from other tasks */
void btif_dispatch_sm_event(btif_av_sm_event_t event, void *p_data, int len);

/*******************************************************************************
**
** Function         btif_av_init
**
** Description      Initializes btif AV if not already done
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_av_init(int service_id);
tls_bt_status_t btif_av_cleanup(int service_id);

/*******************************************************************************
**
** Function         btif_av_is_connected
**
** Description      Checks if av has a connected sink
**
** Returns          uint8_t
**
*******************************************************************************/

uint8_t btif_av_is_connected(void);


/*******************************************************************************
**
** Function         btif_av_is_peer_edr
**
** Description      Check if the connected a2dp device supports
**                  EDR or not. Only when connected this function
**                  will accurately provide a true capability of
**                  remote peer. If not connected it will always be false.
**
** Returns          TRUE if remote device is capable of EDR
**
*******************************************************************************/

uint8_t btif_av_is_peer_edr(void);

#ifdef USE_AUDIO_TRACK
/*******************************************************************************
**
** Function         audio_focus_status
**
** Description      Update Audio Focus State
**
** Returns          None
**
*******************************************************************************/
void audio_focus_status(int state);

/*******************************************************************************
**
** Function         btif_queue_focus_request
**
** Description      This is used to move context to btif and
**                  queue audio_focus_request
**
** Returns          none
**
*******************************************************************************/
void btif_queue_focus_request(void);
#endif

/******************************************************************************
**
** Function         btif_av_clear_remote_suspend_flag
**
** Description      Clears remote suspended flag
**
** Returns          Void
********************************************************************************/
void btif_av_clear_remote_suspend_flag(void);

/*******************************************************************************
**
** Function         btif_av_peer_supports_3mbps
**
** Description      Check if the connected A2DP device supports
**                  3 Mbps EDR. This function will only work while connected.
**                  If not connected it will always return false.
**
** Returns          TRUE if remote device is EDR and supports 3 Mbps
**
*******************************************************************************/
uint8_t btif_av_peer_supports_3mbps(void);

#endif /* BTIF_AV_H */

/******************************************************************************
 *
 *  Copyright (C) 1998-2012 Broadcom Corporation
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
 *  This file contains the HeaLth device profile main functions and state
 *  machine.
 *
 ******************************************************************************/
#include <string.h>

#include "bt_target.h"
#if defined(BTA_HL_INCLUDED) && (BTA_HL_INCLUDED == TRUE)



#include "bta_hl_api.h"
#include "bta_hl_int.h"
#include "bt_common.h"
#include "utl.h"
#include "l2c_api.h"
#include "mca_defs.h"


#if (BTA_HL_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
static char *bta_hl_cch_state_code(tBTA_HL_CCH_STATE state_code);
static char *bta_hl_dch_state_code(tBTA_HL_DCH_STATE state_code);
#endif

extern uint16_t L2CA_AllocateRandomPsm(void);
extern uint16_t L2CA_AllocatePsm(void);
/*****************************************************************************
** DCH State Table
*****************************************************************************/
/*****************************************************************************
** Constants and types
*****************************************************************************/
/* state machine action enumeration list for DCH */
/* The order of this enumeration must be the same as bta_hl_dch_act_tbl[] */
enum {
    BTA_HL_DCH_MCA_CREATE,
    BTA_HL_DCH_MCA_CREATE_CFM,
    BTA_HL_DCH_MCA_CREATE_IND,
    BTA_HL_DCH_MCA_OPEN_CFM,
    BTA_HL_DCH_MCA_OPEN_IND,
    BTA_HL_DCH_MCA_CLOSE,
    BTA_HL_DCH_MCA_CLOSE_CFM,
    BTA_HL_DCH_MCA_CLOSE_IND,
    BTA_HL_DCH_CLOSE_CMPL,
    BTA_HL_DCH_MCA_RCV_DATA,

    BTA_HL_DCH_SDP_INIT,
    BTA_HL_DCH_MCA_RECONNECT,
    BTA_HL_DCH_MCA_RECONNECT_IND,
    BTA_HL_DCH_MCA_RECONNECT_CFM,
    BTA_HL_DCH_CLOSE_ECHO_TEST,
    BTA_HL_DCH_CREATE_RSP,
    BTA_HL_DCH_MCA_ABORT,
    BTA_HL_DCH_MCA_ABORT_IND,
    BTA_HL_DCH_MCA_ABORT_CFM,
    BTA_HL_DCH_MCA_CONG_CHANGE,

    BTA_HL_DCH_SDP_FAIL,
    BTA_HL_DCH_SEND_DATA,
    BTA_HL_DCH_CI_GET_TX_DATA,
    BTA_HL_DCH_CI_PUT_RX_DATA,
    BTA_HL_DCH_CI_GET_ECHO_DATA,
    BTA_HL_DCH_ECHO_TEST,
    BTA_HL_DCH_CI_PUT_ECHO_DATA,
    BTA_HL_DCH_IGNORE
};

typedef void (*tBTA_HL_DCH_ACTION)(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                   tBTA_HL_DATA *p_data);

static const tBTA_HL_DCH_ACTION bta_hl_dch_action[] = {
    bta_hl_dch_mca_create,
    bta_hl_dch_mca_create_cfm,
    bta_hl_dch_mca_create_ind,
    bta_hl_dch_mca_open_cfm,
    bta_hl_dch_mca_open_ind,
    bta_hl_dch_mca_close,
    bta_hl_dch_mca_close_cfm,
    bta_hl_dch_mca_close_ind,
    bta_hl_dch_close_cmpl,
    bta_hl_dch_mca_rcv_data,

    bta_hl_dch_sdp_init,
    bta_hl_dch_mca_reconnect,
    bta_hl_dch_mca_reconnect_ind,
    bta_hl_dch_mca_reconnect_cfm,
    bta_hl_dch_close_echo_test,
    bta_hl_dch_create_rsp,
    bta_hl_dch_mca_abort,
    bta_hl_dch_mca_abort_ind,
    bta_hl_dch_mca_abort_cfm,
    bta_hl_dch_mca_cong_change,

    bta_hl_dch_sdp_fail,
    bta_hl_dch_send_data,
    bta_hl_dch_ci_get_tx_data,
    bta_hl_dch_ci_put_rx_data,
    bta_hl_dch_ci_get_echo_data,
    bta_hl_dch_echo_test,
    bta_hl_dch_ci_put_echo_data,
};


/* state table information */
#define BTA_HL_DCH_ACTIONS             1       /* number of actions */
#define BTA_HL_DCH_ACTION_COL          0       /* position of action */
#define BTA_HL_DCH_NEXT_STATE          1       /* position of next state */
#define BTA_HL_DCH_NUM_COLS            2       /* number of columns in state tables */

/* state table for idle state */
static const uint8_t bta_hl_dch_st_idle[][BTA_HL_DCH_NUM_COLS] = {
    /* Event                                Action 1                    Next state */
    /* BTA_HL_DCH_SDP_INIT_EVT   */     {BTA_HL_DCH_SDP_INIT,           BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_OPEN_EVT       */     {BTA_HL_DCH_MCA_CREATE,         BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_CREATE_IND_EVT */     {BTA_HL_DCH_MCA_CREATE_IND,     BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_CREATE_CFM_EVT */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_OPEN_IND_EVT   */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},

    /* BTA_HL_MCA_OPEN_CFM_EVT   */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_CLOSE_EVT      */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_CLOSE_IND_EVT  */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_CLOSE_CFM_EVT  */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_API_SEND_DATA_EVT  */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},

    /* BTA_HL_MCA_RCV_DATA_EVT   */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_CLOSE_CMPL_EVT */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_RECONNECT_EVT  */     {BTA_HL_DCH_MCA_RECONNECT,      BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_SDP_FAIL_EVT   */     {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_RECONNECT_IND_EVT*/   {BTA_HL_DCH_MCA_RECONNECT_IND,  BTA_HL_DCH_OPENING_ST},

    /* BTA_HL_MCA_RECONNECT_CFM_EVT*/   {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_CLOSE_ECHO_TEST_EVT*/ {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_API_DCH_CREATE_RSP_EVT */ {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_ABORT_EVT */          {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_ABORT_IND_EVT */      {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},

    /* BTA_HL_MCA_ABORT_CFM_EVT */      {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_CONG_CHG_EVT */       {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_CI_GET_TX_DATA_EVT  */    {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_CI_PUT_RX_DATA_EVT  */    {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_CI_GET_ECHO_DATA_EVT  */  {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_ECHO_TEST_EVT  */     {BTA_HL_DCH_ECHO_TEST,          BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_CI_PUT_ECHO_DATA_EVT  */  {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_IDLE_ST}
};

/* state table for opening state */
static const uint8_t bta_hl_dch_st_opening[][BTA_HL_DCH_NUM_COLS] = {
    /* Event                                Action 1                    Next state */
    /* BTA_HL_DCH_SDP_INIT_EVT   */   {BTA_HL_DCH_SDP_INIT,             BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_OPEN_EVT       */   {BTA_HL_DCH_MCA_CREATE,           BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_CREATE_IND_EVT */   {BTA_HL_DCH_IGNORE,               BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_CREATE_CFM_EVT */   {BTA_HL_DCH_MCA_CREATE_CFM,       BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_OPEN_IND_EVT   */   {BTA_HL_DCH_MCA_OPEN_IND,         BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_OPEN_CFM_EVT   */   {BTA_HL_DCH_MCA_OPEN_CFM,         BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_CLOSE_EVT      */   {BTA_HL_DCH_IGNORE,               BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_CLOSE_IND_EVT  */   {BTA_HL_DCH_MCA_CLOSE_IND,        BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CLOSE_CFM_EVT  */   {BTA_HL_DCH_MCA_CLOSE_CFM,        BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_API_SEND_DATA_EVT  */   {BTA_HL_DCH_IGNORE,               BTA_HL_DCH_OPEN_ST},

    /* BTA_HL_MCA_RCV_DATA_EVT   */   {BTA_HL_DCH_IGNORE,               BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_CLOSE_CMPL_EVT */   {BTA_HL_DCH_CLOSE_CMPL,           BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_RECONNECT_EVT  */   {BTA_HL_DCH_MCA_RECONNECT,        BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_SDP_FAIL_EVT   */   {BTA_HL_DCH_SDP_FAIL,             BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_RECONNECT_IND_EVT*/ {BTA_HL_DCH_IGNORE,               BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_MCA_RECONNECT_CFM_EVT*/ {BTA_HL_DCH_MCA_RECONNECT_CFM,    BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_CLOSE_ECHO_TEST_EVT*/ {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_API_DCH_CREATE_RSP_EVT */ {BTA_HL_DCH_CREATE_RSP,         BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_ABORT_EVT */          {BTA_HL_DCH_MCA_ABORT,          BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_ABORT_IND_EVT */      {BTA_HL_DCH_MCA_ABORT_IND,      BTA_HL_DCH_OPENING_ST},

    /* BTA_HL_MCA_ABORT_CFM_EVT */      {BTA_HL_DCH_MCA_ABORT_CFM,      BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_MCA_CONG_CHG_EVT */       {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_CI_GET_TX_DATA_EVT  */    {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_CI_PUT_RX_DATA_EVT  */    {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_CI_GET_ECHO_DATA_EVT  */  {BTA_HL_DCH_CI_GET_ECHO_DATA,   BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_DCH_ECHO_TEST_EVT  */     {BTA_HL_DCH_ECHO_TEST,          BTA_HL_DCH_OPENING_ST},
    /* BTA_HL_CI_PUT_ECHO_DATA_EVT  */  {BTA_HL_DCH_IGNORE,             BTA_HL_DCH_OPENING_ST}
};

/* state table for open state */
static const uint8_t bta_hl_dch_st_open[][BTA_HL_DCH_NUM_COLS] = {
    /* Event                                Action 1                  Next state */
    /* BTA_HL_DCH_SDP_INIT_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_OPEN_EVT       */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_CREATE_IND_EVT */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_CREATE_CFM_EVT */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_OPEN_IND_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_OPEN_CFM_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_CLOSE_EVT      */     {BTA_HL_DCH_MCA_CLOSE,        BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CLOSE_IND_EVT  */     {BTA_HL_DCH_MCA_CLOSE_IND,    BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CLOSE_CFM_EVT  */     {BTA_HL_DCH_MCA_CLOSE_CFM,    BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_API_SEND_DATA_EVT  */     {BTA_HL_DCH_SEND_DATA,        BTA_HL_DCH_OPEN_ST},

    /* BTA_HL_MCA_RCV_DATA_EVT   */     {BTA_HL_DCH_MCA_RCV_DATA,     BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_CLOSE_CMPL_EVT */     {BTA_HL_DCH_CLOSE_CMPL,       BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_RECONNECT_EVT  */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_SDP_FAIL_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_RECONNECT_IND_EVT*/   {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_RECONNECT_CFM_EVT*/   {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_CLOSE_ECHO_TEST_EVT*/ {BTA_HL_DCH_CLOSE_ECHO_TEST,  BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_API_DCH_CREATE_RSP_EVT */ {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_ABORT_EVT */          {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_ABORT_IND_EVT */      {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},

    /* BTA_HL_DCH_ABORT_CFM_EVT */      {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_MCA_CONG_CHG_EVT */       {BTA_HL_DCH_MCA_CONG_CHANGE,  BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_CI_GET_TX_DATA_EVT  */    {BTA_HL_DCH_CI_GET_TX_DATA,   BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_CI_PUT_RX_DATA_EVT  */    {BTA_HL_DCH_CI_PUT_RX_DATA,   BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_CI_GET_ECHO_DATA_EVT  */  {BTA_HL_DCH_CI_GET_ECHO_DATA, BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_DCH_ECHO_TEST_EVT  */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_OPEN_ST},
    /* BTA_HL_CI_PUT_ECHO_DATA_EVT  */  {BTA_HL_DCH_CI_PUT_ECHO_DATA, BTA_HL_DCH_OPEN_ST}
};


/* state table for closing state */
static const uint8_t bta_hl_dch_st_closing[][BTA_HL_DCH_NUM_COLS] = {
    /* Event                                Action 1                  Next state */
    /* BTA_HL_DCH_SDP_INIT_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_OPEN_EVT       */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CREATE_IND_EVT */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CREATE_CFM_EVT */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_OPEN_IND_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_OPEN_CFM_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_CLOSE_EVT      */     {BTA_HL_DCH_MCA_CLOSE,        BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CLOSE_IND_EVT  */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CLOSE_CFM_EVT  */     {BTA_HL_DCH_MCA_CLOSE_CFM,    BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_API_SEND_DATA_EVT  */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},

    /* BTA_HL_MCA_RCV_DATA_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_CLOSE_CMPL_EVT */     {BTA_HL_DCH_CLOSE_CMPL,       BTA_HL_DCH_IDLE_ST},
    /* BTA_HL_DCH_RECONNECT_EVT  */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_SDP_FAIL_EVT   */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_RECONNECT_IND_EVT*/   {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_RECONNECT_CFM_EVT*/   {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_CLOSE_ECHO_TEST_EVT*/ {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_API_DCH_CREATE_RSP_EVT */ {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_ABORT_EVT */          {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_ABORT_IND_EVT */      {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},

    /* BTA_HL_DCH_ABORT_CFM_EVT */      {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_MCA_CONG_CHG_EVT */       {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_CI_GET_TX_DATA_EVT  */    {BTA_HL_DCH_CI_GET_TX_DATA,   BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_CI_PUT_RX_DATA_EVT  */    {BTA_HL_DCH_CI_PUT_RX_DATA,   BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_CI_GET_ECHO_DATA_EVT  */  {BTA_HL_DCH_CI_GET_ECHO_DATA, BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_DCH_ECHO_TEST_EVT  */     {BTA_HL_DCH_IGNORE,           BTA_HL_DCH_CLOSING_ST},
    /* BTA_HL_CI_PUT_ECHO_DATA_EVT  */  {BTA_HL_DCH_CI_PUT_ECHO_DATA, BTA_HL_DCH_CLOSING_ST}
};

/* type for state table */
typedef const uint8_t (*tBTA_HL_DCH_ST_TBL)[BTA_HL_DCH_NUM_COLS];

/* state table */
const tBTA_HL_DCH_ST_TBL bta_hl_dch_st_tbl[] = {
    bta_hl_dch_st_idle,
    bta_hl_dch_st_opening,
    bta_hl_dch_st_open,
    bta_hl_dch_st_closing
};

/*****************************************************************************
** CCH State Table
*****************************************************************************/
/*****************************************************************************
** Constants and types
*****************************************************************************/
/* state machine action enumeration list for CCH */
enum {
    BTA_HL_CCH_SDP_INIT,
    BTA_HL_CCH_MCA_OPEN,
    BTA_HL_CCH_MCA_CLOSE,
    BTA_HL_CCH_CLOSE_CMPL,
    BTA_HL_CCH_MCA_CONNECT,
    BTA_HL_CCH_MCA_DISCONNECT,
    BTA_HL_CCH_MCA_RSP_TOUT,
    BTA_HL_CCH_MCA_DISC_OPEN,
    BTA_HL_CCH_IGNORE
};

/* type for action functions */
typedef void (*tBTA_HL_CCH_ACTION)(uint8_t app_idx, uint8_t mcl_idx, tBTA_HL_DATA *p_data);

/* action function list for MAS */
const tBTA_HL_CCH_ACTION bta_hl_cch_action[] = {
    bta_hl_cch_sdp_init,
    bta_hl_cch_mca_open,
    bta_hl_cch_mca_close,
    bta_hl_cch_close_cmpl,
    bta_hl_cch_mca_connect,
    bta_hl_cch_mca_disconnect,
    bta_hl_cch_mca_rsp_tout,
    bta_hl_cch_mca_disc_open
};


/* state table information */
#define BTA_HL_CCH_ACTIONS             1       /* number of actions */
#define BTA_HL_CCH_NEXT_STATE          1       /* position of next state */
#define BTA_HL_CCH_NUM_COLS            2       /* number of columns in state tables */


/* state table for MAS idle state */
static const uint8_t bta_hl_cch_st_idle[][BTA_HL_CCH_NUM_COLS] = {
    /* Event                          Action 1                  Next state */
    /* BTA_HL_CCH_OPEN_EVT           */ {BTA_HL_CCH_SDP_INIT,       BTA_HL_CCH_OPENING_ST},
    /* BTA_HL_CCH_SDP_OK_EVT         */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_CCH_SDP_FAIL_EVT       */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_MCA_CONNECT_IND_EVT    */ {BTA_HL_CCH_MCA_CONNECT,    BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_MCA_DISCONNECT_IND_EVT */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_CCH_CLOSE_EVT          */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_CCH_CLOSE_CMPL_EVT     */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_CCH_CLOSE_CMPL_EVT     */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_IDLE_ST}
};

/* state table for obex/rfcomm connection state */
static const uint8_t bta_hl_cch_st_opening[][BTA_HL_CCH_NUM_COLS] = {
    /* Event                          Action 1               Next state */
    /* BTA_HL_CCH_OPEN_EVT           */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_OPENING_ST},
    /* BTA_HL_CCH_SDP_OK_EVT         */ {BTA_HL_CCH_MCA_OPEN,       BTA_HL_CCH_OPENING_ST},
    /* BTA_HL_CCH_SDP_FAIL_EVT       */ {BTA_HL_CCH_CLOSE_CMPL,     BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_MCA_CONNECT_IND_EVT    */ {BTA_HL_CCH_MCA_CONNECT,    BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_MCA_DISCONNECT_IND_EVT */ {BTA_HL_CCH_MCA_DISCONNECT, BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_CLOSE_EVT          */ {BTA_HL_CCH_MCA_CLOSE,      BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_CLOSE_CMPL_EVT     */ {BTA_HL_CCH_CLOSE_CMPL,     BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_MCA_RSP_TOUT_IND_EVT   */ {BTA_HL_CCH_MCA_RSP_TOUT,   BTA_HL_CCH_CLOSING_ST}
};

/* state table for open state */
static const uint8_t bta_hl_cch_st_open[][BTA_HL_CCH_NUM_COLS] = {
    /* Event                          Action 1                  Next state */
    /* BTA_HL_CCH_OPEN_EVT           */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_CCH_SDP_OK_EVT         */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_CCH_SDP_FAIL_EVT       */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_MCA_CONNECT_IND_EVT    */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_MCA_DISCONNECT_IND_EVT */ {BTA_HL_CCH_MCA_DISCONNECT, BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_CLOSE_EVT          */ {BTA_HL_CCH_MCA_CLOSE,      BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_CLOSE_CMPL_EVT     */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_OPEN_ST},
    /* BTA_HL_MCA_RSP_TOUT_IND_EVT   */ {BTA_HL_CCH_MCA_RSP_TOUT,   BTA_HL_CCH_CLOSING_ST}
};

/* state table for closing state */
static const uint8_t bta_hl_cch_st_closing[][BTA_HL_CCH_NUM_COLS] = {
    /* Event                          Action 1                  Next state */
    /* BTA_HL_CCH_OPEN_EVT           */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_SDP_OK_EVT         */ {BTA_HL_CCH_CLOSE_CMPL,     BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_CCH_SDP_FAIL_EVT       */ {BTA_HL_CCH_CLOSE_CMPL,     BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_MCA_CONNECT_IND_EVT    */ {BTA_HL_CCH_MCA_DISC_OPEN,  BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_MCA_DISCONNECT_IND_EVT */ {BTA_HL_CCH_MCA_DISCONNECT, BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_CLOSE_EVT          */ {BTA_HL_CCH_MCA_CLOSE,      BTA_HL_CCH_CLOSING_ST},
    /* BTA_HL_CCH_CLOSE_CMPL_EVT     */ {BTA_HL_CCH_CLOSE_CMPL,     BTA_HL_CCH_IDLE_ST},
    /* BTA_HL_MCA_RSP_TOUT_IND_EVT   */ {BTA_HL_CCH_IGNORE,         BTA_HL_CCH_CLOSING_ST}
};

/* type for state table CCH */
typedef const uint8_t (*tBTA_HL_CCH_ST_TBL)[BTA_HL_CCH_NUM_COLS];

/* MAS state table */
const tBTA_HL_CCH_ST_TBL bta_hl_cch_st_tbl[] = {
    bta_hl_cch_st_idle,
    bta_hl_cch_st_opening,
    bta_hl_cch_st_open,
    bta_hl_cch_st_closing
};


/*****************************************************************************
** Global data
*****************************************************************************/

/* HL control block */
#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_HL_CB  bta_hl_cb;
#endif


/*******************************************************************************
**
** Function         bta_hl_cch_sm_execute
**
** Description      State machine event handling function for CCH
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_sm_execute(uint8_t app_idx, uint8_t mcl_idx,
                           uint16_t event, tBTA_HL_DATA *p_data)
{
    tBTA_HL_CCH_ST_TBL  state_table;
    uint8_t               action;
    int                 i;
    tBTA_HL_MCL_CB      *p_cb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
#if (BTA_HL_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    tBTA_HL_CCH_STATE in_state = p_cb->cch_state;
    uint16_t             cur_evt = event;
    APPL_TRACE_DEBUG("HDP CCH Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                     bta_hl_cch_state_code(in_state),
                     bta_hl_evt_code(cur_evt));
#endif
    /* look up the state table for the current state */
    state_table = bta_hl_cch_st_tbl[p_cb->cch_state];
    event &= 0x00FF;
    /* set next state */
    p_cb->cch_state = state_table[event][BTA_HL_CCH_NEXT_STATE];

    for(i = 0; i < BTA_HL_CCH_ACTIONS; i++) {
        if((action = state_table[event][i]) != BTA_HL_CCH_IGNORE) {
            (*bta_hl_cch_action[action])(app_idx, mcl_idx, p_data);
        } else {
            /* discard HDP data */
            bta_hl_discard_data(p_data->hdr.event, p_data);
            break;
        }
    }

#if (BTA_HL_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)

    if(in_state != p_cb->cch_state) {
        APPL_TRACE_DEBUG("HL CCH State Change: [%s] -> [%s] after [%s]",
                         bta_hl_cch_state_code(in_state),
                         bta_hl_cch_state_code(p_cb->cch_state),
                         bta_hl_evt_code(cur_evt));
    }

#endif
}

/*******************************************************************************
**
** Function         bta_hl_dch_sm_execute
**
** Description      State machine event handling function for DCH
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_sm_execute(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                           uint16_t event, tBTA_HL_DATA *p_data)
{
    tBTA_HL_DCH_ST_TBL  state_table;
    uint8_t               action;
    int                 i;
    tBTA_HL_MDL_CB      *p_cb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)
    tBTA_HL_DCH_STATE in_state = p_cb->dch_state;
    uint16_t             cur_evt = event;
    APPL_TRACE_DEBUG("HDP DCH Event Handler: State 0x%02x [%s], Event [%s]", in_state,
                     bta_hl_dch_state_code(in_state),
                     bta_hl_evt_code(cur_evt));
#endif
    /* look up the state table for the current state */
    state_table = bta_hl_dch_st_tbl[p_cb->dch_state];
    event -= BTA_HL_DCH_EVT_MIN;
    /* set next state */
    p_cb->dch_state = state_table[event][BTA_HL_DCH_NEXT_STATE];

    for(i = 0; i < BTA_HL_DCH_ACTIONS; i++) {
        if((action = state_table[event][i]) != BTA_HL_DCH_IGNORE) {
            (*bta_hl_dch_action[action])(app_idx, mcl_idx, mdl_idx, p_data);
        } else {
            /* discard mas data */
            bta_hl_discard_data(p_data->hdr.event, p_data);
            break;
        }
    }

#if (BTA_HL_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)

    if(in_state != p_cb->dch_state) {
        APPL_TRACE_DEBUG("HL DCH State Change: [%s] -> [%s] after [%s]",
                         bta_hl_dch_state_code(in_state),
                         bta_hl_dch_state_code(p_cb->dch_state),
                         bta_hl_evt_code(cur_evt));
    }

#endif
}
/*******************************************************************************
**
** Function         bta_hl_api_enable
**
** Description      Process the API enable request to enable the HL subsystem
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_enable(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL_CTRL    evt_data;

    /* If already enabled then reject this request */
    if(p_cb->enable) {
        APPL_TRACE_ERROR("HL is already enabled");
        evt_data.enable_cfm.status = BTA_HL_STATUS_FAIL;

        if(p_data->api_enable.p_cback) {
            p_data->api_enable.p_cback(BTA_HL_CTRL_ENABLE_CFM_EVT, (tBTA_HL_CTRL *) &evt_data);
        }

        return;
    }

    /* Done with checking. now perform the enable oepration*/
    /* initialize control block */
    wm_memset(p_cb, 0, sizeof(tBTA_HL_CB));
    p_cb->enable = TRUE;
    p_cb->p_ctrl_cback = p_data->api_enable.p_cback;
    evt_data.enable_cfm.status = BTA_HL_STATUS_OK;

    if(p_data->api_enable.p_cback) {
        p_data->api_enable.p_cback(BTA_HL_CTRL_ENABLE_CFM_EVT, (tBTA_HL_CTRL *) &evt_data);
    }
}
/*******************************************************************************
**
** Function         bta_hl_api_disable
**
** Description      Process the API disable request to disable the HL subsystem
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_disable(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL_CTRL    evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;

    if(p_cb->enable) {
        p_cb->disabling = TRUE;
        bta_hl_check_disable(p_data);
    } else {
        status = BTA_HL_STATUS_FAIL;
        evt_data.disable_cfm.status = status;

        if(p_cb->p_ctrl_cback) {
            p_cb->p_ctrl_cback(BTA_HL_CTRL_DISABLE_CFM_EVT, (tBTA_HL_CTRL *) &evt_data);
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_disable status =%s", bta_hl_status_code(status));
    }

#endif
}

/*******************************************************************************
**
** Function         bta_hl_api_update
**
** Description      Process the API registration request to register an HDP applciation
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_update(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_APP_CB  *p_acb = BTA_HL_GET_APP_CB_PTR(0);
    tBTA_HL_STATUS  status = BTA_HL_STATUS_FAIL;
    APPL_TRACE_DEBUG("bta_hl_api_update");

    if(p_cb->enable) {
        status = bta_hl_app_update(p_data->api_update.app_id, p_data->api_update.is_register);

        if(!p_data->api_update.is_register) {
            APPL_TRACE_DEBUG("Deregister");
            wm_memset(&evt_data, 0, sizeof(tBTA_HL));
            evt_data.dereg_cfm.status = status;
            evt_data.dereg_cfm.app_id = p_data->api_update.app_id;

            if(status == BTA_HL_STATUS_OK) {
                evt_data.dereg_cfm.app_handle = p_acb->app_handle;
            }

            if(p_acb->p_cback) {
                p_acb->p_cback(BTA_HL_DEREGISTER_CFM_EVT, (tBTA_HL *) &evt_data);
            }

            return ;
        }
    }

    if(status != BTA_HL_STATUS_OK) {
        if((status != BTA_HL_STATUS_DUPLICATE_APP_ID) &&
                (status != BTA_HL_STATUS_NO_RESOURCE)) {
            if(p_acb) {
                wm_memset(p_acb, 0, sizeof(tBTA_HL_APP_CB));
            }
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_register status =%s", bta_hl_status_code(status));
    }

#endif
    wm_memset(&evt_data, 0, sizeof(tBTA_HL));
    evt_data.reg_cfm.status = status;
    evt_data.reg_cfm.app_id = p_data->api_update.app_id;

    if(status == BTA_HL_STATUS_OK) {
        evt_data.reg_cfm.app_handle = p_acb->app_handle;
    }

    if(p_data->api_reg.p_cback) {
        p_data->api_reg.p_cback(BTA_HL_REGISTER_CFM_EVT, (tBTA_HL *) &evt_data);
    }

    if(status == BTA_HL_STATUS_OK) {
        evt_data.sdp_info_ind.app_handle = p_acb->app_handle;
        evt_data.sdp_info_ind.ctrl_psm = p_acb->ctrl_psm;
        evt_data.sdp_info_ind.data_psm = p_acb->data_psm;
        evt_data.sdp_info_ind.data_x_spec = BTA_HL_SDP_IEEE_11073_20601;
        evt_data.sdp_info_ind.mcap_sup_procs = BTA_HL_MCAP_SUP_PROC_MASK ;

        if(p_data->api_reg.p_cback) {
            p_data->api_reg.p_cback(BTA_HL_SDP_INFO_IND_EVT, (tBTA_HL *) &evt_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_api_register
**
** Description      Process the API registration request to register an HDP applciation
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_register(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    uint8_t           app_idx;
    tBTA_HL_APP_CB  *p_acb = NULL;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_FAIL;

    if(p_cb->enable) {
        if(!bta_hl_is_a_duplicate_id(p_data->api_reg.app_id)) {
            if(bta_hl_find_avail_app_idx(&app_idx)) {
                p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
                p_acb->in_use = TRUE;
                p_acb->app_id = p_data->api_reg.app_id;
                p_acb->p_cback = p_data->api_reg.p_cback;
                p_acb->sec_mask = p_data->api_reg.sec_mask;
                p_acb->dev_type = p_data->api_reg.dev_type;
                strlcpy(p_acb->srv_name, p_data->api_reg.srv_name, BTA_SERVICE_NAME_LEN);
                strlcpy(p_acb->srv_desp, p_data->api_reg.srv_desp, BTA_SERVICE_DESP_LEN);
                strlcpy(p_acb->provider_name, p_data->api_reg.provider_name, BTA_PROVIDER_NAME_LEN);
                bta_hl_cb.p_alloc_psm = L2CA_AllocatePSM;
                p_acb->ctrl_psm = bta_hl_cb.p_alloc_psm();
                p_acb->data_psm = bta_hl_cb.p_alloc_psm();
                p_acb->p_mcap_cback = bta_hl_mcap_ctrl_cback;
                status = bta_hl_app_registration(app_idx);
            } else {
                status = BTA_HL_STATUS_NO_RESOURCE;
            }
        } else {
            status = BTA_HL_STATUS_DUPLICATE_APP_ID;
        }
    }

    if(status != BTA_HL_STATUS_OK) {
        if((status != BTA_HL_STATUS_DUPLICATE_APP_ID) &&
                (status != BTA_HL_STATUS_NO_RESOURCE)) {
            if(p_acb) {
                wm_memset(p_acb, 0, sizeof(tBTA_HL_APP_CB));
            }
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_register status =%s", bta_hl_status_code(status));
    }

#endif
    wm_memset(&evt_data, 0, sizeof(tBTA_HL));
    evt_data.reg_cfm.status = status;
    evt_data.reg_cfm.app_id = p_data->api_reg.app_id;

    if(status == BTA_HL_STATUS_OK) {
        evt_data.reg_cfm.app_handle = p_acb->app_handle;
    }

    if(p_data->api_reg.p_cback) {
        p_data->api_reg.p_cback(BTA_HL_REGISTER_CFM_EVT, (tBTA_HL *) &evt_data);
    }

    if(status == BTA_HL_STATUS_OK) {
        evt_data.sdp_info_ind.app_handle = p_acb->app_handle;
        evt_data.sdp_info_ind.ctrl_psm = p_acb->ctrl_psm;
        evt_data.sdp_info_ind.data_psm = p_acb->data_psm;
        evt_data.sdp_info_ind.data_x_spec = BTA_HL_SDP_IEEE_11073_20601;
        evt_data.sdp_info_ind.mcap_sup_procs = BTA_HL_MCAP_SUP_PROC_MASK ;

        if(p_data->api_reg.p_cback) {
            p_data->api_reg.p_cback(BTA_HL_SDP_INFO_IND_EVT, (tBTA_HL *) &evt_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_api_deregister
**
** Description      Process the API de-registration request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_deregister(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    uint8_t           app_idx;
    tBTA_HL_APP_CB  *p_acb;
    UNUSED(p_cb);

    if(bta_hl_find_app_idx_using_handle(p_data->api_dereg.app_handle, &app_idx)) {
        p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
        p_acb->deregistering = TRUE;
        bta_hl_check_deregistration(app_idx, p_data);
    } else {
        APPL_TRACE_ERROR("Invalid app_handle=%d", p_data->api_dereg.app_handle);
    }
}

/*******************************************************************************
**
** Function         bta_hl_api_cch_open
**
** Description      Process the API CCH open request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_cch_open(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb;
    UNUSED(p_cb);

    if(bta_hl_find_app_idx_using_handle(p_data->api_cch_open.app_handle, &app_idx)) {
        if(!bta_hl_find_mcl_idx(app_idx, p_data->api_cch_open.bd_addr, &mcl_idx)) {
            if(bta_hl_find_avail_mcl_idx(app_idx, &mcl_idx)) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                p_mcb->in_use = TRUE;
                p_mcb->req_ctrl_psm = p_data->api_cch_open.ctrl_psm;
                p_mcb->sec_mask = p_data->api_cch_open.sec_mask;
                bdcpy(p_mcb->bd_addr, p_data->api_cch_open.bd_addr);
                p_mcb->cch_oper = BTA_HL_CCH_OP_LOCAL_OPEN;
            } else {
                status = BTA_HL_STATUS_NO_RESOURCE;
            }
        } else {
            /* Only one MCL per BD_ADDR */
            status = BTA_HL_STATUS_DUPLICATE_CCH_OPEN;
            APPL_TRACE_DEBUG("bta_hl_api_cch_open: CCH already open: status =%d", status)
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
            p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

            if(p_acb->p_cback) {
                bta_hl_build_cch_open_cfm(&evt_data, p_data->api_cch_open.app_id,
                                          p_data->api_cch_open.app_handle,
                                          p_mcb->mcl_handle,
                                          p_data->api_cch_open.bd_addr,
                                          status);
                p_acb->p_cback(BTA_HL_CCH_OPEN_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_cch_open Null Callback");
            }

            return;
        }
    } else {
        status = BTA_HL_STATUS_INVALID_APP_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_cch_open status =%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_OPEN_EVT, p_data);
            break;

        case BTA_HL_STATUS_NO_RESOURCE:
        case BTA_HL_STATUS_FAIL:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_cch_open_cfm(&evt_data, p_data->api_cch_open.app_id,
                                          p_data->api_cch_open.app_handle,
                                          0,
                                          p_data->api_cch_open.bd_addr,
                                          status);
                p_acb->p_cback(BTA_HL_CCH_OPEN_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_cch_open Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("status code=%d", status);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_api_cch_close
**
** Description      Process the API CCH close request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_cch_close(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->api_cch_close.mcl_handle, &app_idx,  &mcl_idx)) {
        p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
        p_mcb->cch_oper = BTA_HL_CCH_OP_LOCAL_CLOSE;
    } else {
        status = BTA_HL_STATUS_INVALID_MCL_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_cch_close status =%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            bta_hl_check_cch_close(app_idx, mcl_idx, p_data, TRUE);
            break;

        case BTA_HL_STATUS_INVALID_MCL_HANDLE:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_cch_close_cfm(&evt_data,
                                           p_acb->app_handle,
                                           p_data->api_cch_close.mcl_handle,
                                           status);
                p_acb->p_cback(BTA_HL_CCH_CLOSE_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_cch_close Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("status code=%d", status);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_api_dch_open
**
** Description      Process the API DCH open request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_dch_open(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL                     evt_data;
    tBTA_HL_STATUS              status = BTA_HL_STATUS_OK;
    uint8_t                       app_idx, mcl_idx, mdl_idx;
    tBTA_HL_APP_CB              *p_acb;
    tBTA_HL_MCL_CB              *p_mcb = NULL;
    tBTA_HL_MDL_CB              *p_dcb;
    tBTA_HL_MDEP_CFG            *p_mdep_cfg;
    uint8_t                       mdep_cfg_idx;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->api_dch_open.mcl_handle, &app_idx, &mcl_idx)) {
        p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
        p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
        APPL_TRACE_DEBUG("bta_hl_api_dch_open: app_ix=%d, mcl_idx=%d, cch_state=%d, mcl_handle=%d", app_idx,
                         mcl_idx, p_mcb->cch_state, p_data->api_dch_open.mcl_handle);

        if(p_mcb->cch_state == BTA_HL_CCH_OPEN_ST) {
            if(bta_hl_find_avail_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
                p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

                if(bta_hl_find_mdep_cfg_idx(app_idx, p_data->api_dch_open.local_mdep_id, &mdep_cfg_idx)) {
                    if(mdep_cfg_idx &&
                            (p_acb->sup_feature.mdep[mdep_cfg_idx].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SINK)) {
                        p_data->api_dch_open.local_cfg = BTA_HL_DCH_CFG_NO_PREF;
                    }

                    if((status = bta_hl_chk_local_cfg(app_idx, mcl_idx, mdep_cfg_idx, p_data->api_dch_open.local_cfg))
                            == BTA_HL_STATUS_OK) {
                        if(p_data->api_dch_open.local_mdep_id != BTA_HL_ECHO_TEST_MDEP_ID) {
                            if(bta_hl_set_ctrl_psm_for_dch(app_idx, mcl_idx, mdl_idx, p_data->api_dch_open.ctrl_psm)) {
                                p_mdep_cfg = BTA_HL_GET_MDEP_CFG_PTR(app_idx, mdep_cfg_idx);
                                p_dcb->in_use                   = TRUE;
                                p_dcb->dch_oper                 = BTA_HL_DCH_OP_LOCAL_OPEN;
                                p_dcb->sec_mask                 = p_data->api_dch_open.sec_mask;
                                p_dcb->local_mdep_id            = p_data->api_dch_open.local_mdep_id;
                                p_dcb->peer_mdep_id             = p_data->api_dch_open.peer_mdep_id;

                                if(p_mdep_cfg->mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                                    p_dcb->peer_mdep_role = BTA_HL_MDEP_ROLE_SOURCE;
                                } else {
                                    p_dcb->peer_mdep_role = BTA_HL_MDEP_ROLE_SINK;
                                }

                                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                                p_dcb->local_cfg            = p_data->api_dch_open.local_cfg;
                                bta_hl_find_rxtx_apdu_size(app_idx, mdep_cfg_idx,
                                                           &p_dcb->max_rx_apdu_size,
                                                           &p_dcb->max_tx_apdu_size);
                                p_dcb->mdl_id               = bta_hl_allocate_mdl_id(app_idx, mcl_idx, mdl_idx);
                                p_dcb->mdl_cfg_idx_included = FALSE;
                            } else {
                                status =  BTA_HL_STATUS_INVALID_CTRL_PSM;
                            }
                        } else {
                            status =  BTA_HL_STATUS_INVALID_LOCAL_MDEP_ID;
                        }
                    }
                } else {
                    status =  BTA_HL_STATUS_INVALID_LOCAL_MDEP_ID;
                }
            } else {
                status = BTA_HL_STATUS_NO_RESOURCE;
            }
        } else {
            status =  BTA_HL_STATUS_NO_CCH;
        }
    } else {
        status = BTA_HL_STATUS_INVALID_MCL_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_dch_open status =%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            if(p_mcb->sdp.num_recs) {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_OPEN_EVT, p_data);
            } else {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_SDP_INIT_EVT, p_data);
            }

            break;

        case BTA_HL_STATUS_INVALID_DCH_CFG:
        case BTA_HL_STATUS_NO_FIRST_RELIABLE:
        case BTA_HL_STATUS_NO_CCH:
        case BTA_HL_STATUS_NO_RESOURCE:
        case BTA_HL_STATUS_FAIL:
        case BTA_HL_STATUS_INVALID_LOCAL_MDEP_ID:
        case BTA_HL_STATUS_INVALID_CTRL_PSM:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_dch_open_cfm(&evt_data,
                                          p_acb->app_handle,
                                          p_data->api_dch_open.mcl_handle,
                                          BTA_HL_INVALID_MDL_HANDLE,
                                          0, 0, 0, 0, 0, status);
                p_acb->p_cback(BTA_HL_DCH_OPEN_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_dch_open Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("Status code=%d", status);
            break;
    }
}
/*******************************************************************************
**
** Function         bta_hl_api_dch_close
**
** Description      Process the API DCH close request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_dch_close(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx, mdl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb;
    tBTA_HL_MDL_CB  *p_dcb;
    UNUSED(p_cb);

    if(bta_hl_find_mdl_idx_using_handle(p_data->api_dch_close.mdl_handle, &app_idx, &mcl_idx,
                                        &mdl_idx)) {
        p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

        if(p_dcb->dch_state != BTA_HL_DCH_OPEN_ST) {
            status =  BTA_HL_STATUS_FAIL;
        }
    } else {
        status = BTA_HL_STATUS_INVALID_MDL_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_dch_close status =%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_EVT, p_data);
            break;

        case BTA_HL_STATUS_FAIL:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                bta_hl_build_dch_close_cfm(&evt_data,
                                           p_acb->app_handle,
                                           p_mcb->mcl_handle,
                                           p_data->api_dch_close.mdl_handle,
                                           status);
                p_acb->p_cback(BTA_HL_DCH_CLOSE_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_dch_close Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("Status code=%d", status);
            break;
    }
}


/*******************************************************************************
**
** Function         bta_hl_api_dch_reconnect
**
** Description      Process the API DCH reconnect request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_dch_reconnect(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx, mdl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb = NULL;
    tBTA_HL_MDL_CB  *p_dcb;
    uint8_t           mdep_cfg_idx;
    uint8_t           mdl_cfg_idx;
    tBTA_HL_MDEP_CFG            *p_mdep_cfg;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->api_dch_reconnect.mcl_handle, &app_idx, &mcl_idx)) {
        p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
        p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

        if(p_mcb->cch_state == BTA_HL_CCH_OPEN_ST) {
            if(bta_hl_find_avail_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
                p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

                if(bta_hl_validate_reconnect_params(app_idx, mcl_idx,  &(p_data->api_dch_reconnect),
                                                    &mdep_cfg_idx, &mdl_cfg_idx)) {
                    if(!bta_hl_is_the_first_reliable_existed(app_idx, mcl_idx) &&
                            (p_acb->mdl_cfg[mdl_cfg_idx].dch_mode != BTA_HL_DCH_MODE_RELIABLE)) {
                        status =  BTA_HL_STATUS_NO_FIRST_RELIABLE;
                    } else {
                        if(bta_hl_set_ctrl_psm_for_dch(app_idx, mcl_idx, mdl_idx, p_data->api_dch_open.ctrl_psm)) {
                            p_dcb->in_use                   = TRUE;
                            p_dcb->dch_oper                 = BTA_HL_DCH_OP_LOCAL_RECONNECT;
                            p_dcb->sec_mask                 = (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
                            p_dcb->local_mdep_id            = p_acb->mdl_cfg[mdl_cfg_idx].local_mdep_id;
                            p_dcb->local_mdep_cfg_idx       = mdep_cfg_idx;
                            p_dcb->local_cfg                = BTA_HL_DCH_CFG_UNKNOWN;
                            p_dcb->mdl_id                   = p_data->api_dch_reconnect.mdl_id;
                            p_dcb->mdl_cfg_idx_included     = TRUE;
                            p_dcb->mdl_cfg_idx              = mdl_cfg_idx;
                            p_dcb->dch_mode                 = p_acb->mdl_cfg[mdl_cfg_idx].dch_mode;
                            p_mdep_cfg = BTA_HL_GET_MDEP_CFG_PTR(app_idx, mdep_cfg_idx);

                            if(p_mdep_cfg->mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                                p_dcb->peer_mdep_role = BTA_HL_MDEP_ROLE_SOURCE;
                                APPL_TRACE_DEBUG("peer mdep role = SOURCE ");
                            } else {
                                p_dcb->peer_mdep_role = BTA_HL_MDEP_ROLE_SINK;
                                APPL_TRACE_DEBUG("peer mdep role = SINK ");
                            }

                            bta_hl_find_rxtx_apdu_size(app_idx, mdep_cfg_idx,
                                                       &p_dcb->max_rx_apdu_size,
                                                       &p_dcb->max_tx_apdu_size);
                        } else {
                            status =  BTA_HL_STATUS_INVALID_CTRL_PSM;
                        }
                    }
                } else {
                    status =  BTA_HL_STATUS_INVALID_RECONNECT_CFG;
                }
            } else {
                status = BTA_HL_STATUS_NO_RESOURCE;
            }
        } else {
            status =  BTA_HL_STATUS_NO_CCH;
        }
    } else {
        status = BTA_HL_STATUS_INVALID_MCL_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_dch_reconnect status=%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            if(p_mcb->sdp.num_recs) {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_RECONNECT_EVT, p_data);
            } else {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_SDP_INIT_EVT, p_data);
            }

            break;

        case BTA_HL_STATUS_INVALID_RECONNECT_CFG:
        case BTA_HL_STATUS_NO_FIRST_RELIABLE:
        case BTA_HL_STATUS_NO_CCH:
        case BTA_HL_STATUS_NO_RESOURCE:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_dch_open_cfm(&evt_data,
                                          p_acb->app_handle,
                                          p_data->api_dch_reconnect.mcl_handle,
                                          BTA_HL_INVALID_MDL_HANDLE,
                                          0, p_data->api_dch_reconnect.mdl_id, 0, 0, 0, status);
                p_acb->p_cback(BTA_HL_DCH_RECONNECT_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_dch_reconnect Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("Status code=%d", status);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_api_dch_echo_test
**
** Description      Process the API Echo test request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_dch_echo_test(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL             evt_data;
    tBTA_HL_STATUS      status = BTA_HL_STATUS_OK;
    uint8_t               app_idx, mcl_idx, mdl_idx;
    tBTA_HL_APP_CB      *p_acb;
    tBTA_HL_MCL_CB      *p_mcb = NULL;
    tBTA_HL_MDL_CB      *p_dcb;
    tBTA_HL_ECHO_CFG    *p_echo_cfg;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->api_dch_echo_test.mcl_handle, &app_idx,  &mcl_idx)) {
        p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
        p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

        if(p_mcb->cch_state == BTA_HL_CCH_OPEN_ST) {
            if(!p_mcb->echo_test) {
                if(bta_hl_find_avail_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
                    p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

                    if((p_data->api_dch_echo_test.local_cfg == BTA_HL_DCH_CFG_RELIABLE) ||
                            (p_data->api_dch_echo_test.local_cfg == BTA_HL_DCH_CFG_STREAMING)) {
                        uint8_t fcs_use = (uint8_t)(p_dcb->chnl_cfg.fcs & BTA_HL_MCA_FCS_USE_MASK);

                        if((p_dcb->p_echo_tx_pkt = bta_hl_get_buf(p_data->api_dch_echo_test.pkt_size, fcs_use)) != NULL) {
                            if(bta_hl_set_ctrl_psm_for_dch(app_idx, mcl_idx, mdl_idx, p_data->api_dch_open.ctrl_psm)) {
                                p_dcb->in_use                   = TRUE;
                                p_dcb->dch_oper                 = BTA_HL_DCH_OP_LOCAL_OPEN;
                                p_dcb->sec_mask                 = (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
                                p_dcb->local_mdep_cfg_idx       = BTA_HL_ECHO_TEST_MDEP_CFG_IDX;
                                p_dcb->local_cfg                = p_data->api_dch_echo_test.local_cfg;
                                p_dcb->local_mdep_id            = BTA_HL_ECHO_TEST_MDEP_ID;
                                p_dcb->peer_mdep_id             = BTA_HL_ECHO_TEST_MDEP_ID;
                                p_dcb->mdl_id                   = bta_hl_allocate_mdl_id(app_idx, mcl_idx, mdl_idx);
                                p_dcb->mdl_cfg_idx_included     = FALSE;
                                p_echo_cfg                      = BTA_HL_GET_ECHO_CFG_PTR(app_idx);
                                p_dcb->max_rx_apdu_size         = p_echo_cfg->max_rx_apdu_size;
                                p_dcb->max_tx_apdu_size         = p_echo_cfg->max_tx_apdu_size;
                                p_mcb->echo_test                = TRUE;
                                p_mcb->echo_mdl_idx             = mdl_idx;
                            } else {
                                status =  BTA_HL_STATUS_INVALID_CTRL_PSM;
                            }
                        } else {
                            status = BTA_HL_STATUS_NO_RESOURCE;
                        }
                    } else {
                        status = BTA_HL_STATUS_INVALID_DCH_CFG;
                    }
                } else {
                    status = BTA_HL_STATUS_NO_RESOURCE;
                }
            } else {
                status = BTA_HL_STATUS_ECHO_TEST_BUSY;
            }
        } else {
            status =  BTA_HL_STATUS_NO_CCH;
        }
    } else {
        status = BTA_HL_STATUS_NO_MCL;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_dch_echo_test status=%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            if(p_mcb->sdp.num_recs) {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_ECHO_TEST_EVT, p_data);
            } else {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_SDP_INIT_EVT, p_data);
            }

            break;

        case BTA_HL_STATUS_ECHO_TEST_BUSY:
        case BTA_HL_STATUS_NO_RESOURCE:
        case BTA_HL_STATUS_INVALID_DCH_CFG:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_echo_test_cfm(&evt_data,
                                           p_acb->app_handle,
                                           p_mcb->mcl_handle,
                                           status);
                p_acb->p_cback(BTA_HL_DCH_ECHO_TEST_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_dch_echo_test Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("Status code=%s", status);
            break;
    }
}


/*******************************************************************************
**
** Function         bta_hl_api_sdp_query
**
** Description      Process the API SDP query request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_sdp_query(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb;
    UNUSED(p_cb);

    if(bta_hl_find_app_idx_using_handle(p_data->api_sdp_query.app_handle, &app_idx)) {
        if(!bta_hl_find_mcl_idx(app_idx, p_data->api_sdp_query.bd_addr, &mcl_idx)) {
            if(bta_hl_find_avail_mcl_idx(app_idx, &mcl_idx)) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                p_mcb->in_use = TRUE;
                bdcpy(p_mcb->bd_addr, p_data->api_sdp_query.bd_addr);
                APPL_TRACE_DEBUG("bta_hl_api_sdp_query p_mcb->app_id %d app_idx %d mcl_idx %d", p_mcb->app_id,
                                 app_idx, mcl_idx);
                p_mcb->app_id = p_data->api_sdp_query.app_id;
                p_mcb->sdp_oper  = BTA_HL_SDP_OP_SDP_QUERY_NEW ;
            } else {
                status = BTA_HL_STATUS_NO_RESOURCE;
            }
        } else {
            p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
            p_mcb->app_id = p_data->api_sdp_query.app_id;

            if(p_mcb->sdp_oper != BTA_HL_SDP_OP_NONE) {
                status = BTA_HL_STATUS_SDP_NO_RESOURCE;
            } else {
                p_mcb->sdp_oper  = BTA_HL_SDP_OP_SDP_QUERY_CURRENT;
            }
        }
    } else {
        status = BTA_HL_STATUS_INVALID_APP_HANDLE;
    }

    if(status == BTA_HL_STATUS_OK) {
        status = bta_hl_init_sdp(p_mcb->sdp_oper, app_idx, mcl_idx, 0xFF);

        if((status != BTA_HL_STATUS_OK) &&
                (p_mcb->sdp_oper == BTA_HL_SDP_OP_SDP_QUERY_NEW)) {
            wm_memset(p_mcb, 0, sizeof(tBTA_HL_MCL_CB));
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_sdp_query status=%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_NO_RESOURCE:
        case BTA_HL_STATUS_FAIL:
        case BTA_HL_STATUS_SDP_NO_RESOURCE:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_sdp_query_cfm(&evt_data,
                                           p_data->api_sdp_query.app_id,
                                           p_data->api_sdp_query.app_handle,
                                           p_data->api_sdp_query.bd_addr,
                                           NULL,
                                           status);
                p_acb->p_cback(BTA_HL_SDP_QUERY_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_sdp_query Null Callback");
            }

            break;

        case BTA_HL_STATUS_OK:
            break;

        default:
            APPL_TRACE_ERROR("Status code=%d", status);
            break;
    }
}




/*******************************************************************************
**
** Function         bta_hl_sdp_query_results
**
** Description      Process the SDP query results
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_sdp_query_results(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL             evt_data;
    tBTA_HL_STATUS      status = BTA_HL_STATUS_OK;
    uint8_t               app_idx = p_data->cch_sdp.app_idx;
    uint8_t               mcl_idx = p_data->cch_sdp.mcl_idx;
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb =  BTA_HL_GET_MCL_CB_PTR(app_idx,  mcl_idx);
    tBTA_HL_SDP         *p_sdp = NULL;
    uint16_t              event;
    uint8_t             release_sdp_buf = FALSE;
    UNUSED(p_cb);
    event = p_data->hdr.event;

    if(event == BTA_HL_SDP_QUERY_OK_EVT) {
        p_sdp = (tBTA_HL_SDP *)GKI_getbuf(sizeof(tBTA_HL_SDP));
        wm_memcpy(p_sdp, &p_mcb->sdp, sizeof(tBTA_HL_SDP));
        release_sdp_buf = TRUE;
    } else {
        status = BTA_HL_STATUS_SDP_FAIL;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_sdp_query_results status=%s", bta_hl_status_code(status));
    }

#endif
    APPL_TRACE_DEBUG("bta_hl_sdp_query_results p_mcb->app_id %d app_idx %d mcl_idx %d", p_mcb->app_id,
                     app_idx, mcl_idx);
    bta_hl_build_sdp_query_cfm(&evt_data, p_mcb->app_id, p_acb->app_handle,
                               p_mcb->bd_addr, p_sdp, status);
    p_acb->p_cback(BTA_HL_SDP_QUERY_CFM_EVT, (tBTA_HL *) &evt_data);

    if(release_sdp_buf) {
        GKI_free_and_reset_buf((void **)&p_sdp);
    }

    if(p_data->cch_sdp.release_mcl_cb) {
        wm_memset(p_mcb, 0, sizeof(tBTA_HL_MCL_CB));
    } else {
        if(p_mcb->close_pending) {
            bta_hl_check_cch_close(app_idx, mcl_idx, p_data, TRUE);
        }

        if(!p_mcb->ctrl_psm) {
            /* Control channel acceptor: do not store the SDP records */
            wm_memset(&p_mcb->sdp, 0, sizeof(tBTA_HL_SDP));
        }
    }
}


/*******************************************************************************
**
** Function         bta_hl_api_delete_mdl
**
** Description      Process the API DELETE MDL request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_delete_mdl(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->api_delete_mdl.mcl_handle, &app_idx, &mcl_idx)) {
        if(bta_hl_is_mdl_value_valid(p_data->api_delete_mdl.mdl_id)) {
            p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

            if(bta_hl_is_mdl_exsit_in_mcl(app_idx,
                                          p_mcb->bd_addr,
                                          p_data->api_delete_mdl.mdl_id)) {
                p_mcb->delete_mdl.mcl_handle =  p_data->api_delete_mdl.mcl_handle;
                p_mcb->delete_mdl.mdl_id = p_data->api_delete_mdl.mdl_id;
                p_mcb->delete_mdl.delete_req_pending = TRUE;

                if(MCA_Delete((tMCA_CL) p_mcb->mcl_handle,
                              p_data->api_delete_mdl.mdl_id) != MCA_SUCCESS) {
                    status = BTA_HL_STATUS_FAIL;
                    wm_memset(&p_mcb->delete_mdl, 0, sizeof(tBTA_HL_DELETE_MDL));
                }
            } else {
                status = BTA_HL_STATUS_NO_MDL_ID_FOUND;
            }
        } else {
            status = BTA_HL_STATUS_INVALID_MDL_ID;
        }
    } else {
        status = BTA_HL_STATUS_INVALID_MCL_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_delete_mdl status=%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            break;

        case BTA_HL_STATUS_FAIL:
        case BTA_HL_STATUS_NO_MDL_ID_FOUND:
        case BTA_HL_STATUS_INVALID_MDL_ID:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                bta_hl_build_delete_mdl_cfm(&evt_data,
                                            p_acb->app_handle,
                                            p_data->api_delete_mdl.mcl_handle,
                                            p_data->api_delete_mdl.mdl_id,
                                            status);
                p_acb->p_cback(BTA_HL_DELETE_MDL_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_delete_mdl Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("status code =%d", status);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_mca_delete_mdl_cfm
**
** Description      Process the DELETE MDL confirmation event
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_mca_delete_mdl_cfm(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx;
    tMCA_RSP_EVT    *p_delete_cfm = &p_data->mca_evt.mca_data.delete_cfm;
    tBTA_HL_MCL_CB  *p_mcb;
    uint8_t         send_cfm_evt = TRUE;
    tBTA_HL_APP_CB  *p_acb;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->mca_evt.mcl_handle, &app_idx, &mcl_idx)) {
        p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

        if(p_mcb->delete_mdl.delete_req_pending) {
            if(p_delete_cfm->rsp_code == MCA_RSP_SUCCESS) {
                if(!bta_hl_delete_mdl_cfg(app_idx,
                                          p_mcb->bd_addr,
                                          p_delete_cfm->mdl_id)) {
                    status = BTA_HL_STATUS_FAIL;
                }
            } else {
                status = BTA_HL_STATUS_FAIL;
            }

            wm_memset(&p_mcb->delete_mdl, 0, sizeof(tBTA_HL_DELETE_MDL));
        } else {
            send_cfm_evt = FALSE;
        }
    } else {
        send_cfm_evt = FALSE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_delete_mdl status=%s", bta_hl_status_code(status));
    }

#endif

    if(send_cfm_evt) {
        p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

        if(p_acb->p_cback) {
            bta_hl_build_delete_mdl_cfm(&evt_data,
                                        p_acb->app_handle,
                                        p_mcb->mcl_handle,
                                        p_delete_cfm->mdl_id,
                                        status);
            p_acb->p_cback(BTA_HL_DELETE_MDL_CFM_EVT, (tBTA_HL *) &evt_data);
        } else {
            APPL_TRACE_ERROR("bta_hl_mca_delete_mdl_cfm Null Callback");
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_mca_delete_mdl_ind
**
** Description      Process the DELETE MDL indication event
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_mca_delete_mdl_ind(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL         evt_data;
    uint8_t           app_idx, mcl_idx, mdl_idx;
    tMCA_EVT_HDR    *p_delete_ind = &p_data->mca_evt.mca_data.delete_ind;
    tBTA_HL_MCL_CB  *p_mcb;
    tBTA_HL_MDL_CB  *p_dcb;
    uint8_t         send_ind_evt = FALSE;
    tBTA_HL_APP_CB  *p_acb;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->mca_evt.mcl_handle, &app_idx, &mcl_idx)) {
        p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

        if(bta_hl_find_mdl_idx(app_idx, mcl_idx, p_delete_ind->mdl_id, &mdl_idx)) {
            p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
            p_dcb->dch_oper = BTA_HL_DCH_OP_REMOTE_DELETE;
        }

        if(bta_hl_delete_mdl_cfg(app_idx,
                                 p_mcb->bd_addr,
                                 p_delete_ind->mdl_id)) {
            send_ind_evt = TRUE;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!send_ind_evt) {
        APPL_TRACE_DEBUG("bta_hl_mca_delete_mdl_ind is_send_ind_evt =%d", send_ind_evt);
    }

#endif

    if(send_ind_evt) {
        p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

        if(p_acb->p_cback) {
            evt_data.delete_mdl_ind.mcl_handle = p_mcb->mcl_handle;
            evt_data.delete_mdl_ind.app_handle = p_acb->app_handle;
            evt_data.delete_mdl_ind.mdl_id = p_delete_ind->mdl_id;
            p_acb->p_cback(BTA_HL_DELETE_MDL_IND_EVT, (tBTA_HL *) &evt_data);
        } else {
            APPL_TRACE_ERROR("bta_hl_mca_delete_mdl_ind Null Callback");
        }
    }
}



/*******************************************************************************
**
** Function         bta_hl_api_dch_abort
**
** Description      Process the API DCH abort request
**
** Returns          void
**
*******************************************************************************/
static void bta_hl_api_dch_abort(tBTA_HL_CB *p_cb, tBTA_HL_DATA *p_data)
{
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    uint8_t           app_idx, mcl_idx, mdl_idx;
    tBTA_HL_APP_CB  *p_acb;
    tBTA_HL_MCL_CB  *p_mcb;
    tBTA_HL_MDL_CB  *p_dcb;
    tBTA_HL         evt_data;
    UNUSED(p_cb);

    if(bta_hl_find_mcl_idx_using_handle(p_data->api_dch_abort.mcl_handle, &app_idx, &mcl_idx)) {
        if(!bta_hl_find_dch_setup_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
            status = BTA_HL_STATUS_NO_MDL_ID_FOUND;
        } else {
            p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(p_dcb->abort_oper) {
                /* abort already in progress*/
                status = BTA_HL_STATUS_FAIL;
            } else {
                p_dcb->abort_oper = BTA_HL_ABORT_LOCAL_MASK;
            }
        }
    } else {
        status = BTA_HL_STATUS_INVALID_MCL_HANDLE;
    }

#if BTA_HL_DEBUG == TRUE

    if(status != BTA_HL_STATUS_OK) {
        APPL_TRACE_DEBUG("bta_hl_api_dch_abort status=%s", bta_hl_status_code(status));
    }

#endif

    switch(status) {
        case BTA_HL_STATUS_OK:
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_ABORT_EVT, p_data);
            break;

        case BTA_HL_STATUS_NO_MDL_ID_FOUND:
        case BTA_HL_STATUS_FAIL:
            p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(p_acb->p_cback) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                bta_hl_build_abort_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       BTA_HL_STATUS_FAIL);
                p_acb->p_cback(BTA_HL_DCH_ABORT_CFM_EVT, (tBTA_HL *) &evt_data);
            } else {
                APPL_TRACE_ERROR("bta_hl_api_dch_abort Null Callback");
            }

            break;

        default:
            APPL_TRACE_ERROR("Status code=%d", status);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_hdl_event
**
** Description      HL main event handling function.
**
** Returns          void
**
*******************************************************************************/
uint8_t bta_hl_hdl_event(BT_HDR *p_msg)
{
    uint8_t app_idx, mcl_idx, mdl_idx;
    uint8_t success = TRUE;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("BTA HL Event Handler: Event [%s]",
                     bta_hl_evt_code(p_msg->event));
#endif

    switch(p_msg->event) {
        case BTA_HL_API_ENABLE_EVT:
            bta_hl_api_enable(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DISABLE_EVT:
            bta_hl_api_disable(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_UPDATE_EVT:
            bta_hl_api_update(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_REGISTER_EVT:
            bta_hl_api_register(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DEREGISTER_EVT:
            bta_hl_api_deregister(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_CCH_OPEN_EVT:
            bta_hl_api_cch_open(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_CCH_CLOSE_EVT:
            bta_hl_api_cch_close(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DCH_OPEN_EVT:
            bta_hl_api_dch_open(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DCH_CLOSE_EVT:
            bta_hl_api_dch_close(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DELETE_MDL_EVT:
            bta_hl_api_delete_mdl(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DCH_RECONNECT_EVT:
            bta_hl_api_dch_reconnect(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DCH_ECHO_TEST_EVT:
            bta_hl_api_dch_echo_test(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_SDP_QUERY_EVT:
            bta_hl_api_sdp_query(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_MCA_DELETE_CFM_EVT:
            bta_hl_mca_delete_mdl_cfm(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_MCA_DELETE_IND_EVT:
            bta_hl_mca_delete_mdl_ind(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_SDP_QUERY_OK_EVT:
        case BTA_HL_SDP_QUERY_FAIL_EVT:
            bta_hl_sdp_query_results(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        case BTA_HL_API_DCH_ABORT_EVT:
            bta_hl_api_dch_abort(&bta_hl_cb, (tBTA_HL_DATA *) p_msg);
            break;

        default:
            if(p_msg->event < BTA_HL_DCH_EVT_MIN) {
                if(bta_hl_find_cch_cb_indexes((tBTA_HL_DATA *) p_msg, &app_idx, &mcl_idx)) {
                    bta_hl_cch_sm_execute(app_idx,
                                          mcl_idx,
                                          p_msg->event, (tBTA_HL_DATA *) p_msg);
                } else {
#if BTA_HL_DEBUG == TRUE
                    APPL_TRACE_ERROR("unable to find control block indexes for CCH: [event=%s]",
                                     bta_hl_evt_code(p_msg->event));
#else
                    APPL_TRACE_ERROR("unable to find control block indexes for CCH: [event=%d]", p_msg->event);
#endif
                    success = FALSE;
                }
            } else {
                if(bta_hl_find_dch_cb_indexes((tBTA_HL_DATA *) p_msg, &app_idx, &mcl_idx, &mdl_idx)) {
                    bta_hl_dch_sm_execute(app_idx,
                                          mcl_idx,
                                          mdl_idx,
                                          p_msg->event, (tBTA_HL_DATA *) p_msg);
                } else {
#if BTA_HL_DEBUG == TRUE
                    APPL_TRACE_ERROR("unable to find control block indexes for DCH : [event=%s]",
                                     bta_hl_evt_code(p_msg->event));
#else
                    APPL_TRACE_ERROR("unable to find control block indexes for DCH: [event=%d]", p_msg->event);
#endif
                    success = FALSE;
                }
            }

            break;
    }

    return(success);
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if (BTA_HL_DEBUG == TRUE) && (BT_USE_TRACES == TRUE)

/*******************************************************************************
**
** Function         bta_hl_cch_state_code
**
** Description      Map CCH state code to the corresponding state string
**
** Returns          string pointer for the associated state name
**
*******************************************************************************/
static char *bta_hl_cch_state_code(tBTA_HL_CCH_STATE state_code)
{
    switch(state_code) {
        case BTA_HL_CCH_IDLE_ST:
            return "BTA_HL_CCH_IDLE_ST";

        case BTA_HL_CCH_OPENING_ST:
            return "BTA_HL_CCH_OPENING_ST";

        case BTA_HL_CCH_OPEN_ST:
            return "BTA_HL_CCH_OPEN_ST";

        case BTA_HL_CCH_CLOSING_ST:
            return "BTA_HL_CCH_CLOSING_ST";

        default:
            return "Unknown CCH state code";
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_state_code
**
** Description      Map DCH state code to the corresponding state string
**
** Returns          string pointer for the associated state name
**
*******************************************************************************/
static char *bta_hl_dch_state_code(tBTA_HL_DCH_STATE state_code)
{
    switch(state_code) {
        case BTA_HL_DCH_IDLE_ST:
            return "BTA_HL_DCH_IDLE_ST";

        case BTA_HL_DCH_OPENING_ST:
            return "BTA_HL_DCH_OPENING_ST";

        case BTA_HL_DCH_OPEN_ST:
            return "BTA_HL_DCH_OPEN_ST";

        case BTA_HL_DCH_CLOSING_ST:
            return "BTA_HL_DCH_CLOSING_ST";

        default:
            return "Unknown DCH state code";
    }
}
#endif  /* Debug Functions */
#endif /* HL_INCLUDED */

/******************************************************************************
 *
 *  Copyright (C) 2003-2012 Broadcom Corporation
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
 *  This is the private file for the file transfer client (FTC).
 *
 ******************************************************************************/
#ifndef BTA_GATTC_INT_H
#define BTA_GATTC_INT_H

#include "bt_target.h"

#include "osi/include/fixed_queue.h"
#include "bta_sys.h"
#include "bta_gatt_api.h"

#include "bt_common.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
enum {
    BTA_GATTC_API_OPEN_EVT   = BTA_SYS_EVT_START(BTA_ID_GATTC),
    BTA_GATTC_INT_OPEN_FAIL_EVT,
    BTA_GATTC_API_CANCEL_OPEN_EVT,
    BTA_GATTC_INT_CANCEL_OPEN_OK_EVT,

    BTA_GATTC_API_READ_EVT,
    BTA_GATTC_API_WRITE_EVT,
    BTA_GATTC_API_EXEC_EVT,
    BTA_GATTC_API_CFG_MTU_EVT,

    BTA_GATTC_API_CLOSE_EVT,

    BTA_GATTC_API_SEARCH_EVT,
    BTA_GATTC_API_CONFIRM_EVT,
    BTA_GATTC_API_READ_MULTI_EVT,
    BTA_GATTC_API_REFRESH_EVT,

    BTA_GATTC_INT_CONN_EVT,
    BTA_GATTC_INT_DISCOVER_EVT,
    BTA_GATTC_DISCOVER_CMPL_EVT,
    BTA_GATTC_OP_CMPL_EVT,
    BTA_GATTC_INT_DISCONN_EVT,

    BTA_GATTC_INT_START_IF_EVT,
    BTA_GATTC_API_REG_EVT,
    BTA_GATTC_API_DEREG_EVT,
    BTA_GATTC_API_LISTEN_EVT,
    BTA_GATTC_API_BROADCAST_EVT,
    BTA_GATTC_API_DISABLE_EVT,
    BTA_GATTC_ENC_CMPL_EVT
};
typedef uint16_t tBTA_GATTC_INT_EVT;

#define BTA_GATTC_SERVICE_CHANGED_LEN    4

/* max client application GATTC can support */
#ifndef     BTA_GATTC_CL_MAX
#define     BTA_GATTC_CL_MAX    32
#endif

/* max known devices GATTC can support */
#ifndef     BTA_GATTC_KNOWN_SR_MAX
#define     BTA_GATTC_KNOWN_SR_MAX    10
#endif

#define BTA_GATTC_CONN_MAX      GATT_MAX_PHY_CHANNEL

#ifndef BTA_GATTC_CLCB_MAX
#define BTA_GATTC_CLCB_MAX      GATT_CL_MAX_LCB
#endif

#define BTA_GATTC_WRITE_PREPARE          GATT_WRITE_PREPARE


/* internal strucutre for GATTC register API  */
typedef struct {
    BT_HDR                  hdr;
    tBT_UUID                app_uuid;
    tBTA_GATTC_CBACK        *p_cback;
} tBTA_GATTC_API_REG;

typedef struct {
    BT_HDR                  hdr;
    tBTA_GATTC_IF           client_if;
} tBTA_GATTC_INT_START_IF;

typedef tBTA_GATTC_INT_START_IF tBTA_GATTC_API_DEREG;
typedef tBTA_GATTC_INT_START_IF tBTA_GATTC_INT_DEREG;

typedef struct {
    BT_HDR                  hdr;
    BD_ADDR                 remote_bda;
    tBTA_GATTC_IF           client_if;
    uint8_t                 is_direct;
    tBTA_TRANSPORT          transport;
} tBTA_GATTC_API_OPEN;

typedef tBTA_GATTC_API_OPEN tBTA_GATTC_API_CANCEL_OPEN;

typedef struct {
    BT_HDR                  hdr;
    tBTA_GATT_AUTH_REQ      auth_req;
    uint16_t                  handle;
    tBTA_GATTC_EVT          cmpl_evt;
} tBTA_GATTC_API_READ;

typedef struct {
    BT_HDR                  hdr;
    tBTA_GATT_AUTH_REQ      auth_req;
    uint16_t                  handle;
    tBTA_GATTC_EVT          cmpl_evt;
    tBTA_GATTC_WRITE_TYPE   write_type;
    uint16_t                  offset;
    uint16_t                  len;
    uint8_t                   *p_value;
} tBTA_GATTC_API_WRITE;

typedef struct {
    BT_HDR                  hdr;
    uint8_t                 is_execute;
} tBTA_GATTC_API_EXEC;

typedef struct {
    BT_HDR                  hdr;
    uint16_t                  handle;
} tBTA_GATTC_API_CONFIRM;

typedef tGATT_CL_COMPLETE tBTA_GATTC_CMPL;

typedef struct {
    BT_HDR                  hdr;
    uint8_t                   op_code;
    tGATT_STATUS            status;
    tBTA_GATTC_CMPL         *p_cmpl;
} tBTA_GATTC_OP_CMPL;

typedef struct {
    BT_HDR              hdr;
    tBT_UUID            *p_srvc_uuid;
} tBTA_GATTC_API_SEARCH;

typedef struct {
    BT_HDR                  hdr;
    tBTA_GATT_AUTH_REQ      auth_req;
    uint8_t                   num_attr;
    uint16_t                  handles[GATT_MAX_READ_MULTI_HANDLES];
} tBTA_GATTC_API_READ_MULTI;

typedef struct {
    BT_HDR                  hdr;
    BD_ADDR_PTR             remote_bda;
    tBTA_GATTC_IF           client_if;
    uint8_t                 start;
} tBTA_GATTC_API_LISTEN;


typedef struct {
    BT_HDR              hdr;
    uint16_t              mtu;
} tBTA_GATTC_API_CFG_MTU;

typedef struct {
    BT_HDR                  hdr;
    BD_ADDR                 remote_bda;
    tBTA_GATTC_IF           client_if;
    uint8_t                   role;
    tBT_TRANSPORT           transport;
    tGATT_DISCONN_REASON    reason;
} tBTA_GATTC_INT_CONN;

typedef struct {
    BT_HDR                  hdr;
    BD_ADDR                 remote_bda;
    tBTA_GATTC_IF           client_if;
} tBTA_GATTC_ENC_CMPL;

typedef union {
    BT_HDR                      hdr;
    tBTA_GATTC_API_REG          api_reg;
    tBTA_GATTC_API_DEREG        api_dereg;
    tBTA_GATTC_API_OPEN         api_conn;
    tBTA_GATTC_API_CANCEL_OPEN  api_cancel_conn;
    tBTA_GATTC_API_READ         api_read;
    tBTA_GATTC_API_SEARCH       api_search;
    tBTA_GATTC_API_WRITE        api_write;
    tBTA_GATTC_API_CONFIRM      api_confirm;
    tBTA_GATTC_API_EXEC         api_exec;
    tBTA_GATTC_API_READ_MULTI   api_read_multi;
    tBTA_GATTC_API_CFG_MTU      api_mtu;
    tBTA_GATTC_OP_CMPL          op_cmpl;
    tBTA_GATTC_INT_CONN         int_conn;
    tBTA_GATTC_ENC_CMPL         enc_cmpl;

    tBTA_GATTC_INT_START_IF     int_start_if;
    tBTA_GATTC_INT_DEREG        int_dereg;
    /* if peripheral role is supported */
    tBTA_GATTC_API_LISTEN       api_listen;

} tBTA_GATTC_DATA;


/* GATT server cache on the client */

typedef struct {
    tBT_UUID            uuid;
    uint16_t              s_handle;
    uint16_t              e_handle;
    // this field is set only for characteristic
    uint16_t              char_decl_handle;
    uint8_t             is_primary;
    tBTA_GATT_CHAR_PROP property;
} tBTA_GATTC_ATTR_REC;


#define BTA_GATTC_MAX_CACHE_CHAR    40
#define BTA_GATTC_ATTR_LIST_SIZE    (BTA_GATTC_MAX_CACHE_CHAR * sizeof(tBTA_GATTC_ATTR_REC))

#ifndef BTA_GATTC_CACHE_SRVR_SIZE
#define BTA_GATTC_CACHE_SRVR_SIZE   600
#endif

enum {
    BTA_GATTC_IDLE_ST = 0,      /* Idle  */
    BTA_GATTC_W4_CONN_ST,       /* Wait for connection -  (optional) */
    BTA_GATTC_CONN_ST,          /* connected state */
    BTA_GATTC_DISCOVER_ST       /* discover is in progress */
};
typedef uint8_t tBTA_GATTC_STATE;

typedef struct {
    uint8_t             in_use;
    BD_ADDR             server_bda;
    uint8_t             connected;

#define BTA_GATTC_SERV_IDLE     0
#define BTA_GATTC_SERV_LOAD     1
#define BTA_GATTC_SERV_SAVE     2
#define BTA_GATTC_SERV_DISC     3
#define BTA_GATTC_SERV_DISC_ACT 4

    uint8_t               state;

    list_t              *p_srvc_cache;  /* list of tBTA_GATTC_SERVICE */
    uint8_t               update_count;   /* indication received */
    uint8_t               num_clcb;       /* number of associated CLCB */


    tBTA_GATTC_ATTR_REC *p_srvc_list;
    uint8_t               cur_srvc_idx;
    uint8_t               cur_char_idx;
    uint8_t               next_avail_idx;
    uint8_t               total_srvc;
    uint8_t               total_char;

    uint8_t               srvc_hdl_chg;   /* service handle change indication pending */
    uint16_t              attr_index;     /* cahce NV saving/loading attribute index */

    uint16_t              mtu;
} tBTA_GATTC_SERV;

#ifndef BTA_GATTC_NOTIF_REG_MAX
#define BTA_GATTC_NOTIF_REG_MAX     15
#endif

typedef struct {
    uint8_t             in_use;
    BD_ADDR             remote_bda;
    uint16_t              handle;
} tBTA_GATTC_NOTIF_REG;

typedef struct {
    tBTA_GATTC_CBACK        *p_cback;
    uint8_t                 in_use;
    tBTA_GATTC_IF           client_if;      /* client interface with BTE stack for this application */
    uint8_t                   num_clcb;       /* number of associated CLCB */
    uint8_t                 dereg_pending;
    tBT_UUID                app_uuid;
    tBTA_GATTC_NOTIF_REG    notif_reg[BTA_GATTC_NOTIF_REG_MAX];
} tBTA_GATTC_RCB;

/* client channel is a mapping between a BTA client(cl_id) and a remote BD address */
typedef struct {
    uint16_t              bta_conn_id;    /* client channel ID, unique for clcb */
    BD_ADDR             bda;
    tBTA_TRANSPORT      transport;      /* channel transport */
    tBTA_GATTC_RCB      *p_rcb;         /* pointer to the registration CB */
    tBTA_GATTC_SERV     *p_srcb;    /* server cache CB */
    tBTA_GATTC_DATA     *p_q_cmd;   /* command in queue waiting for execution */

#define BTA_GATTC_NO_SCHEDULE       0
#define BTA_GATTC_DISC_WAITING      0x01
#define BTA_GATTC_REQ_WAITING       0x10

    uint8_t               auto_update; /* auto update is waiting */
    uint8_t             disc_active;
    uint8_t             in_use;
    tBTA_GATTC_STATE    state;
    tBTA_GATT_STATUS    status;
    uint16_t              reason;
} tBTA_GATTC_CLCB;

/* back ground connection tracking information */
#if GATT_MAX_APPS <= 8
typedef uint8_t tBTA_GATTC_CIF_MASK ;
#elif GATT_MAX_APPS <= 16
typedef uint16_t tBTA_GATTC_CIF_MASK;
#elif GATT_MAX_APPS <= 32
typedef uint32_t tBTA_GATTC_CIF_MASK;
#endif

typedef struct {
    uint8_t                 in_use;
    BD_ADDR                 remote_bda;
    tBTA_GATTC_CIF_MASK     cif_mask;
    tBTA_GATTC_CIF_MASK     cif_adv_mask;

} tBTA_GATTC_BG_TCK;

typedef struct {
    uint8_t             in_use;
    BD_ADDR             remote_bda;
} tBTA_GATTC_CONN;

enum {
    BTA_GATTC_STATE_DISABLED,
    BTA_GATTC_STATE_ENABLING,
    BTA_GATTC_STATE_ENABLED,
    BTA_GATTC_STATE_DISABLING
};

typedef struct {
    uint8_t             state;

    tBTA_GATTC_CONN     conn_track[BTA_GATTC_CONN_MAX];
    tBTA_GATTC_BG_TCK   bg_track[BTA_GATTC_KNOWN_SR_MAX];
    tBTA_GATTC_RCB      cl_rcb[BTA_GATTC_CL_MAX];

    tBTA_GATTC_CLCB     clcb[BTA_GATTC_CLCB_MAX];
    tBTA_GATTC_SERV     known_server[BTA_GATTC_KNOWN_SR_MAX];
} tBTA_GATTC_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* GATTC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_GATTC_CB  bta_gattc_cb;
#else
extern tBTA_GATTC_CB *bta_gattc_cb_ptr;
#define bta_gattc_cb (*bta_gattc_cb_ptr)
#endif

/*****************************************************************************
**  Function prototypes
*****************************************************************************/
extern uint8_t bta_gattc_hdl_event(BT_HDR *p_msg);
extern uint8_t bta_gattc_sm_execute(tBTA_GATTC_CLCB *p_clcb, uint16_t event,
                                    tBTA_GATTC_DATA *p_data);

/* function processed outside SM */
extern void bta_gattc_disable(tBTA_GATTC_CB *p_cb);
extern void bta_gattc_register(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_start_if(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_process_api_open(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_msg);
extern void bta_gattc_process_api_open_cancel(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_msg);
extern void bta_gattc_deregister(tBTA_GATTC_CB *p_cb, tBTA_GATTC_RCB  *p_clreg);
extern void bta_gattc_process_enc_cmpl(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_msg);

/* function within state machine */
extern void bta_gattc_open(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_open_fail(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_open_error(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);

extern void bta_gattc_cancel_open(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_cancel_open_ok(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_cancel_open_error(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);

extern void bta_gattc_conn(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);

extern void bta_gattc_close(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_close_fail(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_disc_close(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);

extern void bta_gattc_start_discover(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_disc_cmpl(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_read(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_write(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_op_cmpl(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_q_cmd(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_search(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_fail(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_confirm(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_execute(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_read_multi(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_ci_open(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_ci_close(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_ignore_op_cmpl(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
extern void bta_gattc_restart_discover(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_msg);
extern void bta_gattc_init_bk_conn(tBTA_GATTC_API_OPEN *p_data, tBTA_GATTC_RCB *p_clreg);
extern void bta_gattc_cancel_bk_conn(tBTA_GATTC_API_CANCEL_OPEN *p_data);
extern void bta_gattc_send_open_cback(tBTA_GATTC_RCB *p_clreg, tBTA_GATT_STATUS status,
                                      BD_ADDR remote_bda, uint16_t conn_id, tBTA_TRANSPORT transport,  uint16_t mtu);
extern void bta_gattc_process_api_refresh(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_msg);
extern void bta_gattc_cfg_mtu(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);
#if BLE_INCLUDED == TRUE
extern void bta_gattc_listen(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_msg);
extern void bta_gattc_broadcast(tBTA_GATTC_CB *p_cb, tBTA_GATTC_DATA *p_msg);
#endif
/* utility functions */
extern tBTA_GATTC_CLCB *bta_gattc_find_clcb_by_cif(uint8_t client_if, BD_ADDR remote_bda,
        tBTA_TRANSPORT transport);
extern tBTA_GATTC_CLCB *bta_gattc_find_clcb_by_conn_id(uint16_t conn_id);
extern tBTA_GATTC_CLCB *bta_gattc_clcb_alloc(tBTA_GATTC_IF client_if, BD_ADDR remote_bda,
        tBTA_TRANSPORT transport);
extern void bta_gattc_clcb_dealloc(tBTA_GATTC_CLCB *p_clcb);
extern tBTA_GATTC_CLCB *bta_gattc_find_alloc_clcb(tBTA_GATTC_IF client_if, BD_ADDR remote_bda,
        tBTA_TRANSPORT transport);
extern tBTA_GATTC_RCB *bta_gattc_cl_get_regcb(uint8_t client_if);
extern tBTA_GATTC_SERV *bta_gattc_find_srcb(BD_ADDR bda);
extern tBTA_GATTC_SERV *bta_gattc_srcb_alloc(BD_ADDR bda);
extern tBTA_GATTC_SERV *bta_gattc_find_scb_by_cid(uint16_t conn_id);
extern tBTA_GATTC_CLCB *bta_gattc_find_int_conn_clcb(tBTA_GATTC_DATA *p_msg);
extern tBTA_GATTC_CLCB *bta_gattc_find_int_disconn_clcb(tBTA_GATTC_DATA *p_msg);

extern uint8_t bta_gattc_enqueue(tBTA_GATTC_CLCB *p_clcb, tBTA_GATTC_DATA *p_data);

extern uint8_t bta_gattc_uuid_compare(const tBT_UUID *p_src, const tBT_UUID *p_tar,
                                      uint8_t is_precise);
extern uint8_t bta_gattc_check_notif_registry(tBTA_GATTC_RCB  *p_clreg, tBTA_GATTC_SERV *p_srcb,
        tBTA_GATTC_NOTIFY  *p_notify);
extern uint8_t bta_gattc_mark_bg_conn(tBTA_GATTC_IF client_if,  BD_ADDR_PTR remote_bda, uint8_t add,
                                      uint8_t is_listen);
extern uint8_t bta_gattc_check_bg_conn(tBTA_GATTC_IF client_if,  BD_ADDR remote_bda, uint8_t role);
extern uint8_t bta_gattc_num_reg_app(void);
extern void bta_gattc_clear_notif_registration(tBTA_GATTC_SERV *p_srcb, uint16_t conn_id,
        uint16_t start_handle, uint16_t end_handle);
extern tBTA_GATTC_SERV *bta_gattc_find_srvr_cache(BD_ADDR bda);

/* discovery functions */
extern void bta_gattc_disc_res_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                                     tGATT_DISC_RES *p_data);
extern void bta_gattc_disc_cmpl_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                                      tGATT_STATUS status);
extern tBTA_GATT_STATUS bta_gattc_discover_procedure(uint16_t conn_id, tBTA_GATTC_SERV *p_server_cb,
        uint8_t disc_type);
extern tBTA_GATT_STATUS bta_gattc_discover_pri_service(uint16_t conn_id,
        tBTA_GATTC_SERV *p_server_cb, uint8_t disc_type);
extern void bta_gattc_search_service(tBTA_GATTC_CLCB *p_clcb, tBT_UUID *p_uuid);
extern const list_t *bta_gattc_get_services(uint16_t conn_id);
extern const tBTA_GATTC_SERVICE *bta_gattc_get_service_for_handle(uint16_t conn_id,
        uint16_t handle);
tBTA_GATTC_CHARACTERISTIC  *bta_gattc_get_characteristic_srcb(tBTA_GATTC_SERV *p_srcb,
        uint16_t handle);
extern tBTA_GATTC_CHARACTERISTIC *bta_gattc_get_characteristic(uint16_t conn_id, uint16_t handle);
extern tBTA_GATTC_DESCRIPTOR *bta_gattc_get_descriptor(uint16_t conn_id, uint16_t handle);
extern void bta_gattc_get_gatt_db(uint16_t conn_id, uint16_t start_handle, uint16_t end_handle,
                                  tls_btgatt_db_element_t **db, int *count);
extern tBTA_GATT_STATUS bta_gattc_init_cache(tBTA_GATTC_SERV *p_srvc_cb);
extern void bta_gattc_rebuild_cache(tBTA_GATTC_SERV *p_srcv, uint16_t num_attr,
                                    tBTA_GATTC_NV_ATTR *attr);
extern void bta_gattc_cache_save(tBTA_GATTC_SERV *p_srvc_cb, uint16_t conn_id);
extern void bta_gattc_reset_discover_st(tBTA_GATTC_SERV *p_srcb, tBTA_GATT_STATUS status);

extern tBTA_GATTC_CONN *bta_gattc_conn_alloc(BD_ADDR remote_bda);
extern tBTA_GATTC_CONN *bta_gattc_conn_find(BD_ADDR remote_bda);
extern tBTA_GATTC_CONN *bta_gattc_conn_find_alloc(BD_ADDR remote_bda);
extern uint8_t bta_gattc_conn_dealloc(BD_ADDR remote_bda);

extern uint8_t bta_gattc_cache_load(tBTA_GATTC_CLCB *p_clcb);
extern void bta_gattc_cache_reset(BD_ADDR server_bda);

#endif /* BTA_GATTC_INT_H */

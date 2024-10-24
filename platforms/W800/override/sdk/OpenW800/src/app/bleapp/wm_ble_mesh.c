/*****************************************************************************
**
**  Name:           wm_ble_mesh.c
**
**  Description:    This file contains the sample functions for mesh application
**
*****************************************************************************/


#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)

#include "mesh/mesh.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "mesh/glue.h"
#include "wm_bt_util.h"
#include "wm_ble_mesh.h"
#include "wm_ble_gap.h"
#include "wm_gpio.h"

/*Mesh models*/
#include "mesh/model_cli.h"
#include "mesh/model_srv.h"
#include "mesh/cfg_cli.h"
#include "wm_ble_mesh_light_model.h"

#include "wm_bt_app.h"
#include "wm_efuse.h"

#include "wm_ble_mesh_health_server.h"
#include "wm_ble_mesh_vnd_model.h"

#define CID_NVAL 0xFFFF

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
 
#define MESH_LIGHT_GPIO_GREEN   WM_IO_PB_00
#define MESH_LIGHT_GPIO_RED     WM_IO_PB_01
#define MESH_LIGHT_GPIO_BLUE    WM_IO_PB_02

static struct ble_gap_event_listener app_mesh_event_listener;
static bool filter_unprov_adv_report = true;
static tls_bt_mesh_at_callback_t at_cb_ptr = NULL;

static uint16_t primary_addr;
static uint16_t primary_net_idx;

#if MYNEWT_VAL(BLE_MESH_CFG_CLI)

static struct bt_mesh_cfg_cli cfg_cli = {
};
static struct bt_mesh_gen_model_cli gen_onoff_cli;
/*predeclaration general on off client model publication update function*/
static int gen_onoff_cli_pub_update(struct bt_mesh_model *mod);
static struct bt_mesh_model_pub gen_onoff_cli_pub = {
    .update = gen_onoff_cli_pub_update,
};

static struct bt_mesh_gen_onoff_srv gen_onoff_srv = {
    .get = tls_light_model_gen_onoff_get,
    .set = tls_light_model_gen_onoff_set,
};
static struct bt_mesh_model_pub gen_onoff_srv_pub;

#endif /* MYNEWT_VAL(BLE_MESH_CFG_CLI) */

static void cfg_heartbeat_sub_func(uint8_t hops, uint16_t feat)
{
    TLS_BT_APPL_TRACE_DEBUG("cfg_heartbeat_sub_func: %d, 0x%04x\r\n", hops, feat);
    return;
}

static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BT_MESH_RELAY_ENABLED,
    .beacon = BT_MESH_BEACON_ENABLED,
#if MYNEWT_VAL(BLE_MESH_FRIEND)
    .frnd = BT_MESH_FRIEND_ENABLED,
#else
    .gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
#if MYNEWT_VAL(BLE_MESH_GATT_PROXY)
    .gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,

    /* 3 transmissions with 20ms interval */
    .net_transmit = BT_MESH_TRANSMIT(2, 20),
    .relay_retransmit = BT_MESH_TRANSMIT(2, 20),
    .hb_sub.func = cfg_heartbeat_sub_func,
    .dist_map = {0},
    
};



static struct bt_mesh_model node_root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
#if MYNEWT_VAL(BLE_MESH_CFG_CLI)
    BT_MESH_MODEL_CFG_CLI(&cfg_cli),
#endif
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

static struct bt_mesh_model node_sec_models[] = {

    BT_MESH_MODEL_GEN_ONOFF_CLI(&gen_onoff_cli, &gen_onoff_cli_pub),    
    BT_MESH_MODEL_GEN_ONOFF_SRV(&gen_onoff_srv, &gen_onoff_srv_pub),
};


static struct bt_mesh_model node_root_vnd_models[] = {
    #if 0
    BT_MESH_MODEL_VND(CID_VENDOR, BT_MESH_MODEL_ID_GEN_ONOFF_SRV, vnd_model_op,
                      /*&vnd_model_pub*/NULL, NULL),
    #endif
    BT_MESH_MODEL_WM_VND(NULL),
};

static struct bt_mesh_model node_sec_vnd_models[] = {

};


static struct bt_mesh_elem node_elements[] = {
    BT_MESH_ELEM(0, node_root_models, node_root_vnd_models),
    BT_MESH_ELEM(1, node_sec_models,  node_sec_vnd_models),
};

static const struct bt_mesh_comp node_comp = {
    .cid = CID_VENDOR,
    .elem = node_elements,
    .elem_count = ARRAY_SIZE(node_elements),
};


static struct bt_mesh_model provisioner_root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
#if MYNEWT_VAL(BLE_MESH_CFG_CLI)
    BT_MESH_MODEL_CFG_CLI(&cfg_cli),
#endif

    BT_MESH_MODEL_GEN_ONOFF_CLI(&gen_onoff_cli, &gen_onoff_cli_pub),

#if MYNEWT_VAL(BLE_MESH_HEALTH_CLI)
    BT_MESH_MODEL_HEALTH_CLI(&health_cli),
#endif

};

static struct bt_mesh_model provisioner_vnd_models[] = {
    BT_MESH_MODEL_VND(CID_VENDOR, BT_MESH_MODEL_ID_GEN_ONOFF_CLI, vnd_model_op,
                      &vnd_model_pub, NULL),
};

static struct bt_mesh_elem provisioner_elements[] = {
    BT_MESH_ELEM(0, provisioner_root_models, provisioner_vnd_models),
};

static const struct bt_mesh_comp provisioner_comp = {
    .cid = CID_VENDOR,
    .elem = provisioner_elements,
    .elem_count = ARRAY_SIZE(provisioner_elements),
};

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void
pub_init(void)
{
    health_pub.msg  = BT_MESH_HEALTH_FAULT_MSG(0);
    gen_onoff_cli_pub.msg = NET_BUF_SIMPLE(2 + 2);
}
static void
pub_deinit(void)
{
    if(health_pub.msg) {
        os_mbuf_free_chain(health_pub.msg);
        health_pub.msg = NULL;
    }
    
    if(gen_onoff_cli_pub.msg) {
        os_mbuf_free_chain(gen_onoff_cli_pub.msg);
        gen_onoff_cli_pub.msg = NULL;
    }
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
    tls_mesh_event_t evt = WM_MESH_OOB_NUMBER_EVT;
    tls_mesh_msg_t msg;
    TLS_BT_APPL_TRACE_DEBUG("OOB Number: %lu\r\n", number);
    msg.oob_output_number_msg.number = number;
    TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
    return 0;
}
static int output_string(const char *str)
{
    tls_mesh_event_t evt = WM_MESH_OOB_STRING_EVT;
    tls_mesh_msg_t msg;
    msg.oob_output_string_msg.str = (char*)str;
    TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
    return 0;
}
static int prov_input(bt_mesh_input_action_t act, u8_t size)
{
    tls_mesh_event_t evt = WM_MESH_OOB_INPUT_EVT;
    tls_mesh_msg_t msg;
    TLS_BT_APPL_TRACE_DEBUG("OOB input: act=0x%02x, size=%d\r\n", act, size);
    msg.oob_input_msg.act = act;
    TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
    return 0;
}
static void prov_input_complete(void)
{
    TLS_BT_APPL_TRACE_DEBUG("prov_input_complete\r\n");
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
    tls_mesh_event_t evt = WM_MESH_PROV_CMPLT_EVT;
    tls_mesh_msg_t msg;
    TLS_BT_APPL_TRACE_DEBUG("Local node provisioned, primary address 0x%04x, net_idx=%d\r\n", addr,
                            net_idx);
    primary_addr = addr;
    primary_net_idx = net_idx;
    msg.prov_cmplt_msg.addr = addr;
    msg.prov_cmplt_msg.net_idx = net_idx;
    TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
}

static uint16_t node_added_net_idx = 0;
static uint16_t node_added_addr = 0;
static uint8_t  node_added_num_elem = 0;

static void prov_node_added(u16_t net_idx, u16_t addr, u8_t num_elem)
{
    //tls_mesh_event_t evt = WM_MESH_NODE_ADDED_EVT;
    //tls_mesh_msg_t msg;
    TLS_BT_APPL_TRACE_DEBUG("Prov node added, primary address 0x%04x, net_idx=%d, num_elem=%d\r\n",
                            addr, net_idx, num_elem);
    //msg.node_added_msg.net_idx = net_idx;
    //msg.node_added_msg.addr = addr;
    //msg.node_added_msg.num_elem = num_elem;
    node_added_net_idx = net_idx;
    node_added_addr = addr;
    node_added_num_elem = num_elem;
    //WM_MESH_PROV_END_EVT will report the prov result at last.
    //TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
}
static void prov_link_close(bt_mesh_prov_bearer_t bearer, bool prov_success)
{
    tls_mesh_event_t evt = WM_MESH_PROV_END_EVT;
    tls_mesh_msg_t msg;
    TLS_BT_APPL_TRACE_DEBUG("prov_link_close: bearer=0x%02x, status:%s\r\n", bearer,
                            prov_success ? "success" : "failed");
    msg.prov_end_msg.success = prov_success;

    if(prov_success) {
        msg.prov_end_msg.net_idx = node_added_net_idx;
        msg.prov_end_msg.addr =  node_added_addr;
        msg.prov_end_msg.num_elem = node_added_num_elem;
    } else {
        msg.prov_end_msg.net_idx = 0xFFFF;
        msg.prov_end_msg.addr =    0xFFFF;
        msg.prov_end_msg.num_elem = 0xFF;
    }

    TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
}

static void prov_link_open(bt_mesh_prov_bearer_t bearer)
{
    TLS_BT_APPL_TRACE_DEBUG("prov_link_open: bearer=0x%02x\r\n", bearer);    
}
static void unprovisioned_beacon(const bt_addr_le_t *addr, u8_t uuid[16],
                                 bt_mesh_prov_oob_info_t oob_info,
                                 u32_t *uri_hash)
{
    tls_mesh_event_t evt = WM_MESH_UNPROVISION_BEACON_EVT;
    tls_mesh_msg_t msg;

    if(filter_unprov_adv_report) { return; }

    TLS_BT_APPL_TRACE_DEBUG("unprovisioned_beacon: [%s][%s] oob_info:0x%04x, net_idx=%d\r\n",
                            bt_hex(addr->val, 6), bt_hex(uuid, 16), oob_info, uri_hash[0]);

    if(at_cb_ptr) {
        memcpy(msg.unprov_msg.addr, addr->val, 6);
        msg.unprov_msg.addr_type = addr->type;
        memcpy(msg.unprov_msg.uuid, uuid, 16);
        msg.unprov_msg.oob_info = oob_info;
        msg.unprov_msg.uri_hash = uri_hash[0];
        TLS_HAL_AT_NOTIFY(at_cb_ptr, evt, &msg);
    }
}

static uint8_t dev_uuid[16] = MYNEWT_VAL(BLE_MESH_DEV_UUID);
static char    dev_name[16] = CONFIG_BT_DEVICE_NAME;

static const struct bt_mesh_prov prov = {
    .name = dev_name,
    .uuid = dev_uuid,
    .output_size = 4,
    .output_actions = BT_MESH_DISPLAY_NUMBER | BT_MESH_BEEP | BT_MESH_VIBRATE | BT_MESH_BLINK,
    .output_number = output_number,
    .output_string = output_string,
    .input_size = 4,
    .input_actions = BT_MESH_DISPLAY_NUMBER | BT_MESH_BEEP | BT_MESH_VIBRATE | BT_MESH_BLINK,
    .input = prov_input,
    .input_complete = prov_input_complete,
    .complete = prov_complete,
    .node_added = prov_node_added,
    .unprovisioned_beacon = unprovisioned_beacon,
    .link_close = prov_link_close,
    .link_open = prov_link_open,
};

static int ble_gap_host_shutdown_cb(struct ble_gap_event *event, void *arg)
{
	if(BLE_GAP_EVENT_HOST_SHUTDOWN == event->type)
	{
        tls_bt_set_mesh_mode(0);
        bt_mesh_deinit();
        pub_deinit();
        TLS_BT_APPL_TRACE_DEBUG("Mesh deinitialized\r\n");
        at_cb_ptr = NULL;
	}

    return 0;
}
static uint8_t transaction_id = 0;

static int gen_onoff_cli_pub_update(struct bt_mesh_model *mod)
{
    TLS_BT_APPL_TRACE_DEBUG("gen_onoff_cli_pub_update\r\n");
    static bool general_onoff_state = false;
    CHECK_SYSTEM_READY();
    #define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK      BT_MESH_MODEL_OP_2(0x82, 0x03)
    #define BT_MESH_MODEL_OP_GEN_ONOFF_SET            BT_MESH_MODEL_OP_2(0x82, 0x02)

    TLS_BT_APPL_TRACE_DEBUG("gen off client publish update to 0x%04x onoff 0x%04x \r\n", gen_onoff_cli_pub.addr, general_onoff_state);
    
    bt_mesh_model_msg_init(gen_onoff_cli_pub.msg,BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
    net_buf_simple_add_u8(gen_onoff_cli_pub.msg, general_onoff_state);
    net_buf_simple_add_u8(gen_onoff_cli_pub.msg, transaction_id++);
    {
        general_onoff_state = !general_onoff_state;
    }

    return 0;    
}

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int
tls_ble_mesh_deinit()
{
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("Mesh freeing resouce\r\n");
    if(at_cb_ptr == NULL) return 0;
    tls_bt_set_mesh_mode(0);
    bt_mesh_deinit();
    pub_deinit();
    TLS_BT_APPL_TRACE_DEBUG("Mesh deinitialized\r\n");
    at_cb_ptr = NULL;
    
    ble_gap_event_listener_unregister(&app_mesh_event_listener);

    return 0;
}
static void notify_flash_flush_available(void)
{
#if (MYNEWT_VAL(BLE_MESH_SETTINGS))
    bt_mesh_settings_flush();
#endif
}


/**
 * mesh main entry.
 *
 * @tls_bt_mesh_at_callback_t  notify the mesh evt to AT level
 * @param role             MESH_ROLE_NODE or MESH_ROLE_PROVISIONER.
 * @param running       running the mesh after initializing
 *
 * @return 0 on success.
 * @return -2 means the role mismatch.
 */

int
tls_ble_mesh_init(tls_bt_mesh_at_callback_t at_cb, tls_bt_mesh_role_t role, bool running)
{
    int err = 0;
    ble_addr_t addr = {0};
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("Mesh initializing...role=%d\r\n", role);
    if(at_cb_ptr) return 0;
    
    tls_bt_set_mesh_mode(1);
    at_cb_ptr = at_cb;
    pub_init();
#define NRPA   0 
    /*default we use nrpa address*/
    #if NRPA
    /* Use NRPA */
    err = ble_hs_id_gen_rnd(1, &addr);
    assert(err == 0);
    err = ble_hs_id_set_rnd(addr.val);
    assert(err == 0);
    #endif
    tls_ble_gap_get_name(dev_name);
    /*using BT MAC as device uuid high 6 bytes*/
    tls_get_bt_mac_addr(&dev_uuid[10]);

    if(role == MESH_ROLE_NODE) {
        err = bt_mesh_init(addr.type, &prov, &node_comp);
    } else {
        err = bt_mesh_init(addr.type, &prov, &provisioner_comp);
    }

    if(err) {
        TLS_BT_APPL_TRACE_DEBUG("Initializing mesh %s failed (err %d)\r\n",
                                role == MESH_ROLE_NODE ? "Node" : "Provisioner", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("Mesh(%s) initialized\r\n", dev_name);
#if (MYNEWT_VAL(BLE_MESH_SETTINGS))
    tls_bt_register_pending_process_callback(notify_flash_flush_available);
    err = bt_mesh_settings_load((role == MESH_ROLE_NODE ? true : false));
#endif

    if(err == -2) {
        TLS_BT_APPL_TRACE_DEBUG("Mesh nvram parameter mismatch with the prefered role, Please erase the nvram or change the role\r\n");
        return err;
    } else {
        /**if invalid nvram area, return success*/
        err = 0;
    }

    TLS_BT_APPL_TRACE_DEBUG("running role=%d, err=%d\r\n", role, err);

    if(role == MESH_ROLE_NODE) {
        TLS_BT_APPL_TRACE_DEBUG("node role enabled\r\n");
        bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
        bt_mesh_role_set(true);
    } else {
        /**For provisioner, start scan if necessary*/
        TLS_BT_APPL_TRACE_DEBUG("provisioner role enabled\r\n");
        if(running) { bt_mesh_scan_enable(); }
    }

    if(bt_mesh_is_provisioned()) {
        TLS_BT_APPL_TRACE_DEBUG("Mesh network restored from flash\r\n");

        if(bt_mesh_role_is_provisioner()) {
            bt_mesh_comp_provision(1);
        }
    } else {
        if((bt_mesh_get_role() == MESH_ROLE_PROVISIONER || bt_mesh_get_role() == MESH_ROLE_UNKNOWN)
                && (role == MESH_ROLE_PROVISIONER)) {
            TLS_BT_APPL_TRACE_DEBUG("provisioner role init\r\n");
            bt_mesh_provision_init();
        }
    }
    
    ble_gap_event_listener_register(&app_mesh_event_listener,
                                ble_gap_host_shutdown_cb, NULL);

    return err;
}

static uint16_t mesh_uart_dst = 0x02;

int tls_ble_mesh_vnd_send_msg(uint8_t *msg, int len)
{
    int rc;
    
 #if 1  
    TLS_BT_APPL_TRACE_VERBOSE("bt_mesh_send %d/%d/%d/%d %s\r\n", 0, mesh_uart_dst, 0, len, bt_hex(msg, len));

    rc = vnd_model_send(0, mesh_uart_dst, 0, msg, len);

#else
    int i = 0;
    static uint8_t value = 0x01;
    uint8_t test_buf[255];

    for(i=0; i<sizeof(test_buf); i++) test_buf[i] = value;
    value ++;
    if(value == 0x00) value++;

    TLS_BT_APPL_TRACE_DEBUG("bt_mesh_send %d/%d/%d/%d %s\r\n", 0, 4, 0, len, bt_hex(test_buf, 220));

    rc = vnd_model_send(0, mesh_uart_dst, 0, test_buf, 200);    
#endif
    if(rc!= 0)
    {
       TLS_BT_APPL_TRACE_ERROR("rc=%d, upper layer will retry\r\n", rc) 
    }
    return rc;
}


int tls_ble_mesh_gen_level_get(uint16_t net_idx, uint16_t dst, uint16_t app_idx, int16_t *state)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_gen_level_get(net_idx, dst, app_idx, state);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Failed to send Generic Level Get (err %d)\r\n", err);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Gen Level State 0x%04x\r\n", state[0]);
    }

    return err;
}


int tls_ble_mesh_gen_level_set(uint16_t net_idx, uint16_t dst, uint16_t app_idx, int16_t val,
                               int16_t *state)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_gen_level_set(net_idx, dst, app_idx, val, state);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Failed to send Generic Level Set (err %d)\r\n", err);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Gen Level State 0x%04x\r\n", state[0]);
    }

    return err;
}

int tls_ble_mesh_gen_off_publish(uint8_t onoff_state)
{
    int err;
    CHECK_SYSTEM_READY();
    #define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK      BT_MESH_MODEL_OP_2(0x82, 0x03)
    #define BT_MESH_MODEL_OP_GEN_ONOFF_SET            BT_MESH_MODEL_OP_2(0x82, 0x02)

    TLS_BT_APPL_TRACE_DEBUG("gen off client publish to 0x%04x onoff 0x%04x \r\n", gen_onoff_cli_pub.addr, onoff_state);
    
    bt_mesh_model_msg_init(gen_onoff_cli_pub.msg,BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
    net_buf_simple_add_u8(gen_onoff_cli_pub.msg, onoff_state);
    net_buf_simple_add_u8(gen_onoff_cli_pub.msg, transaction_id++);
    err = bt_mesh_model_publish(gen_onoff_cli_pub.mod);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("bt_mesh_model_publish err %d\r\n", err);
    }

    return err;
    
}

int tls_ble_mesh_gen_onoff_get(uint16_t net_idx, uint16_t dst, uint16_t app_idx, uint8_t *state)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_gen_onoff_get(net_idx, dst, app_idx, state);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Failed to send Generic OnOff Get (err %d)\r\n", err);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Gen OnOff State 0x%02x\r\n", state[0]);
    }

    return err;
}
#define SEND_WITH_ACK 0
static int send_counter = 0, err_counter = 0;
int tls_ble_mesh_gen_onoff_set(uint16_t net_idx, uint16_t dst, uint16_t app_idx, uint8_t val,
                               uint8_t *state)
{
    int err;
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_gen_onoff_set, dst 0x%04x, val %d\r\n", dst, val);
    CHECK_SYSTEM_READY();
    send_counter++;
    #if SEND_WITH_ACK
    err = bt_mesh_gen_onoff_set(net_idx, dst, app_idx, val, state);
    #else
    err = bt_mesh_gen_onoff_set(net_idx, dst, app_idx, val, NULL);
    #endif
    if(err) {
        err_counter++;
        TLS_BT_APPL_TRACE_ERROR("Failed to send Generic OnOff Set (err %d)\r\n", err);
    } else {
    #if SEND_WITH_ACK
        TLS_BT_APPL_TRACE_DEBUG("Gen OnOff State 0x%02x[%d,%d]\r\n", state[0],send_counter, err_counter);
    #else
        TLS_BT_APPL_TRACE_DEBUG("Gen OnOff State 0x%02x[%d,%d]\r\n", val,send_counter, err_counter);
    #endif
    }

    return err;
}
                               
int tls_ble_mesh_hb_sub_set(uint16_t net_idx, uint16_t dst, tls_bt_mesh_cfg_hb_sub *hb_sub, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    err = bt_mesh_cfg_hb_sub_set(net_idx, dst, (struct bt_mesh_cfg_hb_sub *)hb_sub, status);
    if(err)
    {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Sublication Set failed (err %d)\n", err);
        return err;        
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Sublication Set failed (status 0x%02x)\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model HeartBeat Sublication successfully set\n");
    }

    return err;    
}

int tls_ble_mesh_hb_sub_get(uint16_t net_idx, uint16_t dst, tls_bt_mesh_cfg_hb_sub *hb_sub, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    err = bt_mesh_cfg_hb_sub_get(net_idx, dst, (struct bt_mesh_cfg_hb_sub *)hb_sub, status);
    if(err)
    {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Sublication Get failed (err %d)\n", err);
        return err;        
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Sublication Get failed (status 0x%02x)\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model HeartBeat Sublication Get successfully \n");
    }

    return err;    
}

int tls_ble_mesh_hb_pub_set(uint16_t net_idx, uint16_t dst, const tls_bt_mesh_cfg_hb_pub *hb_pub, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    err = bt_mesh_cfg_hb_pub_set(net_idx, dst, (const struct bt_mesh_cfg_hb_pub *)hb_pub, status);

    if(err)
    {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Publication Set failed (err %d)\n", err);
        return err;        
    }
    
    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Publication Set failed (status 0x%02x)\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model HeartBeat Publication successfully set\n");
    }

    return err;    
}

int tls_ble_mesh_hb_pub_get(uint16_t net_idx, uint16_t dst, tls_bt_mesh_cfg_hb_pub *hb_pub, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    err = bt_mesh_cfg_hb_pub_get(net_idx, dst, (struct bt_mesh_cfg_hb_pub *)hb_pub, status);
    if(err)
    {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Publication Get failed (err %d)\n", err);
        return err;        
    }
    
    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model HeartBeat Publication Get failed (status 0x%02x)\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model HeartBeat Publication Get successfully\n");
    }

    return err;  
}

int tls_ble_mesh_pub_set(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t mod_id,
                         uint16_t cid,
                         tls_bt_mesh_cfg_mod_pub *pub, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_pub_set(net_idx, dst, elem_addr,
                                      mod_id, (struct bt_mesh_cfg_mod_pub *)pub, status);
    } else {
        err = bt_mesh_cfg_mod_pub_set_vnd(net_idx, dst, elem_addr,
                                          mod_id, cid, (struct bt_mesh_cfg_mod_pub *)pub, status);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Model Publication Set failed (err %d)\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model Publication Set failed (status 0x%02x)\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model Publication successfully set\n");
    }

    return err;
}
int tls_ble_mesh_pub_get(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t mod_id,
                         uint16_t cid, tls_bt_mesh_cfg_mod_pub *pub, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_pub_get(net_idx, dst, elem_addr,
                                      mod_id, (struct bt_mesh_cfg_mod_pub *)pub, status);
    } else {
        err = bt_mesh_cfg_mod_pub_get_vnd(net_idx, dst, elem_addr,
                                          mod_id, cid, (struct bt_mesh_cfg_mod_pub *)pub, status);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Model Publication Get failed (err %d)\n", err);
        return 0;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model Publication Get failed (status 0x%02x)\n", status[0]);
        return 0;
    }

    TLS_BT_APPL_TRACE_DEBUG("Model Publication for Element 0x%04x, Model 0x%04x:\r\n"
                            "\tPublish Address:                0x%04x\r\n"
                            "\tAppKeyIndex:                    0x%04x\r\n"
                            "\tCredential Flag:                %u\r\n"
                            "\tPublishTTL:                     %u\r\n"
                            "\tPublishPeriod:                  0x%02x\r\n"
                            "\tPublishRetransmitCount:         %u\r\n"
                            "\tPublishRetransmitInterval:      %ums\r\n",
                            elem_addr, mod_id, pub[0].addr, pub[0].app_idx, pub[0].cred_flag, pub[0].ttl,
                            pub[0].period, BT_MESH_PUB_TRANSMIT_COUNT(pub[0].transmit),
                            BT_MESH_PUB_TRANSMIT_INT(pub[0].transmit));
    return 0;
}
int tls_ble_mesh_sub_get(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t mod_id,
                         uint16_t cid,
                         uint8_t *status, uint16_t *subs, uint32_t *sub_cnt)
{
    int err;
    int i ;
    CHECK_SYSTEM_READY();

    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_sub_get(net_idx, dst, elem_addr,
                                      mod_id, status, subs, sub_cnt);
    } else {
        err = bt_mesh_cfg_mod_sub_get_vnd(net_idx, dst, elem_addr,
                                          mod_id, cid, status, subs, sub_cnt);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Model Subscription Get(err %d)\r\n", err);
        return 0;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_DEBUG("Model Subscription Get failed with status 0x%02x\r\n", status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG(
                        "Model Subscriptions for Element 0x%04x, Model 0x%04x \r\n", elem_addr, mod_id);

        if(!sub_cnt[0]) {
            TLS_BT_APPL_TRACE_DEBUG("\tNone.\r\n");
        }

        for(i = 0; i < sub_cnt[0]; i++) {
            TLS_BT_APPL_TRACE_DEBUG("\t0x%04x\r\n", subs[i]);
        }
    }

    return 0;
}
int tls_ble_mesh_sub_add_va(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint8_t label[16],
                            uint16_t mod_id, uint16_t *sub_addr, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_mod_sub_va_add(net_idx, dst, elem_addr, label,
                                     mod_id, sub_addr, status);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Mod Sub VA Add (err %d)\n", err);
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_DEBUG("Mod Sub VA Add failed with status 0x%02x\n", status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("0x%04x subscribed to Label UUID %s (va 0x%04x)\n",
                                elem_addr, bt_hex(label, 16), sub_addr);
    }

    return err;
}

int tls_ble_mesh_sub_del_va(uint16_t net_idx, uint16_t dst, uint16_t elem_addr,
                            uint8_t label[16], uint16_t mod_id, uint16_t *sub_addr, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_mod_sub_va_del(net_idx, dst, elem_addr,
                                     label, mod_id, sub_addr, status);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Model Subscription Delete (err %d)\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_DEBUG("Model Subscription Delete failed with status 0x%02x\n", status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("0x%04x unsubscribed from Label UUID %s (va 0x%04x)\n",
                                elem_addr, bt_hex(label, 16), sub_addr);
    }

    return err;
}


int tls_ble_mesh_sub_del(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t sub_addr,
                         uint16_t mod_id, uint16_t cid, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_sub_del(net_idx, dst, elem_addr,
                                      sub_addr, mod_id, status);
    } else {
        err = bt_mesh_cfg_mod_sub_del_vnd(net_idx, dst, elem_addr,
                                          sub_addr, mod_id, cid, status);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Model Subscription Del (err %d)\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_DEBUG("Model Subscription Del failed with status 0x%02x\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model subscription Del was successful\n");
    }

    return err;
}

int tls_ble_mesh_sub_add(uint16_t net_idx, uint16_t dst, uint16_t elem_addr, uint16_t sub_addr,
                         uint16_t mod_id, uint16_t cid, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_sub_add(net_idx, dst, elem_addr,
                                      sub_addr, mod_id, status);
    } else {
        err = bt_mesh_cfg_mod_sub_add_vnd(net_idx, dst, elem_addr,
                                          sub_addr, mod_id, cid, status);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Model Subscription Add (err %d)\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_DEBUG("Model Subscription Add failed with status 0x%02x\n",
                                status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Model subscription was successful\n");
    }

    return err;
}

int tls_ble_mesh_friend_set(uint16_t net_idx, uint16_t dst, uint8_t val, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_friend_set(net_idx, dst, val, status);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to Set Friend state (err %d)\r\n", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("Friend state is 0x%02x\r\n", status[0]);
    return 0;
}

int tls_ble_mesh_friend_get(uint16_t net_idx, uint16_t dst, uint8_t *val)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_friend_get(net_idx, dst, val);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to Get Friend state (err %d)\r\n", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("Friend state is 0x%02x\r\n", val[0]);
    return 0;
}

int tls_ble_mesh_proxy_get(uint16_t net_idx, uint16_t dst, uint8_t *proxy)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_gatt_proxy_get(net_idx, dst, proxy);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to Get Gatt proxy Set message (err %d)\r\n", err);
        return err;
    }

    return err;
}
int tls_ble_mesh_proxy_set(uint16_t net_idx, uint16_t dst, uint8_t val, uint8_t *proxy)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_gatt_proxy_set(net_idx, dst, val, proxy);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Gatt proxy Set message (err %d)\r\n", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("GATT Proxy is set to 0x%02x\n", proxy[0]);
    return 0;
}
int tls_ble_mesh_relay_get(uint16_t net_idx, uint16_t dst, uint8_t *relay, uint8_t *transmit)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_relay_get(net_idx, dst, relay, transmit);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Relay Get (err %d)\n", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("Transmit 0x%02x (count %u interval %u ms)\r\n",
                            transmit[0], BT_MESH_TRANSMIT_COUNT(transmit[0]),
                            BT_MESH_TRANSMIT_INT(transmit[0]));
    return 0;
}
int tls_ble_mesh_relay_set(uint16_t net_idx, uint16_t dst, uint8_t relay, uint8_t count,
                           uint8_t interval, uint8_t *status, uint8_t *transmit)
{
    int err;
    uint8_t new_transmit;
    CHECK_SYSTEM_READY();

    if(relay) {
        new_transmit = BT_MESH_TRANSMIT(count, interval);
    } else {
        new_transmit = 0;
    }

    err = bt_mesh_cfg_relay_set(net_idx, dst, relay, new_transmit, status, transmit);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Relay Set (err %d)\n", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("Relay is 0x%02x, Transmit 0x%02x (count %u interval %ums)\n",
                            relay, transmit[0], BT_MESH_TRANSMIT_COUNT(transmit[0]),
                            BT_MESH_TRANSMIT_INT(transmit[0]));
    return 0;
}
int tls_ble_mesh_unbind_app_key(uint16_t net_idx, uint16_t dst, uint16_t elem_addr,
                                uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_app_unbind(net_idx, dst, elem_addr, mod_app_idx, mod_id, status);
    } else {
        err = bt_mesh_cfg_mod_app_unbind_vnd(net_idx, dst, elem_addr, mod_app_idx, mod_id, cid, status);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Model App UnBind (err %d)\r\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model App UnBind failed with status 0x%02x\r\n", status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("AppKey successfully unbound\r\n");
    }

    return err;
}
int tls_ble_mesh_bind_app_key(uint16_t net_idx, uint16_t dst, uint16_t elem_addr,
                              uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid, uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();

    if(dst == bt_mesh_primary_addr()) {
        struct bt_mesh_model *found_model;
        struct bt_mesh_elem *found_elem = bt_mesh_elem_find(elem_addr);
        found_model = bt_mesh_model_find(found_elem, mod_id);

        if(!found_model) {
            return -2;
        }

        return mod_bind(found_model, mod_app_idx);
    }

    /**only cfg_server support bind msg, and cfg_server located on the primary element, so dst equal to elem_addr */
    if(cid == CID_NVAL) {
        err = bt_mesh_cfg_mod_app_bind(net_idx, dst, elem_addr,
                                       mod_app_idx, mod_id, status);
    } else {
        err = bt_mesh_cfg_mod_app_bind_vnd(net_idx, dst, elem_addr,
                                           mod_app_idx, mod_id, cid, status);
    }

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send Model App Bind (err %d)\r\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("Model App Bind failed with status 0x%02x\r\n", status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("AppKey successfully bound\r\n");
    }

    return 0;
}

/**
 * Add the network app key via cfg_cli.
 *
 * @param net_idx         network index.
 * @param dst               addr of the node;
 * @param key_net_idx  the related network index of the app key
 * @param key_app_idx the related app key index
 * @param app_key       key value
 *
 * @return 0 on success.  otherwise see error code
 *
 */

int tls_ble_mesh_add_app_key(uint16_t net_idx, uint16_t dst, uint16_t key_net_idx,
                             uint16_t key_app_idx, uint8_t app_key[16], uint8_t *status)
{
    int err;
    CHECK_SYSTEM_READY();
    err = bt_mesh_cfg_app_key_add(net_idx, dst, key_net_idx,
                                  key_app_idx, app_key, status);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Unable to send App Key Add (err %d)\r\n", err);
        return err;
    }

    if(status[0]) {
        TLS_BT_APPL_TRACE_ERROR("AppKeyAdd failed with status 0x%02x\r\n", status[0]);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("AppKey added, NetKeyIndex 0x%04x AppKeyIndex 0x%04x\r\n",
                                key_net_idx, key_app_idx);
    }

    return err;
}

/**
 * Add the local network app key.
 *
 * @param net_idx         network index.
 * @param app_idx        app key index;
 * @param app_key       app key
 *
 * @return 0 on success.  otherwise see error code
 *
 */

int tls_ble_mesh_add_local_app_key(uint16_t net_idx, uint16_t app_idx, uint8_t app_key[16])
{
    int rc;
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_set_app_key:net_idx[%d], addr[%d], app_key[%s]\r\n", net_idx,
                            app_idx, bt_hex(app_key, 16));
    rc = bt_mesh_app_key_add(net_idx, app_idx, app_key, false);

    if(rc) {
        TLS_BT_APPL_TRACE_ERROR("Unable to Add Local App Key (err %d)\r\n", rc);
    }

    return rc;
}

int tls_ble_mesh_input_oob_number(uint32_t number)
{
    int rc;
    CHECK_SYSTEM_READY();
    rc = bt_mesh_input_number(number);
    return rc;
}

int tls_ble_mesh_input_oob_string(const char *string)
{
    int rc;
    CHECK_SYSTEM_READY();
    rc = bt_mesh_input_string(string);
    return rc;
}

/**
 * Remove a provisioned node from the network.
 *
 * @param net_idx         network index.
 * @addr                       address of the node;
 *
 * @return 0 on success.
 *
 */

int tls_ble_mesh_node_reset(uint16_t net_idx, uint16_t addr, uint8_t *status)
{
    int rc;
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_reset_node:net_idx[%d], addr[%d]\r\n", net_idx, addr);

    if(addr == primary_addr) {
        bt_mesh_reset();
        rc = 0;
        status[0] = 0;
    } else {
        bool result;
        rc = bt_mesh_cfg_node_reset(net_idx, addr, &result);
        status[0] = (uint8_t)result;
        if(rc) {
            TLS_BT_APPL_TRACE_ERROR("Unable to Reset node[%d] (err %d)\r\n", addr, rc);
            return rc;
        }
    }

    TLS_BT_APPL_TRACE_DEBUG("result:%s\r\n", status[0] ? "success" : "failed");
    return rc;
}

/**
 * Provision a device by PB-ADV.
 *
 * @param uuid             device uuid.
 * @net_idx                   net index, default value 0 is primary network;
 * @addr                       assigned device address for provision, 0 means the provisioner will choose the addr auto.
 * @duration                  notify timer indicate which device is provisioning
 *
 * @return 0 on success. otherwise, see detailed error code
 *
 */

int tls_ble_mesh_provisioner_prov_adv(uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
                                      uint8_t duration)
{
    int rc;
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_provisioner_prov_adv:uuid[%s],net_idx[%d],addr[%d],duration[%d] \r\n",
                            bt_hex(uuid, 16), net_idx, addr, duration);
    rc = bt_mesh_provision_adv(uuid, net_idx, addr, duration);
    return rc;
}

int tls_ble_mesh_provisioner_scan(bool enable)
{
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_provisioner_scan:%s\r\n", enable ? "start" : "stop");

    if(enable) {
        //bt_mesh_scan_enable();
        filter_unprov_adv_report = false;
    } else {
        //bt_mesh_scan_disable();
        filter_unprov_adv_report = true;
    }

    return 0;
}
int tls_ble_mesh_clear_local_rpl(void)
{
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_clear_local_rpl\r\n");
    bt_mesh_clear_rpl();
    return 0;
}
int tls_ble_mesh_change_primary_addr(uint16_t primary_addr)
{
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_change_primay_address old:%d, new:%d\r\n", bt_mesh_primary_addr(), primary_addr);
    if(primary_addr == bt_mesh_primary_addr()){
        return 0;
    }
    if(!bt_mesh_is_provisioned()) return BLE_HS_EREJECT;
    bt_mesh_comp_provision(primary_addr);
    bt_mesh_store_net();
    return 0;    
}
int tls_ble_mesh_get_primary_addr(uint16_t *primary_addr)
{
    CHECK_SYSTEM_READY();
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_mesh_get_primary_address ret=%d\r\n", bt_mesh_primary_addr());

    *primary_addr = bt_mesh_primary_addr();

    return 0;    
}
int tls_ble_mesh_change_ttl(uint8_t ttl)
{
    CHECK_SYSTEM_READY();
    struct bt_mesh_cfg_srv *cfg_srv = bt_mesh_cfg_get();
    cfg_srv->default_ttl = ttl;
    bt_mesh_store_cfg(true);
    return 0;
}
int tls_ble_mesh_get_cfg(tls_mesh_primary_cfg_t *cfg)
{
    CHECK_SYSTEM_READY();
    struct bt_mesh_cfg_srv *cfg_srv = bt_mesh_cfg_get();
    cfg->net_transmit_count = TLS_BT_MESH_TRANSMIT_COUNT(cfg_srv->net_transmit);
    cfg->net_transmit_intvl = TLS_BT_MESH_TRANSMIT_INT(cfg_srv->net_transmit);
    cfg->relay = cfg_srv->relay;
    cfg->relay_retransmit_count = TLS_BT_MESH_TRANSMIT_COUNT(cfg_srv->relay_retransmit);
    cfg->relay_retransmit_intvl = TLS_BT_MESH_TRANSMIT_INT(cfg_srv->relay_retransmit);
    cfg->beacon = cfg_srv->beacon;
    cfg->gatt_proxy = cfg_srv->gatt_proxy;
    cfg->frnd = cfg_srv->frnd;
    cfg->default_ttl = cfg_srv->default_ttl;
    
    return 0;
}

int tls_ble_mesh_get_comp(uint16_t net_idx, uint16_t dst, uint8_t *status, char *rsp_data,
                          uint32_t *data_len)
{
    struct os_mbuf *comp = NET_BUF_SIMPLE(32);
    uint8_t page = 0x00;
    int err = 0;
    uint16_t val16;
    int offset = 0;
    uint32_t max_offset = data_len[0];
    CHECK_SYSTEM_READY();
    net_buf_simple_init(comp, 0);
    page = 0;
    err = bt_mesh_cfg_comp_data_get(net_idx, dst, page,
                                    status, comp);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("Getting composition failed (err %d)\r\n", err);
        goto done;
    }

    if(status[0] != 0x00) {
        TLS_BT_APPL_TRACE_ERROR("Got non-success status 0x%02x\r\n", status[0]);
        goto done;
    }

    TLS_BT_APPL_TRACE_DEBUG("Got Composition Data for 0x%04x:\r\n", dst);
    val16 = net_buf_simple_pull_le16(comp);
    offset += sprintf(rsp_data + offset, "%04x,", val16);
    TLS_BT_APPL_TRACE_DEBUG("\tCID      0x%04x\r\n", val16);
    val16 = net_buf_simple_pull_le16(comp);
    offset += sprintf(rsp_data + offset, "%04x,", val16);
    TLS_BT_APPL_TRACE_DEBUG("\tPID      0x%04x\r\n", val16);
    val16 = net_buf_simple_pull_le16(comp);
    offset += sprintf(rsp_data + offset, "%04x,", val16);
    TLS_BT_APPL_TRACE_DEBUG("\tVID      0x%04x\r\n", val16);
    val16 = net_buf_simple_pull_le16(comp);
    offset += sprintf(rsp_data + offset, "%04x,", val16);
    TLS_BT_APPL_TRACE_DEBUG("\tCRPL     0x%04x\r\n", val16);
    val16 = net_buf_simple_pull_le16(comp);
    offset += sprintf(rsp_data + offset, "%04x,", val16);
    TLS_BT_APPL_TRACE_DEBUG("\tFeatures 0x%04x\r\n", val16);

    while(comp->om_len > 4) {
        u8_t sig, vnd;
        u16_t loc;
        int i;
        loc = net_buf_simple_pull_le16(comp);
        sig = net_buf_simple_pull_u8(comp);
        vnd = net_buf_simple_pull_u8(comp);
        offset += sprintf(rsp_data + offset, "%04x,%02x,%02x,", loc, sig, vnd);
        TLS_BT_APPL_TRACE_DEBUG("\n\tElement @ 0x%04x:\r\n", loc);

        if(comp->om_len < ((sig * 2) + (vnd * 4))) {
            TLS_BT_APPL_TRACE_DEBUG("\t\t...truncated data!\r\n");
            break;
        }

        if(sig) {
            TLS_BT_APPL_TRACE_DEBUG("\t\tSIG Models:\r\n");
        } else {
            TLS_BT_APPL_TRACE_DEBUG("\t\tNo SIG Models\r\n");
        }

        for(i = 0; i < sig; i++) {
            u16_t mod_id = net_buf_simple_pull_le16(comp);
            offset += sprintf(rsp_data + offset, "%04x,", mod_id);
            TLS_BT_APPL_TRACE_DEBUG("\t\t\t0x%04x\r\n", mod_id);
        }

        if(vnd) {
            TLS_BT_APPL_TRACE_DEBUG("\t\tVendor Models:\r\n");
        } else {
            TLS_BT_APPL_TRACE_DEBUG("\t\tNo Vendor Models\r\n");
        }

        for(i = 0; i < vnd; i++) {
            u16_t cid = net_buf_simple_pull_le16(comp);
            u16_t mod_id = net_buf_simple_pull_le16(comp);
            offset += sprintf(rsp_data + offset, "%04x,%04x,", cid, mod_id);
            TLS_BT_APPL_TRACE_DEBUG("\t\t\tCompany 0x%04x: 0x%04x\r\n", cid, mod_id);
        }
    }

    assert(offset <= max_offset);
    data_len[0] = offset;
done:
    os_mbuf_free_chain(comp);
    return err;
}

int tls_ble_mesh_erase_cfg(void)
{
    TLS_BT_APPL_TRACE_DEBUG("erase mesh parameter\r\n");
    return bt_mesh_settings_clear();
}
void tls_ble_mesh_led_init(void)
{
  tls_gpio_cfg(MESH_LIGHT_GPIO_RED, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
  tls_gpio_cfg(MESH_LIGHT_GPIO_GREEN, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
  tls_gpio_cfg(MESH_LIGHT_GPIO_BLUE, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
  
  tls_gpio_write(MESH_LIGHT_GPIO_RED, 1);
  tls_gpio_write(MESH_LIGHT_GPIO_GREEN, 1);
  tls_gpio_write(MESH_LIGHT_GPIO_BLUE, 1);
}
void tls_ble_mesh_led_update(uint8_t flag)
{
  if(flag&WM_MESH_LED_FLAG_BIT_GREEN)
  {
    tls_gpio_write(MESH_LIGHT_GPIO_GREEN, 0);
  }else
  {
    tls_gpio_write(MESH_LIGHT_GPIO_GREEN, 1);
  }

  if(flag&WM_MESH_LED_FLAG_BIT_RED)
  {
    tls_gpio_write(MESH_LIGHT_GPIO_RED, 0);
  }else
  {
    tls_gpio_write(MESH_LIGHT_GPIO_RED, 1);
  }  

  if(flag&WM_MESH_LED_FLAG_BIT_BLUE)
  {
    tls_gpio_write(MESH_LIGHT_GPIO_BLUE, 0);
  }else
  {
    tls_gpio_write(MESH_LIGHT_GPIO_BLUE, 1);
  }    
}

#endif


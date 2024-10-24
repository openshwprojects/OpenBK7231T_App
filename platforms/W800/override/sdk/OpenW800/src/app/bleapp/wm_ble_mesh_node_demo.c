/*****************************************************************************
**
**  Name:           wm_ble_mesh_node_demo.c
**
**  Description:    This file contains the sample functions for mesh node application
**
*****************************************************************************/

#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)

#include <assert.h>
#include "mesh/mesh.h"

#include "wm_bt_app.h"
#include "wm_efuse.h"


/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "mesh/glue.h"
#include "wm_bt_util.h"
#include "wm_ble_mesh_node_demo.h"
#include "wm_ble_gap.h"


/*Mesh models*/
#include "wm_ble_mesh_health_server.h"
#include "wm_ble_mesh_gen_onoff_server.h"
#include "wm_ble_mesh_gen_onoff_client.h"
#include "wm_ble_mesh_gen_level_server.h"
#include "wm_ble_mesh_vnd_model.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

uint16_t node_primary_addr;
uint16_t node_primary_net_idx;
static bool node_enabled =false;
static struct ble_gap_event_listener app_mesh_event_listener;

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
};


static struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_op,
                     &gen_onoff_pub_srv, NULL, &gen_onoff_server_cb),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, gen_level_op,
                  &gen_level_pub_srv, NULL),
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_client_op,
                     &gen_onoff_pub_cli, NULL, &gen_onoff_client_cb),

};

static struct bt_mesh_model vnd_models[] = {
    BT_MESH_MODEL_VND(CID_VENDOR, BT_MESH_MODEL_ID_GEN_ONOFF_SRV, vnd_model_op,
                      &vnd_model_pub, NULL),
};

static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp node_comp = {
    .cid = CID_VENDOR,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void
pub_init(void)
{
    health_pub.msg  = BT_MESH_HEALTH_FAULT_MSG(0);
    gen_onoff_pub_srv.msg = NET_BUF_SIMPLE(2 + 2);
    gen_onoff_pub_cli.msg = NET_BUF_SIMPLE(2 + 2);
}
static void
pub_deinit(void)
{
    if(health_pub.msg) {
        os_mbuf_free_chain(health_pub.msg);
        health_pub.msg = NULL;
    }

    if(gen_onoff_pub_srv.msg) {
        os_mbuf_free_chain(gen_onoff_pub_srv.msg);
        gen_onoff_pub_srv.msg = NULL;
    }

    if(gen_onoff_pub_cli.msg) {
        os_mbuf_free_chain(gen_onoff_pub_cli.msg);
        gen_onoff_pub_cli.msg = NULL;
    }
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
    TLS_BT_APPL_TRACE_DEBUG("OOB Number: %lu\r\n", number);
    return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
    TLS_BT_APPL_TRACE_DEBUG("Local node provisioned, primary address 0x%04x, net_idx=%d\r\n", addr,
                            net_idx);
    node_primary_addr = addr;
    node_primary_net_idx = net_idx;
}

static uint8_t dev_uuid[16] = MYNEWT_VAL(BLE_MESH_DEV_UUID);
static char    dev_name[16] = CONFIG_BT_DEVICE_NAME;

static const struct bt_mesh_prov prov = {
    .name = dev_name,
    .uuid = dev_uuid,
    .output_size = 4,
    .output_actions = BT_MESH_DISPLAY_NUMBER | BT_MESH_BEEP | BT_MESH_VIBRATE | BT_MESH_BLINK,
    .output_number = output_number,
    .complete = prov_complete,
};
    
static int ble_gap_host_shutdown_cb(struct ble_gap_event *event, void *arg)
{
    if(BLE_GAP_EVENT_HOST_SHUTDOWN == event->type)
    {
        tls_bt_set_mesh_mode(0);
        bt_mesh_deinit();
        pub_deinit();
        node_enabled = false;
        TLS_BT_APPL_TRACE_DEBUG("Mesh deinitialized\r\n");
    }

    return 0;
}
static void notify_flash_flush_available(void)
{
#if (MYNEWT_VAL(BLE_MESH_SETTINGS))
    bt_mesh_settings_flush();
#endif
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int
tls_ble_mesh_node_deinit(int reason)
{
    CHECK_SYSTEM_READY();
    if(!node_enabled) return 0;
    
    TLS_BT_APPL_TRACE_DEBUG("Mesh freeing resouce\r\n");
    tls_bt_set_mesh_mode(0);
    bt_mesh_deinit();
    pub_deinit();
    TLS_BT_APPL_TRACE_DEBUG("Mesh deinitialized\r\n");
    node_enabled = false;
    ble_gap_event_listener_unregister(&app_mesh_event_listener);
    return 0;
}

int
tls_ble_mesh_node_init(void)
{
    int err = 0;
    ble_addr_t addr;
    CHECK_SYSTEM_READY();
    if(node_enabled) return 0;
    
    TLS_BT_APPL_TRACE_DEBUG("Mesh Node initializing\r\n");
    tls_bt_set_mesh_mode(1);
    pub_init();
    /* Use NRPA */
    #if 1
    err = ble_hs_id_gen_rnd(1, &addr);
    assert(err == 0);
    err = ble_hs_id_set_rnd(addr.val);
    assert(err == 0);
    #endif
    tls_ble_gap_get_name(dev_name);
    /*using BT MAC as device uuid high 6 bytes*/
    tls_get_bt_mac_addr(&dev_uuid[10]);
    
    err = bt_mesh_init(addr.type, &prov, &node_comp);

    if(err) {
        TLS_BT_APPL_TRACE_DEBUG("Initializing mesh node failed (err %d)\r\n", err);
        return err;
    }

    TLS_BT_APPL_TRACE_DEBUG("Mesh(%s) initialized\r\n", dev_name);
#if (MYNEWT_VAL(BLE_MESH_SETTINGS))
    tls_bt_register_pending_process_callback(notify_flash_flush_available);
    err = bt_mesh_settings_load(true);
#endif

    if(err == -2) {
        TLS_BT_APPL_TRACE_DEBUG("Mesh nvram parameter mismatch with the prefered role, Please erase the nvram or change the role\r\n");
        return err;
    } else {
        /**if invalid nvram area, return success*/
        err = 0;
    }

    TLS_BT_APPL_TRACE_DEBUG("node role enabled\r\n");
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    bt_mesh_role_set(true);

    if(bt_mesh_is_provisioned()) {
        TLS_BT_APPL_TRACE_DEBUG("Mesh network restored from flash\r\n");
    }
    
    ble_gap_event_listener_register(&app_mesh_event_listener,
                                ble_gap_host_shutdown_cb, NULL);

    TLS_BT_APPL_TRACE_DEBUG("node role running\r\n");
    node_enabled = true;
    return err;
}
#endif


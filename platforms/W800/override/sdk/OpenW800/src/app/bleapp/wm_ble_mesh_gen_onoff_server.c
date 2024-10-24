#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)

#include <assert.h>
#include "mesh/mesh.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "mesh/glue.h"
#include "wm_gpio.h"
#include "wm_bt_util.h"
#include "wm_ble_mesh.h"
#include "wm_ble_mesh_gen_onoff_server.h"



struct bt_mesh_model_pub gen_onoff_pub_srv;
static uint8_t gen_on_off_state;

static void gen_onoff_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx)
{
    struct os_mbuf *msg = NET_BUF_SIMPLE(3);
    uint8_t *status;
    TLS_BT_APPL_TRACE_DEBUG("#mesh-onoff STATUS\r\n");
    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x04));
    status = net_buf_simple_add(msg, 1);
    *status = gen_on_off_state;

    if(bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        TLS_BT_APPL_TRACE_DEBUG("#mesh-onoff STATUS: send status failed\r\n");
    }

    os_mbuf_free_chain(msg);
}

static void gen_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    TLS_BT_APPL_TRACE_DEBUG("#mesh-onoff GET\r\n");
    gen_onoff_status(model, ctx);
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    int err;
    struct os_mbuf *msg = model->pub->msg;
    TLS_BT_APPL_TRACE_DEBUG("#mesh-onoff SET, %d\r\n", buf->om_data[0]);
    gen_on_off_state = buf->om_data[0];
    if(gen_on_off_state)
    {
        tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_RED|WM_MESH_LED_FLAG_BIT_GREEN|WM_MESH_LED_FLAG_BIT_BLUE);
    }else
    {
        tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_OFF);
    }

    gen_onoff_status(model, ctx);
    /*
      * If a server has a publish address, it is required to
      * publish status on a state change
      *
      * See Mesh Profile Specification 3.7.6.1.2
      *
      * Only publish if there is an assigned address
      */
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS   BT_MESH_MODEL_OP_2(0x82, 0x04)

    if(model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
        TLS_BT_APPL_TRACE_DEBUG("Node:0x%04x publish addr:0x%04x cur 0x%02x\r\n",
                                bt_mesh_model_elem(model)->addr, model->pub->addr, gen_on_off_state);
        bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
        net_buf_simple_add_u8(msg, gen_on_off_state);
        err = bt_mesh_model_publish(model);

        if(err) {
            TLS_BT_APPL_TRACE_DEBUG("bt_mesh_model_publish err %d\r\n", err);
        }
    }
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct os_mbuf *buf)
{
    TLS_BT_APPL_TRACE_DEBUG("#mesh-onoff SET-UNACK, %d\r\n", buf->om_data[0]);
    gen_on_off_state = buf->om_data[0];
    if(gen_on_off_state)
    {
        tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_RED|WM_MESH_LED_FLAG_BIT_GREEN|WM_MESH_LED_FLAG_BIT_BLUE);
    }else
    {
        tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_OFF);
    }
}

const struct bt_mesh_model_op gen_onoff_op[] = {
    { BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_onoff_server_init(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("gen_onoff_server_init\r\n");
    /*Initialize the light gpio settings*/
    tls_ble_mesh_led_init();
    return 0;
}
static int gen_onoff_server_deinit(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("gen_onoff_server_deinit\r\n");
    return 0;
}


const struct bt_mesh_model_cb gen_onoff_server_cb = {
    .init = gen_onoff_server_init,
    .deinit = gen_onoff_server_deinit,
};
#endif

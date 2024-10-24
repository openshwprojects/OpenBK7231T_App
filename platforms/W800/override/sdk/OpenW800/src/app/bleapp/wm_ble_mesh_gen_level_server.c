#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)

#include <assert.h>
#include "mesh/mesh.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "mesh/glue.h"
#include "wm_bt_util.h"
#include "wm_ble_mesh.h"
#include "wm_ble_mesh_gen_level_server.h"

struct bt_mesh_model_pub gen_level_pub_srv;
static int16_t gen_level_state;



static void gen_level_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx)
{
    struct os_mbuf *msg = NET_BUF_SIMPLE(4);
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level STATUS\r\n");
    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x08));
    net_buf_simple_add_le16(msg, gen_level_state);

    if(bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        TLS_BT_APPL_TRACE_DEBUG("#mesh-level STATUS: send status failed\r\n");
    }

    os_mbuf_free_chain(msg);
}

static void gen_level_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level GET\r\n");
    gen_level_status(model, ctx);
}

static void gen_level_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    int16_t level;
    level = (int16_t) net_buf_simple_pull_le16(buf);
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level SET: level=%d\r\n", level);
    gen_level_status(model, ctx);
    gen_level_state = level;
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level: level=%d\r\n", gen_level_state);
}

static void gen_level_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct os_mbuf *buf)
{
    int16_t level;
    level = (int16_t) net_buf_simple_pull_le16(buf);
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level SET-UNACK: level=%d\r\n", level);
    gen_level_state = level;
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level: level=%d\r\n", gen_level_state);
}

static void gen_delta_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
    int16_t delta_level;
    delta_level = (int16_t) net_buf_simple_pull_le16(buf);
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level DELTA-SET: delta_level=%d\r\n", delta_level);
    gen_level_status(model, ctx);
    gen_level_state += delta_level;
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level: level=%d\r\n", gen_level_state);
}

static void gen_delta_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct os_mbuf *buf)
{
    int16_t delta_level;
    delta_level = (int16_t) net_buf_simple_pull_le16(buf);
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level DELTA-SET: delta_level=%d\r\n", delta_level);
    gen_level_state += delta_level;
    TLS_BT_APPL_TRACE_DEBUG("#mesh-level: level=%d\r\n", gen_level_state);
}

static void gen_move_set(struct bt_mesh_model *model,
                         struct bt_mesh_msg_ctx *ctx,
                         struct os_mbuf *buf)
{
}

static void gen_move_set_unack(struct bt_mesh_model *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct os_mbuf *buf)
{
}

const struct bt_mesh_model_op gen_level_op[] = {
    { BT_MESH_MODEL_OP_2(0x82, 0x05), 0, gen_level_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x06), 3, gen_level_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x07), 3, gen_level_set_unack },
    { BT_MESH_MODEL_OP_2(0x82, 0x09), 5, gen_delta_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x0a), 5, gen_delta_set_unack },
    { BT_MESH_MODEL_OP_2(0x82, 0x0b), 3, gen_move_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x0c), 3, gen_move_set_unack },
    BT_MESH_MODEL_OP_END,
};

#endif

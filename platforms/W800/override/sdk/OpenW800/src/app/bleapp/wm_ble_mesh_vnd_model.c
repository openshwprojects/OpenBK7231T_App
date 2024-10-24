#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)

#include <assert.h>
#include "mesh/mesh.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "mesh/glue.h"
#include "wm_bt_util.h"
#include "wm_ble_mesh.h"
#include "wm_ble_mesh_vnd_model.h"

static struct bt_mesh_model *g_vnd_model_ptr = NULL;

struct bt_mesh_model_pub vnd_model_pub;

//#define THROUGHTPUT_TEST
//#define NAME_ADDR_BROADCASE_SUPPORT

#ifdef THROUGHTPUT_TEST
static uint32_t g_send_bytes = 0;
static uint32_t g_recv_bytes = 0;
static uint32_t g_time_last = 0;
/* Attention Timer state */
struct k_delayed_work ones_timer;

static void ones_timer_cb(struct ble_npl_event *work);
#endif

#ifdef NAME_ADDR_BROADCASE_SUPPORT

static void vnd_model_indicate_recv(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf);

static void vnd_model_update_cb(struct ble_npl_event *work);
static uint8_t update_tid = 0x80;
static struct k_delayed_work update_timer;
static uint8_t update_retry = 0;
#endif

static void vnd_model_recv(struct bt_mesh_model *model,
                           struct bt_mesh_msg_ctx *ctx,
                           struct os_mbuf *buf)
{
    struct os_mbuf *msg = NULL;
    uint8_t ack[3] = {0x00, 0x00, 0x00}; 
    TLS_BT_APPL_TRACE_DEBUG("#vendor-model-recv, send_rel:%s\r\n", ctx->send_rel?"true":"false");
    TLS_BT_APPL_TRACE_VERBOSE("data:%s len:%d\r\n", bt_hex(buf->om_data, buf->om_len),buf->om_len);

#ifdef THROUGHTPUT_TEST
        g_recv_bytes += buf->om_len;
#endif

    /*send ack*/
    msg = NET_BUF_SIMPLE(3);
    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_3(0x02, CID_VENDOR));
    os_mbuf_append(msg, ack, sizeof(ack));

    if(bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
        TLS_BT_APPL_TRACE_DEBUG("#vendor-model-recv: send rsp failed\r\n");
    }

    os_mbuf_free_chain(msg);
}

static void vnd_model_ack_recv(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
   TLS_BT_APPL_TRACE_DEBUG("#vendor-model-ack-recv, send_rel:%s\r\n", ctx->send_rel?"true":"false");
   TLS_BT_APPL_TRACE_DEBUG("data:%s len:%d\r\n", bt_hex(buf->om_data, buf->om_len),
                           buf->om_len);
}

#ifdef NAME_ADDR_BROADCASE_SUPPORT 
static void vnd_model_indicate_ack_recv(struct bt_mesh_model *model,
                                        struct bt_mesh_msg_ctx *ctx,
                                        struct os_mbuf *buf)
{
  TLS_BT_APPL_TRACE_DEBUG("#vnd_model_indicate_ack_recv, send_rel:%s\r\n", ctx->send_rel?"true":"false");
  TLS_BT_APPL_TRACE_DEBUG("data:%s len:%d\r\n", bt_hex(buf->om_data, buf->om_len),
                          buf->om_len);
}
#endif
static int vnd_model_init(struct bt_mesh_model *model)
{
    //struct bt_mesh_cfg_srv *cfg = model->user_data;
    TLS_BT_APPL_TRACE_DEBUG("vnd_model_init\r\n");
    g_vnd_model_ptr = model;

#ifdef NAME_ADDR_BROADCASE_SUPPORT    
    k_delayed_work_init(&update_timer, vnd_model_update_cb);
    k_delayed_work_add_arg(&update_timer, (void *)model);
    k_delayed_work_submit(&update_timer, 1 * 3000);    
    update_retry = 0;
#endif    
    
#ifdef THROUGHTPUT_TEST
    g_send_bytes = 0;
    g_recv_bytes = 0;
    g_time_last = tls_os_get_time();
    k_delayed_work_init(&ones_timer, ones_timer_cb);
    k_delayed_work_submit(&ones_timer, 1 * 1000);
#endif
    
    return 0;
}

static int vnd_model_deinit(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("vnd_model_deinit\r\n");
#ifdef THROUGHTPUT_TEST    
    k_delayed_work_deinit(&ones_timer);
#endif
#ifdef NAME_ADDR_BROADCASE_SUPPORT 
    k_delayed_work_deinit(&update_timer);
#endif
    return 0;
}


const struct bt_mesh_model_op vnd_model_op[] = {
    { BT_MESH_MODEL_OP_3(0x01, CID_VENDOR), 0, vnd_model_recv },
    { BT_MESH_MODEL_OP_3(0x02, CID_VENDOR), 0, vnd_model_ack_recv },
#ifdef NAME_ADDR_BROADCASE_SUPPORT    
    { BT_MESH_MODEL_OP_3(0xCE, CID_VENDOR), 0, vnd_model_indicate_recv },
    { BT_MESH_MODEL_OP_3(0xCD, CID_VENDOR), 0, vnd_model_indicate_ack_recv },
#endif    
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_cb vnd_model_cb = {
    .init = vnd_model_init,
    .deinit = vnd_model_deinit,
};


static void vnd_send_start_cback(u16_t duration, int err, void *cb_data)
{
    TLS_BT_APPL_TRACE_VERBOSE("vnd_send_start_cback\r\n");
}
static void vnd_send_end_cback(int err, void *cb_data)
{
    TLS_BT_APPL_TRACE_VERBOSE("vnd_send_end_cback\r\n");
}

static struct bt_mesh_send_cb vnd_send_cb = { 
    .start = vnd_send_start_cback,
    .end = vnd_send_end_cback,
};

#ifdef NAME_ADDR_BROADCASE_SUPPORT

static void vnd_model_indicate_recv(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct os_mbuf *buf)
{
  struct os_mbuf *msg = NULL;
  const struct bt_mesh_prov * prov = bt_mesh_prov_get();
  int payload_length = 0;
  int rc; 

  TLS_BT_APPL_TRACE_DEBUG("#vnd_model_indicate_recv, send_rel:%s, app_idx=%d\r\n", ctx->send_rel?"true":"false", ctx->app_idx);
  TLS_BT_APPL_TRACE_DEBUG("data:%s len:%d\r\n", bt_hex(buf->om_data, buf->om_len),
                          buf->om_len);

  if(ctx->addr == bt_mesh_primary_addr())
  {
    TLS_BT_APPL_TRACE_DEBUG("#vnd_model_indicate_recv, local report, return \r\n");
    return;
  }
  /**Send indicate ack*/
  /**build indicate message*/
  payload_length = 3 + 1 + 2 + 1 + strlen(prov->name);
  msg = NET_BUF_SIMPLE(payload_length);
  bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_3(0xCD, CID_VENDOR));
  net_buf_simple_add_u8(msg, update_tid++);
  net_buf_simple_add_be16(msg, bt_mesh_primary_addr());
  net_buf_simple_add_u8(msg, (uint8_t)strlen(prov->name));
  os_mbuf_append(msg, prov->name, (uint8_t)strlen(prov->name));
  rc = bt_mesh_model_send(model, ctx, msg, &vnd_send_cb, NULL);
  if(rc) {
      TLS_BT_APPL_TRACE_ERROR("#vendor-model-send: send indicate ack, rc=%d\r\n", rc);
  } 
  
  os_mbuf_free_chain(msg);

}
#endif

#ifdef THROUGHTPUT_TEST

static void ones_timer_cb(struct ble_npl_event *work)
{
    BT_DBG("");

    if(1) {
        if(g_send_bytes)TLS_BT_APPL_TRACE_DEBUG("Mesh Send(%04d bytes)[%5.2f Kbps]/s\r\n", g_send_bytes, (g_send_bytes * 8.0 / 1000));
        if(g_recv_bytes)TLS_BT_APPL_TRACE_DEBUG("Mesh Recv(%04d bytes)[%5.2f Kbps]/s\r\n", g_recv_bytes, (g_recv_bytes * 8.0 / 1000));
        g_send_bytes = 0;
        g_recv_bytes = 0;
    }
    k_delayed_work_submit(&ones_timer, 1 * 1000);
}
#endif

#ifdef NAME_ADDR_BROADCASE_SUPPORT 
static void vnd_model_update_cb(struct ble_npl_event *work)
{
    const struct bt_mesh_prov * prov = bt_mesh_prov_get();
    struct bt_mesh_model *model = ble_npl_event_get_arg(work);
    struct os_mbuf *msg = NULL;
    struct bt_mesh_msg_ctx ctx;
    int payload_length = 0;
    int rc;
    
    if(model->pub->addr == 0)
    {

        //TLS_BT_APPL_TRACE_ERROR("vnd_model_update_cb, pub_addr 0x%04x, retry\r\n", model->pub->addr, rc);
        k_delayed_work_submit(&update_timer, 3 * 1000);
        return;
    }
    /**build indicate message*/
    payload_length = 3 + 1 + 2 + 1 + strlen(prov->name);
    msg = NET_BUF_SIMPLE(payload_length);
    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_3(0xCE, CID_VENDOR));
    net_buf_simple_add_u8(msg, update_tid++);
    net_buf_simple_add_be16(msg, bt_mesh_primary_addr());
    net_buf_simple_add_u8(msg, (uint8_t)strlen(prov->name));
    os_mbuf_append(msg, prov->name, (uint8_t)strlen(prov->name));
    
    ctx.net_idx = 0;
    ctx.addr = model->pub->addr;
    ctx.app_idx = 0;
    ctx.send_ttl = bt_mesh_default_ttl_get();
    ctx.send_rel = false;

    rc = bt_mesh_model_send(model, &ctx, msg, &vnd_send_cb, NULL);
    
    os_mbuf_free_chain(msg);

    if(rc)
    {
        TLS_BT_APPL_TRACE_ERROR("#vendor-model-send: send rsp failed, rc=%d, retry\r\n", rc);
        k_delayed_work_submit(&update_timer, 3 * 1000);
    }else{
        update_retry++;
        if(update_retry < 3)
        {
            k_delayed_work_submit(&update_timer, 3 * 1000);
        }
    }
    
}
#endif
int vnd_model_send(uint16_t net_idx, uint16_t dst, uint16_t app_idx, uint8_t *payload, int length)
{
    int rc = -1;
    
    if(!g_vnd_model_ptr) return rc;
    struct bt_mesh_msg_ctx ctx;
    
    struct os_mbuf *msg = NET_BUF_SIMPLE(3);
    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_3(0x01, CID_VENDOR));
    os_mbuf_append(msg, payload, length);

    ctx.net_idx = 0;
    ctx.addr = dst;
    ctx.app_idx = app_idx;
    ctx.send_ttl = bt_mesh_default_ttl_get();
    ctx.send_rel = false;

    rc = bt_mesh_model_send(g_vnd_model_ptr, &ctx, msg, &vnd_send_cb, NULL);
    if(rc) {
        TLS_BT_APPL_TRACE_ERROR("#vendor-model-send: send rsp failed, rc=%d\r\n", rc);
    }
    
    os_mbuf_free_chain(msg);

#ifdef THROUGHTPUT_TEST
    if(rc == 0)
    {
        g_send_bytes += length;
    }
    
#endif

    return rc;
}

#endif

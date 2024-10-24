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
#include "wm_ble_mesh_gen_onoff_client.h"

#define MESH_GPIO_BUTTON WM_IO_PB_05
#define BUTTON_DEBOUNCE_DELAY_MS 250
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET       BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)

#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS    BT_MESH_MODEL_OP_2(0x82, 0x04)


struct bt_mesh_model_pub gen_onoff_pub_cli;

struct sw {
    u8_t sw_num;
    u8_t onoff_state;
    struct ble_npl_callout button_work;
    struct k_delayed_work button_timer;
};

static u8_t trans_id;
static u8_t button_press_cnt;
static struct sw sw;
static u32_t time, last_time;

/*
 * OnOff Model Client Op Dispatch Table
 */
static void gen_onoff_client_status(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct os_mbuf *buf)
{
    u8_t    state;
    state = net_buf_simple_pull_u8(buf);
    TLS_BT_APPL_TRACE_EVENT("Node 0x%04x OnOff status from 0x%04x with state 0x%02x\r\n",
                            bt_mesh_model_elem(model)->addr, ctx->addr, state);
}


const struct bt_mesh_model_op gen_onoff_client_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, gen_onoff_client_status },
    BT_MESH_MODEL_OP_END,
};

static void gen_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct os_mbuf *buf)
{
    TLS_BT_APPL_TRACE_DEBUG("#gen_onoff_client, mesh-onoff SET-UNACK, state=%d\r\n", buf->om_data[0]);
    //gen_on_off_state = buf->om_data[0];
    //hal_gpio_write(LED_2, !gen_on_off_state);
}

static void button_gpio_isr_callback(void *context)
{
    u16 ret;
    enum tls_io_name io_index = (enum tls_io_name)context;
    ret = tls_get_gpio_irq_status(io_index);
    TLS_BT_APPL_TRACE_VERBOSE("button int flag =%d\r\n", ret);

    if(ret) {
        tls_clr_gpio_irq_status(io_index);
        ret = tls_gpio_read(io_index);
        TLS_BT_APPL_TRACE_VERBOSE("after int io =%d\r\n", ret);
    }

    /*
     * One button press within a 1 second interval sends an on message
     * More than one button press sends an off message
     */
    time = k_uptime_get_32();

    /* debounce the switch */
    if(time < last_time + BUTTON_DEBOUNCE_DELAY_MS) {
        last_time = time;
        return;
    }

    if(button_press_cnt == 0) {
        k_delayed_work_submit(&sw.button_timer, K_MSEC(500));
    }

    TLS_BT_APPL_TRACE_DEBUG("button_press_cnt 0x%02x\r\n", button_press_cnt);
    button_press_cnt++;
    /* The variable pin_pos is the pin position in the GPIO register,
     * not the pin number. It's assumed that only one bit is set.
     */
    sw.sw_num = io_index;//pin_to_sw(pin_pos);
    last_time = time;
}

/*
 * Button Count Timer Worker
 */

static void button_cnt_timer(struct ble_npl_event *work)
{
    struct bt_mesh_model *mod_cli = (struct bt_mesh_model *)work->arg;
    struct sw *button_sw = (struct sw *)mod_cli->user_data;
    button_sw->onoff_state = !button_sw->onoff_state;//button_press_cnt == 1 ? 1 : 0;
    TLS_BT_APPL_TRACE_EVENT("button_press_cnt 0x%02x onoff_state 0x%02x\r\n",
                            button_press_cnt, button_sw->onoff_state);
    button_press_cnt = 0;
    k_work_submit(&sw.button_work);
}

/*
 * Button Pressed Worker Task
 */

static void button_pressed_worker(struct ble_npl_event *work)
{
    struct os_mbuf *msg = NET_BUF_SIMPLE(1);
    struct bt_mesh_model *mod_cli, *mod_srv;
    struct bt_mesh_model_pub *pub_cli;
    struct sw *sw = NULL;
    u8_t sw_idx = 0;
    int err;
    mod_cli = (struct bt_mesh_model *)work->arg;
    pub_cli = mod_cli->pub;
    sw = (struct sw *)mod_cli->user_data;
    sw_idx = sw->sw_num;

    /* If unprovisioned, just call the set function.
     * The intent is to have switch-like behavior
     * prior to provisioning. Once provisioned,
     * the button and its corresponding led are no longer
     * associated and act independently. So, if a button is to
     * control its associated led after provisioning, the button
     * must be configured to either publish to the led's unicast
     * address or a group to which the led is subscribed.
     */

    if(node_primary_addr == BT_MESH_ADDR_UNASSIGNED) {
        struct bt_mesh_msg_ctx ctx = {
            .addr = sw_idx + node_primary_addr,
        };
        /* This is a dummy message sufficient
        * for the led server
        */
        net_buf_simple_add_u8(msg, sw->onoff_state);
        gen_onoff_set_unack(mod_srv, &ctx, msg);
        goto done;
    }

    if(pub_cli->addr == BT_MESH_ADDR_UNASSIGNED) {
        goto done;
    }

    TLS_BT_APPL_TRACE_DEBUG("publish to 0x%04x onoff 0x%04x sw_idx 0x%04x\r\n",
                            pub_cli->addr, sw->onoff_state, sw_idx);
    bt_mesh_model_msg_init(pub_cli->msg,
                           BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
    net_buf_simple_add_u8(pub_cli->msg, sw->onoff_state);
    net_buf_simple_add_u8(pub_cli->msg, trans_id++);
    err = bt_mesh_model_publish(mod_cli);

    if(err) {
        TLS_BT_APPL_TRACE_ERROR("bt_mesh_model_publish err %d\r\n", err);
    }

done:
    os_mbuf_free_chain(msg);
}

static int gen_onoff_client_deinit(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("gen_onoff_client_deinit\r\n");
    /*Free timer*/
    k_work_deinit(&sw.button_work);
    k_delayed_work_deinit(&sw.button_timer);
    /*Reset state*/
    sw.onoff_state = 0;
    /*Reset gpio interrupt*/
    tls_gpio_irq_disable(MESH_GPIO_BUTTON);
    tls_gpio_isr_register(MESH_GPIO_BUTTON, NULL, (void *)MESH_GPIO_BUTTON);
    return 0;
}

static int gen_onoff_client_init(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("gen_onoff_client_init\r\n");
    model->user_data = &sw;

    /* Initialize the button debouncer */
    last_time = k_uptime_get_32();
    sw.onoff_state = 0;
    /* Initialize button worker task*/
    k_work_init(&sw.button_work, button_pressed_worker);
    k_work_add_arg(&sw.button_work, model);
    /* Initialize button count timer */
    k_delayed_work_init(&sw.button_timer, button_cnt_timer);
    k_delayed_work_add_arg(&sw.button_timer, model);

    /*Initialize the button input gpio settings*/
    tls_gpio_cfg(MESH_GPIO_BUTTON, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_isr_register(MESH_GPIO_BUTTON, button_gpio_isr_callback, (void *)MESH_GPIO_BUTTON);
    tls_gpio_irq_enable(MESH_GPIO_BUTTON, WM_GPIO_IRQ_TRIG_RISING_EDGE);    
    return 0;
}

const struct bt_mesh_model_cb gen_onoff_client_cb = {
    .init = gen_onoff_client_init,
    .deinit = gen_onoff_client_deinit,
};
#endif


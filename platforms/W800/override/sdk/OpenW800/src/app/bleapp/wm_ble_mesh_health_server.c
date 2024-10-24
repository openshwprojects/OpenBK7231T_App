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
#include "wm_ble_mesh_health_server.h"

#define STANDARD_TEST_ID 0x00
#define TEST_ID 0x01
static int recent_test_id = STANDARD_TEST_ID;

#define FAULT_ARR_SIZE 2
static bool has_reg_fault = true;

struct bt_mesh_model_pub health_pub;

static int
fault_get_cur(struct bt_mesh_model *model,
              uint8_t *test_id,
              uint16_t *company_id,
              uint8_t *faults,
              uint8_t *fault_count)
{
    uint8_t reg_faults[FAULT_ARR_SIZE] = { [0 ... FAULT_ARR_SIZE - 1] = 0xff };
    TLS_BT_APPL_TRACE_DEBUG("fault_get_cur() has_reg_fault %u\r\n", has_reg_fault);
    *test_id = recent_test_id;
    *company_id = CID_VENDOR;
    *fault_count = min(*fault_count, sizeof(reg_faults));
    memcpy(faults, reg_faults, *fault_count);
    return 0;
}

static int
fault_get_reg(struct bt_mesh_model *model,
              uint16_t company_id,
              uint8_t *test_id,
              uint8_t *faults,
              uint8_t *fault_count)
{
    if(company_id != CID_VENDOR) {
        return -BLE_HS_EINVAL;
    }

    TLS_BT_APPL_TRACE_DEBUG("fault_get_reg() has_reg_fault %u\r\n", has_reg_fault);
    *test_id = recent_test_id;

    if(has_reg_fault) {
        uint8_t reg_faults[FAULT_ARR_SIZE] = { [0 ... FAULT_ARR_SIZE - 1] = 0xff };
        *fault_count = min(*fault_count, sizeof(reg_faults));
        memcpy(faults, reg_faults, *fault_count);
    } else {
        *fault_count = 0;
    }

    return 0;
}

static int
fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
    if(company_id != CID_VENDOR) {
        return -BLE_HS_EINVAL;
    }

    has_reg_fault = false;
    return 0;
}

static int
fault_test(struct bt_mesh_model *model, uint8_t test_id, uint16_t company_id)
{
    if(company_id != CID_VENDOR) {
        return -BLE_HS_EINVAL;
    }

    if(test_id != STANDARD_TEST_ID && test_id != TEST_ID) {
        return -BLE_HS_EINVAL;
    }

    recent_test_id = test_id;
    has_reg_fault = true;
    bt_mesh_fault_update(bt_mesh_model_elem(model));
    return 0;
}
static void
attn_on(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("attentation_on\r\n");
    tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_RED); 
}
static void
attn_off(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("attentation_off\r\n");
    tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_OFF);
}
static void
init(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("health model init\r\n");
    tls_ble_mesh_led_init();  
}
static void
deinit(struct bt_mesh_model *model)
{
    TLS_BT_APPL_TRACE_DEBUG("health model deinit\r\n");
    tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_OFF); 
}


static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .init = &init,
    .deinit = &deinit,
    .fault_get_cur = &fault_get_cur,
    .fault_get_reg = &fault_get_reg,
    .fault_clear = &fault_clear,
    .fault_test = &fault_test,
    .attn_on = &attn_on,
    .attn_off = &attn_off,
};

struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};
#endif


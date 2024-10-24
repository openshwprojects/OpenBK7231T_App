
#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)



#include "mesh/mesh.h"
#include "mesh/glue.h"
#include "wm_bt_util.h"
#include "wm_ble_mesh.h"

#include "wm_ble_mesh_light_model.h"


static uint8_t gen_onoff_state;
static int16_t gen_level_state;
static uint32_t onoff_counter = 0;
static void update_light_state(void)
{
    TLS_BT_APPL_TRACE_DEBUG("Light state: onoff=%d lvl=0x%04x[%d]\r\n", gen_onoff_state,
                            (u16_t)gen_level_state, onoff_counter++);


    if(gen_onoff_state)
    {
        tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_RED|WM_MESH_LED_FLAG_BIT_GREEN|WM_MESH_LED_FLAG_BIT_BLUE);
    }else
    {
        tls_ble_mesh_led_update(WM_MESH_LED_FLAG_BIT_OFF);
    }

}

int tls_light_model_gen_onoff_get(struct bt_mesh_model *model, u8_t *state)
{
    *state = gen_onoff_state;
    return 0;
}

int tls_light_model_gen_onoff_set(struct bt_mesh_model *model, u8_t state)
{
    gen_onoff_state = state;
    update_light_state();
    return 0;
}
int tls_light_model_gen_onoff_init(struct bt_mesh_model *model)
{
    return 0;
}
int tls_light_model_gen_onoff_deinit(struct bt_mesh_model *model)
{
    return 0;    
}


int tls_light_model_gen_level_get(struct bt_mesh_model *model, s16_t *level)
{
    *level = gen_level_state;
    return 0;
}

int tls_light_model_gen_level_set(struct bt_mesh_model *model, s16_t level)
{
    gen_level_state = level;

    if((u16_t)gen_level_state > 0x0000) {
        gen_onoff_state = 1;
    }

    if((u16_t)gen_level_state == 0x0000) {
        gen_onoff_state = 0;
    }

    update_light_state();
    return 0;
}
int tls_light_model_gen_level_init(struct bt_mesh_model *model)
{
    return 0;    
}
int tls_light_model_gen_level_deinit(struct bt_mesh_model *model)
{
    return 0;    
}



int tls_light_model_light_lightness_get(struct bt_mesh_model *model, s16_t *lightness)
{
    return tls_light_model_gen_level_get(model, lightness);
}

int tls_light_model_light_lightness_set(struct bt_mesh_model *model, s16_t lightness)
{
    return tls_light_model_gen_level_set(model, lightness);
}
int tls_light_model_light_lightness_init(struct bt_mesh_model *model)
{
    return 0;    
}
int tls_light_model_light_lightness_deinit(struct bt_mesh_model *model)
{
    return 0;
}

#endif

#ifndef __WM_BLE_MESH_VND_MODEL_H__
#define __WM_BLE_MESH_VND_MODEL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern struct bt_mesh_model_pub vnd_model_pub;
extern const struct bt_mesh_model_op vnd_model_op[];
extern const struct bt_mesh_model_cb vnd_model_cb;


#define BT_MESH_MODEL_WM_VND(vnd_data)  \
    BT_MESH_MODEL_VND_CB(CID_VENDOR, BT_MESH_MODEL_ID_GEN_ONOFF_SRV, vnd_model_op, &vnd_model_pub,   \
            vnd_data, &vnd_model_cb)


extern int vnd_model_send(uint16_t net_idx, uint16_t dst, uint16_t app_idx, uint8_t *payload, int length);

#ifdef __cplusplus
}
#endif

#endif

#ifndef __WM_BLE_MESH_NODE_H__
#define __WM_BLE_MESH_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Company ID */
#ifndef CID_VENDOR
#define CID_VENDOR 0x070c
#endif

extern int tls_ble_mesh_node_init(void);
extern int tls_ble_mesh_node_deinit(int reason);

#ifdef __cplusplus
}
#endif

#endif

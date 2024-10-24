#ifndef __WM_BLE_MESH_STORE_H__
#define __WM_BLE_MESH_STORE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void tls_mesh_store_init(void);

extern void tls_mesh_store_deinit(void);


extern int tls_mesh_store_update(const char *key, uint8_t valid, const uint8_t *value, int value_len);

extern int tls_mesh_store_retrieve(char *key, uint8_t *value, int *value_len);

extern int tls_mesh_store_flush(void);

extern int tls_mesh_store_erase(void);

#ifdef __cplusplus
}
#endif

#endif

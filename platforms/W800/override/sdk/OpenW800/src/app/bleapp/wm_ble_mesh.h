#ifndef __WM_BLE_MESH_H__
#define __WM_BLE_MESH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Company ID */
#ifndef CID_VENDOR
#define CID_VENDOR 0x070c
#endif

#define WM_MESH_LED_FLAG_BIT_OFF          0x00
#define WM_MESH_LED_FLAG_BIT_GREEN        0x01
#define WM_MESH_LED_FLAG_BIT_RED          0x02
#define WM_MESH_LED_FLAG_BIT_BLUE         0x04


extern int tls_ble_mesh_init(tls_bt_mesh_at_callback_t at_cb, tls_bt_mesh_role_t role,
                             bool running);
extern int tls_ble_mesh_deinit();

extern void tls_ble_mesh_led_init(void);

extern void tls_ble_mesh_led_update(uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif

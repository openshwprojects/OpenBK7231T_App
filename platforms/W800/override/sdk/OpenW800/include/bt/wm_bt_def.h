/**
 * @file    wm_bt_def.h
 *
 * @brief   Bluetooth Define
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_BT_DEF_H
#define WM_BT_DEF_H

/** Bluetooth Error Status */
typedef enum
{
    TLS_BT_STATUS_SUCCESS, /**< success */
    TLS_BT_STATUS_FAIL,
    TLS_BT_STATUS_NOT_READY,
    TLS_BT_STATUS_NOMEM, /**< alloc memory failed */
    TLS_BT_STATUS_BUSY,
    TLS_BT_STATUS_DONE,        /**< request already completed */
    TLS_BT_STATUS_UNSUPPORTED,
    TLS_BT_STATUS_PARM_INVALID,
    TLS_BT_STATUS_UNHANDLED,
    TLS_BT_STATUS_AUTH_FAILURE,
    TLS_BT_STATUS_RMT_DEV_DOWN,
    TLS_BT_STATUS_AUTH_REJECTED,
    TLS_BT_STATUS_THREAD_FAILED, /**< create internal thread failed */
    TLS_BT_STATUS_INTERNAL_ERROR, /**< controller stack internal error */
    TLS_BT_STATUS_CTRL_ENABLE_FAILED,
    TLS_BT_STATUS_HOST_ENABLE_FAILED,
    TLS_BT_STATUS_CTRL_DISABLE_FAILED,
    TLS_BT_STATUS_HOST_DISABLE_FAILED,

} tls_bt_status_t;

typedef enum
{
    TLS_BT_CTRL_IDLE     =              (1<<0),
    TLS_BT_CTRL_ENABLED  =              (1<<1),
    TLS_BT_CTRL_SLEEPING =              (1<<2),
    TLS_BT_CTRL_BLE_ROLE_MASTER =       (1<<3),
    TLS_BT_CTRL_BLE_ROLE_SLAVE =        (1<<4),
    TLS_BT_CTRL_BLE_ROLE_END =          (1<<5),
    TLS_BT_CTRL_BLE_STATE_IDLE =        (1<<6),
    TLS_BT_CTRL_BLE_STATE_ADVERTISING = (1<<7),
    TLS_BT_CTRL_BLE_STATE_SCANNING =    (1<<8),
    TLS_BT_CTRL_BLE_STATE_INITIATING =  (1<<9),
    TLS_BT_CTRL_BLE_STATE_STOPPING =    (1<<10),
    TLS_BT_CTRL_BLE_STATE_TESTING =     (1<<11),
} tls_bt_ctrl_status_t;

/** Bluetooth Adapter State */
typedef enum
{
    WM_BT_STATE_OFF,
    WM_BT_STATE_ON
} tls_bt_state_t;

/** bluetooth host statck events */
typedef enum
{
    WM_BT_ADAPTER_STATE_CHG_EVT = (0x01<<0),        
    WM_BT_ADAPTER_PROP_CHG_EVT  = (0x01<<1),   
    WM_BT_RMT_DEVICE_PROP_EVT   = (0x01<<2),               
    WM_BT_DEVICE_FOUND_EVT      = (0x01<<3), 
    WM_BT_DISCOVERY_STATE_CHG_EVT=(0x01<<4),
    WM_BT_REQUEST_EVT           = (0x01<<5),
    WM_BT_SSP_REQUEST_EVT       = (0x01<<6),
    WM_BT_PIN_REQUEST_EVT       = (0x01<<7),
    WM_BT_BOND_STATE_CHG_EVT    = (0x01<<8),
    WM_BT_ACL_STATE_CHG_EVT     = (0x01<<9),
    WM_BT_ENERGY_INFO_EVT       = (0x01<<10),
    WM_BT_LE_TEST_EVT           = (0x01<<11),
} tls_bt_host_evt_t;

typedef struct
{
    tls_bt_state_t         status; /**< bluetooth adapter state */
} tls_bt_adapter_state_change_msg_t;

/** Bluetooth Adapter and Remote Device property types */
typedef enum
{
    /* Properties common to both adapter and remote device */
    /**
     * Description - Bluetooth Device Name
     * Access mode - Adapter name can be GET/SET. Remote device can be GET
     * Data type   - bt_bdname_t
     */
    WM_BT_PROPERTY_BDNAME = 0x1,
    /**
     * Description - Bluetooth Device Address
     * Access mode - Only GET.
     * Data type   - bt_bdaddr_t
     */
    WM_BT_PROPERTY_BDADDR,
    /**
     * Description - Bluetooth Service 128-bit UUIDs
     * Access mode - Only GET.
     * Data type   - Array of bt_uuid_t (Array size inferred from property length).
     */
    WM_BT_PROPERTY_UUIDS,
    /**
     * Description - Bluetooth Class of Device as found in Assigned Numbers
     * Access mode - Only GET.
     * Data type   - uint32_t.
     */
    WM_BT_PROPERTY_CLASS_OF_DEVICE,
    /**
     * Description - Device Type - BREDR, BLE or DUAL Mode
     * Access mode - Only GET.
     * Data type   - bt_device_type_t
     */
    WM_BT_PROPERTY_TYPE_OF_DEVICE,
    /**
     * Description - Bluetooth Service Record
     * Access mode - Only GET.
     * Data type   - bt_service_record_t
     */
    WM_BT_PROPERTY_SERVICE_RECORD,

    /* Properties unique to adapter */
    /**
     * Description - Bluetooth Adapter scan mode
     * Access mode - GET and SET
     * Data type   - bt_scan_mode_t.
     */
    WM_BT_PROPERTY_ADAPTER_SCAN_MODE,
    /**
     * Description - List of bonded devices
     * Access mode - Only GET.
     * Data type   - Array of bt_bdaddr_t of the bonded remote devices
     *               (Array size inferred from property length).
     */
    WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES,
    /**
     * Description - Bluetooth Adapter Discovery timeout (in seconds)
     * Access mode - GET and SET
     * Data type   - uint32_t
     */
    WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT,

    /* Properties unique to remote device */
    /**
     * Description - User defined friendly name of the remote device
     * Access mode - GET and SET
     * Data type   - bt_bdname_t.
     */
    WM_BT_PROPERTY_REMOTE_FRIENDLY_NAME,
    /**
     * Description - RSSI value of the inquired remote device
     * Access mode - Only GET.
     * Data type   - int32_t.
     */
    WM_BT_PROPERTY_REMOTE_RSSI,
    /**
     * Description - Remote version info
     * Access mode - SET/GET.
     * Data type   - bt_remote_version_t.
     */

    WM_BT_PROPERTY_REMOTE_VERSION_INFO,

    /**
     * Description - Local LE features
     * Access mode - GET.
     * Data type   - bt_local_le_features_t.
     */
    WM_BT_PROPERTY_LOCAL_LE_FEATURES,

    WM_BT_PROPERTY_REMOTE_DEVICE_TIMESTAMP = 0xFF,
} tls_bt_property_type_t;

/** Bluetooth Adapter Property data structure */
typedef struct
{
    tls_bt_property_type_t type;
    int len;
    void *val;
} tls_bt_property_t;

typedef struct
{
    tls_bt_status_t     status; 
	int num_properties;
	tls_bt_property_t *properties; /**< bluetooth adapter property data */
} tls_bt_adapter_prop_msg_t; 

typedef enum
{ 
    WM_BLE_SCAN_STOP = 0,
    WM_BLE_SCAN_PASSIVE = 1,
    WM_BLE_SCAN_ACTIVE = 2,
    
} wm_ble_scan_type_t;

typedef enum
{
    WM_BLE_ADV_DATA = 0,
    WM_BLE_ADV_RSP_DATA,
} wm_ble_gap_data_t;

typedef enum{
    WM_BLE_ADV_STOP = 0,
    WM_BLE_ADV_IND,
    WM_BLE_ADV_DIRECT_IND_HDC,  /*<high duty cycle, directed advertising>*/
    WM_BLE_ADV_SCAN_IND,
    WM_BLE_ADV_NONCONN_IND,
    WM_BLE_ADV_DIRECT_IND_LDC,  /*<low duty cycle, directed advertising>*/
} wm_ble_adv_type_t;

#define WM_BLE_GAP_EVENT_CONNECT               (0x01<<0)
#define WM_BLE_GAP_EVENT_DISCONNECT            (0x01<<1)
/* Reserved                                 2 */
#define WM_BLE_GAP_EVENT_CONN_UPDATE           (0x01<<3)
#define WM_BLE_GAP_EVENT_CONN_UPDATE_REQ       (0x01<<4)
#define WM_BLE_GAP_EVENT_L2CAP_UPDATE_REQ      (0x01<<5)
#define WM_BLE_GAP_EVENT_TERM_FAILURE          (0x01<<6)
#define WM_BLE_GAP_EVENT_DISC                  (0x01<<7)
#define WM_BLE_GAP_EVENT_DISC_COMPLETE         (0x01<<8)
#define WM_BLE_GAP_EVENT_ADV_COMPLETE          (0x01<<9)
#define WM_BLE_GAP_EVENT_ENC_CHANGE            (0x01<<10)
#define WM_BLE_GAP_EVENT_PASSKEY_ACTION        (0x01<<11)
#define WM_BLE_GAP_EVENT_NOTIFY_RX             (0x01<<12)
#define WM_BLE_GAP_EVENT_NOTIFY_TX             (0x01<<13)
#define WM_BLE_GAP_EVENT_SUBSCRIBE             (0x01<<14)
#define WM_BLE_GAP_EVENT_MTU                   (0x01<<15)
#define WM_BLE_GAP_EVENT_IDENTITY_RESOLVED     (0x01<<16)
#define WM_BLE_GAP_EVENT_REPEAT_PAIRING        (0x01<<17)
#define WM_BLE_GAP_EVENT_PHY_UPDATE_COMPLETE   (0x01<<18)
#define WM_BLE_GAP_EVENT_EXT_DISC              (0x01<<19)
#define WM_BLE_GAP_EVENT_PERIODIC_SYNC         (0x01<<20)
#define WM_BLE_GAP_EVENT_PERIODIC_REPORT       (0x01<<21)
#define WM_BLE_GAP_EVENT_PERIODIC_SYNC_LOST    (0x01<<22)
#define WM_BLE_GAP_EVENT_SCAN_REQ_RCVD         (0x01<<23)
#define WM_BLE_GAP_EVENT_PERIODIC_TRANSFER     (0x01<<24)
#define WM_BLE_GAP_EVENT_HOST_SHUTDOWN         (0x01<<31)


/** Bluetooth Address */
typedef struct
{
    uint8_t address[6];
} __attribute__((packed))tls_bt_addr_t;

typedef struct
{
    tls_bt_status_t     status;
	tls_bt_addr_t     *address;
	int num_properties;
	tls_bt_property_t *properties; /**< bluetooth adapter property data */
} tls_bt_remote_dev_prop_msg_t;

typedef struct
{
	int num_properties;
	tls_bt_property_t *properties; /**< bluetooth adapter property data */
} tls_bt_device_found_msg_t;

/** Bluetooth Adapter Discovery state */
typedef enum
{
    WM_BT_DISCOVERY_STOPPED,
    WM_BT_DISCOVERY_STARTED
} tls_bt_discovery_state_t;

typedef struct
{
	tls_bt_discovery_state_t state;
} tls_bt_discovery_state_chg_msg_t;

/** Bluetooth Device Name */
typedef struct
{
    uint8_t name[249];
} __attribute__((packed))tls_bt_bdname_t;

typedef struct
{
	tls_bt_addr_t *remote_bd_addr;
	tls_bt_bdname_t *bd_name;
	uint32_t cod;
	uint8_t  min_16_digit;
} tls_bt_pin_request_msg_t;

/** Bluetooth SSP Bonding Variant */
typedef enum
{
    WM_BT_SSP_VARIANT_PASSKEY_CONFIRMATION,
    WM_BT_SSP_VARIANT_PASSKEY_ENTRY,
    WM_BT_SSP_VARIANT_CONSENT,
    WM_BT_SSP_VARIANT_PASSKEY_NOTIFICATION
} tls_bt_ssp_variant_t;

/** Bluetooth PinKey Code */
typedef struct
{
    uint8_t pin[16];
} __attribute__((packed))tls_bt_pin_code_t;

typedef struct
{
	tls_bt_addr_t *remote_bd_addr;
	tls_bt_bdname_t *bd_name;
	uint32_t cod;
	tls_bt_ssp_variant_t pairing_variant;
	uint32_t pass_key;
} tls_bt_ssp_request_msg_t;

/** Bluetooth Bond state */
typedef enum
{
    WM_BT_BOND_STATE_NONE,
    WM_BT_BOND_STATE_BONDING,
    WM_BT_BOND_STATE_BONDED
} tls_bt_bond_state_t;

typedef struct
{
	tls_bt_status_t status;
	tls_bt_addr_t *remote_bd_addr;
	tls_bt_bond_state_t state;
} tls_bt_bond_state_chg_msg_t;

/** Bluetooth ACL connection state */
typedef enum
{
    WM_BT_ACL_STATE_CONNECTED,
    WM_BT_ACL_STATE_DISCONNECTED
} tls_bt_acl_state_t;

typedef struct
{
	tls_bt_status_t status;
	tls_bt_addr_t *remote_address;
    uint8_t link_type;
	tls_bt_acl_state_t state;
} tls_bt_acl_state_chg_msg_t;

typedef struct
{
    uint8_t status;
    uint8_t ctrl_state;     /* stack reported state */
    uint64_t tx_time;       /* in ms */
    uint64_t rx_time;       /* in ms */
    uint64_t idle_time;     /* in ms */
    uint64_t energy_used;   /* a product of mA, V and ms */
} __attribute__((packed))tls_bt_activity_energy_info;

typedef struct
{
	tls_bt_activity_energy_info *energy_info;
} tls_bt_energy_info_msg_t;

typedef struct
{
	uint8_t status;
	uint32_t count;
} tls_bt_ble_test_msg_t;

typedef union
{
	tls_bt_adapter_state_change_msg_t	     adapter_state_change;	  
	tls_bt_adapter_prop_msg_t	             adapter_prop;	  
	tls_bt_remote_dev_prop_msg_t		     remote_device_prop;
	tls_bt_device_found_msg_t                device_found;
	tls_bt_discovery_state_chg_msg_t         discovery_state;
	tls_bt_pin_request_msg_t                 pin_request;
	tls_bt_ssp_request_msg_t                 ssp_request;
	tls_bt_bond_state_chg_msg_t              bond_state;
	tls_bt_acl_state_chg_msg_t               acl_state;
	tls_bt_energy_info_msg_t                 energy_info;
	tls_bt_ble_test_msg_t                    ble_test;
} tls_bt_host_msg_t;

/** BT host callback function */
typedef void (*tls_bt_host_callback_t)(tls_bt_host_evt_t event, tls_bt_host_msg_t *p_data);


typedef enum
{
	TLS_BT_LOG_NONE = 0,
	TLS_BT_LOG_ERROR = 1,
	TLS_BT_LOG_WARNING = 2,
	TLS_BT_LOG_API = 3,
	TLS_BT_LOG_EVENT = 4,
	TLS_BT_LOG_DEBUG = 5,
	TLS_BT_LOG_VERBOSE = 6,
} tls_bt_log_level_t;

typedef struct
{
  uint8_t  uart_index;  /**< uart port index, 1~4 */
  uint32_t band_rate;   /**< band rate: 115200 ~ 2M */
  uint8_t data_bit;     /**< data bit:5 ~ 8 */
  uint8_t verify_bit;   /**< 0:NONE, 1 ODD, 2 EVEN */
  uint8_t stop_bit;     /**< 0:1bit; 1:1.5bit; 2:2bits */
} tls_bt_hci_if_t;

typedef enum
{
	TLS_BLE_PWR_TYPE_CONN_HDL0,
	TLS_BLE_PWR_TYPE_CONN_HDL1,
	TLS_BLE_PWR_TYPE_CONN_HDL2,
	TLS_BLE_PWR_TYPE_CONN_HDL3,
	TLS_BLE_PWR_TYPE_CONN_HDL4,
	TLS_BLE_PWR_TYPE_CONN_HDL5,
	TLS_BLE_PWR_TYPE_CONN_HDL6,
	TLS_BLE_PWR_TYPE_CONN_HDL7,
	TLS_BLE_PWR_TYPE_CONN_HDL8,
	TLS_BLE_PWR_TYPE_ADV,
	TLS_BLE_PWR_TYPE_SCAN,
	TLS_BLE_PWR_TYPE_DEFAULT,
} tls_ble_power_type_t;

typedef enum
{
	WM_AUDIO_OVER_HCI = 0,
	WM_AUDIO_INTERNAL_MODE,
} tls_sco_data_path_t;


typedef struct
{
	void (*notify_controller_avaiable_hci_buffer)(int cnt);
	void (*notify_host_recv_h4)(uint8_t *ptr, uint16_t length);
} tls_bt_host_if_t;


/*****************************************************************************
 **  Constants and Type Definitions
 *****************************************************************************/


/** Attribute permissions */
#define WM_GATT_PERM_READ              (1 << 0) /**< bit 0 -  0x0001 */
#define WM_GATT_PERM_READ_ENCRYPTED    (1 << 1) /**< bit 1 -  0x0002 */
#define WM_GATT_PERM_READ_ENC_MITM     (1 << 2) /**< bit 2 -  0x0004 */
#define WM_GATT_PERM_WRITE             (1 << 4) /**< bit 4 -  0x0010 */
#define WM_GATT_PERM_WRITE_ENCRYPTED   (1 << 5) /**< bit 5 -  0x0020 */
#define WM_GATT_PERM_WRITE_ENC_MITM    (1 << 6) /**< bit 6 -  0x0040 */
#define WM_GATT_PERM_WRITE_SIGNED      (1 << 7) /**< bit 7 -  0x0080 */
#define WM_GATT_PERM_WRITE_SIGNED_MITM (1 << 8) /**< bit 8 -  0x0100 */

/** definition of characteristic properties */
#define WM_GATT_CHAR_PROP_BIT_BROADCAST    (1 << 0)   /**< 0x01 */
#define WM_GATT_CHAR_PROP_BIT_READ         (1 << 1)   /**< 0x02 */
#define WM_GATT_CHAR_PROP_BIT_WRITE_NR     (1 << 2)   /**< 0x04 */
#define WM_GATT_CHAR_PROP_BIT_WRITE        (1 << 3)   /**< 0x08 */
#define WM_GATT_CHAR_PROP_BIT_NOTIFY       (1 << 4)   /**< 0x10 */
#define WM_GATT_CHAR_PROP_BIT_INDICATE     (1 << 5)   /**< 0x20 */
#define WM_GATT_CHAR_PROP_BIT_AUTH         (1 << 6)   /**< 0x40 */
#define WM_GATT_CHAR_PROP_BIT_EXT_PROP     (1 << 7)   /**< 0x80 */

#define WM_BLE_MAX_ATTR_LEN    600



/** max client application WM BLE Client can support */
#ifndef WM_BLE_CLIENT_MAX
    #define WM_BLE_CLIENT_MAX              3
#endif

/** max server application WM BLE Server can support */
#define WM_BLE_SERVER_MAX              4
#define WM_BLE_ATTRIBUTE_MAX           50

#ifndef WM_BLE_SERVER_SECURITY
    #define WM_BLE_SERVER_SECURITY         BTA_DM_BLE_SEC_NONE
#endif

#define WM_BLE_INVALID_IF              0xFF
#define WM_BLE_INVALID_CONN            0xFFFF

#define WM_BLE_GATT_TRANSPORT_LE                           0x02
#define WM_BLE_GATT_TRANSPORT_BR_EDR                       0x01
#define WM_BLE_GATT_TRANSPORT_LE_BR_EDR                    0x03

#define WM_BLE_MAX_PDU_LENGTH                              251


/** BLE events */
typedef enum
{
    /** BLE Client events */
    WM_BLE_CL_REGISTER_EVT,      /**< BLE client is registered. */
    WM_BLE_CL_DEREGISTER_EVT,    /**< BLE client is deregistered. */
    WM_BLE_CL_READ_CHAR_EVT,
    WM_BLE_CL_WRITE_CHAR_EVT,
    WM_BLE_CL_PREP_WRITE_EVT,
    WM_BLE_CL_EXEC_CMPL_EVT,     /**< Execute complete event */
    WM_BLE_CL_SEARCH_CMPL_EVT,   /**< GATT discovery complete event */
    WM_BLE_CL_SEARCH_RES_EVT,    /**< GATT discovery result event */
    WM_BLE_CL_READ_DESCR_EVT,
    WM_BLE_CL_WRITE_DESCR_EVT,
    WM_BLE_CL_NOTIF_EVT,         /**< GATT attribute notification event */
    WM_BLE_CL_OPEN_EVT,          /**< BLE open request status  event */
    WM_BLE_CL_CLOSE_EVT,         /**< GATTC  close request status event */
    WM_BLE_CL_LISTEN_EVT,
    WM_BLE_CL_CFG_MTU_EVT,       /**< configure MTU complete event */
    WM_BLE_CL_CONGEST_EVT,       /**< GATT congestion/uncongestion event */
    WM_BLE_CL_REPORT_DB_EVT,
    WM_BLE_CL_REG_NOTIFY_EVT,
    WM_BLE_CL_DEREG_NOTIFY_EVT,


    /** BLE Server events */
    WM_BLE_SE_REGISTER_EVT,      /**< BLE Server is registered */
    WM_BLE_SE_DEREGISTER_EVT,    /**< BLE Server is deregistered */
    WM_BLE_SE_CONNECT_EVT,
    WM_BLE_SE_DISCONNECT_EVT,
    WM_BLE_SE_CREATE_EVT,        /**< Service is created */
    WM_BLE_SE_ADD_INCL_SRVC_EVT,
    WM_BLE_SE_ADD_CHAR_EVT,       /**< char data is added */
    WM_BLE_SE_ADD_CHAR_DESCR_EVT,
    WM_BLE_SE_START_EVT,         /**< Service is started */
    WM_BLE_SE_STOP_EVT,          /**< Service is stopped */
    WM_BLE_SE_DELETE_EVT,
    WM_BLE_SE_READ_EVT,          /**< Read request from client */
    WM_BLE_SE_WRITE_EVT,         /**< Write request from client */
    WM_BLE_SE_EXEC_WRITE_EVT,    /**< Execute Write request from client */
    WM_BLE_SE_CONFIRM_EVT,       /**< Confirm event */
    WM_BLE_SE_RESP_EVT,
    WM_BLE_SE_CONGEST_EVT,       /**< Congestion event */
    WM_BLE_SE_MTU_EVT,

} tls_ble_evt_t;

/* WM BLE Client Host callback events */
/* Client callback function events */

/** Bluetooth 128-bit UUID */
typedef struct
{
    uint8_t uu[16];
} tls_bt_uuid_t;

/* callback event data for WM_BLE_CL_REGISTER_EVT/ event */
typedef struct
{
    uint8_t         status; /**< operation status */
    uint8_t         client_if; /**< Client interface ID */
    tls_bt_uuid_t          app_uuid;  /**< Client uuid*/

} tls_ble_cl_register_msg_t;

/** callback event data for WM_BLE_CL_READ_CHAR_EVT /WM_BLE_CL_READ_CHAR_EVTevent */
typedef struct
{
    uint16_t              conn_id;
    uint8_t         status;
    uint16_t              handle;
    uint16_t              len;
    uint8_t               *value;
    uint16_t              value_type;
} tls_ble_cl_read_msg_t;

/** callback event data for WM_BLE_CL_WRITE_CHAR_EVT/WM_BLE_CL_PREP_WRITE_EVT/WM_BLE_CL_WRITE_DESCR_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint8_t         status;
    uint16_t            handle;
} tls_ble_cl_write_msg_t;

/** callback event data for WM_BLE_CL_EXEC_CMPL_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint8_t         status;
} tls_ble_cl_exec_cmpl_msg_t;

/** callback event data for WM_BLE_CL_SEARCH_CMPL_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint8_t         status; /**< operation status */
} tls_ble_cl_search_cmpl_msg_t;

/** callback event data for WM_BLE_CL_SEARCH_RES_EVT event */
typedef struct
{
    uint16_t              conn_id;
    tls_bt_uuid_t             uuid;
    uint8_t               inst_id;
} tls_ble_cl_search_res_msg_t;

/** callback event data for WM_BLE_CL_NOTIF_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint8_t               *value;
    uint8_t             bda[6];
    uint16_t            handle;
    uint16_t              len;
    bool             is_notify;
} tls_ble_cl_notify_msg_t;

/** callback event data for WM_BLE_CL_OPEN_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint8_t         status; /**< operation status */
    uint8_t         client_if; /**< Client interface ID */
    uint8_t             bd_addr[6];
} tls_ble_cl_open_msg_t;

/** callback event data for WM_BLE_CL_CLOSE_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint8_t         status;
    uint8_t         client_if;
    uint8_t             remote_bda[6];
    uint16_t     reason;         /**< disconnect reason code, not useful when connect event is reported */
} tls_ble_cl_close_msg_t;

/** callback event data for WM_BLE_CL_LISTEN_EVT event */
typedef struct
{
    uint8_t         status;
    uint8_t        client_if;
} tls_ble_cl_listen_msg_t;

/** callback event data for WM_BLE_CL_CFG_MTU_EVT event */
typedef struct
{
    uint16_t conn_id;
    uint8_t         status;
    uint16_t mtu;
} tls_ble_cl_cfg_mtu_msg_t;

typedef struct
{
    uint16_t              conn_id;
    bool             congested; /**< congestion indicator */
} tls_ble_cl_congest_msg_t;

typedef enum
{
    WM_BTGATT_DB_PRIMARY_SERVICE,
    WM_BTGATT_DB_SECONDARY_SERVICE,
    WM_BTGATT_DB_INCLUDED_SERVICE,
    WM_BTGATT_DB_CHARACTERISTIC,
    WM_BTGATT_DB_DESCRIPTOR,
} tls_bt_gatt_db_attribute_type_t;

typedef struct
{
    uint16_t             id;
    tls_bt_uuid_t           uuid;
    tls_bt_gatt_db_attribute_type_t type;
    uint16_t            attribute_handle;

    /*
     * If |type| is |BTGATT_DB_PRIMARY_SERVICE|, or
     * |BTGATT_DB_SECONDARY_SERVICE|, this contains the start and end attribute
     * handles.
     */
    uint16_t            start_handle;
    uint16_t            end_handle;

    /*
     * If |type| is |BTGATT_DB_CHARACTERISTIC|, this contains the properties of
     * the characteristic.
     */
    uint8_t             properties;
} tls_btgatt_db_element_t;

typedef struct
{
	uint16_t conn_id;
	tls_btgatt_db_element_t *db;
	int count;
	uint8_t status;
} tls_ble_cl_gatt_db_msg_t;

typedef struct
{
	uint16_t conn_id;
	uint8_t status;
	bool reg;
	uint16_t handle;
	
} tls_ble_cl_reg_notify_msg_t;


/* WM BLE Server Host callback events */
/* Server callback function events */

/** callback event data for WM_BLE_SE_REGISTER_EVT/WM_BLE_SE_DEREGISTER_EVT event */
typedef struct
{
    uint8_t         status; /* operation status */
    uint8_t         server_if; /* Server interface ID */
    tls_bt_uuid_t app_uuid;
} tls_ble_se_register_msg_t;

/** callback event data for WM_BLE_SE_CONNECT_EVT/WM_BLE_SE_DISCONNECT_EVT event */
typedef struct
{
    uint16_t conn_id;
    uint8_t         server_if; /**< Server interface ID */
    bool connected;
    uint16_t reason;
    uint8_t addr[6];
} tls_ble_se_connect_msg_t;

typedef tls_ble_se_connect_msg_t tls_ble_se_disconnect_msg_t;

/** callback event data for WM_BLE_SE_CREATE_EVT event */
typedef struct
{
    uint8_t         status; /**< operation status */
    uint8_t         server_if;
    bool is_primary;
    uint8_t inst_id;
    tls_bt_uuid_t uuid;
    uint16_t              service_id;
} tls_ble_se_create_msg_t;

/** callback event data for WM_BLE_SE_ADD_INCL_SRVC_EVT event */
typedef struct
{
    uint8_t         status; /**< operation status */
    uint8_t         server_if;
    uint16_t           service_id;
    uint16_t           attr_id;
} tls_ble_se_add_incl_srvc_msg_t;

/** callback event data for WM_BLE_SE_ADDCHAR_EVT event */
typedef struct
{
    uint8_t         status; /**< operation status */
    uint8_t         server_if;
    tls_bt_uuid_t uuid;
    uint16_t              service_id;
    uint16_t              attr_id;
} tls_ble_se_add_char_msg_t;

typedef tls_ble_se_add_char_msg_t tls_ble_se_add_char_descr_msg_t;



/** callback event data for WM_BLE_SE_START_EVT event */
typedef struct
{
    uint8_t         status; /**< operation status */
    uint8_t         server_if;
    uint16_t              service_id;
} tls_ble_se_start_msg_t;

typedef tls_ble_se_start_msg_t tls_ble_se_stop_msg_t;

typedef tls_ble_se_start_msg_t tls_ble_se_delete_msg_t;

typedef struct
{
    uint16_t        conn_id;
    uint32_t        trans_id;
    uint8_t       remote_bda[6];
    uint16_t        handle;
    uint16_t        offset;
    bool       is_long;
} tls_ble_se_read_msg_t;

typedef struct
{
    uint16_t              conn_id;
    uint32_t              trans_id;
    uint8_t             remote_bda[6];
    uint16_t              handle;     /**< attribute handle */
    uint16_t              offset;    /**< attribute value offset, if no offset is needed for the command, ignore it */
    uint16_t              len;        /**< length of attribute value */
    bool             need_rsp;   /**< need write response */
    bool             is_prep;    /**< is prepare write */
    uint8_t               *value;  /**< the actual attribute value */
} tls_ble_se_write_msg_t;

typedef struct
{
    uint16_t              conn_id;
    uint32_t              trans_id;
    uint8_t             remote_bda[6];
    uint8_t exec_write;
} tls_ble_se_exec_write_msg_t;

typedef struct
{
    uint16_t              conn_id;
    uint8_t         status; /**< operation status */

} tls_ble_se_confirm_msg_t;

typedef struct
{
    
    uint8_t         status; /* operation status */
	uint16_t        conn_id;
	uint16_t        trans_id;

} tls_ble_se_response_msg_t;

typedef struct
{
    uint16_t              conn_id;
    bool             congested; /**< congestion indicator */
} tls_ble_se_congest_msg_t;

typedef struct
{
    uint16_t              conn_id;
    uint16_t            mtu;
} tls_ble_se_mtu_msg_t;


/** Union of data associated with HD callback */
typedef union
{
    tls_ble_cl_register_msg_t    cli_register;      /**< WM_BLE_CL_REGISTER_EVT */
    tls_ble_cl_read_msg_t        cli_read;          /**< WM_BLE_CL_READ_EVT */
    tls_ble_cl_write_msg_t       cli_write;         /**< WM_BLE_CL_WRITE_EVT */
    tls_ble_cl_exec_cmpl_msg_t   cli_exec_cmpl;     /**< WM_BLE_CL_EXEC_CMPL_EVT */
    tls_ble_cl_search_cmpl_msg_t cli_search_cmpl;   /**< WM_BLE_CL_SEARCH_CMPL_EVT */
    tls_ble_cl_search_res_msg_t  cli_search_res;    /**< WM_BLE_CL_SEARCH_RES_EVT */
    tls_ble_cl_notify_msg_t      cli_notif;         /**< WM_BLE_CL_NOTIF_EVT */
    tls_ble_cl_open_msg_t        cli_open;          /**< WM_BLE_CL_OPEN_EVT */
    tls_ble_cl_close_msg_t       cli_close;         /**< WM_BLE_CL_CLOSE_EVT */
    tls_ble_cl_listen_msg_t      cli_listen;        /**< WM_BLE_CL_LISTEN_EVT */
    tls_ble_cl_cfg_mtu_msg_t     cli_cfg_mtu;       /**< WM_BLE_CL_CFG_MTU_EVT */
    tls_ble_cl_congest_msg_t     cli_congest;       /**< WM_BLE_CL_CONGEST_EVT */
	tls_ble_cl_gatt_db_msg_t     cli_db;            /* WM_BLE_CL_REPORT_DB_EVT*/
	tls_ble_cl_reg_notify_msg_t  cli_reg_notify;


    tls_ble_se_register_msg_t    ser_register;     /**< WM_BLE_SE_REGISTER_EVT */
    tls_ble_se_connect_msg_t     ser_connect;      /**< WM_BLE_SE_OPEN_EVT */
    tls_ble_se_disconnect_msg_t  ser_disconnect;   /**< WM_BLE_SE_CLOSE_EVT */
    tls_ble_se_create_msg_t      ser_create;       /**< WM_BLE_SE_CREATE_EVT */
    tls_ble_se_add_incl_srvc_msg_t ser_add_incl_srvc;
    tls_ble_se_add_char_msg_t    ser_add_char;     /**< WM_BLE_SE_ADDCHAR_EVT */
    tls_ble_se_add_char_descr_msg_t ser_add_char_descr;
    tls_ble_se_start_msg_t       ser_start_srvc;   /**< WM_BLE_SE_START_EVT */
    tls_ble_se_stop_msg_t        ser_stop_srvc;    /**< WM_BLE_SE_STOP_EVT */
    tls_ble_se_delete_msg_t      ser_delete_srvc;
    tls_ble_se_read_msg_t        ser_read;         /**< WM_BLE_SE_READ_EVT */
    tls_ble_se_write_msg_t       ser_write;        /**< WM_BLE_SE_WRITE_EVT */
    tls_ble_se_exec_write_msg_t  ser_exec_write;   /**< WM_BLE_SE_EXEC_WRITE_EVT */
    tls_ble_se_confirm_msg_t     ser_confirm;      /**< WM_BLE_SE_CONFIRM_EVT */
    tls_ble_se_congest_msg_t     ser_congest;      /**< WM_BLE_CL_CONGEST_EVT */
    tls_ble_se_mtu_msg_t         ser_mtu;
	tls_ble_se_response_msg_t    ser_resp;

} tls_ble_msg_t;

/** WM BLE Client callback function */
typedef void (*tls_ble_callback_t)(tls_ble_evt_t event, tls_ble_msg_t *p_data);

typedef void (*tls_ble_output_func_ptr)(uint8_t *p_data, uint32_t length);


/** BLE dm events */
typedef enum
{
    WM_BLE_DM_SET_ADV_DATA_CMPL_EVT = (0x01<<0),        /**< BLE DM set advertisement data completed*/
    WM_BLE_DM_TIMER_EXPIRED_EVT     = (0x01<<1),        /**< BLE DM timer expired event. */
    WM_BLE_DM_TRIGER_EVT            = (0x01<<2),        /**< BLE DM event trigered event, async processing*/
    WM_BLE_DM_SCAN_RES_EVT          = (0x01<<3),        /**< BLE DM scan result evt*/
	WM_BLE_DM_SET_SCAN_PARAM_CMPL_EVT=(0x01<<4),
	WM_BLE_DM_REPORT_RSSI_EVT       = (0x01<<5),
	WM_BLE_DM_SCAN_RES_CMPL_EVT     = (0x01<<6),
	WM_BLE_DM_SEC_EVT               = (0x01<<7),
	WM_BLE_DM_ADV_STARTED_EVT       = (0x01<<8),
    WM_BLE_DM_ADV_STOPPED_EVT       = (0x01<<9),
	WM_BLE_DM_HOST_SHUTDOWN_EVT     = (0x01<<31),
	
} tls_ble_dm_evt_t;


/** callback event data for WM_BLE_DM_SET_ADV_DATA */
typedef struct
{
    uint8_t         status; /**< operation status */
} tls_ble_dm_set_adv_data_cmpl_msg_t;

typedef struct
{
	uint8_t status;
	uint8_t dm_id; //dummy value; who care this value;
} tls_ble_dm_set_scan_param_cmpl_msg_t;

typedef struct
{
    uint32_t id;
    int32_t func_ptr;
} tls_ble_dm_timer_expired_msg_t;

typedef tls_ble_dm_timer_expired_msg_t tls_ble_dm_evt_triger_msg_t;

typedef struct
{
    uint8_t address[6];                    /**< device address */
    int8_t rssi;                        /**< signal strength */
    uint8_t *value; /**< adv /scan resp value */
} tls_ble_dm_scan_res_msg_t;

typedef struct
{
	uint8_t address[6];
	int8_t rssi;
	uint8_t status;
} tls_ble_report_rssi_msg_t;
typedef struct
{
	uint8_t address[6];
	int8_t transport;
	uint8_t status;    
} tls_ble_sec_msg_t;

typedef struct
{
    uint16_t num_responses;
} tls_ble_dm_scan_res_cmpl_msg_t;

typedef tls_ble_dm_set_adv_data_cmpl_msg_t tls_ble_dm_adv_cmpl_msg_t;
typedef union
{
    tls_ble_dm_set_adv_data_cmpl_msg_t   dm_set_adv_data_cmpl;
    tls_ble_dm_timer_expired_msg_t       dm_timer_expired;
    tls_ble_dm_evt_triger_msg_t          dm_evt_trigered;
    tls_ble_dm_scan_res_msg_t            dm_scan_result;
	tls_ble_dm_set_scan_param_cmpl_msg_t dm_set_scan_param_cmpl;
    tls_ble_dm_scan_res_cmpl_msg_t       dm_scan_result_cmpl;
	tls_ble_report_rssi_msg_t            dm_report_rssi;
    tls_ble_sec_msg_t                    dm_sec_result;
    tls_ble_dm_adv_cmpl_msg_t            dm_adv_cmpl;
    
} tls_ble_dm_msg_t;

typedef struct
{
    bool set_scan_rsp;
    bool include_name;
    bool include_txpower;
	bool pure_data;
    int min_interval;
    int max_interval;
    int appearance;
    uint16_t manufacturer_len;
    uint8_t manufacturer_data[31];
    uint16_t service_data_len;
    uint8_t service_data[31];
    uint16_t service_uuid_len;
    uint8_t service_uuid[31];
} __attribute__((packed)) tls_ble_dm_adv_data_t;

typedef struct
{
    uint16_t      adv_int_min;            /* minimum adv interval */
    uint16_t      adv_int_max;            /* maximum adv interval */
    tls_bt_addr_t   *dir_addr;
} __attribute__((packed)) tls_ble_dm_adv_param_t;

typedef struct
{
    uint16_t      adv_int_min;            /* minimum adv interval */
    uint16_t      adv_int_max;            /* maximum adv interval */
    uint8_t       adv_type;
    uint8_t       own_addr_type;
    uint8_t       chnl_map;
    uint8_t         afp;
    uint8_t         peer_addr_type;
    tls_bt_addr_t   *dir_addr;
} __attribute__((packed)) tls_ble_dm_adv_ext_param_t;


/** WM BLE device manager callback function */
typedef void (*tls_ble_dm_callback_t)(tls_ble_dm_evt_t event, tls_ble_dm_msg_t *p_data);

/** WM BLE dm timer callback function */
typedef void (*tls_ble_dm_timer_callback_t)(uint8_t timer_id);

/** WM BLE device evt triger callback function */
typedef void (*tls_ble_dm_triger_callback_t)(uint32_t evt_id);

typedef void (*tls_ble_scan_res_notify_t)(tls_ble_dm_scan_res_msg_t *msg);

/*********************************************************************************************************/
/* Bluetooth AV connection states */
typedef enum
{
    WM_BTAV_CONNECTION_STATE_DISCONNECTED = 0,
    WM_BTAV_CONNECTION_STATE_CONNECTING,
    WM_BTAV_CONNECTION_STATE_CONNECTED,
    WM_BTAV_CONNECTION_STATE_DISCONNECTING
} tls_btav_connection_state_t;

/* Bluetooth AV datapath states */
typedef enum
{
    WM_BTAV_AUDIO_STATE_REMOTE_SUSPEND = 0,
    WM_BTAV_AUDIO_STATE_STOPPED,
    WM_BTAV_AUDIO_STATE_STARTED,
} tls_btav_audio_state_t;

/** BR-EDR A2DP sink events */
typedef enum
{
    WMBT_A2DP_CONNECTION_STATE_EVT,
	WMBT_A2DP_AUDIO_STATE_EVT,
	WMBT_A2DP_AUDIO_CONFIG_EVT,
	WMBT_A2DP_AUDIO_PAYLOAD_EVT,
} tls_bt_av_evt_t;

typedef struct
{
	tls_btav_connection_state_t stat;
	tls_bt_addr_t *bd_addr;
} tls_bt_av_connection_state_t;

typedef struct
{
	tls_btav_audio_state_t stat;
	tls_bt_addr_t *bd_addr;	
} tls_bt_av_audio_state_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint32_t sample_rate;
	uint8_t channel_count;
} tls_bt_av_audio_config_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t audio_format;
	uint8_t *payload;
	uint16_t payload_length;
} tls_bt_av_audio_payload_t;

typedef union
{
	tls_bt_av_connection_state_t av_connection_state;
	tls_bt_av_audio_state_t av_audio_state;
	tls_bt_av_audio_config_t av_audio_config;
	tls_bt_av_audio_payload_t av_audio_payload;
	
} tls_bt_av_msg_t;

/** WM BT A2DP SINK callback function */
typedef void (*tls_bt_a2dp_sink_callback_t)(tls_bt_av_evt_t event, tls_bt_av_msg_t *p_data);

/**WM BT A2DP SRC callback function */
///////////////TODO////////////////
typedef void (*tls_bt_a2dp_src_callback_t)(tls_bt_av_evt_t event, tls_bt_av_msg_t *p_data);


/** BR-EDR WMBT-RC Controller callback events */

#define WM_BTRC_MAX_ATTR_STR_LEN       255
#define WM_BTRC_UID_SIZE               8
#define WM_BTRC_MAX_APP_SETTINGS       8
#define WM_BTRC_MAX_FOLDER_DEPTH       4
#define WM_BTRC_MAX_APP_ATTR_SIZE      16
#define WM_BTRC_MAX_ELEM_ATTR_SIZE     7

typedef uint8_t tls_btrc_uid_t[WM_BTRC_UID_SIZE];
typedef enum
{
    WM_BTRC_PLAYSTATE_STOPPED = 0x00,    /* Stopped */
    WM_BTRC_PLAYSTATE_PLAYING = 0x01,    /* Playing */
    WM_BTRC_PLAYSTATE_PAUSED = 0x02,    /* Paused  */
    WM_BTRC_PLAYSTATE_FWD_SEEK = 0x03,    /* Fwd Seek*/
    WM_BTRC_PLAYSTATE_REV_SEEK = 0x04,    /* Rev Seek*/
    WM_BTRC_PLAYSTATE_ERROR = 0xFF,    /* Error   */
} tls_btrc_play_status_t;

typedef enum
{
    WM_BTRC_FEAT_NONE = 0x00,    /* AVRCP 1.0 */
    WM_BTRC_FEAT_METADATA = 0x01,    /* AVRCP 1.3 */
    WM_BTRC_FEAT_ABSOLUTE_VOLUME = 0x02,    /* Supports TG role and volume sync */
    WM_BTRC_FEAT_BROWSE = 0x04,    /* AVRCP 1.4 and up, with Browsing support */
} tls_btrc_remote_features_t;
typedef enum
{
    WM_BTRC_NOTIFICATION_TYPE_INTERIM = 0,
    WM_BTRC_NOTIFICATION_TYPE_CHANGED = 1,
} tls_btrc_notification_type_t;

typedef enum
{
    WM_BTRC_PLAYER_ATTR_EQUALIZER = 0x01,
    WM_BTRC_PLAYER_ATTR_REPEAT = 0x02,
    WM_BTRC_PLAYER_ATTR_SHUFFLE = 0x03,
    WM_BTRC_PLAYER_ATTR_SCAN = 0x04,
} tls_btrc_player_attr_t;

typedef enum
{
    WM_BTRC_MEDIA_ATTR_TITLE = 0x01,
    WM_BTRC_MEDIA_ATTR_ARTIST = 0x02,
    WM_BTRC_MEDIA_ATTR_ALBUM = 0x03,
    WM_BTRC_MEDIA_ATTR_TRACK_NUM = 0x04,
    WM_BTRC_MEDIA_ATTR_NUM_TRACKS = 0x05,
    WM_BTRC_MEDIA_ATTR_GENRE = 0x06,
    WM_BTRC_MEDIA_ATTR_PLAYING_TIME = 0x07,
} tls_btrc_media_attr_t;

typedef enum
{
    WM_BTRC_PLAYER_VAL_OFF_REPEAT = 0x01,
    WM_BTRC_PLAYER_VAL_SINGLE_REPEAT = 0x02,
    WM_BTRC_PLAYER_VAL_ALL_REPEAT = 0x03,
    WM_BTRC_PLAYER_VAL_GROUP_REPEAT = 0x04
} tls_btrc_player_repeat_val_t;

typedef enum
{
    WM_BTRC_EVT_PLAY_STATUS_CHANGED = 0x01,
    WM_BTRC_EVT_TRACK_CHANGE = 0x02,
    WM_BTRC_EVT_TRACK_REACHED_END = 0x03,
    WM_BTRC_EVT_TRACK_REACHED_START = 0x04,
    WM_BTRC_EVT_PLAY_POS_CHANGED = 0x05,
    WM_BTRC_EVT_APP_SETTINGS_CHANGED = 0x08,
} tls_btrc_event_id_t;

typedef struct
{
    uint8_t num_attr;
    uint8_t attr_ids[WM_BTRC_MAX_APP_SETTINGS];
    uint8_t attr_values[WM_BTRC_MAX_APP_SETTINGS];
} tls_btrc_player_settings_t;

typedef struct
{
    uint32_t attr_id;
    uint8_t text[WM_BTRC_MAX_ATTR_STR_LEN];
} tls_btrc_element_attr_val_t;

typedef struct
{
    uint8_t attr_id;
    uint8_t num_val;
    uint8_t attr_val[WM_BTRC_MAX_APP_ATTR_SIZE];
} tls_btrc_player_app_attr_t;
typedef struct
{
    uint8_t   val;
    uint16_t  charset_id;
    uint16_t  str_len;
    uint8_t   *p_str;
} tls_btrc_player_app_ext_attr_val_t;

typedef struct
{
    uint8_t   attr_id;
    uint16_t  charset_id;
    uint16_t  str_len;
    uint8_t   *p_str;
    uint8_t   num_val;
    tls_btrc_player_app_ext_attr_val_t ext_attr_val[WM_BTRC_MAX_APP_ATTR_SIZE];
} tls_btrc_player_app_ext_attr_t;

typedef union
{
    tls_btrc_play_status_t play_status;
    tls_btrc_uid_t track; /* queue position in NowPlaying */
    uint32_t song_pos;
    tls_btrc_player_settings_t player_setting;
} tls_btrc_register_notification_t;


typedef enum
{
	WM_BTRC_PASSTHROUGH_RSP_EVT,
	WM_BTRC_GROUPNAVIGATION_RSP_EVT,
	WM_BTRC_CONNECTION_STATE_EVT,
	WM_BTRC_CTRL_GETRCFEATURES_EVT,
	WM_BTRC_CTRL_SETPLAYERAPPLICATIONSETTING_RSP_EVT,
	WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_EVT,
	WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_CHANGED_EVT,
	WM_BTRC_CTRL_SETABSVOL_CMD_EVT,
	WM_BTRC_CTRL_REGISTERNOTIFICATION_ABS_VOL_EVT,
	WM_BTRC_CTRL_TRACK_CHANGED_EVT,
	WM_BTRC_CTRL_PLAY_POSITION_CHANGED_EVT,
	WM_BTRC_CTRL_PLAY_STATUS_CHANGED_EVT,
} tls_btrc_ctrl_evt_t;

typedef struct
{
	int id;
	int key_state;
} tls_btrc_passthrough_rsp_msg_t;

typedef struct
{
	int id;
	int key_state;
} tls_btrc_groupnavigation_rsp_msg_t;

typedef struct
{
	uint8_t state;
	tls_bt_addr_t *bd_addr;
} tls_btrc_connection_state_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	int features;
} tls_btrc_ctrl_getrcfeatures_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t abs_vol;
	uint8_t label;
} tls_btrc_ctrl_setabsvol_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t label;
} tls_btrc_ctrl_registernotification_abs_vol_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t accepted;
} tls_btrc_ctrl_setplayerapplicationsetting_rsp_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t num_attr;
	tls_btrc_player_app_attr_t *app_attrs;
	uint8_t num_ext_attr;
	tls_btrc_player_app_ext_attr_t *ext_attrs;
} tls_btrc_ctrl_playerapplicationsetting_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
    tls_btrc_player_settings_t *p_vals;	
} tls_btrc_ctrl_playerapplicationsetting_changed_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t num_attr;
	tls_btrc_element_attr_val_t *p_attrs;

} tls_btrc_ctrl_track_changed_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint32_t song_len;
	uint32_t song_pos;
} tls_btrc_ctrl_play_position_changed_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	tls_btrc_play_status_t play_status;
} tls_btrc_ctrl_play_status_changed_msg_t;

typedef union
{
	tls_btrc_passthrough_rsp_msg_t				passthrough_rsp;
	tls_btrc_groupnavigation_rsp_msg_t			groupnavigation_rsp;
	tls_btrc_connection_state_msg_t			connection_state;
	tls_btrc_ctrl_getrcfeatures_msg_t			getrcfeatures;
	tls_btrc_ctrl_setabsvol_msg_t				setabsvol;
	tls_btrc_ctrl_registernotification_abs_vol_msg_t 		registernotification_abs_vol;
	tls_btrc_ctrl_setplayerapplicationsetting_rsp_msg_t	setplayerapplicationsetting_rsp;
	tls_btrc_ctrl_playerapplicationsetting_msg_t			playerapplicationsetting;
	tls_btrc_ctrl_playerapplicationsetting_changed_msg_t 	playerapplicationsetting_changed;
	tls_btrc_ctrl_track_changed_msg_t 						track_changed;
	tls_btrc_ctrl_play_position_changed_msg_t	play_position_changed;
	tls_btrc_ctrl_play_status_changed_msg_t	play_status_changed;
	
} tls_btrc_ctrl_msg_t;

/** WM BT RC CTRL callback function */
typedef void (*tls_btrc_ctrl_callback_t)(tls_btrc_ctrl_evt_t event, tls_btrc_ctrl_msg_t *p_data);


typedef enum
{
	WM_BTRC_REMOTE_FEATURE_EVT,
	WM_BTRC_GET_PLAY_STATUS_EVT,
	WM_BTRC_LIST_PLAYER_APP_ATTR_EVT,
	WM_BTRC_LIST_PLAYER_APP_VALUES_EVT,
	WM_BTRC_GET_PLAYER_APP_VALUE_EVT,
	WM_BTRC_GET_PLAYER_APP_ATTRS_TEXT_EVT,
	WM_BTRC_GET_PLAYER_APP_VALUES_TEXT_EVT,
	WM_BTRC_SET_PLAYER_APP_VALUE_EVT,
	WM_BTRC_GET_ELEMENT_ATTR_EVT,
	WM_BTRC_REGISTER_NOTIFICATION_EVT,
	WM_BTRC_VOLUME_CHANGED_EVT,
	WM_BTRC_PASSTHROUGH_CMD_EVT,
} tls_btrc_evt_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
    tls_btrc_remote_features_t features;	
} tls_btrc_remote_features_msg_t;

typedef struct
{
	void *reserved;
} tls_btrc_get_play_status_msg_t;

typedef struct
{
	void *reserved;
} tls_btrc_list_player_app_attr_msg_t;

typedef struct
{
	tls_btrc_player_attr_t attr_id;
} tls_btrc_list_player_app_values_msg_t;

typedef struct
{
	uint8_t num_attr;
	tls_btrc_player_attr_t *p_attrs;

} tls_btrc_get_player_app_value_msg_t;

typedef struct
{
	uint8_t attr_id;
	uint8_t num_val;
	uint8_t *p_vals;
} tls_btrc_get_player_app_attrs_text_msg_t;

typedef struct
{
	uint8_t num_attr;
	tls_btrc_player_attr_t *p_attrs;

} tls_btrc_get_player_app_values_text_msg_t;

typedef struct
{
	tls_btrc_player_settings_t *p_vals;
		
} tls_btrc_set_player_app_value_msg_t;

typedef struct
{
	uint8_t num_attr;
	tls_btrc_media_attr_t *p_attrs;
		
} tls_btrc_get_element_attr_msg_t;

typedef struct
{
	tls_btrc_event_id_t event_id; 
	uint32_t param;
	
} tls_btrc_register_notification_msg_t;

typedef struct
{
	uint8_t volume; 
	uint8_t ctype;
} tls_btrc_volume_change_msg_t;

typedef struct
{
	int id;
	int key_state;

} tls_btrc_passthrough_cmd_msg_t;

typedef union
{
	tls_btrc_remote_features_msg_t		remote_features;
	tls_btrc_get_play_status_msg_t  	get_play_status;
	tls_btrc_list_player_app_attr_msg_t		list_player_app_attr;
	tls_btrc_list_player_app_values_msg_t 	list_player_app_values;
	tls_btrc_get_player_app_value_msg_t   	get_player_app_value;
	tls_btrc_get_player_app_attrs_text_msg_t 	get_player_app_attrs_text;
	tls_btrc_get_player_app_values_text_msg_t 	get_player_app_values_text;
	tls_btrc_set_player_app_value_msg_t 	set_player_app_value;
	tls_btrc_get_element_attr_msg_t 		get_element_attr;
	tls_btrc_register_notification_msg_t 	register_notification;
	tls_btrc_volume_change_msg_t 		volume_change;
	tls_btrc_passthrough_cmd_msg_t 		passthrough_cmd;
	
} tls_btrc_msg_t;

/** WM BT RC callback function */
typedef void (*tls_btrc_callback_t)(tls_btrc_evt_t event, tls_btrc_msg_t *p_data);


/*************************************************************************************************************/

typedef enum
{
    WM_BTHF_CLIENT_CONNECTION_STATE_DISCONNECTED = 0,
    WM_BTHF_CLIENT_CONNECTION_STATE_CONNECTING,
    WM_BTHF_CLIENT_CONNECTION_STATE_CONNECTED,
    WM_BTHF_CLIENT_CONNECTION_STATE_SLC_CONNECTED,
    WM_BTHF_CLIENT_CONNECTION_STATE_DISCONNECTING
} tls_bthf_client_connection_state_t;

typedef enum
{
    WM_BTHF_CLIENT_AUDIO_STATE_DISCONNECTED = 0,
    WM_BTHF_CLIENT_AUDIO_STATE_CONNECTING,
    WM_BTHF_CLIENT_AUDIO_STATE_CONNECTED,
    WM_BTHF_CLIENT_AUDIO_STATE_CONNECTED_MSBC,
} tls_bthf_client_audio_state_t;

typedef enum
{
    WM_BTHF_CLIENT_VR_STATE_STOPPED = 0,
    WM_BTHF_CLIENT_VR_STATE_STARTED
} tls_bthf_client_vr_state_t;

typedef enum
{
    WM_BTHF_CLIENT_VOLUME_TYPE_SPK = 0,
    WM_BTHF_CLIENT_VOLUME_TYPE_MIC
} tls_bthf_client_volume_type_t;

typedef enum
{
    WM_BTHF_CLIENT_NETWORK_STATE_NOT_AVAILABLE = 0,
    WM_BTHF_CLIENT_NETWORK_STATE_AVAILABLE
} tls_bthf_client_network_state_t;

typedef enum
{
    WM_BTHF_CLIENT_SERVICE_TYPE_HOME = 0,
    WM_BTHF_CLIENT_SERVICE_TYPE_ROAMING
} tls_bthf_client_service_type_t;

typedef enum
{
    WM_BTHF_CLIENT_CALL_STATE_ACTIVE = 0,
    WM_BTHF_CLIENT_CALL_STATE_HELD,
    WM_BTHF_CLIENT_CALL_STATE_DIALING,
    WM_BTHF_CLIENT_CALL_STATE_ALERTING,
    WM_BTHF_CLIENT_CALL_STATE_INCOMING,
    WM_BTHF_CLIENT_CALL_STATE_WAITING,
    WM_BTHF_CLIENT_CALL_STATE_HELD_BY_RESP_HOLD,
} tls_bthf_client_call_state_t;

typedef enum
{
    WM_BTHF_CLIENT_CALL_NO_CALLS_IN_PROGRESS = 0,
    WM_BTHF_CLIENT_CALL_CALLS_IN_PROGRESS
} tls_bthf_client_call_t;

typedef enum
{
    WM_BTHF_CLIENT_CALLSETUP_NONE = 0,
    WM_BTHF_CLIENT_CALLSETUP_INCOMING,
    WM_BTHF_CLIENT_CALLSETUP_OUTGOING,
    WM_BTHF_CLIENT_CALLSETUP_ALERTING

} tls_bthf_client_callsetup_t;

typedef enum
{
    WM_BTHF_CLIENT_CALLHELD_NONE = 0,
    WM_BTHF_CLIENT_CALLHELD_HOLD_AND_ACTIVE,
    WM_BTHF_CLIENT_CALLHELD_HOLD,
} tls_bthf_client_callheld_t;

typedef enum
{
    WM_BTHF_CLIENT_RESP_AND_HOLD_HELD = 0,
    WM_BTRH_CLIENT_RESP_AND_HOLD_ACCEPT,
    WM_BTRH_CLIENT_RESP_AND_HOLD_REJECT,
} tls_bthf_client_resp_and_hold_t;

typedef enum
{
    WM_BTHF_CLIENT_CALL_DIRECTION_OUTGOING = 0,
    WM_BTHF_CLIENT_CALL_DIRECTION_INCOMING
} tls_bthf_client_call_direction_t;

typedef enum
{
    WM_BTHF_CLIENT_CALL_MPTY_TYPE_SINGLE = 0,
    WM_BTHF_CLIENT_CALL_MPTY_TYPE_MULTI
} tls_bthf_client_call_mpty_type_t;

typedef enum
{
    WM_BTHF_CLIENT_CMD_COMPLETE_OK = 0,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR_NO_CARRIER,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR_BUSY,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR_NO_ANSWER,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR_DELAYED,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR_BLACKLISTED,
    WM_BTHF_CLIENT_CMD_COMPLETE_ERROR_CME
} tls_bthf_client_cmd_complete_t;

typedef enum
{
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_0 = 0,
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_1,
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_2,
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_3,
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_4,
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_1x,
    WM_BTHF_CLIENT_CALL_ACTION_CHLD_2x,
    WM_BTHF_CLIENT_CALL_ACTION_ATA,
    WM_BTHF_CLIENT_CALL_ACTION_CHUP,
    WM_BTHF_CLIENT_CALL_ACTION_BTRH_0,
    WM_BTHF_CLIENT_CALL_ACTION_BTRH_1,
    WM_BTHF_CLIENT_CALL_ACTION_BTRH_2,
} tls_bthf_client_call_action_t;

typedef enum
{
    WM_BTHF_CLIENT_SERVICE_UNKNOWN = 0,
    WM_BTHF_CLIENT_SERVICE_VOICE,
    WM_BTHF_CLIENT_SERVICE_FAX
} tls_bthf_client_subscriber_service_type_t;

typedef enum
{
    WM_BTHF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    WM_BTHF_CLIENT_IN_BAND_RINGTONE_PROVIDED,
} tls_bthf_client_in_band_ring_state_t;

/* Peer features masks */
#define WM_BTHF_CLIENT_PEER_FEAT_3WAY   0x00000001  /* Three-way calling */
#define WM_BTHF_CLIENT_PEER_FEAT_ECNR   0x00000002  /* Echo cancellation and/or noise reduction */
#define WM_BTHF_CLIENT_PEER_FEAT_VREC   0x00000004  /* Voice recognition */
#define WM_BTHF_CLIENT_PEER_FEAT_INBAND 0x00000008  /* In-band ring tone */
#define WM_BTHF_CLIENT_PEER_FEAT_VTAG   0x00000010  /* Attach a phone number to a voice tag */
#define WM_BTHF_CLIENT_PEER_FEAT_REJECT 0x00000020  /* Ability to reject incoming call */
#define WM_BTHF_CLIENT_PEER_FEAT_ECS    0x00000040  /* Enhanced Call Status */
#define WM_BTHF_CLIENT_PEER_FEAT_ECC    0x00000080  /* Enhanced Call Control */
#define WM_BTHF_CLIENT_PEER_FEAT_EXTERR 0x00000100  /* Extended error codes */
#define WM_BTHF_CLIENT_PEER_FEAT_CODEC  0x00000200  /* Codec Negotiation */

/* Peer call handling features masks */
#define WM_BTHF_CLIENT_CHLD_FEAT_REL           0x00000001  /* 0  Release waiting call or held calls */
#define WM_BTHF_CLIENT_CHLD_FEAT_REL_ACC       0x00000002  /* 1  Release active calls and accept other
                                                              (waiting or held) cal */
#define WM_BTHF_CLIENT_CHLD_FEAT_REL_X         0x00000004  /* 1x Release specified active call only */
#define WM_BTHF_CLIENT_CHLD_FEAT_HOLD_ACC      0x00000008  /* 2  Active calls on hold and accept other
                                                              (waiting or held) call */
#define WM_BTHF_CLIENT_CHLD_FEAT_PRIV_X        0x00000010  /* 2x Request private mode with specified
                                                              call (put the rest on hold) */
#define WM_BTHF_CLIENT_CHLD_FEAT_MERGE         0x00000020  /* 3  Add held call to multiparty */
#define WM_BTHF_CLIENT_CHLD_FEAT_MERGE_DETACH  0x00000040  /* 4  Connect two calls and leave
                                                              (disconnect from) multiparty */


typedef enum
{
	WM_BTHF_CLIENT_CONNECTION_STATE_EVT,
	WM_BTHF_CLIENT_AUDIO_STATE_EVT,
	WM_BTHF_CLIENT_VR_CMD_EVT,
	WM_BTHF_CLIENT_NETWORK_STATE_EVT,
	WM_BTHF_CLIENT_NETWORK_ROAMING_EVT,
	WM_BTHF_CLIENT_NETWORK_SIGNAL_EVT,
	WM_BTHF_CLIENT_BATTERY_LEVEL_EVT,
	WM_BTHF_CLIENT_CURRENT_OPERATOR_EVT,
	WM_BTHF_CLIENT_CALL_EVT,
	WM_BTHF_CLIENT_CALLSETUP_EVT,
	WM_BTHF_CLIENT_CALLHELD_EVT,
	WM_BTHF_CLIENT_RESP_AND_HOLD_EVT,
	WM_BTHF_CLIENT_CLIP_EVT,
	WM_BTHF_CLIENT_CALL_WAITING_EVT,
	WM_BTHF_CLIENT_CURRENT_CALLS_EVT,
	WM_BTHF_CLIENT_VOLUME_CHANGE_EVT,
	WM_BTHF_CLIENT_CMD_COMPLETE_EVT,
	WM_BTHF_CLIENT_SUBSCRIBER_INFO_EVT,
	WM_BTHF_CLIENT_IN_BAND_RING_TONE_EVT,
	WM_BTHF_CLIENT_LAST_VOICE_TAG_NUMBER_EVT,
	WM_BTHF_CLIENT_RING_INDICATION_EVT,
	WM_BTHF_CLIENT_AUDIO_PAYLOAD_EVT,
} tls_bthf_client_evt_t;

typedef struct
{
	tls_bthf_client_connection_state_t state;
	unsigned int peer_feat;
	unsigned int chld_feat;
	tls_bt_addr_t *bd_addr;
} tls_bthf_client_connection_state_msg_t;

typedef struct
{
	tls_bthf_client_audio_state_t state;
	tls_bt_addr_t *bd_addr;	
} tls_bthf_client_audio_state_msg_t;

typedef struct
{
	tls_bthf_client_vr_state_t state;

} tls_bthf_client_vr_cmd_msg_t;

typedef struct
{
	tls_bthf_client_network_state_t state;

} tls_bthf_client_network_state_msg_t;

typedef struct
{
	tls_bthf_client_service_type_t type;

} tls_bthf_client_network_roaming_msg_t;

typedef struct
{
	int signal_strength;

} tls_bthf_client_network_signal_msg_t;

typedef struct
{
	int battery_level;

} tls_bthf_client_battery_level_msg_t;

typedef struct
{
	char* name;

} tls_bthf_client_current_operator_msg_t;

typedef struct
{
	tls_bthf_client_call_t call;

} tls_bthf_client_call_msg_t;

typedef struct
{
	tls_bthf_client_callsetup_t callsetup;

} tls_bthf_client_callsetup_msg_t;

typedef struct
{
	tls_bthf_client_callheld_t callheld;

} tls_bthf_client_callheld_msg_t;

typedef struct
{
	tls_bthf_client_resp_and_hold_t resp_and_hold;

} tls_bthf_client_resp_and_hold_msg_t;

typedef struct
{
	char *number;

} tls_bthf_client_clip_msg_t;

typedef struct
{
	char *number;

} tls_bthf_client_call_waiting_msg_t;

typedef struct
{
	int index;
	tls_bthf_client_call_direction_t dir;
	tls_bthf_client_call_state_t state;
	tls_bthf_client_call_mpty_type_t mpty;
	char *number;
} tls_bthf_client_current_calls_msg_t;

typedef struct
{
	tls_bthf_client_volume_type_t type;
	int volume;

} tls_bthf_client_volume_change_msg_t;

typedef struct
{
	tls_bthf_client_cmd_complete_t type;
	int cme;

} tls_bthf_client_cmd_complete_msg_t;

typedef struct
{
	const char *name;
	tls_bthf_client_subscriber_service_type_t type;

} tls_bthf_client_subscriber_info_msg_t;

typedef struct
{
	tls_bthf_client_in_band_ring_state_t state;

} tls_bthf_client_in_band_ring_tone_msg_t;

typedef struct
{
	char *number;

} tls_bthf_client_last_voice_tag_number_msg_t;

typedef struct
{
	int ring;

} tls_bthf_client_ring_indication_msg_t;

typedef struct
{
	tls_bt_addr_t *bd_addr;
	uint8_t audio_format;
	uint8_t *payload;
	uint16_t payload_length;
} tls_bthf_audio_payload_msg_t;


typedef union
{
	tls_bthf_client_connection_state_msg_t	connection_state_msg;
	tls_bthf_client_audio_state_msg_t		audio_state_msg;
	tls_bthf_client_vr_cmd_msg_t			vr_cmd_msg;
	tls_bthf_client_network_state_msg_t	 	network_state_msg;
	tls_bthf_client_network_roaming_msg_t	network_roaming_msg;
	tls_bthf_client_network_signal_msg_t    network_signal_msg;
	tls_bthf_client_battery_level_msg_t	    battery_level_msg;
	tls_bthf_client_current_operator_msg_t	current_operator_msg;
	tls_bthf_client_call_msg_t 				call_msg;
	tls_bthf_client_callsetup_msg_t 		callsetup_msg;
	tls_bthf_client_callheld_msg_t 			callheld_msg;
	tls_bthf_client_resp_and_hold_msg_t 	resp_and_hold_msg;
	tls_bthf_client_clip_msg_t 				clip_msg;
	tls_bthf_client_call_waiting_msg_t 		call_waiting_msg;
	tls_bthf_client_current_calls_msg_t 	current_calls_msg;
	tls_bthf_client_volume_change_msg_t 	volume_change_msg;
	tls_bthf_client_cmd_complete_msg_t 		cmd_complete_msg;
	tls_bthf_client_subscriber_info_msg_t 	subscriber_info_msg;
	tls_bthf_client_in_band_ring_tone_msg_t in_band_ring_tone_msg;
	tls_bthf_client_last_voice_tag_number_msg_t last_voice_tag_number_msg;
	tls_bthf_client_ring_indication_msg_t 	ring_indication_msg;
	tls_bthf_audio_payload_msg_t            audio_payload_msg;

} tls_bthf_client_msg_t;

/** WM BT HFP CLIENT callback function */
typedef void (*tls_bthf_client_callback_t)(tls_bthf_client_evt_t event, tls_bthf_client_msg_t *p_data);


/******************************************************************************************/
/* Security Setting Mask */
#define WM_SPP_SEC_NONE            0x0000    /* No security*/
#define WM_SPP_SEC_AUTHORIZE       0x0001    /*Authorization required (only needed for out going connection ) */
#define WM_SPP_SEC_AUTHENTICATE    0x0012    /*Authentication required*/
#define WM_SPP_SEC_ENCRYPT         0x0024    /*Encryption required*/
#define WM_SPP_SEC_MODE4_LEVEL4    0x0040    /*Mode 4 level 4 service, i.e. incoming/outgoing MITM and P-256 encryption*/
#define WM_SPP_SEC_MITM            0x3000    /*Man-In-The_Middle protection*/
#define WM_SPP_SEC_IN_16_DIGITS    0x4000    /*Min 16 digit for pin code*/
typedef uint16_t wm_spp_sec_t;

#define WM_SPP_MAX_SCN             31

typedef enum {
    WM_SPP_ROLE_CLIENT     = 0,          
    WM_SPP_ROLE_SERVER      = 1,
} tls_spp_role_t;

typedef enum {
    WM_SPP_INIT_EVT                    = 0,               
    WM_SPP_DISCOVERY_COMP_EVT          = 8,               
    WM_SPP_OPEN_EVT                    = 26,              
    WM_SPP_CLOSE_EVT                   = 27,              
    WM_SPP_START_EVT                   = 28,               
    WM_SPP_CL_INIT_EVT                 = 29,              
    WM_SPP_DATA_IND_EVT                = 30,              
    WM_SPP_CONG_EVT                    = 31,               
    WM_SPP_WRITE_EVT                   = 33,              
    WM_SPP_SRV_OPEN_EVT                = 34,              
} tls_spp_event_t;

typedef struct {
    uint8_t    status;         
} tls_spp_init_msg_t ;

typedef struct {
    uint8_t    status;        
    uint8_t    scn_num;       
    uint8_t    scn[WM_SPP_MAX_SCN];   
} tls_spp_disc_comp_msg_t;

typedef struct {
        uint8_t      status;        
        uint32_t     handle;                   
        uint8_t     addr[6];        
} tls_spp_open_msg_t;

typedef struct {
        uint8_t    status;        
        uint32_t   handle;        
        uint32_t   new_listen_handle;         
        uint8_t     addr[6];       
} tls_spp_srv_open_msg_t;

typedef struct {
        uint8_t    status;         
        uint32_t   port_status;    
        uint32_t   handle;        
        bool       local;         
} tls_spp_close_msg_t; 

typedef struct {
        uint8_t    status;        
        uint32_t   handle;         
        uint8_t    sec_id;         
        bool       use_co_rfc;        
} tls_spp_start_msg_t; 

typedef struct {
        uint8_t    status;         
        uint32_t   handle;         
        uint8_t    sec_id;        
        bool       use_co_rfc;         
} tls_spp_cli_init_msg_t;

typedef struct {
        uint8_t    status;         
        uint32_t   handle;         
        int        length;            
        bool       congest;          
} tls_spp_write_msg_t;

typedef struct {
        uint8_t    status;        
        uint32_t   handle;        
        uint16_t   length;           
        uint8_t    *data;         
} tls_spp_data_ind_msg_t; 

typedef struct {
        uint8_t    status;        
        uint32_t   handle;        
        bool       congest;           
} tls_spp_cong_msg_t; 

typedef union
{
	tls_spp_init_msg_t	            init_msg;
	tls_spp_disc_comp_msg_t		    disc_comp_msg;
	tls_spp_open_msg_t			    open_msg;
	tls_spp_srv_open_msg_t	 	    srv_open_msg;
	tls_spp_close_msg_t	            close_msg;
	tls_spp_start_msg_t             start_msg;
	tls_spp_cli_init_msg_t	        cli_init_msg;
	tls_spp_write_msg_t	            write_msg;
	tls_spp_data_ind_msg_t 			data_ind_msg;
	tls_spp_cong_msg_t 		        congest_msg;

} tls_spp_msg_t;

/** WM BT SPP callback function */
typedef void (*tls_bt_spp_callback_t)(tls_spp_event_t event, tls_spp_msg_t *p_data);
typedef enum
{
    BLE_UART_SERVER_MODE,
    BLE_UART_CLIENT_MODE,
    BLE_UART_UNKNOWN_MODE,
} tls_ble_uart_mode_t;

typedef enum{
    UART_OUTPUT_DATA=0,
    UART_OUTPUT_CMD_ADVERTISING,
    UART_OUTPUT_CMD_CONNECTED,
    UART_OUTPUT_CMD_DISCONNECTED,
} tls_uart_msg_out_t;

/**uart output function pointer, ble server send the received data to uart */
typedef void (*tls_ble_uart_output_ptr)(tls_uart_msg_out_t type,uint8_t *payload, int length);

/**uart sent function pointer, after ble server sending the uart data, it will be called */
typedef void (*tls_ble_uart_sent_ptr)(tls_ble_uart_mode_t mode, int status);


/**WM Mesh definition*/

typedef enum{
    MESH_ROLE_UNKNOWN = 0x00,
    MESH_ROLE_NODE,
    MESH_ROLE_PROVISIONER
} tls_bt_mesh_role_t;

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define BIT_MASK(n) (BIT(n) - 1)

#define TLS_BT_MESH_TRANSMIT_INT(transmit) ((((transmit) >> 3) + 1) * 10)

#define TLS_BT_MESH_TRANSMIT_COUNT(transmit) (((transmit) & (uint8_t)BIT_MASK(3)))

#define TLS_BT_MESH_TRANSMIT(count, int_ms) ((count) | (((int_ms / 10) - 1) << 3))

#define TLS_BT_MESH_PUB_TRANSMIT(count, int_ms) TLS_BT_MESH_TRANSMIT(count,           \
                                 (int_ms) / 5)

#define TLS_BT_MESH_PUB_TRANSMIT_INT(transmit) ((((transmit) >> 3) + 1) * 50)
#define TLS_BT_MESH_PUB_TRANSMIT_COUNT(transmit) TLS_BT_MESH_TRANSMIT_COUNT(transmit)

typedef struct {
    uint8_t net_transmit_count;         /* Network Transmit state */
    uint8_t net_transmit_intvl;         /* Network Transmit state */
    uint8_t relay;                /* Relay Mode state */
    uint8_t relay_retransmit_count;     /* Relay Retransmit state */
    uint8_t relay_retransmit_intvl;     /* Relay Retransmit state */
    uint8_t beacon;               /* Secure Network Beacon state */
    uint8_t gatt_proxy;           /* GATT Proxy state */
    uint8_t frnd;                 /* Friend state */
    uint8_t default_ttl;          /* Default TTL */
} tls_mesh_primary_cfg_t;

typedef struct {
    uint16_t  addr;
    uint16_t  app_idx;
    uint8_t   cred_flag;
    uint8_t   ttl;
    uint8_t   period;
    uint8_t   transmit;
} tls_bt_mesh_cfg_mod_pub ;

typedef struct {
    uint16_t dst;
    uint8_t  count;
    uint8_t  period;
    uint8_t  ttl;
    uint16_t feat;
    uint16_t net_idx;
} tls_bt_mesh_cfg_hb_pub;

typedef struct  {
    uint16_t src;
    uint16_t dst;
    uint8_t  period;
    uint8_t  count;
    uint8_t  min;
    uint8_t  max;
} tls_bt_mesh_cfg_hb_sub;

typedef struct{
    uint8_t addr[6];
    uint8_t addr_type;
    uint8_t uuid[16];
    uint32_t oob_info;
    uint32_t uri_hash;
    
} tls_mesh_unprov_msg_t;

typedef struct{
    uint16_t net_idx;
    uint16_t addr;
    uint8_t num_elem;

} tls_mesh_node_added_msg_t;

typedef struct{
    uint16_t net_idx;
    uint16_t addr;

} tls_mesh_prov_complete_msg_t;

typedef struct{
    char *str;
} tls_mesh_oob_output_str_msg_t;

typedef struct{
    uint32_t number;
} tls_mesh_oob_output_number_msg_t;

typedef struct{
    uint32_t act;
} tls_mesh_oob_input_msg_t;

typedef struct{
    bool success;
    uint16_t net_idx;
    uint16_t addr;
    uint8_t num_elem;    
    
} tls_mesh_prov_end_msg_t;

typedef union
{
    tls_mesh_unprov_msg_t                  unprov_msg;
    tls_mesh_node_added_msg_t              node_added_msg;
    tls_mesh_oob_output_str_msg_t          oob_output_string_msg;
    tls_mesh_oob_output_number_msg_t       oob_output_number_msg;
    tls_mesh_prov_complete_msg_t           prov_cmplt_msg;
    tls_mesh_oob_input_msg_t               oob_input_msg;
    tls_mesh_prov_end_msg_t                prov_end_msg;
    
} tls_mesh_msg_t;


typedef enum{
    WM_MESH_UNPROVISION_BEACON_EVT = (0x01<<1),
    WM_MESH_SECURE_BEACON_EVT      = (0x01<<2),
    WM_MESH_NODE_ADDED_EVT         = (0x01<<3),
    WM_MESH_OOB_STRING_EVT         = (0x01<<4),
    WM_MESH_OOB_NUMBER_EVT         = (0x01<<5),    
    WM_MESH_PROV_CMPLT_EVT         = (0x01<<6),
    WM_MESH_OOB_INPUT_EVT          = (0x01<<7),
    WM_MESH_PROV_END_EVT           = (0x01<<8),
    
} tls_mesh_event_t;

typedef void (*tls_bt_mesh_at_callback_t)(tls_mesh_event_t event, tls_mesh_msg_t *p_data);

typedef void (*tls_bt_controller_sleep_enter_func_ptr)(uint32_t sleep_duration_ms);

typedef void (*tls_bt_controller_sleep_exit_func_ptr)(void);

typedef void (*tls_bt_app_pending_process_func_ptr)(void);

#define TLS_HAL_AT_NOTIFY(P_CB, PARAM1, PARAM2)\
    if (P_CB) {                             \
        P_CB(PARAM1, PARAM2);               \
    } 



#define TLS_HAL_CBACK(P_CB, P_CBACK, ...)\
    if (P_CB && P_CB->P_CBACK) {            \
        P_CB->P_CBACK(__VA_ARGS__);         \
    }                                       \
    else {                                  \
        assert(0);  \
    }

#endif /* WM_BT_DEF_H */


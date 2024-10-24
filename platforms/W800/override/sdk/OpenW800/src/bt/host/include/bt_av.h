/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_INCLUDE_BT_AV_H
#define ANDROID_INCLUDE_BT_AV_H



/* Bluetooth AV connection states */
typedef enum {
    BTAV_CONNECTION_STATE_DISCONNECTED = 0,
    BTAV_CONNECTION_STATE_CONNECTING,
    BTAV_CONNECTION_STATE_CONNECTED,
    BTAV_CONNECTION_STATE_DISCONNECTING
} btav_connection_state_t;

/* Bluetooth AV datapath states */
typedef enum {
    BTAV_AUDIO_STATE_REMOTE_SUSPEND = 0,
    BTAV_AUDIO_STATE_STOPPED,
    BTAV_AUDIO_STATE_STARTED,
} btav_audio_state_t;


/** Callback for connection state change.
 *  state will have one of the values from btav_connection_state_t
 */
typedef void (* btav_connection_state_callback)(btav_connection_state_t state,
        tls_bt_addr_t *bd_addr);

/** Callback for audiopath state change.
 *  state will have one of the values from btav_audio_state_t
 */
typedef void (* btav_audio_state_callback)(btav_audio_state_t state,
        tls_bt_addr_t *bd_addr);

/** Callback for audio configuration change.
 *  Used only for the A2DP sink interface.
 *  state will have one of the values from btav_audio_state_t
 *  sample_rate: sample rate in Hz
 *  channel_count: number of channels (1 for mono, 2 for stereo)
 */
typedef void (* btav_audio_config_callback)(tls_bt_addr_t *bd_addr,
        uint32_t sample_rate,
        uint8_t channel_count);

/** BT-AV callback structure. */
typedef struct {
    /** set to sizeof(btav_callbacks_t) */
    size_t      size;
    btav_connection_state_callback  connection_state_cb;
    btav_audio_state_callback audio_state_cb;
    btav_audio_config_callback audio_config_cb;
} btav_callbacks_t;

/**
 * NOTE:
 *
 * 1. AVRCP 1.0 shall be supported initially. AVRCP passthrough commands
 *    shall be handled internally via uinput
 *
 * 2. A2DP data path shall be handled via a socket pipe between the AudioFlinger
 *    android_audio_hw library and the Bluetooth stack.
 *
 */
/** Represents the standard BT-AV interface.
 *  Used for both the A2DP source and sink interfaces.
 */
typedef struct {

    /** set to sizeof(btav_interface_t) */
    size_t          size;
    /**
     * Register the BtAv callbacks
     */
    tls_bt_status_t (*init)(btav_callbacks_t *callbacks);

    /** connect to headset */
    tls_bt_status_t (*connect)(tls_bt_addr_t *bd_addr);

    /** dis-connect from headset */
    tls_bt_status_t (*disconnect)(tls_bt_addr_t *bd_addr);

    /** Closes the interface. */
    void (*cleanup)(void);

    /** Sends Audio Focus State. */
    void (*set_audio_focus_state)(int focus_state);

    /** Sets the audio track gain. */
    void (*set_audio_track_gain)(float gain);
} btav_interface_t;



#endif /* ANDROID_INCLUDE_BT_AV_H */

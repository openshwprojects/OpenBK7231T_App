/*
 * Copyright (C) 2015 The Android Open Source Project
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

#if defined (__cplusplus) || (cplusplus)
extern "C" {
#endif

/**
 * Implements an API to be used by A2DP to do streaming of music over a media
 * stream. This API provides mechanism to create and control playback of the
 * media stream depending on the data bits coming from the remote device. The
 * media stream is played over the default audio media stream type and hence
 * volume control is handled entirely by the Audio Manager (which is also the
 * original motivation for this solution.
 *
 * TODO: Once the AudioManager provides support for patching audio sources with
 * volume control we should deprecate this file.
 */

/**
 * Creates an audio track object and returns a void handle. Use this handle to the
 * following functions.
 *
 * The ownership of the handle is maintained by the caller of this API and it should eventually be
 * deleted using BtifAvrcpAudioTrackDelete (see below).
 */
void *BtifAvrcpAudioTrackCreate(int trackFreq, int channelType);

/**
 * Starts the audio track.
 */
void BtifAvrcpAudioTrackStart(void *handle);

/**
 * Pauses the audio track.
 */
void BtifAvrcpAudioTrackPause(void *handle);

/**
 * Sets audio track gain.
 */
void BtifAvrcpSetAudioTrackGain(void *handle, float gain);

/**
 * Stop / Delete the audio track.
 * Delete should usually be called stop.
 */
void BtifAvrcpAudioTrackStop(void *handle);
void BtifAvrcpAudioTrackDelete(void *handle);

/**
 * Writes the audio track data to file.
 *
 * Used only for debugging.
 */
int BtifAvrcpAudioTrackWriteData(void *handle, void *audioBuffer, int bufferlen);

#if defined (__cplusplus) || (cplusplus)
}
#endif

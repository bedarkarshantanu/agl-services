/*
 * Copyright (C) 2017 "Audiokinetic Inc"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AHL_INTERFACE_INCLUDE
#define AHL_INTERFACE_INCLUDE

///// API /////

// Endpoint types
#define AHL_ENDPOINTTYPE_SOURCE "source" // source devices
#define AHL_ENDPOINTTYPE_SINK "sink" // sink devices

// Stream state
#define AHL_STREAM_STATE_IDLE  "idle"       // Stream is inactive
#define AHL_STREAM_STATE_RUNNING "running"  // Stream is active and running
#define AHL_STREAM_STATE_PAUSED "paused"    // Stream is active but paused

// Stream mute state
#define AHL_STREAM_UNMUTED "off"            // Stream is not muted
#define AHL_STREAM_MUTED "on"               // Stream is muted

// Property/Volume/Action events
#define AHL_ENDPOINT_PROPERTY_EVENT "ahl_endpoint_property_event"
#define AHL_ENDPOINT_VOLUME_EVENT "ahl_endpoint_volume_event"
#define AHL_POST_ACTION_EVENT "ahl_post_action"
#define AHL_STREAM_STATE_EVENT "ahl_stream_state_event"

// Stream state event types
#define AHL_STREAM_EVENT_START "start"    // Stream is inactive
#define AHL_STREAM_EVENT_STOP "stop"      // Stream is running
#define AHL_STREAM_EVENT_PAUSE "pause"    // Audio stream paused
#define AHL_STREAM_EVENT_RESUME "resume"  // Audio stream resumed
#define AHL_STREAM_EVENT_MUTE "mute"      // Audio stream muted
#define AHL_STREAM_EVENT_UNMUTE "unmute"  // Audio stream unmuted

///// Interpret returned or configuration information /////

// Known audio domain string definitions (for configuration file format and device URI interpretation)
#define AHL_DOMAIN_ALSA "alsa"
#define AHL_DOMAIN_PULSE "pulse"
#define AHL_DOMAIN_GSTREAMER "gstreamer"
#define AHL_DOMAIN_EXTERNAL "external"

// ALSA Device URI type
#define AHL_DEVICEURITYPE_ALSA_HW "hw"             // Alsa hardware device URI
#define AHL_DEVICEURITYPE_ALSA_DMIX "dmix"         // Alsa Dmix device URI (only for playback devices)
#define AHL_DEVICEURITYPE_ALSA_DSNOOP "dsnoop"     // Alsa DSnoop device URI (only for capture devices)
#define AHL_DEVICEURITYPE_ALSA_SOFTVOL "softvol"   // Alsa softvol device URI
#define AHL_DEVICEURITYPE_ALSA_PLUG "plug"         // Alsa plug device URI
#define AHL_DEVICEURITYPE_ALSA_OTHER "other"       // Alsa domain URI device of unspecified type
#define AHL_DEVICEURITYPE_NOT_ALSA "nonalsa"

// Define default behavior of audio role when interrupting lower priority sources (in configuration)
#define AHL_INTERRUPTBEHAVIOR_CONTINUE "continue" // Continue to play when interrupted (e.g. media may be ducked)
#define AHL_INTERRUPTBEHAVIOR_CANCEL "cancel"     // Abort playback when interrupted (e.g. non-important HMI feedback that does not make sense later)
#define AHL_INTERRUPTBEHAVIOR_PAUSE "pause"       // Pause source when interrupted, to be resumed afterwards (e.g. non-temporal guidance)

///// Naming convention /////

// Standardized name for common audio roles (not enforced in any way, just helps compatibility)
#define AHL_ROLE_Emergency "emergency"         // critical alerts/alarms
#define AHL_ROLE_Warning "warning"             // Safety-relevant alerts/alarms
#define AHL_ROLE_CustomHigh "customhigh"       //
#define AHL_ROLE_Phone "phone"                 // Voice communications (e.g. handsfree, speech recognition)
#define AHL_ROLE_Navigation "navigation"       // Important user information where user action is expected (e.g. navigation instruction)
#define AHL_ROLE_CustomMedium "customMedium"   //
#define AHL_ROLE_Video "video"                 // Video content
#define AHL_ROLE_Streaming "streaming"         // Streaming content
#define AHL_ROLE_Multimedia "multimedia"       // Multimedia content
#define AHL_ROLE_Radio "radio"                 // Radio content
#define AHL_ROLE_CustomLow "customLow"         //
#define AHL_ROLE_Fallback "fallback"           //


// Standardized list of properties (not enforced in any way, just helps compatibility)
#define AHL_PROPERTY_BALANCE "balance"
#define AHL_PROPERTY_FADE "fade"
#define AHL_PROPERTY_EQ_LOW "eq_bass"
#define AHL_PROPERTY_EQ_MID "eq_mid"
#define AHL_PROPERTY_EQ_HIGH "eq_treble"

// Standardized list of events (not enforced in any way, just helps compatibility)
#define AHL_EVENTS_PLAYSOUND "play_sound"
#define AHL_EVENTS_ECHOCANCEL_ENABLE "echocancel_enable"
#define AHL_EVENTS_ECHOCANCEL_DISABLE "echocancel_disable"

#endif // AHL_INTERFACE_INCLUDE

------------------------------------------------------------------------
                  AGL Audio High Level Binding
------------------------------------------------------------------------

# Audio High Level Binding
-----------------------------
The Audio High Level Binding is the upper layer in the Audio 4A architecture.
The binding provide a simple, unified single entry point for all AGL audio applications.

Here are the features provide by the binding:

- Expose all audio device capabilities in uniform way to applications
- Provide display name for device to applications  (e.g. for user selection)
- Provide target device URI (e.g. stream to selected endpoint) to applications 
- Automatically retrieve associated volume control for ALSA softvol URI
- Allow fine grain security permissions control, policy enforcement and application audio stream control isolation
- Priority-based and audio role specific endpoint selection / stream routings (automatic or explicit)
- Aggregation of different audio domains (ALSA, Pulse)
- Audio stream controls (volume, mute, state, properties)


# Glossary
-----------------------------
The binding define the following term:

- AHL                : Audio High Level Binding
- Audio role         : Specific set of audio policy rules applied to a group of audio stream
- Endpoint           : Virtual audio sink or source device.
- Stream             : Audio connection between an application and an endpoint (source or sink).
- Audio 4A Framework : AGL Audio Framework using a set of low level, HAL and HLB bindings.
- Policy Engine      : Static library define in ahl-policy.c implementing audio policy.
- URI		     : Uniform Resource Identifier (e.g. ALSA PCM name)


# Policy Engine
------------------------------

The sample implementation of the policy engine is implemented as a static library.
The interface between the policy engine and the audio high level binding is a simple JSON interface.
This allows users to easily replace it with their own policy engine.


# Endpoint Selection
------------------------------
The AHL JSON configuration file defines a number of possible endpoints per audio role. The endpoints are listed in order of priority, from highest to lowest priority.
At initialization time, the AHL will validate each endpoint and only keep a list of available endpoints per audio role. Inactive endpoints are discarded and are not accessible to applications.

Applications can request a list of available endpoints for a specific audio role by calling the API/Verb get_endpoints.
Applications can decide to open a stream on a specific endpoint from the list by specifying an EndpointID or let AHL select automatically an endpoint based on it audio role endpoint priority list.
In the latter case, the endpoint selected will be the first available endpoint on the audio-role specific list.

# Events
-------------------------------
AHL will generate 4 types of events, defined in ahl-interface.h:

- **AHL_STREAM_STATE_EVENT**

   Applications are automatically susbcribed to this event and will only receive events for streams they have opened.
- **AHL_ENDPOINT_VOLUME_EVENT**

   Applications need to subscribe to this event to receive volume change notifications.
- **AHL_ENDPOINT_PROPERTY_EVENT**

   Applications need to subscribe to this event to receive property change notifications.
- **AHL_POST_ACTION_EVENT**

   Applications need to subscribe to this event to receive action notifications. Note: This is for future use cases, involving sound generation for example.


# AHL Configuration File and System Configuration
----------------------------------------------
Please refer to README.md documentation inside subfolder:

conf.d/project/README.md


# Cloning Audio High Level Binding from Git
-------------------------------------------------------

```
# Initial clone with submodules
git clone --recurse-submodules https://github.com/Audiokinetic-Automotive/afb-audiohighlevel.git
cd  audio-binding
# Do not forget submodules with pulling
git pull --recurse-submodules https://github.com/Audiokinetic-Automotive/afb-audiohighlevel.git

```

# System libraries Dependencies
------------------------------------------------------------------
- libasound (version 1.1.2 or latest)
- libsystemd (version 222 or latest)
- libmicrohttpd (version 0.9.55 or latest)
- afb-daemon (version 2.0 or latest)
- json-c
- libafbwsc
- glib-2.0

# AGL Binding Dependencies
-------------------------------------------------------------------
AGL Audio High Level Binding is part of the AGL Audio 4A framework,
It requires the following AGL bindings:

- **4a-alsa-core**

   Alsa Low Level Binding

   source: https://gerrit.automotivelinux.org/gerrit/#/admin/projects/src/4a-alsa-core
- **4a-hal-reference**

   Hardware Abstraction Layer Binding

   source: https://gerrit.automotivelinux.org/gerrit/#/admin/projects/src/4a-hal-reference 


# Compile AGL Audio High Level Binding
--------------------------------------

Set INSTALL_PREFIX variable to your local AGL binding install folder.


```
export INSTALL_PREFIX=~/opt
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX ..
make
make install

```

# Launch command to test and usage (the actual list of HAL to use may be specific to each hardware setup, please adapt name and ldpath parameters to match with your system configuration)


```
afb-daemon --name=audio4a --workdir=.--ldpaths=./lib:../agl-service-audio-4a/lib/afb-audiohighlevel.so:../4a-hal-reference/lib/afb-hal-intel-hda.so:../4a-alsa-core/lib/afb-alsa-4a.so --port=1234 --roothttp=./htdocs --token="" --verbose

```

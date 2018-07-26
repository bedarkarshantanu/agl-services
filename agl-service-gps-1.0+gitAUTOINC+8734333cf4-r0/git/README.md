# GPS Service

## Overview

GPS service reports current WGS84 coordinates from GNSS devices via the gpsd application.

## Verbs

| Name               | Description                             | JSON Parameters                                     |
| ------------------ |:----------------------------------------|:----------------------------------------------------|
| subscribe          | subscribe to gps/gnss events            | *Request:* {"value": "location"}                    |
| unsubscribe        | unsubscribe to gps/gnss events          | *Request:* {"value": "location"}                    |
| location           | get current gps/gnss coordinates        | See **location Event JSON Response** section        |
| record             | start/stop recording gps data           | See **Recording/Replaying Feature** section         |

## Events

| Name               | Description                             |
| ------------------ |:----------------------------------------|
| location           | event that reports gps/gnss coordinates |

### location Event JSON Response

| Parameter Name | Description                                                  |
|----------------|:-------------------------------------------------------------|
| altitude       | altitude in meters                                           |
| latitude       | latitude in degrees                                          |
| longitude      | longitude in degrees                                         |
| speed          | velocity in meters per second                                |
| track          | heading in degrees                                           |
| timestamp      | timestamp in ISO8601 format *(example: 2018-01-25T13:15:22)* |

## Recording/Replaying Feature

Entering *record* mode you must send **{"state": "on"}** with the **record** verb which will have a JSON response of
**{"filename": "gps_YYYYMMDD_hhmm.log"}** pointing to log under *app-data/agl-service-gps*

Now to enter *replaying* mode you must symlink or copy a GPS dump to *app-data/agl-service-gps/recording.log* and restart
the service. From then on out the previously recorded GPS data will loop infinitely which is useful for testing or
demonstration purposes.

## Environment variables

| Name            | Description                      |
|-----------------|:---------------------------------|
| AFBGPS\_HOST    | hostname to connect to           |
| AFBGPS\_SERVICE | service to connect to (tcp port) |


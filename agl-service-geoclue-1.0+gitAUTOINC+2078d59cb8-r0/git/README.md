# GeoClue Service

## Overview

GeoClue service uses the respective package to determine the vehicles location based on geolocation services
using WiFi SSIDs, and IP addresses.

## Verbs

| Name               | Description                             | JSON Parameters                                                                            |
|--------------------|:----------------------------------------|:-------------------------------------------------------------------------------------------|
| subscribe          | subscribe to geofence events            | *Request:* {"value": "location"}                                                           |
| unsubscribe        | unsubscribe to geofence events          | *Request:* {"value": "location"}                                                           |
| location           | get current GeoClue coordinates         | *Response:* {"latitude": 45.50, "longitude": -122.25, "accuracy": 20000, "altitude": 4000} |

*accuracy* is the calculated radius in meters that the geolocation reading is likely within.

## Events

| Name               | Description                          | JSON Response                                                                               |
|--------------------|:-------------------------------------|:--------------------------------------------------------------------------------------------|
| location           | event that reports GeoClue status    |  *Response:* {"latitude": 45.50, "longitude": -122.25, "accuracy": 20000, "altitude": 4000} |

### location Event Notes

Additonal fields that added to the JSON response when available are *speed* in meters per second, and *heading* in degrees.

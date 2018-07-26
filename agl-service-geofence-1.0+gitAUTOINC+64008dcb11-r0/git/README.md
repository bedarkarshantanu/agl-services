# GeoFence Service

## Overview

GeoFence service allows events to be triggered when vehicle enters, leaves, or dwells within
a defined bounding box

## Verbs

| Name               | Description                             | JSON Parameters                                                                            |
| ------------------ |:----------------------------------------|:-------------------------------------------------------------------------------------------|
| subscribe          | subscribe to geofence events            | *Request:* {"value": "fence"}                                                              |
| unsubscribe        | unsubscribe to geofence events          | *Request:* {"value": "fence"}                                                              |
| add_fence          | add geofence bounding box               | *Request:* {"name": "fence_name", "bbox": [...]}                                           |
| remove_fence       | remove named geofence                   | *Request:* {"name": "fence_name" }                                                         |
| list_fences        | list current bounding boxes and state   | *Response:* array of {"name": "fence_name, "bbox": [...], "within": false, "dwell": false} |
| dwell_transition   | get/set dwell transition time interval  | *Request:* {"value": 10} *Response:* {"seconds": 10}                                       |

## Bounding Box

Fence boundaries are defined with a bounding box parameter (i.e. bbox) in the following format

<pre>
 ...
 "bbox": {
        "min_latitude": 45.600136,
        "max_latitude": 45.600384,
        "min_longitude": -122.499217,
        "max_longitude": -122.498732,
  }
 ...
</pre>

## Events

| Name               | Description                          | JSON Response                              |
| ------------------ |:-------------------------------------|:-------------------------------------------|
| fence              | event that reports geofence status   | {"name": "fence_name", "state": "entered"} |

### fence Event Notes

*state* parameter in event response can have one of the following values  *entered*, *exited*, or *dwell*

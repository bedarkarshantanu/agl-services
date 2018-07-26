# MediaPlayer Service

## Overview

MediaPlayer service controls playback of media from a playlist using one provided from
*agl-service-mediascanner* and reports status via events.

## Verbs

| Name               | Description                             | JSON Parameters                                 |
|:-------------------|:----------------------------------------|:------------------------------------------------|
| subscribe          | subscribe to respectice events          | *Request:* {"value": "playlist"}                |
| unsubscribe        | unsubscribe to respective events        | *Request:* {"value": "playlist"}                |
| controls           | controls for media playback             | See **MediaPlayer Controls** section            |
| metadata           | get current metadata of selected media  | See **metadata Reporting** section              |
| playlist           | get current playlist of media           | See **playlist JSON Response** section          |

### MediaPlayer Controls

Media playback can be controlled with sending on the following action commands within a JSON request within
the parameter of *value* (i.e. *{"value": "play"}*)

| Name            | Description                                               | JSON Request Example                        |
|:----------------|:----------------------------------------------------------|:--------------------------------------------|
| play            | start playing media                                       | {"value": "play}                            |
| pause           | stop playing media                                        | {"value": "pause"}                          |
| previous        | skip to previous item in playlist                         | {"value": "previous"}                       |
| next            | skip to next item in playlist                             | {"value": "next"}                           |
| seek            | seek position (in milliseconds) within current track      | {"value": "seek", "position": 50000}        |
| fast-forward    | seek forward (in milliseconds) within current track       | {"value": "fast-forward", "position": 2000} |
| rewind          | seek backward (in milliseconds) within current track      | {"value": "rewind", "position": 2000}       |
| pick-track      | select media item in playlist via index number            | {"value": "pick-track", "index": 4}         |
| volume          | set volume 0-100% for media stream                        | {"value": "volume, "volume": 40}            |
| loop            | loop playlist                                             | {"value": "loop", "state": "true"}          |

### metadata Reporting

JSON response for *metadata* request parameters

| Name        | Description                                        |
|:------------|----------------------------------------------------|
| index       | index number within playlist                       |
| duration    | length of track in milliseconds                    |
| position    | current position in milliseconds                   |
| volume      | current volume in percent                          |
| path        | path to media on filesystem                        |
| title       | title for current track                            |
| album       | album name for current track                       |
| artist      | artist name for current track                      |
| genre       | genre type for current track                       |
| image       | *(optional)* base64 encoded data URI for album art |

### playlist JSON Response

JSON response is an array of playlist entries with the parameter name of *list*.

| Name        | Description                                     |
|:------------|-------------------------------------------------|
| index       | index number within playlist                    |
| duration    | *(optional)* length of track in milliseconds    |
| path        | path to media on filesystem                     |
| title       | title for playlist entry                        |
| album       | album name for playlist entry                   |
| artist      | artist name for playlist entry                  |
| genre       | genre type for playlist entry                   |

## Events

| Name               | Description                                  |
|--------------------|:---------------------------------------------|
| playlist           | event that reports playlist changes          |
| metadata           | event that reports playback status           |

### playlist Event Notes

JSON response data is an array of the same fields documented in **playlist JSON Response** section

### metadata Event Notes

JSON response data is the same fields documented in **metadata Reporting** section *with the exception
of album art due to performance issues*

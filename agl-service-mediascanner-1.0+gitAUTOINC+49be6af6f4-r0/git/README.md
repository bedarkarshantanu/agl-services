# MediaScanner Service

## Overview

MediaScanner service use the database from respective *lightmediascanner* media scan, and also
triggers playlist updates when storage media changes.

## Verbs

| Name           | Description                | JSON Parameters                        |
|:---------------|:---------------------------|:---------------------------------------|
| subscribe      | subscribe to media events  | *Request:* {"value":"media_added"}     |
| unsubscribe    | unsubcribe to media events | *Request:* {"value":"media_added"}     |
| media_result   | get current media playlist | See **media_result Reporting** section |

### media_result Reporting

JSON response for *media_result* request parameters is an array of dictionary entries
with the following fields.

| Name        | Description                                 |
|:------------|---------------------------------------------|
| duration    | length of track in milliseconds             |
| path        | path to media on filesystem                 |
| title       | title for media entey                       |
| album       | album name for media entry                  |
| artist      | artist name for media entry                 |
| genre       | genre type for media entry                  |
| type        | media entry data type *(e.g audio, video)*  |

## Events

| Name           | Description                                        |
|:---------------|:---------------------------------------------------|
| media_added    | event that reports storage media insertion         |
| media_removed  | event that reports storage media removal           |

### media_added Event JSON Response

JSON response for this event has the same results as documented in **media_result Reporting ** sections.

### media_removed Event JSON Response

JSON response has a single field **Path** that is the location of media that has been removed.

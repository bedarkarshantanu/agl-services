# Radio Service

## Overview

Radio Service allows tuning of rtl-sdr based devices to radio stations and receive
respective audio stream.

## Verbs

| Name               | Description                                 | JSON Parameters                                                |
|:-------------------|:-------------------- -----------------------|:---------------------------------------------------------------|
| subscribe          | subscribe to radio events                   | *Request:* {"value": "frequency"}                              |
| unsubscribe        | unsubscribe to radio events                 | *Request:* {"value": "frequency"}                              |
| frequency          | get/set tuned radio frequency               | *Request:* {"value": 101100000}                                |
| band               | get/set current band type (e.g. AM, FM)     | *Request:* {"band": "FM"} *Response:* {"band": "FM"}           |
| band_supported     | check if a certain band is supported        | *Request:* {"band": "FM"} *Response:* {"supported": 1}         |
| frequency_range    | get frequency range for band type           | *Request:* {"band": "FM"} *Response:* {"min": ..., "max": ...} |
| frequency_step     | get frequency step/spacing for band type    | *Request:* {"band": "FM"} *Response:* {"step": 200000}         |
| start              | start radio playback                        |                                                                |
| stop               | stop radio playback                         |                                                                |
| scan_start         | start scanning for station                  | *Request:* {"direction": "forward" or "backward"}              |
| scan_stop          | stop scanning for station                   |                                                                |
| stereo_mode        | get/set stereo or mono mode                 | *Request:* {"value": "stereo" or "mono"}                       |

## Events

### frequency Event JSON Response

JSON response has a single field **frequency** which is the currently tuned frequency.

### station_found Event JSON Response

JSON response has a single field **value** of the frequency of the discovered radio station.

# AGL Radio Tuner Binding

## FM Band Plan Selection

The FM band plan may be selected by adding:
```
fmbandplan=X
```
to the [radio] section in /etc/xdg/AGL.conf, where X is one of the
following strings:

US = United States / Canada
JP = Japan
EU = European Union
ITU-1
ITU-2

Example:
```
[radio]
fmbandplan=JP
```

## Implementation Specific Confguration

### USB RTL-SDR adapter

The scanning sensitivity can be tweaked by adding:
```
scan_squelch_level=X
```
to the [radio] section in /etc/xdg/AGL.conf, where X is an integer.  Lower
values make the scanning more sensitive.  Default value is 140.

Example:
```
[radio]
scan_squelch_level=70
```

### M3ULCB Kingfisher Si4689

The scanning sensitivity can be tweaked by adding:
```
scan_valid_snr_threshold=X
scan_valid_rssi_threshold=Y
```
to the [radio] section in /etc/xdg/AGL.conf, where X and Y are integers
between -127 and 127.  The SNR value is in units of dB, and the RSSI is in
units of dBuV.  Lower values make the scanning more sensitive.  Default
values in the Si4689 are 10 and 17, respectively. You may determine the
values that the Si4689 is seeing when tuning by examining the results of
tuning in the systemd journal, looking for lines like:

Example:
```
[radio]
scan_valid_snr_threshold=7
scan_valid_rssi_threshold=10
```

## Known Issues

### M3ULCB Kingfisher

Initial setup for a new Kingfisher board requires booting an image with
Kingfisher support and running the commands:
```
si_init
si_firmware_update
```
This installs the provided firmware into the flash attached to the Si4689.

Since all operations are currently done by calling a patched version of
Cogent Embedded's si_ctl utility, scanning currently cannot be interrupted.
Additionally, sometimes a failure in scanning seems to result in muted
state that currently has not been debugged.

# IIODEVICES Service

## Overview

The iiodevices service provides access to data from industrial i/o devices.
For now it permits to get data from acceleration, gyroscope and electronic compass.

## General Scheme

As soon as a client subscribes to the agl-service-iiodevices binding,
the binding reads values from the sensors and sends it to subscribers as events.
Subscribers can choose their frequency and indicate what values they want at
subscription.

## Verbs

| Name               | Description                                 | JSON Parameters                                                   |
|:-------------------|:--------------------------------------------|:---------------------------------------------------------------   |
| subscribe          | subscribe to 9 axis events                  | *Request:* {"event": "acceleration", "args":"xy", "frequency": "10" }|
| unsubscribe        | unsubscribe to accelero events              | *Request:* {"event": "acceleration" } |

## Events

For now, there are 3 different events matching with the different available sensors.

* "acceleration": is for acceleration data
* "gyroscope": is for gyroscope data
* "compass": is for electronic compass data

## Frequency

The frequency is in Hertz, if the frequency is not set, events are triggered via a file descriptor.

## Client Demo example

Here is an example to show how to get data from iiodevices with the demo client.

### Launch afb-client-demo

First, launch the client demo with right port and right TOKEN, the example below
matches with ff(6.x) AGL version.

``` bash
afb-client-demo ws://localhost:1055/api?token=HELLO
```

### Subscribe to acceleration

Here is a list of different examples to subscribe to acceleration data.

#### Subscribe to acceleration with x,y and z axis with a frequency of 0.1 Hz

``` bash
iiodevices subscribe { "event": "acceleration",  "args": "xyz", "frequency": "0.1" }
```

#### Subscribe to acceleration with x and z axis

``` bash
iiodevices subscribe { "event": "acceleration",  "args": "xz" }
```

Events will be sent each time a new value is available (sent by iiodevice).

### Unsubscribe to acceleration

``` bash
iiodevices unsubscribe { "event": "acceleration" }
```

## Remaining issues

- Provide a json file to configure the device name and the channel name.
- Handle several values simultaneously, see triggers.
- Update this binding for other iiodevices.
- Only read channel values at the maximum frequency.
- Change args values into json arrays.

## M3ULCB Kingfisher

M3ULCB Kingfisher is equipped with a 9 axis sensor device (LSM9DS0) and the R-Car Starter
Kit can read sensor values via I2C interface and iiodevices are provided for
these sensors.

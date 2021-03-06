# Signal Composer

## Architecture

Here is a quick picture about the signaling architecture :

![GlobalArchitecture]

Key here are on both layers, **low** and **high**.

- **Low levels** are in charge of handling data exchange protocol to
 decode/encode and retransmit with an AGL compatible format, most convenient
 way using **Application Framework** events. These are divided into two parts,
 which are :
  - Transport Layer plug-in that is able to read/write a protocol.
  - Decoding/Encoding part that exposes signals and a way to access them.
- **High level signal composer** gathers multiple **low level** signaling
 sources and creates new virtuals signals from the **raw** signals defined (eg.
 signal made from gps latitude and longitude that computes the heading of
 vehicle). It is modular and each signal source should be handled by specific
 plugins which take care of get the underlying event from **low level** or
 define signaling composition with simple or complex operation to output value
 from **raw** signals

A transport plug-in is a shared library that shares a common API to be
compatible with **low level** services that is:

- **open/close**: method to open a handle which could be a socket, file or
 device by example.
- **read/write**: method to read and write a stream of data.

Configuration is made by sending a special packet using a write method to the
handle. In brief, this could be compared to the layer 1 and 2 of [OSI model].

There are three main parts with **Signal Composer**:

- Configuration files which could be splitted in differents files. That will
 define:
  - metadata with name of **signal composer** api name
  - additionnals configurations files
  - plugins used if so, **low level** signals sources
  - signals definitions
- Dedicated plugins
- Signal composer API

## Terminology

Here is a little terminology guide to set the vocabulary:

- **api**: a binding API name
- **action**: a function called at a certain time
- **callbacks**: a function called at a certain time
- **event**: the raw event that materialize the signal
- **plugins**: a C/C++ code with functions that be called by signal composer
 service
- **sources**: an external binding API that generate signals for the signal
 composer service
- **signals**: an event generated by the **Application Framework**
- **virtual signals**: a signal composed of **raw signals** with value
 calculated by a **callbacks**
- **raw signals**: an event generated by a **low level** binding

[OSI model]: https://en.wikipedia.org/wiki/OSI_model
[GlobalArchitecture]: pictures/Global_Signaling_Architecture.png "Global architecture"

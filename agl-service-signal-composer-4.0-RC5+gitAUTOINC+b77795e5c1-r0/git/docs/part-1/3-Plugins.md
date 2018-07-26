# Plugins

Plugins are C/C++ shared library loaded by the binding to execute some
simple routine. Routine could be on reception of a new signal or at sources
initialization time or signal subscription with the respective JSON field
**onReceived**, **init** and **getSignals**.

A default plugin (builtin) is provided with 2 functions:

- **defaultOnReceived**: set and record a new signal value and its timestamp
 in the signal composer service. It simply tooks the incoming event JSON object
 and search for *key* **value** and **timestamp** then call function
 `setSignalValue`.
- **setSignalValueWrap**: a **lua2c** function the could be called from any LUA
 script to record a new signal value.

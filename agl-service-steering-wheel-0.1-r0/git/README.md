Information
====
<br>This service use logitel G29 to get wheel info for application same as low-level-can-service.
<br>It will read information from usb g29 as /dev/input/js0.
<br>And give steering wheel information for application.

* Hardware: Renesas m3ulcb
* Software: Daring Dab 4.0.0
* binding name: af-steering-wheel-binding
* provide api: steering-wheel
* verbs: list, subscribe, unsubscribe
* support device: Logitech G29 steering wheel
* steering wheel information is write in steering_wheel_map.json
    * VehicleSpeed			< Engine Speed * Gear parameter / 100 >
    * EngineSpeed			< 0～10000 >
    * AcceleratorPedalPosition	< 0～100 >
    * TransmissionGearInfo    < 0、1、2、3、4、5、6 >
    * TransmissionMode		< 0、1、2、3、4、5、6 >
    * SteeringWheelAngle		< 0～360 >
    * TurnSignalStatus		< 0:None / 1:RightTurn / 2:LeftTurn >
    * LightStatusBrake		< 0:None / 1:BrakeEnable >
* verbs
    * list
    * subscribe { "event" : "EngineSpeed" }
    * unsubscribe { "event" : "EngineSpeed" }

How to compile and install
====
<br>	These is a sample recipes for af-steering-wheel-binding, you can just add that recipes into your project and bitbake.
<br>	Sample Recipes: agl-service-steering-wheel_0.1.bb

How to use
====
For AGL Application
----
<br>1 add these code below into config.xml
```
  <param name="steering-wheel" value="ws"/>
```
<br>2 add WebSocket into qml file and subscribe event name what you need.
<br>You can find a file named [token-websock.qml] in app-framework-binder.git.
<br>It's a template for qml. You need modify these informations to connect agl-service-steering.
```
property string address_str: "ws://localhost:5555/api?token=3210"
property string api_str: "steering-wheel"
property string verb_str: ""
```

For demo application
----
<br>1 you can run this service by shell script
```
afb-daemon --token=3210 --ldpaths=${steering-wheel-library-path} --port=5555 --rootdir=. &
```
<br>2 use afb-client-demo to get event.
```
afb-client-demo ws://localhost:5555/api?token=3210
steering-wheel list
steering-wheel subscribe { "event" : "EngineSpeed" }
steering-wheel unsubscribe { "event" : "EngineSpeed" }
```

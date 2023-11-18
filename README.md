# guarduino
Guardino is an Arduino (Mega) Sketch which is intended to provide a low-cost, high reliability, home security hardware interface which seamlessly interfaces into HomeAssistant via MQTT autodiscovery.
<br/>
<img src="https://github.com/mkachline/guarduino/blob/main/images/guarduino-ha.jpg" alt="drawing" height=380 />

## Guarduino Features
* Monitors reed switches via BOTH "NO" and "NC" pins, thus enabling reliable detection of "Open", "Closed", "Unknown" and "No Power".
* Monitors (PIR/microwave) motion detectors via BOTH "data" pin AND "power sense" pin, thus enabling reliable detection of "motion", "quiet" and "No power."
* Supports HomeAssistant MQTT "Auto Discovery", NO "configuration.yaml" changes are needed.
* Supports monitoring multiple (20 or more?) concurrent, "two pin" sensors.
* Supports two-way toggle and read of HomeAssistant "Switches" via MQTT.
* Supports reading zero to many DS18x temperature sensors.
* Presents dynamic icons based on sensor type and current sensor state.

## Quick Start (HomeAssistant Users)
1. Install the "Mosquitto Broker" Add-On into your HomeAssistant instance.
2. Install the "MQTT" Integration into your HomeAssistant instance.
3. Edit guarduino.ino
4. Set your Mosquitto IP address (MQTT_ADDRESS) in guarduino.ino
5. Set your Mosquitto Password (MQTT_PASSWORD) in guarduino.ino
6. Install Arduino libraries listed on top of guarduino.ino
7. Compile, then upload into your Arduino Mega.
8. Boot your Arduino (Serial Baud rate 19200)
9. HomeAssistant | Settings | Devices & Services | MQTT
10. Wait for your Guarduino to show up.

## More Information
Visit the [Guarduino Wiki](https://github.com/mkachline/guarduino/wiki)


## Guarduino History
Guarduino was born after I experienced a theft/break-in into my garage where every hand tool I owned was stolen. Determined to prevent this again, I wanted to monitor entry points into the garage (and house) using devices which I could RELY on, and which did not cost a fortune. 

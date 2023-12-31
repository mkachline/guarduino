# Guarduino
Guardino is an Arduino (Mega) Sketch which provides a low-cost, high reliability, "wired" home security hardware interface which reports into HomeAssistant using MQTT autodiscovery.
<br/>
<img src="https://github.com/mkachline/guarduino/blob/main/images/guarduino-ha.jpg" alt="drawing" height=380 />

## Guarduino Features
* Monitors wired reed switches via BOTH "NO" and "NC" pins, thus enabling reliable detection of "Open", "Closed", "Unknown" and "No Power".
* Monitors wired (PIR/microwave) motion detectors via BOTH "data" pin AND "power sense" pins, thus enabling reliable detection of "motion", "quiet", "unknown" and "no power."
* Supports HomeAssistant MQTT "Auto Discovery". NO "configuration.yaml" changes are needed.
* Supports monitoring multiple (20 or more?) concurrent, "two pin" sensors.
* Supports two-way toggle and read of HomeAssistant "Switches" via MQTT.
* Supports reading zero to many wired, DS18x temperature sensors.
* Presents dynamic icons based on sensor type and current sensor state.
* Runs on (affordable) Arduino Mega hardware.

## Quick Start (HomeAssistant Users)
1. Install the "Mosquitto Broker" Add-On into your HomeAssistant instance.
2. Install the "MQTT" Integration into your HomeAssistant instance.
3. Edit guarduino.ino
4. Set your Mosquitto IP address (MQTT_ADDRESS) in guarduino.ino
5. Set your Mosquitto Password (MQTT_PASSWORD) in guarduino.ino
6. Install Arduino dependency libraries listed on top of guarduino.ino
7. Compile, then upload into your Arduino Mega.
8. Boot your Guarduino (Serial Baud rate 19200)
9. HomeAssistant | Settings | Devices & Services | MQTT
10. Wait for your Guarduino to show up.

## More Information
Visit the [Guarduino Wiki](https://github.com/mkachline/guarduino/wiki)


## Guarduino History
Guarduino was born after I was victim of a break-in into my garage where,  every hand tool I owned was stolen. Determined to prevent this again, I wanted to monitor entry points into the garage (and house) using devices which I could RELY on, and which did not cost a fortune. I found that most existing projects either supported "one pin" reed switches, and/or, were not capable of supporting numerous devices on an Arduino, typically due to memory constraints.

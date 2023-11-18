# guarduino
Guardino is an Arduino (Mega) Sketch which is intended to provide a low-cost, high reliability, home security hardware interface which seamlessly interfaces into HomeAssistant via MQTT autodiscovery.

## Features:
* Monitors reed switches via BOTH "NO" and "NC" pins, thus enabling reliable detection of "Open", "Closed", "Unknown" and "No Power".
* Monitors (PIR/radar) motion detectors via BOTH "data" pin AND "power sense" pin, thus enabling reliable detection of "motion", "quiet" and "No power."
* Supports HomeAssistant MQTT "Auto Discovery", NO "configuration.yaml" changes are needed.
* Supports monitoring 20 or more concurrent, "two pin" sensors.
* Supports two-way toggle and read of HomeAssistant "Switches" via MQTT.
* Supports reading zero to many DS18x temperature sensors.
* Supports dynamic icons based on sensor type and current state of the sensor.

## Quick Start
1. git clone
2. Edit guarduino.ino
3. Set a MAC address.
4. Set your MQTT IP
5. Set your MQTT user
6. Set your MQTT password
8. Install needed libraries (See top of guarduino.ino)
9. Compile, upload into your Arduino Mega. Boot.
10. HomeAssistant | Settings | Devices & Services | MQTT
11. Wait for your Guarduino to show up.


## History
Guarduino was born after I experienced a break-in into my garage where every hand tool I owned was stolen. Determined to prevent this again, I wanted to monitor entry points into the garage (and house) using devices which I could RELY on, and which did not cost a fortune. 

# guarduino
Guardino is an Arduino Sketch which is intended to provide a low-cost, high reliability, home security hardware interface which seamlessly interfaces into HomeAssistant via MQTT autodiscovery.

## Features:
* Monitors reed switches via BOTH "NO" and "NC" pins, thus enabling reliable detection of "Open", "Closed", "Unknown" and "No Power".
* Monitors (PIR/radar) motion detectors via BOTH "data" pin AND "power sense" pin, thus enabling reliable detection of "motion", "quiet" and "No power."
* Supports HomeAssistant MQTT "Auto Discovery", "Plug and Play", NO "configuration.yaml" changes are needed.
* Supports monitoring 20 or more concurrent, "two pin" sensors.
* Supports two-way toggle and read of HomeAssistant "Switches" via MQTT.
* Supports reading zero to many 1-Wire DS18x temperature sensors.
* Supports dynamic icons based on sensor type and current state.

## Quick Start
1. git clone
2. Open project in Arduino IDE
3. Configure a MAC address,
4. Configure MQTT IP, user and password.
5. Configure which pins will be which devices.
6. Install needed libraries
7. Compile, upload image into your Arduino Mega. Boot.
8. Watch for inbound messages on your MQTT server.


## History
Guarduino was born after I experienced a break-in into my garage where every hand tool I owned was stolen. Determined to prevent this again, I wanted to monitor entry points into the garage (and house) using devices which I could RELY on, and which did not cost a fortune. 

# guarduino
Guardino is an Arduino Sketch which is intended to provide a low-cost, high reliability, home security hardware interface which seamlessly interfaces into HomeAssistant via MQTT autodiscovery.



## Quick Start
1. git clone
2. Open project in Arduino IDE
3. Configure a MAC address,
4. Configure MQTT server IP, user and password.
5. Install needed libraries
6. Compile, upload image into your Aruino Mega
7. Wire up a door sensor with three wires, 5v, "NO", and "NC"
8. Plug "NO" into arduino pin 22
9. Plug "NC" into arduino pin 23
10. Boot Arduino
11. Watch for inbound mssages on your MQTT server.



## History
Guarduino was born after I experienced a break-in into my garage where every hand tool I owned was stolen. Determined to prevent this again, I wanted to monitor entry points into the garage (and house) using devices which I could RELY on, and which did not cost a fortune. 

## Features:
* Monitors reed switches with BOTH data pins, thus enabling reliable detection of "Open", "Closed", "Unknown" and "No Power".
* Monitors (PIR) motion detectors via BOTH "data" pin AND "power sense" pin, thus enabling reliable detection of "motion", "quiet" and "No power."
* Supports HomeAssistant MQTT "Auto Discovery", "Plug and Play", NO "configuration.yaml" changes are needed.
* Designed to use EVERY available pin on the Arduino Mega; monitoring up to 20, "two pin" sensors.
* Supports "two pin" Door and Window Reed Switches
* Supports "two pin" Motion (PIR and radar) Sensors
* Supports reading and toggling HomeAssistant "Switch" sensors
* Supports reading zero to many 1-Wire DS18x temperature sensors.
* Built-In support for icons based on sensor type and current state of the sensor.


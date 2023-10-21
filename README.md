# guarduino
Guardino is an Arduino Sketch which is intended to provide a low-cost, high accuracy security system. Guarduino was written and tested to integrate seamlessly with HomeAsssitant using HA's MQTT "Auto Discovery".



# Quick Start
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
Guarduino was born after I had someone break into my garage and steal every hand tool I owned (and used) for automobile repair. I wanted to monitor entry into the garage (and house) using devices which I could rely on, regardless of temperature, time of day, distance, etc.

## Features:
* By design, monitors reed switches with TWO data pins, enabling detection of "Open", "Closed" and "Unknown".
* Written to communicate seamlessly with HomeAssistant's MQTT broker.
* Designed to use EVERY available pin on the Arduino Mega.
* Supports "two pin" Door Reed Switches
* Supports "two pin" Window Reed Switches
* Supports "two pin" Motion Sensors
* Supports reading and setting "Switches"
* Supports reading zero to many 1-Wire DS18x temperature sensors.
* Buit-in icon toggle with status toggle.
* Supports HomeAssistant MQTT "Auto Discovery", NO "cnofiguration.yaml" changes are needed.

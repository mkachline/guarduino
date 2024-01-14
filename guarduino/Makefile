upload:
	avrdude -v -D -U flash:w:build/guarduino.ino.hex -P /dev/ttyACM0 -b 115200 -c wiring -p m2560

tail:
	stty -F /dev/ttyACM0 raw 115200
	cat /dev/ttyACM0

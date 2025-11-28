
build/guarduino.ino.hex: arduino-libs
	#https://github.com/arduino/arduino-cli
	arduino-cli compile -b arduino:avr:mega --output-dir build guarduino.ino



ARDUINO_LIBS = \
  ArduinoJson \
  BoardIdentify \
  DallasTemperature \
  Ethernet \
  MemoryUsage \
  OneWire \
  PubSubClient \
  SD
.PHONY: arduino-libs
arduino-libs:
	@for lib in $(ARDUINO_LIBS); do \
		arduino-cli lib install "$$lib"; \
	done

.PHONY: clean
clean:
	rm -vf build/guarduino.*


.PHONY: install
install:
	arduino-cli board list
	arduino-cli upload -b arduino:avr:mega -p /dev/ttyACM0 --verbose ./guarduino.ino
	#avrdude -v -D -U flash:w:build/guarduino.ino.hex -P /dev/ttyACM0 -b 115200 -c wiring -p m2560
	arduino-cli monitor -p /dev/ttyACM0 -b arduino:avr:mega --config 115200

.PHONY: log
log:
	arduino-cli monitor -p /dev/ttyACM0 -b arduino:avr:mega --config 115200
	#stty -F /dev/ttyACM0 raw 115200
	#cat /dev/ttyACM0

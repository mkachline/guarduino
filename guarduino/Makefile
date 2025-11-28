
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


upload:
	avrdude -v -D -U flash:w:build/guarduino.ino.hex -P /dev/ttyACM0 -b 115200 -c wiring -p m2560

tail:
	stty -F /dev/ttyACM0 raw 115200
	cat /dev/ttyACM0

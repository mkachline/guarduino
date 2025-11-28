#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "guarduino.h"
#include <ctype.h>

#define MQTT_DEFAULT_PORT 1883

// Note: adjust this chip select pin to match your hardware's SD CS pin.
// Common values are 4 or 10 depending on shield/module.
static const uint8_t SDCARD_CS_PIN = 4;

// Forward declarations for helper functions used below.
static bool macStringToMacAddr(const char *macstr, size_t macstrSize);
static sensorType sensorTypeFromString(const char *s);
static bool isPinReserved(int pin);

// Read the JSON config at `filepath` and populate globals. If file or SD unavailable, leave defaults.
bool readSDConfig(const char *filepath) {
    // Clear globals that we'll populate
    memset(mqtt_password, '\0', sizeof(mqtt_password));
    memset(mqtt_username, '\0', sizeof(mqtt_username));
    mqtt_port = MQTT_DEFAULT_PORT;
    for (int i = 0; i < 6; ++i) mac[i] = 0;
    // zero sensors
    for (int i = 0; i < 64; ++i) { allSensors[i].type = unused; allSensors[i].pin1 = -1; allSensors[i].pin2 = -1; }

    if (!SD.begin(SDCARD_CS_PIN)) {
        Serial.print("SD.begin() failed - SD card unavailable or wrong CS pin (using pin ");
        Serial.print(SDCARD_CS_PIN);
        Serial.println(")");
        return false;
    }

    File f = SD.open(filepath);
    if (!f) {
        Serial.println("guarduino.json not found on SD card");
        return false;
    }

    // Read file into memory-limited JSON document
    const size_t capacity = 2048;
    StaticJsonDocument<capacity> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.print("Failed parse guarduino.json from SD Card. Err: ");
        Serial.println(err.c_str());
        return false;
    }

    // macaddress
    if (doc.containsKey("macaddress")) {
        const char *macstr = doc["macaddress"];
        if (macstr) {
            // Use strnlen with a max bound to avoid unbounded strlen
            size_t maclen = 0;
            const size_t MAX_MAC_LEN = 32; // generous upper bound for MAC string
            while (maclen < MAX_MAC_LEN && macstr[maclen] != '\0') maclen++;
            if (!macStringToMacAddr(macstr, maclen)) {
                Serial.println("Invalid macaddress in guarduino.json");
            }
        }
    } else {
        Serial.println("Warning: 'macaddress' missing from guarduino.json");
        return false;
    }


    // mqtt settings
    if (doc.containsKey("mqtt_address")) {
        const char *ipstr = doc["mqtt_address"];
        if (ipstr) {
            unsigned int a,b,c,d;
            if (sscanf(ipstr, "%u.%u.%u.%u", &a,&b,&c,&d) == 4) {
                mqtt_address = IPAddress((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d);
            } else {
                Serial.println("mqtt_address not a valid IPv4 string; using 0.0.0.0");
            }
        }
    } else {
        Serial.println("Warning: 'mqtt_address' missing from guarduino.json");
        return false; 
    }

    mqtt_port = MQTT_DEFAULT_PORT;
    if (doc.containsKey("mqtt_port")) {
        mqtt_port = doc["mqtt_port"].as<int>();
    } else {
        // tell user mqtt_port is missing, using default, and what that default is
        Serial.print("Warning: 'mqtt_port' missing from guarduino.json; assuming default port ");
        Serial.println(MQTT_DEFAULT_PORT);
    }


    if (doc.containsKey("mqtt_username")) {
        const char *u = doc["mqtt_username"];
        if (u) {
            memset(mqtt_username, '\0', sizeof(mqtt_username));
            strncpy(mqtt_username, u, sizeof(mqtt_username)-1);
        }
    } else {
        // tell usermqtt_username is missing from guarduino.json
        Serial.println("Warning: 'mqtt_username' missing from guarduino.json");
        return false;
    }

    if (doc.containsKey("mqtt_password")) {
        const char *p = doc["mqtt_password"];
        if (p) {
            memset(mqtt_password, '\0', sizeof(mqtt_password));
            strncpy(mqtt_password, p, sizeof(mqtt_password)-1);
        }
    } else {
        // Tell user mqtt_password is missing from guarduino.json
        Serial.println("Warning: 'mqtt_password' missing from guarduino.json");
        return false;
    }


    // sensors array
    if (doc.containsKey("sensors")) {
        JsonArray arr = doc["sensors"].as<JsonArray>();
        int idx = 0;
        for (JsonObject obj : arr) {
            if (idx >= 64) break;

            const char *type = obj["type"];
            int pin1 = obj["pin1"] | -1;
            int pin2 = obj["pin2"] | -1;

            // Skip sensors that use reserved pins
            if (isPinReserved(pin1) || isPinReserved(pin2)) {
                Serial.print("Skipping sensor on reserved pin(s): pin1=");
                Serial.print(pin1);
                Serial.print(", pin2=");
                Serial.println(pin2);
                continue;
            }
            allSensors[idx].type = sensorTypeFromString(type);
            allSensors[idx].pin1 = (int8_t)pin1;
            allSensors[idx].pin2 = (int8_t)pin2;
            idx++;
            // tell user a one-line summary of "this" sensor just read.
            Serial.print("Loaded sensor: type=");
            Serial.print(type ? type : "null");
            Serial.print(", pin1=");
            Serial.print(pin1);
            Serial.print(", pin2=");
            Serial.println(pin2);
        }
        
        // remaining entries already set to unused above
    } else {
        Serial.println("Warning: 'sensors' array missing from guarduino.json");
        Serial.println("Example 'sensors' array:");
        Serial.println(R"([
  { "type": "door2", "pin1": 5 },
  { "type": "motion2", "pin1": 6, "pin2": 7 }
])");
        return false;
    }


    Serial.println("Loaded configuration from guarduino.json");
    return true;
}


// Parse a MAC address string like "01:23:45:67:89:ab" or "01-23-45-67-89-ab" or "0123456789ab"
// Populate the global `mac[6]` array on success. Returns true on success.
static bool macStringToMacAddr(const char *macstr, size_t macstrSize) {
    if (!macstr || macstrSize <= 0) return false;
    uint8_t tmp[6] = {0,0,0,0,0,0};
    int nibbleCount = 0;

    for (size_t i = 0; i < macstrSize; ++i) {
        char c = macstr[i];
        if (c == ':' || c == '-' || c == '\0') continue;
        if (!isxdigit((unsigned char)c)) return false;
        int val = 0;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'a' && c <= 'f') val = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') val = 10 + (c - 'A');
        else return false;

        int byteIndex = nibbleCount / 2;
        if (byteIndex >= 6) return false; // too many nibbles
        if ((nibbleCount % 2) == 0) {
            tmp[byteIndex] = (uint8_t)(val << 4);
        } else {
            tmp[byteIndex] |= (uint8_t)val;
        }
        ++nibbleCount;
    }

    if (nibbleCount != 12) return false; // must be exactly 12 hex digits

    for (int i = 0; i < 6; ++i) {
        mac[i] = tmp[i];
    }
    return true;
}


// Check if this pin is one which is generally "reserved".
static bool isPinReserved(int pin) {
    // Reserved pins that cannot be used for sensors
    const int8_t reservedPins[] = {
        0, 1,           // Serial (RX/TX)
        SDCARD_CS_PIN,  // SD card CS (pin 4)
        ONE_WIRE_GPIO,  // OneWire for DS18x sensors (pin 8)
        10,             // Ethernet CS (W5x00 default)
        50, 51, 52, 53  // SPI bus (MISO, MOSI, SCK, SS)
    };
    const size_t reservedPinsCount = sizeof(reservedPins) / sizeof(reservedPins[0]);

    if (pin < 0) return false; // -1 means unused/unset
    for (size_t i = 0; i < reservedPinsCount; i++) {
        if (reservedPins[i] == pin) return true;
    }
    return false;
}

static sensorType sensorTypeFromString(const char *s) {
    if (!s) return unused;
    // Use bounded string comparison (max 32 chars for sensor type names)
    const size_t MAX_TYPE_LEN = 32;
    if (strncmp(s, "door2", MAX_TYPE_LEN) == 0) return door2;
    if (strncmp(s, "garagedoor2", MAX_TYPE_LEN) == 0) return garagedoor2;
    if (strncmp(s, "window2", MAX_TYPE_LEN) == 0) return window2;
    if (strncmp(s, "motion2", MAX_TYPE_LEN) == 0) return motion2;
    if (strncmp(s, "motion2_laser", MAX_TYPE_LEN) == 0) return motion2_laser;
    if (strncmp(s, "switch1", MAX_TYPE_LEN) == 0) return switch1;
    if (strncmp(s, "switch1_radiator", MAX_TYPE_LEN) == 0) return switch1_radiator;
    if (strncmp(s, "switch1_fan", MAX_TYPE_LEN) == 0) return switch1_fan;
    if (strncmp(s, "switch1_fire", MAX_TYPE_LEN) == 0) return switch1_fire;
    if (strncmp(s, "switch1_alarmlight", MAX_TYPE_LEN) == 0) return switch1_alarmlight;
    return unused;
}

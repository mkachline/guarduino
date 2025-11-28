#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "guarduino.h"
#include <ctype.h>

// Note: adjust this chip select pin to match your hardware's SD CS pin.
// Common values are 4 or 10 depending on shield/module.
static const uint8_t SDCARD_CS_PIN = 4;

// Forward declarations for helper functions used below.
static bool macStringToMacAddr(const char *macstr, size_t macstrSize);
static sensorType sensorTypeFromString(const char *s);

// Read the JSON config at `filepath` and populate globals. If file or SD unavailable, leave defaults.
void readSDConfig(const char *filepath) {
    // Clear globals that we'll populate
    memset(mqtt_password, '\0', sizeof(mqtt_password));
    memset(mqtt_username, '\0', sizeof(mqtt_username));
    mqtt_port = MQTT_DEFAULT_PORT;
    for (int i = 0; i < 6; ++i) mac[i] = 0;
    // zero sensors
    for (int i = 0; i < 64; ++i) { allSensors[i].type = unused; allSensors[i].pin1 = -1; allSensors[i].pin2 = -1; }

    if (!SD.begin(SDCARD_CS_PIN)) {
        Serial.println("SD.begin() failed - SD card unavailable or wrong CS pin");
        return;
    }

    File f = SD.open(filepath);
    if (!f) {
        Serial.println("guarduino.json not found on SD card");
        return;
    }

    // Read file into memory-limited JSON document
    const size_t capacity = 2048;
    StaticJsonDocument<capacity> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.print("Failed parse guarduino.json: ");
        Serial.println(err.c_str());
        return;
    }

    // macaddress
    if (doc.containsKey("macaddress")) {
        const char *macstr = doc["macaddress"];
        if (macstr) {
            if (!macStringToMacAddr(macstr, strlen(macstr))) {
                Serial.println("Invalid macaddress in guarduino.json");
            }
        }
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
    }

    if (doc.containsKey("mqtt_port")) {
        mqtt_port = doc["mqtt_port"].as<int>();
    }

    if (doc.containsKey("mqtt_username")) {
        const char *u = doc["mqtt_username"];
        if (u) strncpy(mqtt_username, u, sizeof(mqtt_username)-1);
    }

    if (doc.containsKey("mqtt_password")) {
        const char *p = doc["mqtt_password"];
        if (p) strncpy(mqtt_password, p, sizeof(mqtt_password)-1);
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
            allSensors[idx].type = sensorTypeFromString(type);
            allSensors[idx].pin1 = (int8_t)pin1;
            allSensors[idx].pin2 = (int8_t)pin2;
            idx++;
        }
        // remaining entries already set to unused above
    }

    Serial.println("Loaded configuration from guarduino.json");
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


static sensorType sensorTypeFromString(const char *s) {
    if (!s) return unused;
    if (strcmp(s, "door2") == 0) return door2;
    if (strcmp(s, "garagedoor2") == 0) return garagedoor2;
    if (strcmp(s, "window2") == 0) return window2;
    if (strcmp(s, "motion2") == 0) return motion2;
    if (strcmp(s, "motion2_laser") == 0) return motion2_laser;
    if (strcmp(s, "switch1") == 0) return switch1;
    if (strcmp(s, "switch1_radiator") == 0) return switch1_radiator;
    if (strcmp(s, "switch1_fan") == 0) return switch1_fan;
    if (strcmp(s, "switch1_fire") == 0) return switch1_fire;
    if (strcmp(s, "switch1_alarmlight") == 0) return switch1_alarmlight;
    return unused;
}

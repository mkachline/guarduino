#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "guarduino.h"

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

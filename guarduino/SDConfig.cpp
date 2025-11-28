#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <IniFile.h>
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
static void printDirectory(File dir, int numTabs);


// Read the INI config at `filepath` and populate globals. If file or SD unavailable, leave defaults.
bool readSDConfig(const char *filepath) {
    Sd2Card card;
    if(! card.init(SPI_HALF_SPEED, SDCARD_CS_PIN)) {
        Serial.println("Sd2Card.init() failed.");
        return false;
    }
    switch (card.type()) {
        case SD_CARD_TYPE_SD1:
          Serial.println("SD Card Type: SD1");
          break;

        case SD_CARD_TYPE_SD2:
          Serial.println("SD Card Type: SD2");
          break;

        case SD_CARD_TYPE_SDHC:
          Serial.println("SD Card Type: SDHC");
          break;

        default:
          Serial.println("SD Card Type: Unknown");
    }

    SdVolume volume;
    if (!volume.init(card)) {
        Serial.println("Could not find FAT16/FAT32 partition.");
        Serial.println("Please validte your SD card partition is formatted either FAT16 or FAT32.");
        Serial.println("Note: exFAT is NOT supported.");
        return false;
    } else {
        Serial.println("Found FAT16/FAT32 partition on SD Card.");
    }

    Serial.print("Initializing SD card using pin ");
    Serial.print(SDCARD_CS_PIN);
    Serial.println("...");
    for(int i = 0; i < 5; i++) {
        if (!SD.begin(SDCARD_CS_PIN)) {
            Serial.print(".");
            delay(2000);
            continue;
        } else {
            break;
        }
        if(i == 4) {
            Serial.println("SD.begin() failed to initialize.");
            return false;
        }   
    }

    // Validate File exists.
    if(SD.exists(filepath)) {
        Serial.print("Found SD Card File: ");
        Serial.println(filepath);
    } else {
        Serial.print("File: ");
        Serial.print(filepath);
        Serial.println(" not found on SD Card.");
        Serial.println(" Files Found on Card .....");
        File root = SD.open("/");
        printDirectory(root, 0);
        return false;
    }

    // Open the INI file
    const size_t bufferLen = 80;
    char buffer[bufferLen];
    IniFile ini(filepath);
    
    if (!ini.open()) {
        Serial.print("Cannot Open INI File: ");
        Serial.println(filepath);
        return false;
    }

    if (!ini.validate(buffer, bufferLen)) {
        Serial.print("INI file ");
        Serial.print(filepath);
        Serial.print(" not valid: ");
        Serial.println(buffer);
        ini.close();
        return false;
    }

    // Read macaddress from [network] section
    for (int i = 0; i < 6; ++i) mac[i] = 0; // Zero out Mac address
    if (ini.getValue("network", "macaddress", buffer, bufferLen)) {
        size_t maclen = strlen(buffer);
        if (!macStringToMacAddr(buffer, maclen)) {
            Serial.print("Invalid macaddress in ");
            Serial.println(filepath);
            ini.close();
            return false;
        }
        Serial.print("Read macaddress: ");
        Serial.println(buffer);
    } else {
        Serial.print("Warning: 'macaddress' missing from ");
        Serial.println(filepath);
        ini.close();
        return false;
    }

    // Read MQTT address
    mqtt_address = IPAddress(127, 0, 0, 1);
    memset(&mqtt_address, 0, sizeof(mqtt_address));
    if (ini.getValue("network", "mqtt_address", buffer, bufferLen)) {
        unsigned int a, b, c, d;
        if (sscanf(buffer, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            mqtt_address = IPAddress((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d);
            Serial.print("Read mqtt_address: ");
            Serial.println(buffer);
        } else {
            Serial.println("mqtt_address not a valid IPv4 string; using 0.0.0.0");
        }
    } else {
        Serial.print("Warning: 'mqtt_address' missing from ");
        Serial.println(filepath);
        ini.close();
        return false;
    }

    // Read MQTT port
    mqtt_port = MQTT_DEFAULT_PORT;
    if (ini.getValue("network", "mqtt_port", buffer, bufferLen)) {
        mqtt_port = atoi(buffer);
        Serial.print("Read mqtt_port: ");
        Serial.println(buffer);
    } else {
        Serial.print("Warning: 'mqtt_port' missing from ");
        Serial.print(filepath);
        Serial.print(" Assuming default port ");
        Serial.println(MQTT_DEFAULT_PORT);
    }

    // Read MQTT username
    memset(mqtt_username, '\0', sizeof(mqtt_username));
    if (ini.getValue("network", "mqtt_username", buffer, bufferLen)) {
        strncpy(mqtt_username, buffer, sizeof(mqtt_username) - 1);
        Serial.print("Read mqtt_username: ");
        Serial.println(buffer);
    } else {
        Serial.print("Warning: 'mqtt_username' missing from ");
        Serial.println(filepath);
        ini.close();
        return false;
    }

    // Read MQTT password
    memset(mqtt_password, '\0', sizeof(mqtt_password));
    if (ini.getValue("network", "mqtt_password", buffer, bufferLen)) {
        strncpy(mqtt_password, buffer, sizeof(mqtt_password) - 1);
        Serial.print("Read mqtt_password: ");
        Serial.println(buffer);
    } else {
        Serial.print("Warning: 'mqtt_password' missing from ");
        Serial.println(filepath);
        ini.close();
        return false;
    }

    // Read sensors - try each sensorN section from sensor0 to sensor63
    for (int i = 0; i < 64; ++i) { 
        allSensors[i].type = unused; 
        allSensors[i].pin1 = -1; 
        allSensors[i].pin2 = -1; 
    }

    int sensorCount = 0;
    for (int i = 0; i < 64; i++) {
        char sectionName[16];
        snprintf(sectionName, sizeof(sectionName), "sensor%d", i);
        
        // Check if this sensor section exists
        if (!ini.getValue(sectionName, "type", buffer, bufferLen)) {
            // No more sensors defined
            continue;
        }
        
        const char *type = buffer;
        
        // Read pin1
        int pin1 = -1;
        if (ini.getValue(sectionName, "pin1", buffer, bufferLen)) {
            pin1 = atoi(buffer);
        }
        
        // Read pin2
        int pin2 = -1;
        if (ini.getValue(sectionName, "pin2", buffer, bufferLen)) {
            pin2 = atoi(buffer);
        }
        
        // Skip sensors that use reserved pins
        if (isPinReserved(pin1) || isPinReserved(pin2)) {
            Serial.print("Skipping sensor on reserved pin(s): pin1=");
            Serial.print(pin1);
            Serial.print(", pin2=");
            Serial.println(pin2);
            continue;
        }
        
        allSensors[sensorCount].type = sensorTypeFromString(type);
        allSensors[sensorCount].pin1 = (int8_t)pin1;
        allSensors[sensorCount].pin2 = (int8_t)pin2;
        sensorCount++;
        
        // tell user a one-line summary of "this" sensor just read.
        Serial.print("Read sensor: type=");
        Serial.print(type);
        Serial.print(", pin1=");
        Serial.print(pin1);
        Serial.print(", pin2=");
        Serial.println(pin2);
    }
    
    ini.close();

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



static void printDirectory(File dir, int numTabs) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            // no more files
            break;
        }

        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }

        Serial.print(entry.name());

        if (entry.isDirectory()) {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        } else {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }

        entry.close();
    }
}



#include <Ethernet.h> // https://www.arduino.cc/reference/en/libraries/ethernet/
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/tree/master
#include <MemoryUsage.h> // https://github.com/Locoduino/MemoryUsage/tree/master
#include <OneWire.h> // https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include "guarduino.h"


#define MQTT_DEFAULT_PORT 1883

// Config values populated from SD card by readConfig_SD()
IPAddress mqtt_address; // Populated from guarduino.json (ipv4 string)
char mqtt_password[128];
int mqtt_port;
char mqtt_username[64];

// MAC address (6 bytes) populated by macStringToMacAddr()
byte mac[6] = {0,0,0,0,0,0};

// CHANGEME: Here is where you assign hardware pins to "sensor types."
// See also: https://github.com/mkachline/guarduino/wiki
// See also: https://content.arduino.cc/assets/Pinout-Mega2560rev3_latest.pdf
// Statically allocated sensor table (max 64). Populated by `readConfig_SD()`.
baseSensor_t allSensors[64] = { };
// No changes needed from here down.


OneWire oneWire(ONE_WIRE_GPIO);
EthernetClient ethClient;
PubSubClient pubsubClient(ethClient);
bool didCallback = false;

static uint64_t oldPinReadings = 0; // Here, we're assuming up to 64 pins will be read/tracked.
static unsigned long lastReadAt = millis();
#define HEARTBEAT (5 *1000) // If nothing happens, send a message every N milliseconds.


void setup() {    
    Serial.begin(115200);
    Serial.print("Start ");
    Serial.print(GUARDUINO_URL);
    Serial.print(" version: ");
    Serial.println(SOFTWARE_VERSION);

    // Set these pins for Ethernet and SDCard to cooporate.
    pinMode(4, OUTPUT);  // SD CS
    pinMode(10, OUTPUT); // Ethernet CS


    digitalWrite(4, HIGH); // SD Off
    digitalWrite(10, HIGH); // Ethernet Off
    if (!readSDConfig("CONFIG.INI")) {
        Serial.println("CONFIG.INI load error. Rebooting in 10 seconds...");
        delay(10000);
        asm volatile ("jmp 0");  // Reboot by jumping to address 0
    }
    digitalWrite(4, HIGH); // SD Off
    digitalWrite(10, HIGH); // Ethernet Off

    pubsubClient.setBufferSize( MQTT_MAX_PACKET_SIZE + 256);
    pubsubClient.setServer(mqtt_address, mqtt_port);
    pubsubClient.setCallback(mqttCallback);
    
    if (setupEthernet()) {
      setupSensors(allSensors, sizeof(allSensors));    
      setupDS18Sensors();

      oldPinReadings = readSensors(0, allSensors, sizeof(allSensors));
    }
    FREERAM_PRINT; // https://github.com/Locoduino/MemoryUsage/tree/master    
}



void loop() {      
    
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    FREERAM_PRINT; // https://github.com/Locoduino/MemoryUsage/tree/master    
    

    // Anything we need to do immediately?
    //bool pinChange = didPinsChange();
    bool didTimeout = ((millis() - lastReadAt) > HEARTBEAT);
    bool mqttConnected = pubsubClient.connected();    

    if(! mqttConnected) {
        Serial.println("MQTT NOT Connected");
        setupEthernet();
        pubsubReconnect();
        setupSensors(allSensors, sizeof(allSensors));        
        setupDS18Sensors();        
        return;
    }
    pubsubClient.loop();
    
    // Read digital pins.     
    uint64_t newPinReadings = 0;
    newPinReadings = readSensors(0, allSensors, sizeof(allSensors));
    bool didPinsChange = (oldPinReadings != newPinReadings);


    if(! mqttConnected) { return; };

    // Check for Timeout BEFORE pinschange
    if(didTimeout) {

      // Send Temps only on timeout.
      allds18x = readDS18xSensors();
      mqttds18xSendDiscovery(allds18x, allds18x_count);
      mqttds18xSendData(allds18x, allds18x_count);

      didPinsChange = true; // Assume this to trigger below.
    }

    // Check for callback BEFORE pinsChange
    if(didCallback) {
      newPinReadings = readSensors(newPinReadings, allSensors, sizeof(allSensors)); // Avoid race condition with callback.
      oldPinReadings = newPinReadings;
      didCallback = false;
      didPinsChange = true;
    }

    if(didPinsChange) {
      sendSensorsMQTT(newPinReadings, allSensors, sizeof(allSensors));
      //mqttSwitchSendData(newPinReadings);
      lastReadAt = millis();
      oldPinReadings = newPinReadings;      
    }

}


void pubsubReconnect(void) {

    char deviceName[24];
    getDeviceName(deviceName, sizeof(deviceName));

    if(! pubsubClient.connected()) {
      setupEthernet();
      pubsubClient.setServer(mqtt_address, mqtt_port);
      pubsubClient.setCallback(mqttCallback);
      if(! pubsubClient.connect(deviceName, mqtt_username, mqtt_password)) {
        Serial.print("Failed connect ");
        Serial.print(mqtt_username);
        Serial.print(":");
        Serial.print(mqtt_password);
        Serial.print("@");
        Serial.print(mqtt_address[0]);
        Serial.print(".");
        Serial.print(mqtt_address[1]);
        Serial.print(".");
        Serial.print(mqtt_address[2]);
        Serial.print(".");
        Serial.print(mqtt_address[3]);
        Serial.print(":");
        Serial.print(mqtt_port);
        Serial.println("");
        Serial.println(deviceName);

        return;
      }
    }

    
    // TODO: Send Discovery here.
    // setupSwitchSensors(); // Subscribe to MQTT Topics.
    mqttSensorSendDiscovery(0);
    //pubsubClient.subscribe(HA_TOPIC_DATA);    

}


void mqttCallback(char *topic, byte *payloadBytes, unsigned int length) {
    // Listen for changes in one of our Switches here.
    size_t charsLength = length + 1;
    char *payloadChars =  (char *) calloc(charsLength, sizeof(char));
    if(! payloadChars) { return; }
    memset(payloadChars, '\0', charsLength);
    strncpy(payloadChars, (const char *) payloadBytes, length);
    
    Serial.println("CALLBACK!!");    

    handleCallbackSwitches(payloadChars);    
    didCallback = true;

    free(payloadChars);
    payloadChars = NULL;
}



static bool setupEthernet(void) {

    // Validate "mac" is populated (not all zeros). Log such, and return false if not.
    bool macIsValid = false;
    for (size_t i = 0; i < 6; i++) {
        if (mac[i] != 0) {
            macIsValid = true;
            break;;
        }
    }
    if (!macIsValid) {
        Serial.println("Invalid MAC address; cannot setup Ethernet.");
        return false;
    }

    Ethernet.begin(mac);
    delay(2000); // Let DHCP do it's thing.

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet hardware not found.");
      return false;
    } else if (Ethernet.hardwareStatus() == EthernetW5100) {
      Serial.println("W5100 Ethernet controller detected.");
    } else if (Ethernet.hardwareStatus() == EthernetW5200) {
      Serial.println("W5200 Ethernet controller detected.");
    } else if (Ethernet.hardwareStatus() == EthernetW5500) {
      Serial.println("W5500 Ethernet controller detected.");
    }
    
    
    if(Ethernet.linkStatus() == Unknown) {
      Serial.println("Ethernet.linkStatus(): Unknown");
    }    
    if(Ethernet.linkStatus() == LinkON) {
      Serial.println("Ethernet.linkStatus(): On");
      return true;
    }
    if(Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet.linkStatus(): OFF");
    }

    return false;
}

int allSensorCount(void) {
  size_t totalsize = sizeof(allSensors);
  size_t onesize = sizeof(baseSensor_t);

  if(totalsize == 0) return 0;
  return(totalsize / onesize);
}

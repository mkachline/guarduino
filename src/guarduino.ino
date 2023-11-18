#include <Ethernet.h> // https://www.arduino.cc/reference/en/libraries/ethernet/
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/tree/master
#include <MemoryUsage.h> // https://github.com/Locoduino/MemoryUsage/tree/master
#include <OneWire.h> // https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include "guarduino.h"


#define MQTT_ADDRESS IPAddress(192,168,1,5) // CHANGEME: Your Mosquitto Server IP Address
#define MQTT_PASSWORD "xxxxxxxxxxxxxxxxxxxxxxxxxxxxeechai8aeniiZen6hiagoXoxxxxxxxxxxxxx" // HA | Settings | Devices | MQTT | Configure | "Re-Configure MQTT" | Password
#define MQTT_PORT 1883 // HA | Settings | Devices | MQTT | Configure | "Re-Configure MQTT" | Port
#define MQTT_USERNAME "homeassistant" // HA | Settings | Devices | MQTT | Configure | "Re-Configure MQTT" | Username

#define MACADDRESS { 0x00, 0xEA, 0xBB, 0xCC, 0x01, 0x05  } // Mac Address for your Guarduino.

// CHANGEME: Here is where you assign hardware pins to "sensor types."
// See also: https://github.com/mkachline/guarduino/wiki
// See also: https://content.arduino.cc/assets/Pinout-Mega2560rev3_latest.pdf
#define ONE_WIRE_GPIO 8 // Don't change this unless you have a good reason.
baseSensor allSensors[] = {
/* { sensortype, pin1, pin2 } */  
  {door2, 22, 23},  
  {door2, 24, 25},  
  {door2, 26, 27},
  //{door2, 28, 29},
  {motion2, 30, 31}, 
  {motion2, 32, 33},
  {motion2, 34, 35},
  //{window2, 36, 37},
  {window2, 38, 39}, 
  {window2, 40, 41},
  {window2, 42, 43}, 
  {window2, 44, 45}, 
  //{unused, 46, 47}, 
  //{unused, 48, 49}, 

  //{unused, A0, A1},
  //{unused, A2, A3}, 
  //{unused, A4, A5},
  //{unused, A6, A7},  
  //{unused, A8, A9},   
  //{unused, A10, A11},  
  
  //{unused, A12, -1},
  //{unused, A13, -1},
  //{unused, A14, -1},
  //{unused, 10, 11},
  //{unused, 12, 13},
  {switch1_radiator, 7, -1},


  {reserved, 0, 1}, // Comms
  {reserved, ONE_WIRE_GPIO, -1}, // Pin 8
  {reserved, LED_BUILTIN, -1}, // On-board LED, as defined in arduino.h
  {reserved, 14, 15}, // Comms
  {reserved, 16, 17}, // Comms
  {reserved, 18, 19}, // Comms
  {reserved, 20, 21}, // Comms
  {reserved, 50, 51}, // SPI
  {reserved, 52, 53}, // SPI
};
// No changes needed from here down.


OneWire oneWire(ONE_WIRE_GPIO);
byte mac[] = MACADDRESS;
EthernetClient ethClient;
PubSubClient pubsubClient(ethClient);
bool didCallback = false;

static uint64_t oldPinReadings = 0; // Here, we're assuming up to 64 pins will be read/tracked.
static unsigned long lastReadAt = millis();
#define HEARTBEAT (5 *1000) // If nothing happens, send a message every N milliseconds.


void setup() {    
    Serial.begin(19200);
    Serial.print("Start ");
    Serial.print(GUARDUINO_URL);
    Serial.print(" version: ");
    Serial.println(SOFTWARE_VERSION);
    pubsubClient.setBufferSize( MQTT_MAX_PACKET_SIZE + 256);
    pubsubClient.setServer(MQTT_ADDRESS, MQTT_PORT);
    pubsubClient.setCallback(mqttCallback);
    
    setupEthernet();
    setupSensors(allSensors, sizeof(allSensors));    
    setupDS18Sensors();

    oldPinReadings = readSensors(0, allSensors, sizeof(allSensors));   
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
      pubsubClient.setServer(MQTT_ADDRESS, MQTT_PORT);
      pubsubClient.setCallback(mqttCallback);
      if(! pubsubClient.connect(deviceName, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.print("Failed connect ");
        Serial.print(MQTT_USERNAME);
        Serial.print(":");
        Serial.print(MQTT_PASSWORD);
        Serial.print("@");
        Serial.print(MQTT_ADDRESS);
        Serial.print(":");
        Serial.print(MQTT_PORT);
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
    char *payloadChars = malloc(charsLength * sizeof(char));
    if(! payloadChars) { return; }
    memset(payloadChars, '\0', charsLength);
    strncpy(payloadChars, payloadBytes, length);
    
    Serial.println("CALLBACK!!");    

    handleCallbackSwitches(payloadChars);    
    didCallback = true;

    free(payloadChars);
    payloadChars = NULL;
}



static void setupEthernet(void) {

    Ethernet.begin(mac);
    delay(2000); // Let DHCP do it's thing.
   
    if(Ethernet.linkStatus() == Unknown) {
      Serial.println("Ethernet.linkStatus(): Unknown");
    }    
    if(Ethernet.linkStatus() == LinkON) {
      Serial.println("Ethernet.linkStatus(): On");
    }
    if(Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet.linkStatus(): OFF");
    }
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.");
    } else if (Ethernet.hardwareStatus() == EthernetW5100) {
      Serial.println("W5100 Ethernet controller detected.");
    } else if (Ethernet.hardwareStatus() == EthernetW5200) {
      Serial.println("W5200 Ethernet controller detected.");
    } else if (Ethernet.hardwareStatus() == EthernetW5500) {
      Serial.println("W5500 Ethernet controller detected.");
    }
}

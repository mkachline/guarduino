#include "guarduino.h"
#include <OneWire.h> // https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library

extern EthernetClient ethClient;
extern PubSubClient pubsubClient;
static char *getds18xStateTopic(ds18x_t thisds18x);
static size_t mqttds18xDiscovery(ds18x_t thisds18x, size_t paramSize);


/**
 * DS18x "1-Wire" Temperature Sensors
 */
 extern OneWire oneWire;
DallasTemperature sensors(&oneWire);
unsigned char allds18x_count = 0;
ds18x_t *allds18x = NULL;




void setupDS18Sensors(void) {
  sensors.begin(); // Dallas One-Wire
}


ds18x_t *readDS18xSensors(void) {  
  DeviceAddress thisDeviceAddress;
  char addressname[24];
  char sensorName[64];
  char *entityName = NULL;
  ds18x_t *newds18x = NULL;
  
  allds18x_count = sensors.getDS18Count(); // global variable
  if(allds18x_count > 0) { 
    newds18x = malloc(allds18x_count * sizeof(ds18x_t));
    memset(newds18x, '\0', allds18x_count);
    for(int i = 0; i < allds18x_count; i++) {
      ds18x_t *thisds18x = &newds18x[i];

      if (! sensors.getAddress(thisds18x->address, i)) continue;

      sensors.requestTemperaturesByAddress(thisds18x->address);
      float tempF = sensors.getTempF(thisds18x->address);
      thisds18x->temp_f = tempF;
    }
  }

  if(allds18x != NULL) { // global variable
    free(allds18x);
  }
  allds18x = newds18x;
  return allds18x;
}


/*
 * Populates destbuf with the text version of whatever this ds18x "address" is.
 * Inspired by: https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/Multiple/Multiple.ino ::printAddress()
 */
static void ds18xName(char *destbuf, size_t destbufsize, ds18x_t thisds18x) {
    memset(destbuf, '\0', destbufsize);
    
    for(uint8_t i = 0; i < 8; i++) {
      char thisbyte[4];
      memset(thisbyte, '\0', sizeof(thisbyte));
      if(thisds18x.address[i] < 16) {
        strlcat(thisbyte, "00", sizeof(thisbyte));
      } else {
        sprintf(thisbyte, "%02x", thisds18x.address[i]);
      }
      strlcat(destbuf, thisbyte, (destbufsize-1));
    }
}


/*
 * Sends MQTT messages for each ds18x sensor we know about.
 * TODO: Include an attribute sensors.isParasitePowerMode()
 */
void mqttds18xSendData(ds18x_t *allds18x, unsigned int ds18xcount) {
  Serial.println("mqttds18xSendData()");
  
  for(int i = 0; i < ds18xcount; i++) {
    ds18x_t *thisds18x = &allds18x[i];

    // Arduino's "sprintf()" doesn't handle floats, hence the silliness here.
    float tempF = thisds18x->temp_f;
    int intval = int(tempF);
    int fractionval = (int(100*(tempF - int(tempF)))); // https://forum.arduino.cc/t/sprintf-a-float-number/1013193
    char tempString[8];
    memset(tempString, '\0', sizeof(tempString));
    snprintf(tempString, sizeof(tempString) - 1, "%d.%02d", intval, fractionval);
    

    // Send MQTT DATA here.
    char *ds18xStateTopic = getds18xStateTopic(*thisds18x);
    if(ds18xStateTopic) {
      Serial.print("SEND ");
      Serial.print(ds18xStateTopic);
      Serial.print(" ");
      Serial.print(tempString);
      Serial.println("\n");
      pubsubClient.publish(ds18xStateTopic, tempString, false);
      ethClient.flush();
      free(ds18xStateTopic);
      ds18xStateTopic = NULL;
    }
  }


}



/**
 * For each ds18x found on the 1-Wire bus
  Send Discovery to HomeAssistant MQTT.
  // https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  // https://www.home-assistant.io/integrations/sensor.mqtt/
  // https://community.home-assistant.io/t/mqtt-auto-discovery-and-json-payload/409459
*/
void mqttds18xSendDiscovery(ds18x_t *allds18x, unsigned int ds18xcount) {
  Serial.println("mqttds18xSendDiscovery()");
  char sensorName[64];
  
  for(int i = 0; i < ds18xcount; i++) {
    ds18x_t *thisds18x = &allds18x[i];
    mqttds18xDiscovery(*thisds18x, 0);
  }
}



/*
 * Returns malloc'ed string to the tune of ...
 *
 * Examples:
 * aha/sensor/deviceName/ds18xaddressname/state 
 */
static char *getds18xStateTopic(ds18x_t thisds18x) {
  char deviceName[24];
  getDeviceName(deviceName, sizeof(deviceName));
  
  char sensorName[24];
  ds18xName(sensorName, sizeof(sensorName), thisds18x);

  size_t topicsize = 0;
  topicsize += strlen(HA_TOPIC_DATA);
  topicsize += strlen("/sensor/");
  topicsize += strlen(deviceName);
  topicsize += strlen("/");
  topicsize += strlen(sensorName);
  topicsize += strlen("/state");
  topicsize += 4; // Safety margin.

  char *topic = (char *) malloc(topicsize * sizeof(char));
  if(topic) {
    memset(topic, '\0', topicsize);
    snprintf(topic, topicsize - 1, "%s/sensor/%s/%s/state", HA_TOPIC_DATA, deviceName, sensorName);
  }

  return topic;
}


/**
 *
 * https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
 * "Discovery Topic" section
 *
 * Examples:
 * homeassistant/sensor/ds18xnamehere/config
 */
static char *getds18xDiscoveryTopic(ds18x_t thisds18x) {
  char sensorName[24];
  ds18xName(sensorName, sizeof(sensorName), thisds18x);

  size_t topicsize = 0;
  topicsize += strlen(HA_TOPIC_DISCOVERY);
  topicsize += strlen("/sensor/");
  topicsize += strlen(sensorName);
  topicsize += strlen("/config");
  topicsize += 4; // Safety buffer
  char *topic = malloc(topicsize * sizeof(char));
  memset(topic, '\0', topicsize);
  snprintf(topic, topicsize, "%s/sensor/%s/config", HA_TOPIC_DISCOVERY, sensorName);

  return topic;  
}



/*
 * {"device_class": "temperature", "name": "Temperature", "state_topic": "homeassistant/sensor/sensorBedroom/state", "unit_of_measurement": "°C", "value_template": "{{ value_json.temperature}}","unique_id": "temp01ae", "device": {"identifiers": ["bedroom01ae"], "name": "Bedroom" }}
 */
static size_t mqttds18xDiscovery(ds18x_t thisds18x, size_t paramSize = 0) {
  char buffer[64];
  size_t payloadsize = 0;

  Serial.println("");
  Serial.print("mqttds18xDiscovery paramSize=");
  Serial.print(paramSize);
  Serial.print("  ");



  char deviceName[24];
  getDeviceName(deviceName, sizeof(deviceName));

  char sensorName[32];
  ds18xName(sensorName, sizeof(sensorName), thisds18x);


  if(paramSize > 0) { 
    // Tell MQTT how many bytes are about to come for this discovery topic.
    char *ds18xDiscoveryTopic = getds18xDiscoveryTopic(thisds18x);
    if(! ds18xDiscoveryTopic) return 0;
    pubsubClient.beginPublish(ds18xDiscoveryTopic, paramSize, false); // Here, we must give our pre-computed "paramsize"
    free(ds18xDiscoveryTopic);
    ds18xDiscoveryTopic = NULL;
  }

  // Opening Bracket
  payloadsize += mqttsend((paramSize>0), "{");
  

  // (Entity) Name
  // No loading comma on first key:value pair  
  payloadsize += mqttsend((paramSize>0), "\"name\":\"");
  payloadsize += mqttsend((paramSize>0), sensorName);
  payloadsize += mqttsend((paramSize>0), "\"");


  // Device Class
  payloadsize += mqttsend((paramSize>0), ", \"device_class\":\"temperature\"");


  // State Topic
  char *sensorStateTopic = getds18xStateTopic(thisds18x);
  if(sensorStateTopic) {
    payloadsize += mqttsend((paramSize>0), ",");
    payloadsize += mqttsend((paramSize>0), "\"state_topic\":\"");
    payloadsize += mqttsend((paramSize>0), sensorStateTopic);
    payloadsize += mqttsend((paramSize>0), "\"");
    free(sensorStateTopic);
    sensorStateTopic = NULL;
  }

  // Icon
  // https://pictogrammers.com/library/mdi/
  payloadsize += mqttsend((paramSize>0), ", \"icon\":\"mdi:thermometer\"");


  // Unique ID
  payloadsize += mqttsend((paramSize>0), ",");
  payloadsize += mqttsend((paramSize>0), "\"unique_id\":\"");
  payloadsize += mqttsend((paramSize>0), sensorName);
  payloadsize += mqttsend((paramSize>0), "\"");

  


  // Device (Json sub-object)
  char *devicePayload = getDeviceDiscoveryPayload();
  if(devicePayload) {
    payloadsize += mqttsend((paramSize>0), ",");
    payloadsize += mqttsend((paramSize>0), "\"device\": ");
    payloadsize += mqttsend((paramSize>0), devicePayload);
    free(devicePayload);
    devicePayload = NULL;
  }
  
  
  // JSON closing paretheses
  payloadsize += mqttsend((paramSize>0), "}");



  if(paramSize == 0) {
    // Now that we know our total payload size, call ourselves again with that size.
    // That second "go-around", we'll actually talk to MQTT.
    return mqttds18xDiscovery(thisds18x, payloadsize);
  } else {
    pubsubClient.endPublish();
    Serial.println("");
  }


  return payloadsize;
}


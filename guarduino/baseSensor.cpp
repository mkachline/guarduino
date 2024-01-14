#include <Ethernet.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/tree/master
#include <Board_Identify.h> // https://github.com/MattFryer/Board_Identify
#include "guarduino.h"


static size_t mqttSensorDiscovery(baseSensor_t thisSensor, uint64_t pinReadings, size_t paramSize);
static sensorStates getSensorStateEnum(baseSensor_t sensor, uint64_t pinReadings);
static const char *getSensorStateName(baseSensor_t sensor, uint64_t pinReadings);
static const char *getSensorStateIcon(baseSensor_t sensor, uint64_t pinReadings);






/**
 * Step through each of "allSensors" and send "discovery/data" pairs to MQTT, based on readings found
 * in "pinReadings". Ignores any sensors of status 'offline'.
 */
void sendSensorsMQTT(uint64_t pinReadings, baseSensor_t *allSensors, size_t allSensorsSize) {
  for(int i = 0; i < (allSensorsSize / sizeof(baseSensor_t)); i++) {
    baseSensor_t thisSensor = allSensors[i];

    switch(thisSensor.type) {      
      case reserved:
        continue;
        break;

      case door2:
      case garagedoor2:
      case window2:
      case motion2:
      case motion2_laser:
      case switch1:
      case switch1_radiator:
      case switch1_fan:
      case switch1_fire:
      case switch1_alarmlight:

        sensorStates thisState = getSensorStateEnum(thisSensor, pinReadings);
        if(thisState == unknown) continue;
        if(thisState == door2_offline) continue;
        if(thisState == garagedoor2_offline) continue;
        if(thisState == window2_offline) continue;
        if(thisState == motion2_offline) continue;

        // Send "Discovery" first. This births the entity on the HA device. This also 'sets' the icon according to pinReadings.   
        mqttSensorDiscovery(thisSensor, pinReadings, 0);

        // Send "the Reading" for this sensor.    
        char *sensorStateTopic = getSensorStateTopic(thisSensor);
        if(sensorStateTopic) {
          char thisName[48];
          getSensorName(thisName, sizeof(thisName), thisSensor);
          const char *reading = getSensorStateName(thisSensor, pinReadings);    
          pubsubClient.publish(sensorStateTopic, reading, false);
          free(sensorStateTopic);
          sensorStateTopic = NULL;
        }   
        break;
    } // thisSensor.type

  } // thisSensor
}




/**
 * For each door in our 'allSensors' array
  Send Discovery to HomeAssistant MQTT.
  // https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  // https://www.home-assistant.io/integrations/sensor.mqtt/
  // https://community.home-assistant.io/t/mqtt-auto-discovery-and-json-payload/409459
*/
void mqttSensorSendDiscovery(uint64_t pinReadings) {
  Serial.println("mqttSensorSendDiscovery()");
  char sensorName[32];
  
  
  for(int i = 0; i < allSensorCount(); i++) {
    baseSensor_t *thisSensor = &allSensors[i];    
    mqttSensorDiscovery(*thisSensor, pinReadings, 0);
  }
}


/**
  * Arduino_MACADDR
  * Board make + last 3 bytes of mac address.
  */
void getDeviceName(char *destbuf, size_t destbufsize) {
  memset(destbuf, '\0', destbufsize);
  byte macBytes[6];
  Ethernet.MACAddress(macBytes); // https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.macaddress/  
  snprintf(destbuf, destbufsize, "%s_%02X%02X%02X", BoardIdentify::make, macBytes[3], macBytes[4], macBytes[5]);
  
}


void getSensorName(char *destbuf, size_t destbufsize, baseSensor_t thisSensor) {
  if(destbufsize < strlen("someplaceholder_NNNN_NNNNNN")) return;
  
  byte macBytes[6];
  Ethernet.MACAddress(macBytes); // https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.macaddress/  
  snprintf(destbuf, destbufsize, "%s_%02X%02X%02X", BoardIdentify::make, macBytes[3], macBytes[4], macBytes[5]);
 
  memset(destbuf, '\0', destbufsize);
  switch(thisSensor.type) {
    case door2:
      snprintf(destbuf, destbufsize, "door_%02d%02d_%02X%02X%02X", thisSensor.pin1, thisSensor.pin2, macBytes[3], macBytes[4], macBytes[5]);
      break;
    case garagedoor2:
      snprintf(destbuf, destbufsize, "garagedoor_%02d%02d_%02X%02X%02X", thisSensor.pin1, thisSensor.pin2, macBytes[3], macBytes[4], macBytes[5]);
      break;    
    case window2:
      snprintf(destbuf, destbufsize, "window_%02d%02d_%02X%02X%02X", thisSensor.pin1, thisSensor.pin2, macBytes[3], macBytes[4], macBytes[5]);
      break;
    case motion2:
      snprintf(destbuf, destbufsize, "motion_%02d%02d_%02X%02X%02X", thisSensor.pin1, thisSensor.pin2, macBytes[3], macBytes[4], macBytes[5]);
      break;
    case motion2_laser:
      snprintf(destbuf, destbufsize, "laser_%02d%02d_%02X%02X%02X", thisSensor.pin1, thisSensor.pin2, macBytes[3], macBytes[4], macBytes[5]);
      break;

    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      snprintf(destbuf, destbufsize, "switch_%02d_%02X%02X%02X", thisSensor.pin1, macBytes[3], macBytes[4], macBytes[5]);
      break;
    default:
      break;
  }  

}


/*
 *
 * https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
 * "Discovery Topic" section
 *
 * Examples:
 * homeassistant/binary_sensor/garden/config
 *
 */
static char *getSensorDiscoveryTopic(baseSensor_t thisSensor) {   
  char sensorName[48];
  getSensorName(sensorName, sizeof(sensorName), thisSensor);

  size_t topicsize = 0;
  topicsize += strlen(HA_TOPIC_DISCOVERY);
  topicsize += strlen("/need_need_enough_space_here_for_sensor_name/");
  topicsize += strlen(sensorName);
  topicsize += strlen("/config");
  topicsize += 1;  
  char *topic = (char *) calloc(topicsize, sizeof(char));
  memset(topic, '\0', topicsize);


  switch(thisSensor.type) {
    case reserved:
      break;
      
    case door2:
    case garagedoor2:
    case window2:
    case motion2:
    case motion2_laser:
      snprintf(topic, topicsize, "%s/sensor/%s/config", HA_TOPIC_DISCOVERY, sensorName);
      break;

    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      snprintf(topic, topicsize, "%s/switch/%s/config", HA_TOPIC_DISCOVERY, sensorName);
      break;

    default:
      snprintf(topic, topicsize, "%s/notsupported/%s/config", HA_TOPIC_DISCOVERY, sensorName);
      break;

  }

  return topic;  
}




/**
 *
 * Examples:
 * aha/sensor/deviceNameHere/door_0809/state
 * aha/sensor/sensorBedroom/state
 * aha/switch/switch_NN/state/sw
 */

char *getSensorStateTopic(baseSensor_t thisSensor) {

  char deviceName[32];
  getDeviceName(deviceName, sizeof(deviceName));

  char sensorName[48];
  getSensorName(sensorName, sizeof(sensorName), thisSensor);


  size_t topicsize = 0;
  topicsize += strlen(HA_TOPIC_DATA);
  topicsize += strlen("/sensororswitch/");
  topicsize += strlen(deviceName);
  topicsize += strlen("/");
  topicsize += strlen(sensorName);
  topicsize += strlen("/state");
  topicsize += 4; // Safety margin.

  char *topic = (char *) calloc(topicsize, sizeof(char));
  if(topic) {
    memset(topic, '\0', topicsize);
    switch(thisSensor.type) {
      case switch1:
      case switch1_radiator:
      case switch1_fan:
      case switch1_fire:
      case switch1_alarmlight:
        snprintf(topic, topicsize - 1, "%s/switch/%s/%s/state", HA_TOPIC_DATA, deviceName, sensorName);
        break;
      default:
        snprintf(topic, topicsize - 1, "%s/sensor/%s/%s/state", HA_TOPIC_DATA, deviceName, sensorName);
        break;
      
    }
  }

  return topic;
}


/**
 * Used for switches. 
 * This is the topic which HA will publish to, and our switch should listen to, for "on/off" commands.
 * https://www.home-assistant.io/integrations/switch.mqtt#command_topic
 *
 */
char *getDeviceCommandTopic(void) {

  char deviceName[24];
  getDeviceName(deviceName, sizeof(deviceName));

  size_t topicsize = 0;
  topicsize += strlen(HA_TOPIC_DATA);
  topicsize += strlen("/switch/");
  topicsize += strlen(deviceName);
  topicsize += strlen("/cmd");
  topicsize += 4; // Safety margin.

  char *topic = (char *) calloc(topicsize, sizeof(char));
  snprintf(topic, topicsize - 1, "%s/switch/%s/cmd", HA_TOPIC_DATA, deviceName);

  return topic;
}




/*
 * Sends an HA specific MQTT "Discovery" packet for the given sensor. This is used by HA to "auto-learn" about this particular sensor.
 * 
 * This function calls itself recursively!
 * The first call (what you should do), paramSize=0, thus meaning "just calculate how many bytes you'll send."
 * The second call, "paramSize" is the number of bytes calculated above, in which case, we'll pubsubClient.write() as we go.
 * This recursive nature is done to optimize for minimal memory usage.
 * https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
 * https://www.home-assistant.io/integrations/sensor.mqtt/
 * https://community.home-assistant.io/t/mqtt-auto-discovery-and-json-payload/409459
 *
 * Example Payload sent:
 * {"device_class": "temperature", "name": "Temperature", "state_topic": "homeassistant/sensor/sensorBedroom/state", "unit_of_measurement": "°C", "value_template": "{{ value_json.temperature}}","unique_id": "temp01ae", "device": {"identifiers": ["bedroom01ae"], "name": "Bedroom" }}
 */
static size_t mqttSensorDiscovery(baseSensor_t thisSensor, uint64_t pinReadings, size_t paramSize = 0) {
  char buffer[64];
  size_t payloadsize = 0;
  const bool shouldSend = (paramSize > 0);


  // Do NOT send discovery (at all) if there is "no power" on this particular sensor.
  sensorStates thisState = getSensorStateEnum(thisSensor, pinReadings);
  if(thisState == unknown) return 0;
  if(thisState == door2_offline) return 0;
  if(thisState == garagedoor2_offline) return 0;
  if(thisState == window2_offline) return 0;
  if(thisState == motion2_offline) return 0;


  Serial.println("");
  Serial.print("mqttSensorDiscovery paramSize=");
  Serial.print(paramSize);
  Serial.print(" thisState=");
  Serial.print(thisState);
  Serial.print("  ");

  
  if(paramSize > 0) { 
    // Tell MQTT how many bytes are about to come for this discovery topic.
    char *sensorDiscoveryTopic = getSensorDiscoveryTopic(thisSensor);
    if(! sensorDiscoveryTopic) return 0;
    pubsubClient.beginPublish(sensorDiscoveryTopic, paramSize, false); //Must send pre-computed "paramsize"here.
    free(sensorDiscoveryTopic);
    sensorDiscoveryTopic = NULL;
  }
  
  char deviceName[24];
  getDeviceName(deviceName, sizeof(deviceName));

  char sensorName[48];
  getSensorName(sensorName, sizeof(sensorName), thisSensor);


  // Opening parenthesis
  payloadsize += mqttsend(shouldSend, "{");
  

  // https://www.home-assistant.io/integrations/sensor.mqtt/#name
  // No leading comma on first key:value pair
  // Plain english name.
  memset(buffer, '\0', sizeof(buffer));
  switch(thisSensor.type) {
    case door2:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s Door NOPin:%02d NCPin:%02d\"", deviceName, thisSensor.pin1, thisSensor.pin2);
      break;
    case garagedoor2:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s GarageDoor NOPin:%02d NCPin:%02d\"", deviceName, thisSensor.pin1, thisSensor.pin2);
      break;      
    case window2:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s Window NOPin:%02d NCPin:%02d\"", deviceName, thisSensor.pin1, thisSensor.pin2);
      break;
    case motion2:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s Motion Pin:%02d PwrSns:%02d\"", deviceName, thisSensor.pin1, thisSensor.pin2);
      break;    
    case motion2_laser:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s Laser Pin:%02d PwrSns:%02d\"", deviceName, thisSensor.pin1, thisSensor.pin2);
      break;

    case switch1:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s Switch Pin:%02d\"", deviceName, thisSensor.pin1);
      break;
    case switch1_radiator:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s RadiatorSwitch Pin:%02d\"", deviceName, thisSensor.pin1);
      break;
    case switch1_fan:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s FanSwitch Pin:%02d\"", deviceName, thisSensor.pin1);
      break;
    case switch1_fire:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s FireSwitch Pin:%02d\"", deviceName, thisSensor.pin1);
      break;
    case switch1_alarmlight:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s AlarmSwitch Pin:%02d\"", deviceName, thisSensor.pin1);
      break;

    default:
      snprintf(buffer, sizeof(buffer) - 1, "\"name\":\"%s Sensor Pin1:%02d Pin2:%02d\"", deviceName, thisSensor.pin1, thisSensor.pin2);
      break;
  }  
  payloadsize += mqttsend(shouldSend, buffer);

  
  // https://www.home-assistant.io/integrations/sensor.mqtt/#unique_id
  payloadsize += mqttsend(shouldSend, ",");
  payloadsize += mqttsend(shouldSend, "\"unique_id\":\"");
  payloadsize += mqttsend(shouldSend, deviceName);
  payloadsize += mqttsend(shouldSend, "_");
  payloadsize += mqttsend(shouldSend, sensorName);
  payloadsize += mqttsend(shouldSend, "\"");

  // https://www.home-assistant.io/integrations/sensor.mqtt/#object_id
  payloadsize += mqttsend(shouldSend, ",");
  payloadsize += mqttsend(shouldSend, "\"object_id\":\"");
  payloadsize += mqttsend(shouldSend, deviceName);
  payloadsize += mqttsend(shouldSend, "_");
  payloadsize += mqttsend(shouldSend, sensorName);
  payloadsize += mqttsend(shouldSend, "\"");
  
  
  // https://www.home-assistant.io/integrations/sensor.mqtt/#state_topic
  char *sensorStateTopic = getSensorStateTopic(thisSensor);
  if(sensorStateTopic) {
    payloadsize += mqttsend(shouldSend, ",");
    payloadsize += mqttsend(shouldSend, "\"state_topic\":\"");
    payloadsize += mqttsend(shouldSend, sensorStateTopic);
    payloadsize += mqttsend(shouldSend, "\"");

    free(sensorStateTopic);
    sensorStateTopic = NULL;
  }

  // https://www.home-assistant.io/integrations/switch.mqtt#command_topic
  // Switches only
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      char *deviceCommandTopic = getDeviceCommandTopic();
      if(deviceCommandTopic) {
        payloadsize += mqttsend(shouldSend, ",");
        payloadsize += mqttsend(shouldSend, "\"command_topic\":\"");
        payloadsize += mqttsend(shouldSend, deviceCommandTopic);
        payloadsize += mqttsend(shouldSend, "\"");      
        free(deviceCommandTopic);
        deviceCommandTopic = NULL;
      }
      break;
    default:
      break;
  }

  // https://www.home-assistant.io/integrations/switch.mqtt#optimistic
  // Switches only
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"optimistic\":false");
      break;
    default:
      break;
  }

  // https://www.home-assistant.io/integrations/switch.mqtt#payload_off
  // Switches Only
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"payload_off\":\"");
      payloadsize += mqttsend(shouldSend, switchValueOFF(thisSensor));
      payloadsize += mqttsend(shouldSend, "\"");
      break;
    default:
      break;
  }


  // https://www.home-assistant.io/integrations/switch.mqtt#payload_on
  // Switches Only
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"payload_on\":\"");
      payloadsize += mqttsend(shouldSend, switchValueON(thisSensor));
      payloadsize += mqttsend(shouldSend, "\"");
      break;
    default:
      break;
  }


  // https://www.home-assistant.io/integrations/switch.mqtt#state_off
  // Switches Only
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"state_off\":\"OFF\"");
      break;
    default:
      break;
  }

  // https://www.home-assistant.io/integrations/switch.mqtt#state_on
  // Switches Only
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"state_on\":\"ON\"");
      break;
    default:
      break;
  }

  // https://www.home-assistant.io/integrations/sensor.mqtt/#device_class
  // Always 'null' unless needed otherwise.
  switch(thisSensor.type) {
    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"device_class\":\"switch\"");
      break;
    default:
      payloadsize += mqttsend(shouldSend, ",");
      payloadsize += mqttsend(shouldSend, "\"device_class\":null");
      break;
  }
  
  
  // https://www.home-assistant.io/integrations/sensor.mqtt/#expire_after  
  payloadsize += mqttsend(shouldSend, ",");
  payloadsize += mqttsend(shouldSend, "\"expire_after\":60");
  
  
  // Icon
  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, "\"icon\":\"%s\"", getSensorStateIcon(thisSensor, pinReadings));  
  if(strlen(buffer) > 0) {
    payloadsize += mqttsend(shouldSend, ",");
    payloadsize += mqttsend(shouldSend, buffer);    
  }

  // Device
  char *devicePayload = getDeviceDiscoveryPayload();
  if(devicePayload) {
    payloadsize += mqttsend(shouldSend, ",");
    payloadsize += mqttsend(shouldSend, "\"device\":");
    payloadsize += mqttsend(shouldSend, devicePayload);  
    
    free(devicePayload);
    devicePayload = NULL;
  }
  
  // Ending Bracket
  payloadsize += mqttsend(shouldSend, "}");


  if(paramSize == 0) {
    // Now that we know our total payload size, call ourselves again with that size.
    // That second "go-around", we'll actually talk to MQTT.
    return mqttSensorDiscovery(thisSensor, pinReadings, payloadsize);
  } else {
    pubsubClient.endPublish();
    Serial.println("");
  }
  
  return payloadsize;
}





/*
 * https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
 * Supported Abbreviations for device registry configuration
 *
 * Examples:
 * {"identifiers": ["bedroom01ae"], "name": "Bedroom" }
 * {"identifiers": ["bedroom01ae"], "name": "Bedroom" }
 * {"identifiers": ["garden01ad"], "name": "Garden" }
 */
char *getDeviceDiscoveryPayload(void) {
  char buffer[80]; // "Connections" is our longest string to fit here.
  IPAddress myIP = Ethernet.localIP();
  size_t jsonsize = 8;
  char *json = (char *) calloc(jsonsize,  sizeof(char));
  memset(json, '\0', jsonsize);


  char deviceName[32];
  getDeviceName(deviceName, sizeof(deviceName));
  // TODO: MAC Address Here.


  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, "{");
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);


  // NO LEADING comma, on our FIRST key/value pair.
  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, "\"identifiers\":[\"%s\"]", deviceName); // TODO Incorporate MAC Address
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);

  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, ", \"name\":\"%s\"", deviceName);
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);

  //https://www.home-assistant.io/integrations/sensor.mqtt/#model
  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, ", \"model\":\"%d.%d.%d.%d\"", myIP[0], myIP[1], myIP[2], myIP[3]);
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);
  
  //https://www.home-assistant.io/integrations/sensor.mqtt/#hw_version
  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, ", \"hw_version\":\"%s\"", BoardIdentify::model);
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);
  
  //https://www.home-assistant.io/integrations/sensor.mqtt/#manufacturer
  memset(buffer, '\0', sizeof(buffer));
  //snprintf(buffer, sizeof(buffer) - 1, ", \"hw_version\":\"%s\"", BoardIdentify::mcu);
  //snprintf(buffer, sizeof(buffer) - 1, ", \"manufacturer\":\"%s\"", BoardIdentify::make);
  snprintf(buffer, sizeof(buffer) - 1, ", \"manufacturer\":\"%s\"", GUARDUINO_URL);
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);

  
  //https://www.home-assistant.io/integrations/sensor.mqtt/#sw_version
  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, ", \"sw_version\":\"%s\"", SOFTWARE_VERSION);
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);


  //https://www.home-assistant.io/integrations/sensor.mqtt/#connections
  memset(buffer, '\0', sizeof(buffer));
  byte macBytes[6];
  Ethernet.MACAddress(macBytes); // https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.macaddress/  
  snprintf(buffer, sizeof(buffer) - 1, ", \"connections\":[[\"mac\",\"%02X:%02X:%02X:%02X:%02X:%02X\"],[\"ip\", \"%d.%d.%d.%d\"]]", macBytes[0], macBytes[1], macBytes[2], macBytes[3], macBytes[4], macBytes[5], myIP[0], myIP[1], myIP[2], myIP[3]);
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);


  // Closing JSON bracket.
  memset(buffer, '\0', sizeof(buffer));
  snprintf(buffer, sizeof(buffer) - 1, "}");
  jsonsize += strlen(buffer);
  json = (char*) realloc(json, jsonsize);
  strlcat(json, buffer, jsonsize);


  return json;
}



/**
 *  If "shouldSend" is true, mqtt sends "nulltermstring"
 *  In any case, returns the number of bytes we did/would send.
 *  This one function is handy for both determining "length to ultimately send" as well as actually sending.
 */
size_t mqttsend(const bool shouldSend, const char *nulltermstring) {
  if(! nulltermstring) return 0;
  
  size_t len = strlen(nulltermstring);
  if(shouldSend) {
    pubsubClient.write( (uint8_t *) nulltermstring, len);
    Serial.print(nulltermstring);
  }
  
  return len;
}





/**
 * Iterate through our list of baseSensors and setup pins for each sensor.
 * This function intended to be called in the arduino setup() method.
 */
void setupSensors(baseSensor_t *sensors, size_t sensorsSize) 
{
  int sensorCount = sensorsSize / sizeof(baseSensor_t);
  char sensorName[24];
  char *sensorCommandTopic = NULL;

  // Onboard LED.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Our configured pins.
  for(int i = 0; i < sensorCount; i++) {
    baseSensor_t *thisSensor = &sensors[i];
    switch(thisSensor->type) {
      case door2:
      case garagedoor2:
      case window2:      
      case motion2:
      case motion2_laser:
        getSensorName(sensorName, sizeof(sensorName), *thisSensor);
        Serial.print("Setup ");
        Serial.println(sensorName);
        pinMode(thisSensor->pin1, INPUT);
        pinMode(thisSensor->pin2, INPUT);
        break;
        
      case switch1:
      case switch1_radiator:
      case switch1_fan:
      case switch1_fire:
      case switch1_alarmlight:
        getSensorName(sensorName, sizeof(sensorName), *thisSensor);
        Serial.print("Setup ");
        Serial.println(sensorName);
        pinMode(thisSensor->pin1, OUTPUT);
        digitalWrite(thisSensor->pin1, LOW);

        sensorCommandTopic = getDeviceCommandTopic();
        if(sensorCommandTopic) {
          Serial.print("SUBSCRIBE ");
          Serial.print(sensorCommandTopic);
          Serial.println("");
          pubsubClient.subscribe(sensorCommandTopic);
          free(sensorCommandTopic);
          sensorCommandTopic = NULL;
        }
        break;
        
      default:
        break;          
    }
  }
}



/**
 * Given a sensor, and a set of all readings, return the enumerated "state" of the sensor.
 */
sensorStates getSensorStateEnum(baseSensor_t sensor, uint64_t pinReadings) {
  bool pin1data = false;
  bool pin2data = false;
  sensorStates theState = unknown;
  
  switch(sensor.type) {
    
    // Door2 and Window2: "pin1" is "no" aka "normally open", "pin2" is "nc" aka "normally closed"
    case door2:
      pin1data = getBit(pinReadings, sensor.pin1);
      pin2data = getBit(pinReadings, sensor.pin2);
      if( (pin1data==true) && (pin2data == true)) theState = door2_fault;
      if( (pin1data==true) && (pin2data == false)) theState = door2_open;
      if( (pin1data==false) && (pin2data == true)) theState = door2_closed;
      if( (pin1data==false) && (pin2data == false)) theState = door2_offline;
      break;

    case garagedoor2:
      pin1data = getBit(pinReadings, sensor.pin1);
      pin2data = getBit(pinReadings, sensor.pin2);
      if( (pin1data==true) && (pin2data == true)) theState = garagedoor2_fault;
      if( (pin1data==true) && (pin2data == false)) theState = garagedoor2_open;
      if( (pin1data==false) && (pin2data == true)) theState = garagedoor2_closed;
      if( (pin1data==false) && (pin2data == false)) theState = garagedoor2_offline;
      break;

    case window2:
      pin1data = getBit(pinReadings, sensor.pin1);
      pin2data = getBit(pinReadings, sensor.pin2);
      if( (pin1data==true) && (pin2data == true)) theState = window2_fault;
      if( (pin1data==true) && (pin2data == false)) theState = window2_open;
      if( (pin1data==false) && (pin2data == true)) theState = window2_closed;
      if( (pin1data==false) && (pin2data == false)) theState = window2_offline;
      break;

    case motion2:
    case motion2_laser:
      // Motion2: "pin1" is "data", pin2 is "power sense"
      pin1data = getBit(pinReadings, sensor.pin1);
      pin2data = getBit(pinReadings, sensor.pin2);
      if( (pin1data==true) && (pin2data == true)) theState = motion2_motion;
      if( (pin1data==true) && (pin2data == false)) theState = motion2_fault;
      if( (pin1data==false) && (pin2data == true)) theState = motion2_quiet;
      if( (pin1data==false) && (pin2data == false)) theState = motion2_offline;
      break;

    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      // Switch1: "pin1" is "switch status", "pin2" is unused.
      pin1data = getBit(pinReadings, sensor.pin1);      
      if( (pin1data == HIGH) ) theState = switch1_on;
      if( (pin1data == LOW) ) theState = switch1_off;
      break;      
  }

  return theState;
}


/**
 * Returns a const char* of the MDI icon to use for this sensor which has it's current state.
 * This function, when "Send Discovery" is called with every state change,  allows HA to 
 * display a dynamically different icon for this sensor based on the current state of the sensor.
 * https://pictogrammers.com/library/mdi/
 */
const char *getSensorStateIcon(baseSensor_t sensor, uint64_t pinReadings) {
  sensorStates theState = getSensorStateEnum(sensor, pinReadings);
  static const char *icon_unknown = "mdi:help-circle"; 
  static const char *icon_alert = "mdi:alert-circle-outline";
  static const char *icon_nopower = "mdi:power-plug-off";

  static const char *icon_door2_open = "mdi:door-open";
  static const char *icon_door2_closed = "mdi:door-closed";
  if(sensor.type == door2) {
    if(theState == door2_open) return icon_door2_open;
    if(theState == door2_closed) return icon_door2_closed;
    if(theState == door2_fault) return icon_alert;
    if(theState == door2_offline) return icon_nopower;
  }

  static const char *icon_garagedoor2_open = "mdi:garage-open";
  static const char *icon_garagedoor2_closed = "mdi:garage";
  if(sensor.type == garagedoor2) {
    if(theState == garagedoor2_open) return icon_garagedoor2_open;
    if(theState == garagedoor2_closed) return icon_garagedoor2_closed;
    if(theState == garagedoor2_fault) return icon_alert;
    if(theState == garagedoor2_offline) return icon_nopower;
  }

  static const char *icon_window2_open = "mdi:window-open";
  static const char *icon_window2_closed = "mdi:window-closed";
  if(sensor.type == window2) {
    if(theState == window2_open) return icon_window2_open;
    if(theState == window2_closed) return icon_window2_closed;
    if(theState == window2_fault) return icon_alert;
    if(theState == window2_offline) return icon_nopower;
  }

  static const char *icon_motion2_motion = "mdi:motion-sensor";
  static const char *icon_motion2_quiet = "mdi:meditation";
  if(sensor.type == motion2) {
    if(theState == motion2_motion) return icon_motion2_motion;
    if(theState == motion2_quiet) return icon_motion2_quiet;
    if(theState == motion2_fault) return icon_alert;
    if(theState == motion2_offline) return icon_nopower;
  }

  static const char *icon_motion2_laser_motion = "mdi:motion-sensor";
  static const char *icon_motion2_laser_quiet = "mdi:laser-pointer";
  if(sensor.type == motion2_laser) {
    if(theState == motion2_motion) return icon_motion2_laser_motion;
    if(theState == motion2_quiet) return icon_motion2_laser_quiet;
    if(theState == motion2_fault) return icon_alert;
    if(theState == motion2_offline) return icon_nopower;
  }

  static const char *icon_switch1_on = "mdi:light-switch";
  static const char *icon_switch1_off = "mdi:light-switch-off";
  if(sensor.type == switch1) {
    if(theState == switch1_on) return icon_switch1_on;
    if(theState == switch1_off) return icon_switch1_off;
  }

  static const char *icon_switch1_radiator_on = "mdi:radiator";
  static const char *icon_switch1_radiator_off = "mdi:radiator-off";
  if(sensor.type == switch1_radiator) {
    if(theState == switch1_on) return icon_switch1_radiator_on;
    if(theState == switch1_off) return icon_switch1_radiator_off;
  }

  static const char *icon_switch1_fan_on = "mdi:fan";
  static const char *icon_switch1_fan_off = "mdi:fan-off";
  if(sensor.type == switch1_fan) {
    if(theState == switch1_on) return icon_switch1_fan_on;
    if(theState == switch1_off) return icon_switch1_fan_off;
  }

  static const char *icon_switch1_fire_on = "mdi:fire";
  static const char *icon_switch1_fire_off = "mdi:fire-off";
  if(sensor.type == switch1_fire) {
    if(theState == switch1_on) return icon_switch1_fire_on;
    if(theState == switch1_off) return icon_switch1_fire_off;
  }

  static const char *icon_switch1_alarmlight_on = "mdi:alarm-light-outline";
  static const char *icon_switch1_alarmlight_off = "mdi:alarm-light-off";
  if(sensor.type == switch1_alarmlight) {
    if(theState == switch1_on) return icon_switch1_alarmlight_on;
    if(theState == switch1_off) return icon_switch1_alarmlight_off;
  }
  
  return icon_unknown;
}


/**
 * This is the value which HA displays for this "entity". This value is returned based on status computed in pinReadings for this sensor.
 */
const char *getSensorStateName(baseSensor_t sensor, uint64_t pinReadings) {
  static const char *name_unknown = "unknown";   
  static const char *name_open = "open";
  static const char *name_closed = "closed";
  static const char *name_off = "OFF";
  static const char *name_on = "ON";
  static const char *name_offline = "offline"; // https://www.home-assistant.io/integrations/sensor.mqtt/#payload_not_available
  static const char *name_fault = "wiringfault";   
  static const char *name_motion = "motion";
  static const char *name_quiet = "quiet";
  static const char *name_motion_nopower = "motion-fault";
  sensorStates theState;

  switch(sensor.type) {
    case door2:
    case garagedoor2:
    case window2:
    case motion2:
      theState = getSensorStateEnum(sensor, pinReadings);
      if(theState == door2_open) return name_open;  
      if(theState == door2_closed) return name_closed;
      if(theState == door2_fault) return name_fault;
      if(theState == door2_offline) return name_offline;
      if(theState == garagedoor2_open) return name_open;  
      if(theState == garagedoor2_closed) return name_closed;  
      if(theState == garagedoor2_fault) return name_fault;
      if(theState == garagedoor2_offline) return name_offline;
      
      if(theState == window2_open) return name_open;
      if(theState == window2_closed) return name_closed;
      if(theState == window2_fault) return name_fault;
      if(theState == window2_offline) return name_offline;

      if(theState == motion2_motion) return name_motion;
      if(theState == motion2_quiet) return name_quiet;
      if(theState == motion2_fault) return name_motion_nopower;
      if(theState == motion2_offline) return name_offline;
      break;

    case switch1:
    case switch1_radiator:
    case switch1_fan:
    case switch1_fire:
    case switch1_alarmlight:
      theState = getSensorStateEnum(sensor, pinReadings);
      if(theState == switch1_on) return name_on;
      if(theState == switch1_off) return name_off;
      break;      


    default:
      break;
  }

  return name_unknown;
}


/*
 * Look through each baseSensor in "sensors", and read pins for each "readable" sensor.
 * Ultimately, returns a 64 bit integer, starting with "oldbits", with appropriate sensor bits 
 * modified (and all other bits left in-place.)
 */
uint64_t readSensors(uint64_t oldBits, baseSensor_t *sensors, size_t sensorsSize)
{
  uint64_t newBits = oldBits;
  
  for(int i = 0; i < (sensorsSize / sizeof(baseSensor_t)); i++) {
    baseSensor_t thisSensor = sensors[i];
    switch(thisSensor.type) {
      case door2:
      case garagedoor2:
      case window2:
      case motion2:
      case motion2_laser:
        newBits = readBit(newBits, thisSensor.pin1);
        newBits = readBit(newBits, thisSensor.pin2);
        break;
      case switch1:
      case switch1_radiator:
      case switch1_fan:
      case switch1_fire:
      case switch1_alarmlight:
        newBits = readBit(newBits, thisSensor.pin1);
        break;

      default:
        break;
    }
  }
  
  return newBits;
}


/**
 * Reads digital "pin", and puts results into "bitarray", returning modified "bitarray"
 */
// https://www.geeksforgeeks.org/set-clear-and-toggle-a-given-bit-of-a-number-in-c/
uint64_t readBit(uint64_t bitarray, int8_t pin)
{
  if(pin < 0) { return bitarray; } 
  
  uint8_t bitvalue = digitalRead(pin);
  if(bitvalue == HIGH) {    
    bitarray |= ((uint64_t) 1 << pin);    
  } else {
    bitarray &= ~((uint64_t) 1 << pin);    
  }

  
  return bitarray;
}



/**
 * Returns binary value of "pin" from "bitarray".
 */
bool getBit(uint64_t bitarray, int8_t pin)
{
    int bitValue = ((uint64_t) (bitarray >> pin) & 1);
    return (bitValue == 1);
}

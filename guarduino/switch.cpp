#include "guarduino.h"
// https://github.com/dawidchyrzynski/arduino-home-assistant/blob/main/examples/led-switch/led-switch.ino




void handleCallbackSwitches(const char *callbackValue) {
  Serial.print("handleCallbackSwitches(): ");
  Serial.println(callbackValue);  
  baseSensor_t *thisSensor = NULL;

  for(int i = 0; i < allSensorCount(); i++) {

    // Is this a sensor worth considering?
    thisSensor = &allSensors[i];
    switch(thisSensor->type) {
      case switch1:
      case switch1_radiator:
      case switch1_fan:
      case switch1_fire:
      case switch1_alarmlight:
        break; // Consider this sensor below.
      default:
        thisSensor = NULL; // No need to consider this sensor.
        break;
    }
    if(! thisSensor) continue; // Look at the next sensor.


    if(strncmp(callbackValue, switchValueON(*thisSensor), strlen(callbackValue)) == 0) {
      pinMode(thisSensor->pin1, OUTPUT);
      digitalWrite(thisSensor->pin1, HIGH);
      Serial.print("SWITCH ON: ");
      Serial.print(switchValueON(*thisSensor));
      Serial.println("");
    }

    if(strncmp(callbackValue, switchValueOFF(*thisSensor), strlen(callbackValue)) == 0) {
      pinMode(thisSensor->pin1, OUTPUT);
      digitalWrite(thisSensor->pin1, LOW);
      Serial.print("SWITCH OFF: ");
      Serial.print(switchValueOFF(*thisSensor));
      Serial.println("");
    }

  }  
}





/**
 * Since we have only a single callback for all of our switches, our HA "payload_on" and "payload_off" values must tell us "which switch?"
 * switch_NN-ON
 * switch_NN-OFF
 */
const char *switchValueON(baseSensor_t thisSensor) {
  static char fullValue[16];
  memset(fullValue, '\0', sizeof(fullValue));
  sprintf(fullValue, "switch_%02d-ON", thisSensor.pin1);
  return fullValue;  
}
const char *switchValueOFF(baseSensor_t thisSensor) {
  static char fullValue[16];
  memset(fullValue, '\0', sizeof(fullValue));
  sprintf(fullValue, "switch_%02d-OFF", thisSensor.pin1);
  return fullValue;  
}





/**
 * Since we have only a single callback for all of our switches, our HA "payload_on" and "payload_off" values must tell us "which switch?"
 * switch_NN-ON
 * switch_NN-OFF
 * https://www.home-assistant.io/integrations/switch.mqtt#payload_on
 * https://www.home-assistant.io/integrations/switch.mqtt#payload_off
 */
const char *readSwitchSensor(uint64_t pinReadings, baseSensor_t thisSensor)  {
  bool data = getBit(pinReadings, thisSensor.pin1);

  if(data == HIGH) { return switchValueON(thisSensor); }
  if(data == LOW) { return switchValueOFF(thisSensor); }

  return NULL;
}

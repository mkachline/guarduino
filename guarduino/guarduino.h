#ifndef _GUARDUINO_H_
#define _GUARDUINO_H_
#include <Ethernet.h>          // https://www.arduino.cc/reference/en/libraries/ethernet/
#include <PubSubClient.h>      // https://github.com/knolleary/pubsubclient/tree/master
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#define GUARDUINO_URL "https://github.com/mkachline/guarduino/"

#define HA_TOPIC_DATA "aha"                // Mosquitto Data topic. You probably don't need to change this.
#define HA_TOPIC_DISCOVERY "homeassistant" // Mosquitto Discovery topic. You probably don't need to change this.
#define SOFTWARE_VERSION "2024.01.13.8"

enum sensorType
{
    unused = 0,
    reserved = 0,
    door2,
    garagedoor2,
    window2,
    motion2,
    motion2_laser,
    switch1,
    switch1_radiator,
    switch1_fan,
    switch1_fire,
    switch1_alarmlight,
};

enum sensorStates
{
    unknown = 0,
    door2_open,
    door2_closed,
    door2_offline,
    door2_fault,
    garagedoor2_open,
    garagedoor2_closed,
    garagedoor2_offline,
    garagedoor2_fault,
    window2_open,
    window2_closed,
    window2_offline,
    window2_fault,
    motion2_motion,
    motion2_quiet,
    motion2_offline,
    motion2_fault,
    switch1_on,
    switch1_off
};

typedef struct baseSensor_t
{
    sensorType type;
    int8_t pin1;
    int8_t pin2;
};

typedef struct ds18x_t
{
    int8_t pin1;
    DeviceAddress address;
    float temp_f;
    float temp_f_old;
    float temp_f_oldest;
} ds18x_t;
extern unsigned char allds18x_count;
extern ds18x_t *allds18x;

extern byte mac[];
extern baseSensor_t allSensors[];
extern void mqttCallback(char *topic, byte *payloadBytes, unsigned int length);
extern bool didCallback;
extern PubSubClient pubsubClient;

extern int allSensorCount(void);

// switch.cpp
extern void setupSwitchSensors(void);
extern uint64_t readSwitchPins(uint64_t allPinReadings);
extern void mqttSwitchSendData(uint64_t pinReadings);
extern void handleCallbackSwitches(const char *callbackValue);
extern const char *readSwitchSensor(uint64_t pinReadings, baseSensor_t thisSensor);
extern const char *switchValueON(baseSensor_t thisSensor);
extern const char *switchValueOFF(baseSensor_t thisSensor);

// baseSensor
extern int allSensorCount(void);
extern size_t mqttsend(const bool shouldSend, const char *nulltermstring);
extern uint64_t readSensors(uint64_t currentBits, baseSensor_t *sensors, size_t sensorsSize);
extern void mqttSensorSendDiscovery(uint64_t pinReadings);
extern void getDeviceName(char *destbuf, size_t destbufsize);
extern void getSensorName(char *destbuf, size_t destbufsize, baseSensor_t thisSensor);
extern char *getSensorStateTopic(baseSensor_t thisSensor);
extern char *getDeviceCommandTopic(void);
extern char *getDeviceDiscoveryPayload(void);
extern uint64_t readBit(uint64_t bitarray, int8_t pin);
extern bool getBit(uint64_t bitarray, int8_t pin);
extern void setupSensors(baseSensor_t *sensors, size_t sensorsSize);
extern void sendSensorsMQTT(uint64_t pinReadings, baseSensor_t *allSensors, size_t allSensorsSize);

// ds18x
extern void setupDS18Sensors(void);
extern ds18x_t *readDS18xSensors(void);
extern void mqttds18xSendData(ds18x_t *allds18x, unsigned int ds18xcount);
extern void mqttds18xSendDiscovery(ds18x_t *allds18x, unsigned int ds18xcount);

#define BOGUS_TEMPERATURE 222.22
#endif /* _GUARDUINO_H_ */

// secrets
#include <Arduino.h>

String ssid = "";
String password = "";
String hhostname = "LiveWire";
String thingspeakHost = "api.thingspeak.com";
String thingspeakKey = "";
unsigned int statusPin = 4;
unsigned int dhtPin = 5;

unsigned int samplesSendCount = 5;
unsigned long dhtInterval = 2500;

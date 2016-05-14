# ESP8266 Current Cost Envi

## Background Info
This project is an implementation of an ESP8266 interface to a Current Cost Envi 
power monitor (https://hackaday.io/project/8232-live-wire-esp8266envi).  It was written before features like SPIFFS were
available, so it has much room for improvement.  It also includes a DHT-22 temperature/humidity
device.

At this point, the hardware/code has been running about 6 months (466K samples stored
on thingspeak.com).  The code here is slightly repackaged for github.  It compiles, but I haven't 
loaded this version on hardware to ensure nothing was broken.

## Goals


## Dependencies

* ESP8266 Arduino library

* Other libraries

  * Time (https://github.com/PaulStoffregen/Time)
  
  * DHT (https://github.com/adafruit/DHT-sensor-library)
  


## Hardware Programming and Operation

0. use Arduino IDE or VSMicro to compile and upload code; my setup:
  * /libraries directories in ../Arduino/libraries directory
  * /espcc files in a main project (../espcc) directory

0. serial console output (Serial1) for debug/monitoring


# ESP8266 Current Cost Envi
![alt tag](https://raw.githubusercontent.com/flynnwt/espcc/master/espcc/livewire-screen.png)

![alt tag](https://raw.githubusercontent.com/flynnwt/espcc/master/espcc/livewire-thingspeak.png)

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

### Testing

Send a sample at 57600 to UART0:

```
<msg><src>CC128-v0.15</src><dsb>01790</dsb><time>18:41:25</time><tmprF>74.0</tmprF><sensor>0</sensor><id>03764</id><type>1</type><ch1><watts>00655</watts></ch1><ch2><watts>00532</watts></ch2></msg>
```
 

## Examples

```
http://192.168.0.105/api/system
{
"started":1463212161,
"samples":2350,
"bad":22,
"skipped":352,
"truncated":0,
"failedConnects":0,
"failedPosts":0,
"timeoutPosts":394
}

http://192.168.0.105/api/last
{
"id":4675,
"ts":1463227764,
"watts1":629,
"watts2":624,
"temp":71.40,
"dhtT":69.98,
"dhtH":35.90
}
```
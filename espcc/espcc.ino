#include <Esp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <vector>

#include <DHT.h>
#include <Time.h>

#include <utilities.h>
#include <flasher.h>
#include <ntp.h>
#include <web.h>
#include <thingspeak.h>

#include "envi.h"

// *** CONFIGURATION ***
// defined in config.cpp
extern String ssid;
extern String password;
extern String hhostname;
extern String thingspeakHost;
extern String thingspeakKey;
extern unsigned int statusPin;
extern unsigned int dhtPin;

extern unsigned int samplesSendCount;
extern unsigned long dhtInterval;
// **********************

bool reboot = true;
time_t bornOn = 0;
bool checkResult = false;
int chars;
unsigned long dhtLastRead;
float dhtT, dhtH, dhtHI;

EspClass* esp;
Web* web;
MDNSResponder mdns;
Flasher* status;
Envi* envi;
Thingspeak* thingspeak;
DHT* dht;

// other stuff...
// local storage: add SPIFFs and keep:
//  minutely averages for a day (1440)
//  hourly averages for a month (720)
//  daily averages for a year (360)
// add api for year, month, day samples
// save last 5 or 10 errors in system info (

unsigned int sampleCount = 0;
unsigned int failedConnects = 0;
unsigned int failedPosts = 0;
unsigned int timeoutPosts = 0;
Sample currentSample;

void processLine(String text) {
  String values;

  Serial1.print("Free heap: ");
  Serial1.println(esp->getFreeHeap());

  Serial1.print(ntpTimestamp());
  Serial1.print(" Received #");
  Serial1.print(sampleCount);
  Serial1.print(" (");
  Serial1.print(envi->totalSamples);
  Serial1.print(",");
  Serial1.print(envi->skippedSamples);
  Serial1.print(",");
  Serial1.print(envi->badSamples);
  Serial1.print(")");
  Serial1.print(": ");
  Serial1.println(envi->inString);

  currentSample = envi->add(text);
  if (!currentSample.empty) {
    sampleCount++;
  } else {
    Serial1.println("IGNORED!");
  }
  if (sampleCount >= samplesSendCount) {
    Sample s = envi->averageLast(sampleCount);
    values = "field1=" + String(s.watts1);
    values += "&field2=" + String(s.watts2);
    values += "&field3=" + String(s.temp);
    values += "&field4=" + String(dhtT);
    values += "&field5=" + String(dhtH);
    values += "&field7=" + String(sampleCount);
    if (reboot) {
      values += "&field8=1";
    } else {
      values += "&field8=0";
    }
    Serial1.println("POSTing " + values);
    if (checkResult) {
      timeoutPosts++;
    }
    if (thingspeak->post(values)) {
      checkResult = true;
      // this should be done after POST result received to prevent lost samples, but then
      //  duplicate sends, etc. need to be handled (multiple pending sends, etc.)
      sampleCount = 0;
    } else {
      Serial.println("*****");
      Serial1.println("Did not connect!");
      // don't toss samples; try again next time
      failedConnects++;
    }
  }
}

// could add timestamp for dht also, but not really that important...
void processDHT() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' 
  dhtH = dht->readHumidity();
  dhtT = dht->readTemperature(true);
  dhtLastRead = millis();

  if (isnan(dhtH) || isnan(dhtT)) {
    Serial1.println("Failed to read from DHT sensor!");
    dhtH = -1.0;
    dhtT = -1.0;
    dhtHI = -1.0;
    return;
  }

  dhtHI = dht->computeHeatIndex(dhtT, dhtH);

  Serial1.print("Temperature: ");
  Serial1.print(dhtT);
  Serial1.print("F ");
  Serial1.print("Humidity: ");
  Serial1.print(dhtH);
  Serial1.print("% ");
  Serial1.print("Feels like: ");
  Serial1.print(dhtHI);
  Serial1.println("F ");
  return;

}

void apiSystem() {
  JSON res;
  res.add("started", bornOn);
  res.add("samples", envi->totalSamples);
  res.add("bad", envi->badSamples);
  res.add("skipped", envi->skippedSamples);
  res.add("truncated", envi->truncatedSamples);
  res.add("failedConnects", failedConnects);
  res.add("failedPosts", failedPosts);
  res.add("timeoutPosts", timeoutPosts);
  web->server->send(200, "application/json", res.stringify());
}

void apiLast() {
  JSON res;
  if (currentSample.empty) {
    res.add("empty", currentSample.empty);
  } else {
    res.add("id", currentSample.id);
    res.add("ts", currentSample.ts);
    res.add("watts1", currentSample.watts1);
    res.add("watts2", currentSample.watts2);
    res.add("temp", currentSample.temp);
    res.add("dhtT", dhtT);
    res.add("dhtH", dhtH);
  }
  web->server->send(200, "application/json", res.stringify());
}

void rootPage() {
  String html = "";
  html += "<html><head><title>Live Wire</title>";
  html += "<link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css\" rel=\"stylesheet\">";
  html += "<link href=\"https://cdnjs.cloudflare.com/ajax/libs/c3/0.4.10/c3.min.css\" rel=\"stylesheet\">";
  html += "<link href='https://fonts.googleapis.com/css?family=Roboto' rel='stylesheet' type='text/css'>";
  html += "<link href='https://fonts.googleapis.com/css?family=Yanone+Kaffeesatz' rel='stylesheet' type='text/css'>";
  html += "<style>";
  html += "body {color:white; background-color:black;}\n";
  html += ".jumbotron {background:linear-gradient(to right, red , blue)}\n";
  html += ".jumbotron h1 {padding-left:5%; color:white}\n";
  html += ".jumbotron h3 {padding-left:20%; font-family:'Yanone Kaffeesatz';color:#C0C0C0}\n";
  html += ".jumbotron h5 {float: right; padding-right: 50px; font-family:'Yanone Kaffeesatz'}\n";
  html += ".latest {padding-left:100px; color:red; font-size:32px}\n";
  html += ".latest span {padding-left:25px}\n";
  html += ".c3-legend-item-ch1 text {fill: white}\n";
  html += ".c3-legend-item-ch2 text {fill: white}\n";
  html += ".c3-axis-x-label {fill: white}\n";
  html += ".c3-axis-x .tick {fill: white}\n";
  html += ".c3-axis-y-label {fill: white}\n";
  html += ".c3-axis-y .tick {fill: white}\n";
  html += ".c3-tooltip {color: red}\n";
  html += "#status {cursor:pointer;}\n";
  html += "#info {position:absolute;right:25%;top:5%;color:white;background-color:rgba(0,0,0,0.25);border:2px solid white}\n";
  html += "#info td {padding:5px}\n";
  html += "</style>";
  html += "<table id=\"info\"><tbody><tr><td>Bad</td><td class=\"bad\"></td></tr><tr><td>Skipped</td><td class=\"skipped\"></td></tr><tr><td>Truncated</td><td class=\"truncated\"></td></tr><tr><td>Failed Connects</td><td class=\"failedConnects\"></td></tr><tr><td>Failed POSTs</td><td class=\"failedPosts\"></td></tr><tr><td>Timeout POSTs</td><td class=\"timeoutPosts\"></td></tr></tbody></table>";
  html += "</head><body><div class=\"jumbotron\"><h1>Live Wire</h1><h3>Envi Power Monitor</h3><h5 id=\"status\"><span class=\"born\"></span>...<span class=\"samples\"></span></h5></div>";
  html += "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js\"></script>";
  html += "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/d3/3.5.6/d3.min.js\"></script>";
  html += "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/c3/0.4.10/c3.min.js\"></script>";
  html += "<div class=\"latest\"><span class=\"temp\"></span><span class=\"watts1\"></span><span class=\"watts2\"></span><span class=\"updated\"></span></div>";
  html += "<div class=\"text\" style=\"white-space:pre;padding-left:25px\"></span></div>";
  html += "<div id=\"chartWatts\"></div>";
  html += "<script>\n";
  html += "var samplesWatts1=[], samplesWatts2=[], samplesTs=[];\n";
  html += "var lastSampleId=-1, lastSampleTime;\n";
  //html += "function check() {$.ajax('/api/text').done(function(res) {console.log(res);$('.text').text(res.text);}).fail(function(err, res) {console.log(err, res)})};";
  //html += "setInterval(check, 6000);";
  html += "function latest() {\n$.ajax('/api/last')\n.done(function(res) {console.log(res);\nif (!res.empty && (res.id !== lastSampleId)) {\n$('#chartWatts').show();lastSampleId=res.id;lastSampleTime=new Date().toLocaleString();samplesWatts1.push(res.watts1);samplesWatts2.push(res.watts2);samplesTs.push(res.ts);updateChart();\n$('.latest .temp').text('Temp: ' + res.temp + '\xB0');\n$('.latest .watts1').text('Ch1: ' + res.watts1 + 'W');\n$('.latest .watts2').text('Ch2: ' + res.watts2 + 'W');$('.latest .updated').text(lastSampleTime);\n} else {\n}})\n.fail(function(err, res) {console.log(err, res)})};\n";
  html += "latest();setInterval(latest, 6000);\n";
  html += "var chartWatts = c3.generate({bindto:'#chartWatts', axis:{x:{type:'timeseries', tick:{rotate:45,format:function(e,d){return dateToString(e);}}}},data:{x:'x',columns:[[],[]]}});";
  html += "function updateChart() {chartWatts.load({columns: [['x'].concat(samplesTs),['ch1'].concat(samplesWatts1),['ch2'].concat(samplesWatts2)]})};\n";
  html += "function systemInfo() {\n$.ajax('/api/system')\n.done(function(res) {console.log(res);\n$('.born').text('Active since ' + new Date(res.started*1000).toLocaleString());$('#info .bad').text(res.bad);$('#info .skipped').text(res.skipped);$('#info .truncated').text(res.truncated);$('#info .failedConnects').text(res.failedConnects);$('#info .failedPosts').text(res.failedPosts);$('#info .timeoutPosts').text(res.timeoutPosts);\n$('.samples').text('Samples: ' + res.samples);\n})\n.fail(function(err, res) {console.log(err, res)})};\n";
  html += "systemInfo();setInterval(systemInfo, 10000);\n";
  html += "function dateToString(e) {var date = new Date(e * 1000);return date.toTimeString().substring(0,8);}\n";
  html += "$(document).ready(function(){$('#info').hide();$('#chartWatts').hide();\n";
  html += "$('#status').mouseover(function(e) {$('#info').show();}).mouseout(function(e) {$('#info').hide()})\n";
  html += "})";
  html += "</script>\n";
  html += "</body></html>";
  web->server->send(200, "text/html", html);
}

void setup() {

  esp = new EspClass();

  Serial1.begin(115200);
  //Serial1.setDebugOutput(true);
  Serial1.println("Setup...");

  status = new Flasher(4, 100, 100);
  status->start();

  WiFi.hostname(hhostname.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial1.println("Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    status->process();
    delay(10);
  }

  Serial1.println("");
  Serial1.print("Connected to ");
  Serial1.println(ssid);
  Serial1.print("IP address: ");
  Serial1.println(WiFi.localIP());

  if (mdns.begin(hhostname.c_str(), WiFi.localIP())) {
    Serial1.println("MDNS responder started.");
  }

  web = new Web();
  web->addRoot(rootPage);
  web->addRoute("/api/last", apiLast);
  web->addRoute("/api/system", apiSystem);
  Serial1.println("HTTP server started.");

  Serial1.print("Getting time...");
  ntpBegin(3600);
  while (timeStatus() == timeNotSet) {
    status->process();
    delay(10);
  }
  bornOn = now();
  Serial1.println(ntpTimestamp());

  envi = new Envi();
  envi->maxSamples = samplesSendCount;
  thingspeak = new Thingspeak(thingspeakHost, thingspeakKey);

  dht = new DHT(dhtPin, DHT22);
  dht->begin();
  dhtLastRead = 0;

  Serial1.println("Monitoring serial port...");
  Serial.begin(57600);

  status = new Flasher(4, 100, 100);

}

void loop() {
  unsigned int i;

  // loop processes
  web->server->handleClient();
  status->process();
  now();

  // check DHT periodically
  if (millis() > dhtLastRead + dhtInterval) {
    processDHT();
  }

  // check serial in
  chars = envi->readLine(Serial.read());
  if (chars > 0) {
    processLine(envi->inString);
    status->times(3);
  } else if (chars == -2) {
    Serial1.println("*****");
    Serial1.println("readLine() overflow!");
    Serial1.println(envi->inString);
  }

  // collect response if POSTed
  if (checkResult) {
    thingspeak->process();
    if (thingspeak->complete()) {
      Serial1.print("POST RESULT: ");
      Serial1.print(thingspeak->result());
      Serial1.print(" ");
      i = 0;
      while (!thingspeak->chunk(i).equals("")) {
        Serial1.print(thingspeak->chunk(i));
        Serial1.print(" ");
        i++;
      }
      Serial1.println();
      if (thingspeak->code() != 200) {
        Serial1.println(thingspeak->code());
        Serial1.println("*** FAILED!!!");
        failedPosts++;
      } else {
        reboot = false;     // clear once a good result is received
      }
      checkResult = false;
    }
  }
}
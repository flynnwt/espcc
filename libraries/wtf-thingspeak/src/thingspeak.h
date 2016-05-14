#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <vector>

#include <utilities.h>

#define IDLE 0
#define RESULT_0 1
#define RESULT_1 2
#define HEADERS_0 3
#define HEADERS_1 4
#define CHUNKSIZE_0 5
#define CHUNKSIZE_1 6
#define CHUNKVALUE_0 7
#define COMPLETE 8

class Thingspeak {
  String host;
  String key;
  WiFiClient client;
  unsigned int state;
  String curLine;
  unsigned int chunkSize;
  int _code;
  String _result;
  std::vector<String> _headers;
  std::vector<String> _chunks;

  bool _post(String url);

public:
  Thingspeak(String host, String key);
  bool post(String values);
  bool post(String values, String timestamp);
  bool process();
  int code();
  String result();
  String header(unsigned int n);
  String chunk(unsigned int n);
  bool complete();
};
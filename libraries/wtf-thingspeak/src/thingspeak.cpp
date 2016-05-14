#include "thingspeak.h"

Thingspeak::Thingspeak(String host, String key) {
  this->host = host;
  this->key = key;
}

bool Thingspeak::post(String values) {
  String url = "/update?key=" + key + "&" + values;
  return _post(url);
}

bool Thingspeak::post(String values, String timestamp) {
  String time;
  String url = "/update?key=" + key + "&" + values + "&" + timestamp;
  return _post(url);
}

String Thingspeak::result() {
  return _result;
}

int Thingspeak::code() {
  return _code;
}

String Thingspeak::header(unsigned int n) {
  if (n < _headers.size()) {
    return _headers[n];
  } else {
    return "";
  }
}

String Thingspeak::chunk(unsigned int n) {
  if (n < _chunks.size()) {
    return _chunks[n];
  } else {
    return "";
  }
}

bool Thingspeak::complete() {
  return state == COMPLETE;
}

bool Thingspeak::_post(String url) {
  //WiFiClient client; don't reallocate new client - not sure if it matters

  if (!client.connect(host.c_str(), 80)) {
    return false;
  }

  //this->client = client;
  this->_code = -1;
  this->state = RESULT_0;
  this->_result = "";
  this->_headers.clear();
  this->_chunks.clear();
  this->curLine = "";
  client.println("POST " + url + " HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
  return true;
}

bool Thingspeak::process() {
  char c;
  std::vector<String> tokens;

  if (client.connected()) {
    while (client.available()) {
      c = client.read();
      switch (state) {
      case IDLE:  // idle
        break;
      case RESULT_0: // reading response
        switch (c) {
        case '\r':
          state = RESULT_1;
          break;
        default:
          curLine += char(c);
          break;
        }
        break;
      case RESULT_1: // reading response, \r seen
        switch (c) {
        case '\n':
          _result = curLine;
          tokens = splitText(_result, ' ');
          _code = atoi(tokens[1].c_str());
          chunkSize = 0;
          state = HEADERS_0;
          curLine = "";
          break;
        default:
          curLine += '\r';
          curLine += char(c);
          state = RESULT_0;
          break;
        }
        break;
      case HEADERS_0: // reading header
        switch (c) {
        case '\r':
          state = HEADERS_1;
          break;
        default:
          curLine += char(c);
          break;
        }
        break;
      case HEADERS_1: // reading header, \r seen
        switch (c) {
        case '\n':
          if (curLine.length() == 0) {
            chunkSize = 0;
            state = CHUNKSIZE_0;
          } else {
            _headers.push_back(curLine);
            state = HEADERS_0;
          }
          curLine = "";
          break;
        default:
          curLine += '\r';
          curLine += char(c);
          state = HEADERS_0;
          break;
        }
        break;
      case CHUNKSIZE_0: // reading body chunk count
        switch (c) {
        case '\r':
          state = CHUNKSIZE_1;
          break;
        default:
          chunkSize = chunkSize * 10 + (c - '0');
          break;
        }
        break;
      case CHUNKSIZE_1:   // reading body chunk count, \r seen
        if (c == '\n') {
          if (chunkSize > 0) {
            state = CHUNKVALUE_0;
          } else {
            state = COMPLETE;
          }
        } else {
          // error
        }
        break;
      case CHUNKVALUE_0: // reading body chunk
        curLine += char(c);
        if (curLine.length() == chunkSize) {
          _chunks.push_back(curLine);
          chunkSize = 0;
          curLine = "";
          state = CHUNKSIZE_0;
        }
        break;
      }
    }
    return true;
  } else {
    client.stopAll();
    return false;
  }
}


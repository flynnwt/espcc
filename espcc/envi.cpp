#include "envi.h"

unsigned int Sample::nextId = 1;

Sample::Sample() {
  this->empty = true;
  this->id = nextId++;  // id=0 could be used to indicate empty
}

Sample::Sample(double temp, unsigned int watts1, unsigned int watts2) {
  this->empty = false;
  this->id = nextId++;
  this->ts = now();
  this->temp = temp;
  this->watts1 = watts1;
  this->watts2 = watts2;
}

// replace tags with ',' as they arrive; replace multiple ',' with one when returning
//
// relying on crlf instead of <msg> tags to delimit messages; it looks like sometimes
//  the messages come back-to-back during history, so could lose a realtime; could change
//  to find <msg> start, check for <hist> and then discard until </msg>
//
int Envi::readLine(int c) {
  static bool newLine = true;
  static bool inTag = false;

  if (newLine) {
    newLine = false;
    inString = "";
  }
  if (inString.length() >= maxInString) {
    newLine = true;
    truncatedSamples++;
    return -2;
  }
  if (c > 0) {
    switch (c) {
    case '\t':
    case '\f':
    case '\b':
    case '\n': // skip 
      break;
    case '\r': // end 
      newLine = true;
      inString.replace(",,,,,", ",");
      inString.replace(",,,,", ",");
      inString.replace(",,,", ",");
      inString.replace(",,", ",");
      return inString.length();
      break;
    case '"':
      inString += "\\";
      inString += "\"";
      break;
    case '\\':
      inString += "\\";
      inString += "\\";
      break;
    case '/':
      if (!inTag) {
        inString += "\\";
        inString += "/";
      }
      break;
    case '<':
      inTag = true;
      inString += ",";
      break;
    case '>':
      inTag = false;
      break;
    default:
      if (!inTag) {
        inString += char(c);
      }
    }
  }
  return -1;
}

Sample Envi::add(Sample s) {
  if (!s.empty) {
    if (maxSamples > 0) {
      if (samples.size() < maxSamples) {
        samples.push_back(s);
      } else {
        samples.push_back(s);
        samples.erase(samples.begin());
      }
    }
    return s;
  } else {
    return s;
  }
}

Sample Envi::add(String s) {
  return add(processText(s));
}

void Envi::clear() {
  samples.clear();
}

Sample Envi::averageLast(unsigned int n) {
  unsigned int i, start;
  double temp = 0.0;
  unsigned int watts1 = 0, watts2 = 0;

  if (samples.size() > 0) {
    if (n > samples.size()) {
      start = 0;
      n = samples.size();
    } else {
      start = samples.size() - n;
    }

    for (i = start; i < samples.size(); i++) {
      temp += samples[i].temp;
      watts1 += samples[i].watts1;
      watts2 += samples[i].watts2;
    }

    temp = temp / n;
    watts1 = watts1 / n;
    watts2 = watts2 / n;

    return Sample(temp, watts1, watts2);
  } else {
    return Sample();
  }
}

// expect nn.n
bool verifyTemp(String s) {
  if (s.length() == 4) {
    return isdigit(s[0]) && isdigit(s[1]) && (s[2] == '.') && isdigit(s[3]);
  } else {
    return false;
  }
}

// expect nnnnn
bool verifyWatts(String s) {
  if (s.length() == 5) {
    return isdigit(s[0]) && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3]) && isdigit(s[4]);
  } else {
    return false;
  }
}

Sample Envi::processText(String text) {
  double temp;
  unsigned int watts1, watts2;
  Sample s;
  std::vector<String> tokens = splitText(text, ',');

  totalSamples++;
  if (tokens.size() == 11) {  // realtime
    if (verifyTemp(tokens[4]) && verifyWatts(tokens[8]) && verifyWatts(tokens[9])) {
      temp = atof(tokens[4].c_str());
      watts1 = atoi(tokens[8].c_str());
      watts2 = atoi(tokens[9].c_str());
      s = Sample(temp, watts1, watts2);
    } else {
      badSamples++;
    }
  } else if (tokens[6].equals("kwhr")) {  // history
    skippedSamples++;
  } else {
    badSamples++;
  }
  return s;
}


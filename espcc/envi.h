#pragma once
#include <Arduino.h>
#include <vector>

#include <Time.h>
#include <utilities.h>

/*
Real-Time Message:
<msg><src>CC128-v0.15</src><dsb>01792</dsb><time>18:29:49</time><tmprF>70.4</tmprF><sensor>0</sensor><id>03764</id><type>1</type><ch1><watts>00587</watts></ch1><ch2><watts>00547</watts></ch2></msg>
*/

class Sample {
private:
  static unsigned int nextId;
public:
  unsigned int id;
  time_t ts;
  bool empty;
  double temp;
  unsigned int watts1;
  unsigned int watts2;

  Sample();
  Sample(double temp, unsigned int watts1, unsigned int watts2);
};

class Envi {
  std::vector<Sample> samples;
  Sample processText(Sample s);
  Sample processText(String text);

public:
  unsigned int maxInString = 500;  // after stripping
  String inString;
  unsigned int totalSamples = 0;
  unsigned int badSamples = 0;
  unsigned int skippedSamples = 0;
  unsigned int truncatedSamples = 0;
  unsigned int maxSamples = 0;

  int readLine(int c);
  Sample add(String s);
  Sample add(Sample s);
  void clear();
  Sample averageLast(unsigned int n);

};
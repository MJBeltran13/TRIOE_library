#ifndef TRIOE_H
#define TRIOE_H

#include <Arduino.h>

class TRIOE {
  public:
    TRIOE(int pin);
    float readTemperature();
  private:
    int _pin;
};

#endif

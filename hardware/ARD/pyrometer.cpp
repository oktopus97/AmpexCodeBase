#include "pyrometer.h"
#include <Arduino.h>

//constructor
Pyrometer::Pyrometer() {
  //this line fails as Arduino.h was not imported in pyrometer.h 
  this->_input_pin = A0;
  for (int i = 0; i < sizeof(this->_function) / sizeof(this->_function[0]); i++)
    pinMode(this->_function[i], OUTPUT);
  pinMode(this->_input_pin, INPUT);
};

int Pyrometer::getReading() {
  //TODO calibration
  return analogRead(this->_input_pin);
};

//factor is 0-7
//0 emission = 0.1
//7 emission = 1.099
//can be adjusted using compact connect
int Pyrometer::setEmission(int factor) {
    delay(150);
    Serial.println("\ndebug");
    Serial.print(HIGH && (factor & B001));
    Serial.print(HIGH && (factor & B010));
    Serial.print(HIGH && (factor & B100));
    Serial.println();
    //reversed
    digitalWrite(this->_function[2], HIGH && (factor & B001));
    digitalWrite(this->_function[1], HIGH && (factor & B010));
    digitalWrite(this->_function[0], HIGH && (factor & B100));
};

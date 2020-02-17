#include "Arduino.h"

#define PINSENSOR1   12
#define PINSENSOR2   32

#define GPIOSPIN 10

class GPIOSensor
{
    int calibration_factor;
  public:
    int pin, min1, max1, fixpoint, index;
    void getReadings(int * readings);

    //a method will come to save us all
    //(and do the calibration automatically)
    void setCalibrationFactor(int calib);

    void setFixpoint(int f);

    GPIOSensor(int p, int mi, int ma);
};

void GPIOSensor::getReadings(int * readings){
  readings[index] = analogRead(pin);
  if (min1 < readings[index] < max1){
    printf("Not implemented yet");
 };
}

//a method will come to save us all
//(and do the calibration automatically)
void GPIOSensor::setCalibrationFactor(int calib){
  calibration_factor = calib;
};

void GPIOSensor::setFixpoint(int f) {
  fixpoint = f;
};

GPIOSensor::GPIOSensor(int p, int mi, int ma){
  static int sensor_index = 0;
  index = sensor_index;
  ++sensor_index;
  pin = p;
  min1 = mi;
  max1 = ma;
};

GPIOSensor sens1(PINSENSOR1, 8, 7);
GPIOSensor sens2(PINSENSOR2, 5, 9);

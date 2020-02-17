/*
 *The Main arduino program,
 *Procedure ->
 *	setup() will be run -->
 *         *will connect to the wifi network and the mqtt broker
 *	   *will conduct automated safety procedurres
 *         *send setup completed signal to the broker '1'
 *  loop() will be looped forever ->
 *         for each time step it will
 *           *react if there's a signal from the server
 *           *read sensor data
 *           *check if everything all right with the safety procedures
 *           *upload server data
 */
#include <Arduino.h>
  
#include "signals.h"
#include "conn.h"
#include "board.h"

int controls[] = {GPIOSPIN};
GPIOSensor sensList[2] = {sens1, sens2};
float readings[sizeof(sensList)/sizeof(GPIOSensor)];

int dataGenerated = 0;
int millisPassed;

void setup(){

//for usb debugging
Serial.begin(9600);

//connect to wifi
#if MODE == WIFI

#endif

#if !TEST
  for (int i = 1; i <= sizeof(sensList)/sizeof(sensList[0]); ++i)
  {
    pinMode(sensList[i].pin, INPUT);
  }
  for (int i = 1; i <= sizeof(controls)/sizeof(controls[0]); ++i)
  {
    pinMode(sensList[i].pin, OUTPUT);
  }
#endif
}

void loop(){

}

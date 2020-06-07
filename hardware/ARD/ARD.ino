#include <Wire.h>
#include <EEPROM.h>
#include "enums.h"
#include "pyrometer.h"

#define NOSENSORS 4
#define BUFFER_SIZE 64
#define COMMAND_SUB_BUFFER BUFFER_SIZE-16 


//---------------------i2c--------------------------
#define ESP8266 0
#define ARDUINO 1

_status currentStatus;
//address on the eeprom for the status
int status_address = 0x1;

void clear_buffer(uint8_t * buff, int len) {
  uint8_t *ptr = buff;
  for (ptr; (int)(ptr - buff) < len; ptr++) *ptr = -1;
};

uint8_t * get_buffer(int l) {
    uint8_t * buff = new uint8_t[l];
    clear_buffer(buff, l);
    return buff;
};


void send_i2c(uint8_t * buff, int len) {
  uint8_t * ptr = (uint8_t*)buff;
  for (ptr; (int)(ptr-buff) < len; ptr++) {
 
    Wire.write(*ptr);
    Serial.write(*ptr);
    delay(15);
  }
  Serial.println();
  delay(500);
  clear_buffer(buff, len);
};

int last_comm;
int recv_i2c(uint8_t * buff) {
  last_comm = millis();
  uint8_t * ptr = (uint8_t*)buff;
  while(!Wire.available()){
    delay(100);
  }
  while (Wire.available()) {
    *ptr = Wire.read();
    ptr++;
  }
  delay(500);
  return ptr - buff;
};
uint8_t * _Buffer = get_buffer(BUFFER_SIZE);


//---------------------i2ccallbacks-------------------------------
int available_bytes = 0;

void requested() {
  if (*(_Buffer + COMMAND_SUB_BUFFER) == 255) {
    buffCurrentReadings((uint8_t*)_Buffer);
    send_i2c(_Buffer,  1 + sizeof(float) * NOSENSORS);
  } else {
    send_i2c((uint8_t*)(_Buffer+COMMAND_SUB_BUFFER), available_bytes);
    available_bytes = 0;
  }
};

int interrupt_timer = 0;

void received(int howMany) {
  last_comm = millis();
  recv_i2c(_Buffer+COMMAND_SUB_BUFFER);
  //all command responses should be on the last 16  bytes of the buffer
  switch (*(_Buffer+COMMAND_SUB_BUFFER)){
    case _PING:
      digitalWrite(13, HIGH);
      clear_buffer(_Buffer, howMany);
      *(_Buffer+COMMAND_SUB_BUFFER) =_PING;
      available_bytes++;
      break;

    case GPIO_READ:{
        float val = GPIORead((char) *(_Buffer + (COMMAND_SUB_BUFFER+1)),*(_Buffer + (COMMAND_SUB_BUFFER+2)), 1);
        clear_buffer(_Buffer, howMany);
        memcpy(_Buffer + (BUFFER_SIZE-16), &val, sizeof(val));
        break;
    }
    case GPIO_WRITE:
      GPIOWrite((char)*(_Buffer+(COMMAND_SUB_BUFFER+1)), *(_Buffer + (COMMAND_SUB_BUFFER+2)), *(_Buffer +(COMMAND_SUB_BUFFER+3)));
      clear_buffer(_Buffer, howMany);
      *_Buffer = 1;      
      break;

    case SETSTATUS:
      if (interrupt_timer != 0) interrupt_timer = 0;
      currentStatus = (_status)*(_Buffer + COMMAND_SUB_BUFFER +1);
      EEPROM.write(status_address, currentStatus);
      Serial.print("\nStatus is ");
      Serial.print(currentStatus);
      Serial.println();
      clear_buffer(_Buffer+COMMAND_SUB_BUFFER, howMany);
      *_Buffer = currentStatus;
      break;
    default:
      Serial.println("COMMAND NOT VALID");
      clear_buffer(_Buffer+COMMAND_SUB_BUFFER, howMany);
      *_Buffer = 0;
      break;
  }
  return;
}
//-------------GPIO---------------------
float GPIORead(char mod, uint8_t pin, float multiplier){ 
  float val;
  if ((char) *(_Buffer + 1) == 'a') 
   val = analogRead(pin)* multiplier;
  else 
   val = digitalRead( pin);
  return val ;
  
}
void GPIOWrite(char mod, uint8_t pin, uint8_t val){
  if (mod == 'a') { 
    analogWrite(pin, val);
  }  else { 
    digitalWrite(pin, val);  
  };
}


//------------procedural ------------

//hardware code -->
Pyrometer pyro;
// TODO Motor code
//Motor motor;

//sensor indexes are
//  0 -> Pyrometer 
bool fiber_in_cage = false;

void procLoop (_status curr) {
  switch (curr)
  {
    case INIT:
      Serial.println("INIT LOOP");
      break;
    case READY:
      Serial.println("READY LOOP");
      break;
    case HEAT: 
      Serial.println("HEAT LOOP");
      break;
    case SPIN:
      Serial.println("SPIN LOOP");
      break;
    case COOL:
      Serial.println("COOL LOOP");
      break;
    case INTERRUPT:
      if (interrupt_timer == 0) {
        interrupt_timer = millis();
      }
      Serial.println("INTERRUPTED");
      if (interrupt_timer > 30000) currentStatus = READY;
      break;
   }
};

float getReading(int sensor_index) {
  switch (sensor_index) {
    case 0:
      return pyro.getReading();
    default:
      return 100.0;
  }
}
void buffCurrentReadings(uint8_t * buff) {
  float reading;
  uint8_t *ptr = buff;
  *ptr = currentStatus;
  ptr++;
  for (int i=0; i< NOSENSORS; i ++) {
     reading = getReading(i);
     memcpy(ptr, (void*)&reading, sizeof(reading));
     ptr += sizeof(reading);
  }
}


//----------------main-----------------


void setup() {
  
  Serial.begin(9600);
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(A0, INPUT);
  Wire.begin(ARDUINO);
  Wire.onReceive(received);
  Wire.onRequest(requested);
  currentStatus = (_status)EEPROM.read(status_address);
  
}



void loop() {
  if (millis() - last_comm> 10000) currentStatus = INIT;
  delay(1000);
  procLoop(currentStatus);
}

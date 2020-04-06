#include <Wire.h>
#define NOSENSORS 4
#define BUFFER_SIZE 64
#define COMMAND_SUB_BUFFER BUFFER_SIZE-16 


//---------------------i2c--------------------------
#define ESP8266 0
#define ARDUINO 1

enum i2cCommands:uint8_t{
  _PING=1,
  SETSTATUS,
  GPIO_READ,
  GPIO_WRITE
};
enum _status:uint8_t{
  READY=1,
  SENSORERR,
  WAITING
  //spinning usw.
  
};
 _status currentStatus = READY;

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
    delay(15);
  }
  Serial.print("Sending ");
  Serial.print(len);
  Serial.print(" bytes");
  Serial.println();
  
  delay(500);
  clear_buffer(buff, len);
};

int recv_i2c(uint8_t * buff) {
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

void received(int howMany) {
  Serial.print("Received ");
  Serial.print(howMany);
  Serial.print(" Bytes \n");
  recv_i2c(_Buffer+COMMAND_SUB_BUFFER);
  //all command responses should be on the last 16  bytes of the buffer
  switch (*(_Buffer+COMMAND_SUB_BUFFER)){
    case _PING:
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
      Serial.println("lolo");
      GPIOWrite((char)*(_Buffer+(COMMAND_SUB_BUFFER+1)), *(_Buffer + (COMMAND_SUB_BUFFER+2)), *(_Buffer +(COMMAND_SUB_BUFFER+3)));
      clear_buffer(_Buffer, howMany);
      *_Buffer = 1;      
      break;

    case SETSTATUS:
      currentStatus = (_status)*(_Buffer + COMMAND_SUB_BUFFER +1);
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
    Serial.println(pin);
    Serial.println(val);
    digitalWrite(pin, val);  
  };
}


//------------procedural ------------

//hardware code -->
int sensors[NOSENSORS] = {1, 3, 4,5};
float multipliers[NOSENSORS] = {1,2,3,4};
float getReading(int sensor_index) {
  //TODO
  return multipliers[sensor_index];
}
void buffCurrentReadings(uint8_t * buff) {
  float reading;
  uint8_t *ptr = buff;
  *ptr = currentStatus;
  ptr++;
  for (int i; i< NOSENSORS; i ++) {
     reading = getReading(i);
     memcpy(ptr, (void*)&reading, sizeof(reading));
     ptr += sizeof(reading);
  }
}


//----------------main-----------------


void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN,OUTPUT);
  Wire.begin(ARDUINO);
  Wire.onReceive(received);
  Wire.onRequest(requested);
}



void loop() {
  delay(100);
}

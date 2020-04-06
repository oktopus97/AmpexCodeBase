#include <Wire.h>
#include "i2c.cpp"
#define ESP8266 0
#define ARDUINO 1

#define NOSENSORS 4
#define BUFFER_SIZE 64


//---- enums---------
enum _command:uint8_t {
  _PING=1,
  //procedual commands
  START,
  STOP,
  //debug commands
  GPIO_READ, //byte 2 -> analog(a) or digital(d) byte 3 -> pin number byte
  GPIO_WRITE //byte 2 -> analog(a) or digital(d) byte 3 -> pin number byte 4 -> 1, 0 (digital) value 0-255 (analog)
};

enum _status:uint8_t{
  INIT,
  //ready after full sensor check
  READY
  //spinning usw.

};

//functions
void send_i2c(uint8_t *, int);
int recv_i2c(uint8_t *);
uint8_t * get_buffer(int);
void clear_buffer(uint8_t * buff, int len);

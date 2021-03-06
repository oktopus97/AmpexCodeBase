enum ConnectionMode:uint8_t {
  MQTT=0,
  SER
};

enum i2cCommands:uint8_t{
  PING=1,
  SETSTATUS,
  GPIO_READ,
  GPIO_WRITE
};

enum Commands:uint8_t{
  SET_STAT=1,
  SET_IP,
  SET_BROKER,
  SET_WIFI,
  GPIOREAD,
  GPIOWRITE,
  PINGA,
  PINGR,
  INVALID,
};
//------status------
enum Status:uint8_t{
  INIT=1,
  READY,
  HEAT,
  SPIN,
  COOL,
  INTERRUPT

};

enum LogLevels:uint8_t {
  DATA=1,
  ERR=2,
  WARN=3,
  DEBUG=4
};

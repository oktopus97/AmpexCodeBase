enum i2cCommands:uint8_t{
  _PING=1,
  SETSTATUS,
  GPIO_READ,
  GPIO_WRITE
};
enum _status:uint8_t{
  INIT =1,
  READY,
  HEAT,
  SPIN,
  COOL,
  INTERRUPT
};

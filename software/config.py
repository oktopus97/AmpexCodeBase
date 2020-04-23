##make sure to change hardware variables 
##else finger weg !!
from enum import IntEnum

#makesure the enums are the same with $PROJECT-DIR/hardware/Wemos_ARD/enums.h !!!
class ConnectionModes(IntEnum):
    MQTT=0
    SER=1

class MachineStatus(IntEnum):
    INIT = 1
    READY = 2
    SENSORERR = 3
    WAITING = 4
    SPINNING = 5
    FINISHED = 6
    INTERRUPT = 7

class Commands(IntEnum):
    SET_STAT = 1
    SET_IP = 2
    SET_BROKER = 3
    SET_WIFI = 4
    SET_MODE = 5
    GPIOREAD = 6
    GPIOWRITE = 7
    PINGA = 8
    PINGR = 9

CONFIG_BROKER_PORT = 1883
CONFIG_BROKER_IP = 'localhost' #localhost

CONFIG_LOCAL_SERVER = True

CONFIG_SERIAL_PORT = "/dev/ttyUSB0"

CONFIG_MAXPLOT = 500

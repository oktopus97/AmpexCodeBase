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
    HEAT = 3
    SPIN = 4
    COOL = 5
    INTERRUPT = 6

class Commands(IntEnum):
    SET_STAT = 1
    SET_IP = 2
    SET_BROKER = 3
    SET_WIFI = 4
    GPIOREAD = 5
    GPIOWRITE = 6
    PINGA = 7
    PINGR = 8
    INVALID = 9

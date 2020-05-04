import threading
import queue
import time
from enums import MachineStatus
from tkinter import StringVar

ready = False

class CommunicationError(Exception):
    def __init__(self, exc, fatal=False, errcode=0):
        self.errcode = errcode
        self.fatal = fatal
        prefix = "FAT" if self.fatal else "ERR"
        self.expression = prefix + str(errcode) + " " + exc
        self.message = exc

class IBlockException(CommunicationError):
    """
    err codes ->
        0 -> unknown error
        fatal
        1 -> NO COMMUNICATION

    """
    pass


class ControllerException(CommunicationError):
    pass


times = None
sensors = None
ready_callback = None
sensor_vars = None
status_var = None
ready = False
ping = False
should_status = None

def set_plots(t, s):
    global times, sensors
    times, sensors = t, s

def set_sensor_vars(sv):
    global sensor_vars
    sensor_vars = sv

def ping_callback():
    global pinged
    ping = True

max_line = 500 
log_stream = None
log_lines = 0

def log_thread(log_queue, log_stopper):
    global log_stream, log_lines
    def get_new_log():
        global log_stream, log_lines
        log_stream = open("log", "wb")
        log_stream.truncate(0)
        log_lines = 0
        log_stream.seek(0)
    
    log_queue = log_queue
    log_stopper = log_stopper
    get_new_log()
    
    while True:
        line = log_queue.get()
        log_stream.seek(0, 2)
        if type(line) == str:
            line = line.encode()
        if line[-1] != b'\n':
            line += b'\n'
        log_stream.write(line)
        
        if log_stopper.is_set():
            return

log_event = threading.Event()
log_queue = queue.Queue()
log_thr = None

should_status = MachineStatus.READY.value 

def log(text):
    global log_queue
    log_queue.put(text)

def start_logging():
    global log_thr, log_queue, log_event
    log_thr = threading.Thread(target=log_thread, args=(log_queue, log_event))
    log_thr.daemon = True
    log_thr.start()

def stop_logging():
    global log_thr
    log_event.set()
    time.sleep(0.5)
    log_thr.join()
    log_thr = None

def log_handler(msg):
    global times, status_var, sensors, sensor_vars
    if type(msg) != str:
        msg = msg.decode("ascii")
    if len(msg) < 5:
        return
    splitted = msg.split(";");
    
    if splitted[3] == "[nan,]":
        return
    times.append(splitted[1][splitted[1].find("["):-1])

    status_var.set("Machine Status: " + \
            str(MachineStatus(int(splitted[2][1:-1])).value))
    splitted = splitted[3].replace("]", "")
    splitted = splitted.replace("[", "")
    splitted = splitted.split(',')
    sens  = ['ex1', 'ex2', 'ex3', 'ex4']
    if len(splitted) < len(sens):
        return
    for i, sensor in enumerate(sens):
        sensor_vars[sensor].set(sensor + ": " + splitted[i])
        sensors[sensor][0].append(float(splitted[i]))





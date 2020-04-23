import threading
import queue
import time

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

def set_plots(t, s):
    global times, sensors
    times, sensors = t, s

def set_sensor_vars(sv):
    global sensor_vars
    sensor_vars = sv

def ping_callback():
    global pinged
    ping = True

log_size = 8 * 1024 # mb
max_line = 500 
def log_thread(log_queue, log_stopper):
    def get_new_log():
        log_stream = open("log", "wb")
        log_stream.truncate(log_size)
        log_lines = 0
        log_stream.seek(0)
    
    log_queue = log_queue
    log_stopper = log_stopper
    get_new_log()
    
    while True:
        line = log_queue.get()
        if type(line) == str:
            line = line.encode()
        if line[-1] != b'\n':
            line += b'\n'
        log_stream.write(line)
        log_lines += 1
        if log_lines > 500:
            get_new_log()
        if log_stopper.is_set():
            return

log_event = threading.Event()
log_queue = queue.Queue()

def log(text):
    log_queue.put(text)

def start_logging():
    log_thread = threading.Thread(target=log_thread, args=(log_queue, log_event))
    log_thread.daemon = True
    log_thread.start()

def stop_logging():
    log_event.set()
    time.sleep(0.5)
    log_thread.join()
    log_thread = None

def log_handler(msg):
    global times, status_var, sensors, sensor_vars
    print(msg)
    splitted = msg.decode("utf-8").split();
    if splitted[3] == "[nan,]":
        return
    times.append(splitted[1][1:-1])

    status_var.set("Machine Status: " + MachineStatus(int(splitted[2][1:-1])).name)
    splitted = splitted[3][1:-1].split(',')
    sens  = ['ex1', 'ex2', 'ex3', 'ex4']
    for i, sensor in enumerate(sens):
        sensor_vars[sensor].set(sensor + ": " + splitted[i])
        sensors[sensor][0].append(float(splitted[i]))





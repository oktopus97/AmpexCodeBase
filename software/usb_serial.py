import serial
import os
import time
import threading
import common
import queue
from config import *

class SerialException(serial.serialutil.SerialException):
    pass

class Serializer():
    pinged = False

    def __init__(self, serial_port):
        self.port = serial_port
        
        self.connected = False
        
        self.command_reader = open("command", "r+")
        self.log = open("log", "wb")
        self.command_writer = open("command", "wb")
        
        self._recv_event = threading.Event()

        self._write_q = queue.Queue()
        self._response_q = queue.Queue()
        self._dialog_q = queue.Queue()
        self._answers_q = queue.Queue()
    
    def writeline(self, string):
        self._write_q.put(string.encode() + b"\n")
    
    def dialog(self, msg, *args, **kwargs):
        #func starts at line 107
        def throws():
            self.stop_loop()
            if kwargs.get('exc'):
                raise kwargs['exc'](*kwargs.get("eargs"))
            else:
                raise SerialException('Dialog Failed')
        def _write(m):
            self._recv_event.clear()
            self.writeline(str(m))
            tries = 30
            while True:
                tries -= 1
                time.sleep(0.5)
                if self._recv_event.is_set():
                    time.sleep(0.5)
                    break
                if tries < 1:
                    print("recv event not set")
                    throws()

        def get_response():
            tries = 10
            while True:
                try:
                    time.sleep(0.5)
                    ret = self._response_q.get(0)
                    break
                except queue.Empty:
                    tries -= 1
                
                if tries < 1:
                    return False
            return ret.decode("ascii")


        def check_response(string_):
            response = get_response()
            
            if response == False or response.find(string_) == -1:
                throws()
            
            response = response.split('\r\n')

            print(f"answer-> {response}")

            return response
        

        #func starts here->
        if type(msg) == tuple:
            msg, event = msg[0], msg[1]
            e = True
        else:
            e = False
        print(f"ask -> {msg}")

        self.serial.flush()
        self._recv_event.clear()
        time.sleep(0.5)
        resp = None
        _write(msg) 
        if len(args) == 0:
            time.sleep(0.2)

            if e:
                event.set()
            
            return check_response("OK")
        else:
            for i, arg in enumerate(args):
                _write(arg)
                if i < len(args) - 1:
                    check_response("?")
                else:
                    if e:
                        event.set()
                    return check_response("OK")
    
    def send_command(self, command):
        event = threading.Event()
        self._dialog_q.put((command, event))
            
        while True:
            if event.is_set():
                break
            else:
                time.sleep(0.5)
        return self._answers_q.get()
        
    def ping_machine(self):
        resp = self.send_command(Commands.PINGR.value)

        if resp[0][0] != str(Commands.PINGR.value):
            self.stop_loop()
            raise common.IBlockException("Ping Failed", True, 3)
        elif resp[1][0] != str(Commands.PINGA.value):
            self.stop_loop()
            raise common.IBlockException("UNEXPECTED BEHAVIOUR", False, 3)
        else:
            if self.connected is False:
                self.connected = True
            return True
    
    def set_status(self, status: MachineStatus):
        print(f"set {status}")
        resp = self.send_command(str(Commands.SET_STAT.value) + str(status.value))

    def start_loop(self):
        self.serial = serial.Serial(self.port)
        self.serial.flush()

        self.stopper = threading.Event()
        self.reader = threading.Thread(target=self.reader_thread, args=(self.stopper, self._recv_event,))
        self.writer = threading.Thread(target=self.writer_thread, args=(self.stopper,))
        self.dialogger = threading.Thread(target=self.dialog_thread, args=(self.stopper,))
        self.reader.daemon = True
        self.writer.daemon = True
        self.dialogger.daemon = True
        self.reader.start()
        self.dialogger.start()
        self.writer.start()
    
    def stop_loop(self):
        self.stopper.set()
        self.connected = False
        time.sleep(0.5)
        self.serial.close()
    
    def command_handler(self, command):
        commands_list = command.decode("ascii").split("\r\n")
        for com in commands_list:
            if com == "":
                continue
            if int(com) == Commands.PINGR.value:
                self._dialog_q.put(Commands.PINGA.value)
    

    def dialog_thread(self, ev):
        while True:
            jobs = []
            
            while True:
                if ev.is_set():
                    return
                job = self._dialog_q.get()
                ans = self.dialog(job)
                self._answers_q.put(ans)
                time.sleep(1)

    def reader_thread(self, ev, recv):
        dialog = False
        command = False
        while True:
            if ev.is_set():
                return
            if self.serial.in_waiting < 1:
                continue
            
            byte = self.serial.read()
            line = byte
            while byte != b"\n":
                byte = self.serial.read()
                line += byte
            print(f"read-> {line}")

            if line[0] ==91:
                common.log(line)
                continue

            if line[0] == 35:
                ret = b""
                command = True
                continue
            if line[0] == 42:
                self.command_handler(ret)
                command = False
            if command:
                ret += line

            if line[0] == 94:
                ret = b""
                dialog = True
                continue
            if line[0] == 36:
                dialog = False
                self._response_q.put(ret)
                recv.set()
            if dialog:
                ret += line
                continue

            #your command has passed all if clauses
            self.serial.flush()
                
    def writer_thread(self, ev):
        self.serial.flush()
        while True:
            try:
                line = self._write_q.get(0)
                print(f"writing {line}")
                self.serial.write(line)
            except queue.Empty:
                pass
            if ev.is_set():
                return


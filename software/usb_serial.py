import serial
import os
import time
import threading
import common
import queue
from enums import *

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
        self.stopper = threading.Event()

        self._write_q = queue.Queue()
        self._response_q = queue.Queue()
        self._dialog_q = queue.Queue()
        self._answers_q = queue.Queue()
    
    def writeline(self, string):
        self._write_q.put(string.encode() + b"\n")
    
    def dialog(self, msg, event, *args, **kwargs):
        #func starts at line 107
        def throws():
            return self.dialog(msg, event, *args, **kwargs)

        def _write(m):
            self._recv_event.clear()
            self.writeline(str(m))
            tries = 30
            while True:
                tries -= 1
                time.sleep(0.5)
                if self._recv_event.is_set():
                    time.sleep(0.5)
                    return True
                if tries < 1:
                    if self.stopper.is_set():
                        return None
                    else:
                        print("recv event not set")
                        throws()
                        return None

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
        print(f"ask -> {msg}")

        self.serial.flush()
        self._recv_event.clear()
        time.sleep(0.5)
        resp = None
        
        if _write(msg) is None: return 

        if len(args) == 0:
            time.sleep(0.2)

            event.set()
            
            return check_response("OK")
        else:
            for i, arg in enumerate(args):
                if _write(arg) is None: return
                if i < len(args) - 1:
                    check_response("?")
                else:
                    if e:
                        event.set()
                    return check_response("OK")
    
    def send_command(self, command, *args):
        event = threading.Event()
        self._dialog_q.put((command, event, args))
            
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
    
    def set_status(self, status):
        common.should_status = status if type(status) == int else status.value
        resp = self.send_command(str(Commands.SET_STAT.value) + str(common.should_status))
        return True

    def set_ip(self, ip, gateway, subnet):
        self.send_command(str(Commands.SET_IP.value), ip, gateway, subnet)
    
    def set_broker(self, broker_ip, broker_port):
        self.send_command(str(Commands.SET_BROKER.value), broker_ip, broker_port)

    def set_wifi(self, ssid, passwd):
        self.send_command(str(Commands.SET_WIFI.value), ssid, passwd)

    def start_loop(self):
        self.serial = serial.Serial(self.port)
        self.serial.flush()

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
        time.sleep(1)
        self.serial.close()
        with self._dialog_q.mutex:
            self._dialog_q.queue.clear()
        with self._answers_q.mutex:
            self._answers_q.queue.clear()
    
    def command_handler(self, command):
        commands_list = command.decode("ascii").split("\r\n")
        for com in commands_list:
            if com == "":
                continue
            if int(com) == Commands.PINGR.value:
                
                self._dialog_q.put((Commands.PINGA.value, threading.Event(), ()))
    

    def dialog_thread(self, ev):
            
        while True:
            if ev.is_set():
                return
            job, event, args = self._dialog_q.get()
            ans = self.dialog(job, event, *args)
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

            if "Controller Not Found" in line.decode():
                with self._dialog_q.mutex:
                    self._dialog_q.queue.clear()
                time.sleep(0.5)
                self.ping_machine()
                self.set_status(common.should_status)
                continue

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


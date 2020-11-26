import serial
import os
import time
import threading
import common
import queue
import atexit
import configparser
from enums import *

class SerialException(serial.serialutil.SerialException):
    pass

class Serializer():
    pinged = False

    def __init__(self, serial_port, var, prop, after, ready):
        self.port = serial_port
        self.serial = None

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

        self.status = var
        self.prop = prop

        self.ready = ready

        self.connection_ev = threading.Event()
        self.waiting_loop = None

        self.config = configparser.ConfigParser()
        self.config.read("conf.ini")
        
        self.dtr = threading.Event()

        self.after = after
        self.init_serial()

    def droptest(self):
        self.send_command("d")
        return True

    def wait_for_usb(self):
        while True:
            if not os.path.exists(self.config["DEFAULTS"]["CONFIG_SERIAL_PORT"]):
                self.status.set("USB Cable not connected")
                continue
            try:
                self.start_loop()
                self.prop = True
                self.connection_ev.set()
                break
            except serial.serialutil.SerialException as e:
                if e.errno == 2:
                    self.status.set("USB Cable not connected")
                elif e.errno == 13:
                    port = self.config["DEFAULTS"]["CONFIG_SERIAL_PORT"]
                    self.status.set(f"run sudo chmod 777 {port}")
            except FileNotFoundError:
                self.status.set("USB Cable not connected")


    def init_serial(self):
        if self.waiting_loop is None:
            self.connection_ev.clear()
            self.waiting_loop = threading.Thread(target=self.wait_for_usb)
            self.waiting_loop.daemon = True
            self.waiting_loop.start()
        if self.connection_ev.is_set():
            self.waiting_loop.join()
            self.waiting_loop = None
            self.ready("usb")
            self.status.set("Machine Detected")
        else:
            self.after(1000, self.init_serial)

        atexit.register(self.stop_loop)

    def writeline(self, string):
        self._write_q.put(string.encode() + b"\n")

    def dialog(self, msg, event, *args, **kwargs):
        #func starts at line 107
        self.tries = 0
        def throws():
            self.tries += 1
            if self.tries > 2:
                raise SerialException("No Response")
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
            print(f"response-> {response}")

            if response == False or response.find(string_) == -1:
                print("throws")
                throws()

            response = response.split('\r\n')
            if string_ != "?" and len(response) > 1 and str(Commands.INVALID.value) == response[1][0]:
                throws()


            print(f"answer-> {response}")

            return response


        #func starts here->
        while self.dtr.is_set() is False:
            print("dtr set")
            time.delay(0.1)
        print(f"ask -> {msg}")
        print(f"args -> {args}")

        self.serial.flush()
        self._recv_event.clear()
        time.sleep(0.5)
        resp = None

        if _write(msg) is None: return


        if len(args) == 0:
            time.sleep(0.2)

            event.set()
            self.dtr.clear()

            return check_response("OK")
        else:
            check_response("?")
            for i, arg in enumerate(args):
                if _write(arg + "%") is None: return
                if i < len(args) - 1:
                    print(check_response("?"))
                else:
                    event.set()
                    self.dtr.clear()
                    return check_response("OK")

    def send_command(self, command, *args):
        if not self.prop:
            print("not prop")
            return False
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
        if resp is False:
            return False

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
        if resp is False:
            return False

        return True

    def set_ip(self, ip, gateway, subnet):
        resp = self.send_command(str(Commands.SET_IP.value), ip, gateway, subnet)
        if resp is False:
            return False

    def set_broker(self, broker_ip, broker_port):
        if len(str(broker_port)) != 4:
            broker_port = str(broker_port)
            broker_port = (4 - len(str)) * "0" + broker_port
        resp = self.send_command(str(Commands.SET_BROKER.value), broker_ip, broker_port)
        if resp is False:
            return False

    def set_wifi(self, ssid, passwd):
        self.send_command(str(Commands.SET_WIFI.value), ssid, passwd)

    def start_loop(self):
        self.serial = serial.Serial(self.port)
        print("Serial object initialized")
        while True:
            if self.serial.in_waiting:
                self.serial.read()
            else:
                break
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
        try:
            self.serial.close()
        except AttributeError:
            pass
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
            if self.dtr.is_set() is False:
                continue

            try:
                job, event, args = self._dialog_q.get(0)
                self.serial.write(b"0\n")
                ans = self.dialog(job, event, *args)
                self._answers_q.put(ans)
                time.sleep(1)
            except queue.Empty:
                continue

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
            while True:
                try:
                    dec = line.decode()
                    break
                except UnicodeDecodeError:
                    line = line[1:]

            if "Controller Not Found" in dec:
                with self._dialog_q.mutex:
                    self._dialog_q.queue.clear()
                time.sleep(0.5)
                self.ping_machine()
                self.set_status(common.should_status)
                continue
            if line[:3] == b"DTR":
                self.serial.flush()
                self.dtr.set()
                continue

            if line[0] == 91:#[
                common.log(line)
                continue

            if line[0] == 35:##
                ret = b""
                command = True
                continue
            if line[0] == 42:#*
                self.command_handler(ret)
                command = False
            if command:
                ret += line

            if line[0] == 94:#^
                ret = b""
                dialog = True
                continue
            if line[0] == 36 or line[0] == 63: #$ and ?
                if line[0] == 63:
                    ret += b"?"
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

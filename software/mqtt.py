import paho.mqtt.client as mqtt
from enums import *
import common
import configparser
import subprocess
import time
import atexit


class MQTTClient(mqtt.Client):
    def __init__(self, **kwargs):
        self.config = configparser.ConfigParser()
        self.config.read("conf.ini")

        self.status = kwargs.pop("status_var")
        self.status_prop = kwargs.pop("status_prop")
        super().__init__(**kwargs)
        self.connected = False
        self.serv = False
        self.init_server()


    def connect_to_serv(self):

        self.will_set("/ServerStatus", payload="Offline", qos=0, retain=True)
        self.connect(self.config["DEFAULTS"]["config_broker_ip"], int(self.config["DEFAULTS"]["config_broker_port"]))
        self.loop_start()
        ip_addr = self.config["DEFAULTS"]["config_broker_ip"]
        self.status.set("Local Server running" if self.serv else
                        "Conected to server @{}".format(ip_addr))
    def init_server(self):
        if bool(self.config["DEFAULTS"]["CONFIG_LOCAL_SERVER"]):
            print("Starting Local Server")
            self.server = subprocess.Popen(['mosquitto', "-p",
                    str(self.config["DEFAULTS"]["config_broker_port"])])
            time.sleep(1)
            self.serv = True
            atexit.register(self.kill_server)
        self.connect_to_serv()
        return True

    def kill_server(self):
        if self.serv:
            subprocess.check_output(["pkill", "mosquitto"])

    def on_disconnect(self, *args, **kwargs):
        self.ready = False

    def on_connect(self, client, userdata, flags, rc):
        self.publish("/ServerStatus", payload="Online", qos=0, retain=True)
        self.subscribe('/Data')
        self.subscribe('/Error')
        self.subscribe('/Info')

    def on_message(self, client, userdata, message):
        print(f"mqtt message {message.topic} {message.payload}")
        if message.topic == '/Data':
            common.log(message.payload)
        if message.topic == '/Info':
            if message.payload == b'IBLOCK CONNECT':
                self.connected = True
                self.status.set(self.status.get() + " IBlock Connected")
                self.status_prop = True
            if message.payload == b'IBLOCK DISCONNECT':
                self.connected = False
                self.status.set(self.status.get() + " IBlock Disconnected")
                self.status_prop = True
            if message.payload == str(Commands.PINGR.value).encode():
                self.publish("/Commands", str(Commands.PINGA.value) +";")

        if message.topic == '/Error':
            common.status_var.set('ERROR: ' + message.payload.decode())

    def set_status(self, stat):
        comm = str(Commands.SET_STAT.value) + ";" +\
                str(stat if type(stat) == int else stat.value) + ";"
        self.publish("/Commands/proc", comm)

    def ping_iboss(self):
        ping = False
        self.publish("/Commands", str(Commands.PING.value) + ";")

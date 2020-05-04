import paho.mqtt.client as mqtt
from enums import *
import common

class MQTTClient(mqtt.Client):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._connected = False
        self.will_set("/ServerStatus", payload="Offline", qos=0, retain=True)
        self.connect(CONFIG_BROKER_IP, CONFIG_BROKER_PORT)
        self.loop_start()
        self.ready = False

    def on_disconnect(self, *args, **kwargs):
        self.ready = False

    def on_connect(self, client, userdata, flags, rc):
        self.publish("/Commands/Status", payload="Online", qos=0, retain=True)
        self.subscribe('/Data')
        self.subscribe('/Error')
        self.subscribe('/Info')

    def on_message(self, client, userdata, message):
        print(f"mqtt message {message.topic} {message.payload}")
        if message.topic == '/Data':
            common.log(message.payload)
        if message.topic == '/Info':
            if message.payload == b'IBLOCK CONNECT':
                self.ready = True
                self.set_status(common.should_status)
            if message.payload == b'IBLOCK DISCONNECT':
                self.ready = False
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

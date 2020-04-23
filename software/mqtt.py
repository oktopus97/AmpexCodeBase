import paho.mqtt.client as mqtt
from config import *
import common

class MQTTClient(mqtt.Client):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._connected = False
        self.connect(CONFIG_BROKER_IP, CONFIG_BROKER_PORT)
        self.loop_start()

    def on_connect(self, client, userdata, flags, rc):
        client.subscribe('/Data')
        client.subscribe('/Error')
        client.subscribe('/Info')

    def on_message(self, client, userdata, message):
        if messag.topic == '/Data':
            common.log_handler(message.payload)
        if message.topic == '/Info':
            if message.payload == b'IBLOCK CONNECT':
                common.ready_callback()
            if message.payload == str(MQTCommands.PING.value).encode():
                common.ping_callback();

        if message.topic == '/Error':
            common.status_var.set('ERROR: ' + message.payload)

    def set_status(self, stat):
        comm = str(MQTTCommands.SET_STATUS.value) + \
                str(stat if type(stat) == int else stat.value)
        self.publish("/Commands", comm)

    def ping_iboss(self):
        ping = False
        self.publish("/Commands", MQTTCommands.PING.value)

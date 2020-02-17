import paho.mqtt.client as client
import paho.mqtt.subscribe as subscribe
import time
import gui
import asyncio
import threading
from software.constants import host, port

started = False
connected = False
status = 'IBlock Disconnected'
gui.set_status(status)

start = 0

auth = {'username': 'master', 'password': '9931ab12'}
sensor_dict = {'sensor1', ('','C',)}

def set_status(stat):
    global status, connected
    if stat == b'CONNACK':
        stat = 'IBlock Connected, waiting for start signal'
        connected = True
    gui.set_status(stat)
    status = stat

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("connected OK Returned code=",rc)
    else:
        print("Bad connection Returned code=",rc)


def on_message(client, userdata, message):
    if message.topic == 'status':
        set_status(message.payload)
    if message.topic == 'error':
        raise NotImplementedError('Error handling non existent')
    if message.topic[0:10] == 'sensor_data':
        sensor = message.topic[message.topic.index('/'):]
        with open('{}.txt'.format(sensor), 'a') as f:
            f.write(message.payload)
        
        with open('times.txt', a) as f:
            f.write(time.time() - start)
        
        sensor_dict[sensor][0] = message.payload
        gui.set_current_vals(sensor_dict)

client = client.Client(client_id='master')
client.on_connect = on_connect
client.on_message = on_message

def connect():
    global connected, client, host, port
    if connected:
        print('already connected')
        return

    #ping the client every 5 sec
    client.connect(host, port, keepalive=60)
    
    for sensor in sensor_dict:
        client.subscribe('sensor_data/{}'.format(sensor))
    client.subscribe('error')
    client.subscribe('status')
    client.loop_start()
    time.sleep(4)


def start():
    global started, connected, client
    if not connected:
        print('not connected to iblock')
        return

    if started:
        print('already started')
        return
    client.publish('commands', 'start')
    set_status('Spinning')
    started = True

def stop():
    global started, client
    if not started:
        print('not running')

    client.publish('commands', 'stop')
    client.loop_stop()
    started = False

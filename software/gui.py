import os, sys
from serial.serialutil import SerialException
import common
import usb_serial
sys.path.append(os.getcwd())
from tkinter import *
import matplotlib
from matplotlib import pyplot as plt
import matplotlib.animation as anim
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
matplotlib.use('TkAgg')
import time
import subprocess

import mqtt
from config import *
matplotlib.style.use("ggplot")

class MainWindow(Tk):
    def __init__(self):
        super().__init__()
        self.initiated = True

        self.ip = subprocess.check_output(['hostname', '-I']).decode('utf-8').split(' ')[0]
        self.title('MoonFibre Spinner Controller @' + self.ip)
        self.toolbar = Frame(self)

        self.status = StringVar()
        self.status.set('No Connection')
        self.machine_status = StringVar()
        self.usb_status = StringVar()
        self.wifi_status = StringVar()

        Label(self.toolbar, textvariable=self.status).grid(row=1, column=0);
        Label(self.toolbar, textvariable=self.machine_status).grid(row=2, column=0);


        self.start_button = Button(self.toolbar, text='Start', command=self.start)
        self.start_button.grid(row=0, column=3)
        self.start_button['state'] = 'disabled'

        self.stop_button = Button(self.toolbar, text='Stop', command=self.stop)
        self.stop_button.grid(row=0, column=4)
        self.stop_button['state'] = 'disabled'

        self.config_button = Button(self.toolbar, text='Configure', command=lambda : controller.show_frame("ConfigFrame"))
        self.config_button.grid(row=0, column=0)
        
        self._mqtt_status = False
        self.mqtt_button = Button(self.toolbar, text="Start MQTT", 
                                    command=self.mqtt_fkt)
        self.mqtt_button.grid(row=0, column=1);

        self._serial_status = False
        self.serial_button = Button(self.toolbar, text="Start Serial", 
                                    command=self.serial_fkt)
        self.serial_button.grid(row=0, column=2)

        self.toolbar.grid(row=0, column=0, sticky='nsew')

        self.container = Frame(self)
        self.container.grid(row=1, column=0)

        self.frames = {}
        for F in (PlotsFrame, ConfigFrame):
            page_name = F.__name__
            frame = F(master=self.container, controller=self)
            self.frames[page_name] = frame
            frame.grid(row=0, column=0, sticky="nsew")
        
        self.sensor_vars = {}
        for i, name in enumerate(self.frames["PlotsFrame"].sensors.keys()):
            self.sensor_vars[name] = StringVar() 
            self.sensor_vars[name].set(name)
            Label(self, textvariable=self.sensor_vars[name]).grid(row=2+i, column=99)

        self.show_frame("PlotsFrame")
        common.set_sensor_vars(self.sensor_vars)
        common.status_var = self.machine_status

    @property
    def mqtt_status(self):
        return self._mqtt_status
    @mqtt_status.setter
    def mqtt_status(self, val):
        self._mqtt_status = val
        if val:
            self.wifi_status.set(f"Server connected @{CONFIG_BROKER_IP}")
        else:
            self.wifi_status.set('Server disconnected')

        if not self._mqtt_status or self._serial_status:
            self.initiated = False
            self.status.set("Disconnected")

    @property
    def serial_status(self):
        return self._serial_status
    @serial_status.setter
    def serial_status(self, val):
        self._serial_status = val
        if val:
            self.usb_status.set('disconnected USB')
        else:
            self.usb_status.set('disconnected USB')
        if not self._mqtt_status or self._serial_status:
            self.inititated = False
            self.status.set("Disconnected")
        else:
            self.status.set('Connected')


    def show_frame(self, page_name):
        '''Show a frame for the given page name'''
        if page_name == 'PlotsFrame':
            self.config_button['text'] = 'Configuration'
            self.config_button['command'] = lambda: self.show_frame('ConfigFrame')
        elif page_name == 'ConfigFrame':
            self.config_button['text'] = 'Plots'
            self.config_button['command'] = lambda: self.show_frame('PlotsFrame')

        frame = self.frames[page_name]
        frame.tkraise()

    def mqtt_disconnected(self):
        if CONFIG_LOCALSERVER:
            self.server.kill()
            self.server.wait()
        self.mqtt_status=False
        self.mqtt_button["text"] = "Start MQTT"

    def mqtt_fkt(self):
        """
        for imlementation look class MQTTClient
        """
        print("mqtt button pressed")
        if self.mqtt_status:
            self.mqtt_disconnected()
            return
        else:
            if CONFIG_LOCAL_SERVER:
                print("Starting Local Server")
                #stop if mosquitto is already running, so communication with the
                #server client is possible
                if subprocess.run(["pkill",  "mosquitto"]).returncode != 1:
                    time.sleep(3)
                self.status.set('Running Server')
                self.server = subprocess.Popen(['mosquitto', "-p", str(CONFIG_BROKER_PORT)])
                time.sleep(1)
                serv = True
            else:
                serv = False
            self.mqtt_status = True
            self.status.set("Local Server running" if serv else 
                            "Conected to server @{}".format(CONFIG_BROKER_IP))
            self.mqtt_client = mqtt.MQTTClient(client_id=('Ampex GUI @'+ self.ip))
            self.mqtt_button["text"] = "Stop MQTT"   
            if self.initiated is False:
                self.mqtt_client.ping_machine()
                self.ready()
    
    def usb_disconnected(self):
        self.serial_button["text"] = "Start Serial"
        self.serial_status = False
        self.serializer.stop_loop()

    def serial_fkt(self):
        if self.serial_status:
            self.usb_disconnected()
            return self.serial_status
        else:
            self.serializer = usb_serial.Serializer(CONFIG_SERIAL_PORT)

            tries = 0
            while True:
                try:
                    self.serializer.start_loop()
                    break
                except SerialException as e:
                    tries += 1;
                    if tries > 10: 
                        if e.errno == 2:
                            self.status.set("NO USB FOUND") #TODO red
                            return -1
                        if e.errno == 13:
                            self.status.set(f"run sudo chmod 777 {CONFIG_SERIAL_PORT}")
                            return -1
            print("serial loop started")
            self.serial_status = True
            while True:
                pi = self.serializer.ping_machine()
                print(f"ping answer -> {pi}")
                if pi  == 1:
                    break

            self.serial_status = True
            self.serial_button["text"] = "Stop Serial"
            self.ready()

            self.initiated = True

            return self.serial_status
        
    def get_clt(self):
        if self.serial_status:
            return self.serializer
        elif self.mqtt_status:
            return self.mqtt_client
        else:
            raise common.ControllerException('No Connection')

    def ready(self):
        clt = self.get_clt()
        clt.set_status(MachineStatus.READY)
        
        common.ready = True
        self.start_button["state"] = "normal"

    def start(self):
        clt = self.get_clt()
        self.clt.set_status(MachineStatus.SPINNING)
        self.start_button["state"] = "disabled"
        self.stop_button["state"] = "normal"
        time.sleep(3)
        self.PlotsFrame.start_plotting()
    
    def stop(self):
        clt = self.get_clt()
        self.clt.set_status(MachineStatus.INTERRUPT)
        self.stop_button["state"] = "disabled"
        self.start_button["state"] = "normal"

class PlotsFrame(Frame):
    def __init__(self, master, controller):
        super().__init__(master)
        self.master = master
        self.controller = controller
        self.fig, self.axes = plt.subplots(nrows=2, ncols=2)
        self.canvas = FigureCanvasTkAgg(self.fig, self)
        self.times = []

        #example sensors
        self.sensors = {'ex1':([], self.axes[0][0]), 'ex2':([], self.axes[0][1]),
                'ex4':([], self.axes[1][0]), 'ex3':([], self.axes[1][1])}

        
        for key, value in self.sensors.items():
            value[1].set_title(key)
        self.canvas.get_tk_widget().pack()

        common.set_plots(self.times, self.sensors)
    
    def update_plots(self, i):
        for value in self.sensors.values():
            value[1].clear()
            value[1].plot(value[0])
        self.canvas.draw()
   
    def start_plotting(self):
        ani = anim.FuncAnimation(self.fig, self.update_plots)


class ConfigFrame(Frame):
    def __init__(self, master, controller):
        super().__init__(master)
        self.controller = controller

if __name__ == "__main__":
    app = MainWindow()
    
    app.mainloop()

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
import configparser

import mqtt
from enums import *
matplotlib.style.use("ggplot")

class MainWindow(Tk):
    def __init__(self):
        super().__init__()
        self.config = configparser.ConfigParser()
        self.config.read("conf.ini")

        self.initiated = True

        self.ip = subprocess.check_output(['hostname', '-I']).decode('utf-8').split(' ')[0]
        self.title('MoonFiber Spinner Controller @' + self.ip)
        self.toolbar = Frame(self)

        self.wifi_status = StringVar()
        self.usb_status = StringVar()
        self.machine_status = StringVar()

        for i, var in enumerate((self.machine_status, self.usb_status,
                                    self.wifi_status)):
            var.set("No Connection")
            Label(self.toolbar, textvariable=var).grid(row=i+1, column=0);



        self.start_button = Button(self.toolbar, text='Start', command=self.start)
        self.start_button.grid(row=0, column=3)
        self.start_button['state'] = 'disabled'

        self.stop_button = Button(self.toolbar, text='Stop', command=self.stop)
        self.stop_button.grid(row=0, column=4)
        self.stop_button['state'] = 'disabled'

        self.droptest_button = Button(self.toolbar, text='Drop Test', command=self.droptest)
        self.droptest_button.grid(row=0, column=5)
        self.droptest_res = StringVar()
        Label(self.toolbar, textvariable=self.droptest_res)
        
        self.config_button = Button(self.toolbar, text='Configure', command=lambda : controller.show_frame("ConfigFrame"))
        self.config_button.grid(row=0, column=0)

        self._mqtt_status = False

        self.mqtt_client = mqtt.MQTTClient(client_id=('Ampex GUI @'+ self.ip),
                                           status_var=self.wifi_status,
                                           status_prop=self.mqtt_status,
                                           ready=self.ready)

        self._serial_status = False

        self.serializer = usb_serial.Serializer(self.config["DEFAULTS"]["CONFIG_SERIAL_PORT"],
                                                var=self.usb_status,
                                                prop=self.serial_status,
                                                after=lambda delay, callback: self.after(delay, callback),
                                                ready=self.ready)

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
    
    def droptest(self):
        if self.serializer.droptest():
            self.droptest_res.set("Drop Detected")
    @property
    def mqtt_status(self):
        return self._mqtt_status
    @mqtt_status.setter
    def mqtt_status(self, val):
        self._mqtt_status = val
        if not self._mqtt_status or self._serial_status:
            self.initiated = False

    def get_serial_status(self):
        return self.serial_status
    @property
    def serial_status(self):
        return self._serial_status

    @serial_status.setter
    def serial_status(self, val):
        self._serial_status = val

        if not self._mqtt_status or self._serial_status:
            self.inititated = False

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

    def get_clt(self, **kwargs):
        if self.serial_status or kwargs.get("force", None) == "usb":
            return self.serializer
        elif self.mqtt_status or kwargs.get("force", None) == "mqtt":
            return self.mqtt_client
        else:
            raise common.ControllerException('No Connection')

    def ready(self, connection, **kwargs):
        if self.initiated is False:
            self.initiated = True
        if connection == "mqtt":
            self.mqtt_status = True
        elif connection == "usb":
            self.get_clt(force="usb").ping_machine()
            self.serial_status = True

        common.ready = True
        self.start_button["state"] = "normal"
        self.frames["PlotsFrame"].start_plotting()

    def start(self):
        clt = self.get_clt()
        common.should_status = MachineStatus.HEAT
        clt.set_status(common.should_status)
        self.start_button["state"] = "disabled"
        self.stop_button["state"] = "normal"
        time.sleep(3)

    def stop(self):
        clt = self.get_clt()
        common.should_status = MachineStatus.INTERRUPT
        clt.set_status(common.should_status)
        self.stop_button["state"] = "disabled"
        self.start_button["state"] = "normal"

class PlotsFrame(Frame):
    def __init__(self, master, controller):
        super().__init__(master)
        self.master = master
        self.controller = controller
        self.fig, self.axes = plt.subplots(nrows=2, ncols=2)
        self.canvas = FigureCanvasTkAgg(self.fig, self)
        self.log = open("log", "r+")
        self.log.seek(0)
        self.times = []

        #example sensors
        self.sensors = {'ex1':([], self.axes[0][0]), 'ex2':([], self.axes[0][1]),
                'ex4':([], self.axes[1][0]), 'ex3':([], self.axes[1][1])}


        for key, value in self.sensors.items():
            value[1].set_title(key)
        self.canvas.get_tk_widget().pack()

        common.set_plots(self.times, self.sensors)

    def update_plots(self):
        if self.plot:
            self.after(2000, self.update_plots)
        if self.log_lines == 500:
            self.log.seek(0)
        self.log.seek(0)

        lines = self.log.readlines()
        for line in lines:
            common.log_handler(line)
        self.log.seek(0)
        self.log.truncate(0)

        for value in self.sensors.values():
            value[1].clear()
            value[1].plot(value[0])
        self.canvas.draw()

    def start_plotting(self):
        self.log_lines = 0
        common.start_logging()
        self.plot = True
        self.after(1000, self.update_plots)

    def stop_plotting(self):
        common.stop_logging()
        self.plot = False

class ConfigFrame(Frame):
    def __init__(self, master, controller):
        super().__init__(master)
        self.conf = master.master.config
        self.serial_status = master.master.get_serial_status
        self.get_clt = master.master.get_clt
        self.controller = controller
        self.local_server = IntVar()
        self.local_server.set(int(self.conf["DEFAULTS"]["config_local_server"]))
        Checkbutton(self, text="Local Server",
                variable=self.local_server,
                command=self.local_server_check).grid(row=0, column=0)
        self.entries = {}
        for i, conf in enumerate(self.conf["DEFAULTS"].keys()):
            if conf == "config_local_server":
                continue
            Label(self, text=conf[6::].replace("_", " ")).grid(row=i+1, column=0)
            entry = Entry(self)
            entry.grid(row=i+1, column=1)
            self.entries[conf] = entry
        self.read_defaults()

        Button(self, text="Save Config", command=self.save).grid(row=1, column=3)
        flashers = []
        for i, feature in enumerate(("IP", "Broker Settings", "WiFi Settings")):
            function_name = "flash_" + feature.replace(" ", "_")
            flashers.append(Button(self,text=f"Flash {feature}",
                command=getattr(self, function_name)))
            flashers[-1].grid(row=1, column=i+4)

    def save(self):
        for key, entry in self.entries.items():
            self.conf["DEFAULTS"][key] = entry.get()
            with open("conf.ini", "w") as f:
                self.conf.write(f)

    def flash_IP(self):
        self.save()
        if not self.serial_status():
            print("Serial Not On")
            return
        self.get_clt().set_ip(self.conf["DEFAULTS"]["config_machine_ip"],
                        self.conf["DEFAULTS"]["config_gateway"],
                        self.conf["DEFAULTS"]["config_subnet"])
    def flash_Broker_Settings(self):
        if not self.serial_status():
            print("Serial Not On")
            return
        self.save()
        self.get_clt().set_broker(self.conf["DEFAULTS"]["config_broker_ip"],
                                  self.conf["DEFAULTS"]["config_broker_port"])

    def flash_WiFi_Settings(self):
        if not self.serial_status():
            print("Serial Not On")
            return
        self.save()
        self.get_clt().set_wifi(self.conf["DEFAULTS"]["config_wifi_ssid"],
                                self.conf["DEFAULTS"]["config_wifi_password"])

    def read_defaults(self):
        for key, entry in self.entries.items():
            entry.delete(0, END)
            entry.insert(0, self.conf["DEFAULTS"][key])

    def local_server_check(self):
        if self.local_server.get():
            self.conf["DEFAULTS"]["config_local_server"] = "1"
            self.conf["DEFAULTS"]["config_broker_ip"] = "localhost"
            self.entries["config_broker_ip"].configure(state="disabled")
        else:
            self.conf["DEFAULTS"]["config_local_server"] = "0"
            self.entries["config_broker_ip"].configure(state="normal")
        with open("conf.ini", "w") as f:
            f.seek(0)
            self.conf.write(f)




if __name__ == "__main__":
    app = MainWindow()

    app.mainloop()

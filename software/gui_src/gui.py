import os, sys
sys.path.append(os.getcwd())
from tkinter import *
from tools import TextWithVar
import matplotlib
from matplotlib import pyplot as plt
import matplotlib.animation as animation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
matplotlib.use('TkAgg')
import threading
import queue

import software.gui_src.comms

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('-m', '--mode', help='mode of gui')
parser.add_argument('-ip', help='ip address of mqtt', default='127.0.0.1')
parser.add_argument('-port', help='ip address of mqtt', default='8883')

args = parser.parse_args()

import software.constants as c
c.host = args.ip
c.port = int(args.port)

#our app
root = Tk()
root.title('MoonFibre IBlock Controller')

#adjust plotting grid
fig, axes = plt.subplots(nrows=2, ncols=2)
canvas = FigureCanvasTkAgg(fig, root)
sensor1_readings = []
times = []

#animations
def animate(i, axis, readings_list, file_to_read):
    with open(os.getcwd() + '/software/gui_src/' + file_to_read, 'r') as f:
        f.seek(0)
        readings_list = f.read().split('\n')

    with open(os.getcwd() + '/software/gui_src/times.txt', 'r') as f:
        f.seek(0)
        times = f.read().split('\n')
    axis.clear()
    axis.plot(times, readings_list)

ani1 = animation.FuncAnimation(fig, animate, fargs=(axes[0][0], sensor1_readings, 'sensor1.txt'), interval=1000)

#buttons
def on_connect():
    comms.connect()

def on_start():
    comms.start()

def on_stop():
    comms.stop()

connect = Button(root, text='Connect', command=on_connect)
start = Button(root, text='Start', command=on_start)
stop = Button(root, text='Stop', command=on_stop)

#text variables to show the status and current readings
status_text = StringVar()
status = Label(root, textvariable=status_text)

currents_text = StringVar()
currents_text.set('temp = 21 C')
currents = Label(root, textvariable=currents_text)

def set_status(status):
    status_text.set(status)

def set_current_vals(sensor_dict):
    string = ''
    for sensor in sensor_dict:
        string += '{} = {} {} \n'.format(sensor, sensor_dict[sensor][1], sensor_dict[sensor][0])
    currents_text.set(string)
#comms module is using the above functions, safer to import after decleration 
import comms

#setup gui
canvas.draw()
canvas.get_tk_widget().grid(row=0, column=0)

currents.grid(row=0,column=1)
if args.mode == 'wifi':
    #some info about the server from the pipe
    serverpipe = StringVar()
    serverpipe.set('initiating mosquitto server...')
    cli = TextWithVar(root, textvariable=serverpipe)
    cli.grid(row=0, column=10)

    stdin_q = queue.Queue()

    def start_stdin_reading():
        global stdin_q
        def stdin_thread(q):
            q = q
            while 1:
                q.put(sys.stdin.read())

        thread = threading.Thread(target=stdin_thread, args=(stdin_q,))
        thread.daemon = True
        thread.start()

    def print_stdin():
        global serverpipe, stdin_q
        old = serverpipe.get().split('\n')
        if len(old) > 15:
            old = old[-15:]
        try:
            serverpipe.set("\n".join(old) + stdin_q.get_nowait() + '\n')
        except queue.Empty:
            pass

        root.after(100, print_stdin)
    
    start_stdin_reading()
    root.after(100,print_stdin)

for i, button in enumerate((connect, start, stop)):
    button.grid(row=1, column=i)

status.grid(row=2, column=0)

root.mainloop()

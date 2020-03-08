* connect the wires ->
        ** grey -> 16
        ** blue  -> 18
        ** if connected to computer: orange -> GND

# 2 server setup
* set the ips and wifi settings ->

 ** edit midterm_test.ino lines 3-11
 ** compile and upload from arduino ( sudo priviliges maybe necessary run through terminal `sudo arduino`


* run server ->
** run `mosquitto -p $port`
** Monitor if the client has connected

# Notes
** if there is no connection press the reset button on arduino
** if the wifi is connected led on the arduino will be lit


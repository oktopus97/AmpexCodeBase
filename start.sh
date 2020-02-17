#!/bin/bash
cd ~/AmpexCode
if [ "$1" = "wifi" ]
then
	echo $1
	#pipe the server to python script
	mosquitto -p 8883 -v -c $(pwd)/conf/mosquitto.conf | python3 $(pwd)/software/gui_src/gui.py -m "$1"
else
	python3 $(pwd)/software/gui_src/gui.py -m "$1"
fi


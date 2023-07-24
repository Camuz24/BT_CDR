#!/bin/bash

echo "Buon Allenamento :)"
sleep 15

rfkill block bluetooth
rfkill unblock bluetooth
bluetoothctl power on

sudo DISPLAY=:0.0 /home/pi/Desktop/FES_MP_Motor/build/FES_MP_Motor_FES & 
sleep 10; sudo DISPLAY=:0.1 /home/pi/Desktop/BT_CDR/build/bluetooth_socket_server

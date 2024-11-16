# hackyLapse

ESP32 HID time lapse app with captive web configurator for LILYGO T-Dongle


This project was quickly cobbled together from a bunch of other ESP32 
projects because I needed a long-running time lapse controller for 
capturing night images on my iPhone. 

This is a platformio project (https://platformio.org) and should build with the latest platformio.  
Quick CLI install: https://docs.platformio.org/en/latest/core/installation/index.html


Building and installing
Plug in your LILYGO T-dongle (while holding the button if it doesn't work the first time)

.pio run -t upload

You may get an error about "atomic.h" not found.  Just delete the line that has #include "atomic.h" from the indicated file and build again.


Using:
PLug the T-dongle into a USB port.  It will say "Pair Bluetooth".  On the device you're using to take pictures, pair the device.  It will behave as an HID keyboard and hit the "volume up" button after the timer expires to take a picture.  This is a standard shutter shortcut, but if you need another button, look for SHUTTER_KEY in src/ble_task.c and change it to what you want.


The timer defaults to 30 seconds, and repeats forever.  If you want to change it or use the shutter manually without touching the phone, use another device and connect to the HackyLapse wifi access point.  It will ask for a password.  The default password is 'hackylapse'.  Once connected, it will automatically open a window that has timer controls, pause/play, and a manual shutter button.



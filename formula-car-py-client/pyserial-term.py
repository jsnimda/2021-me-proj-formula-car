#!/usr/bin/env python3

import serial
import time

ser = serial.serial_for_url("socket://esp32-js.local:23", timeout=0)

for i in range(1000):
    while ser.inWaiting():
        # Warning! inWaiting() is returning incorrect bytes (1 byte only)!!
        # (read_all() is not properly working too!)
        # r_size = ser.inWaiting()
        # b = ser.read(1024)
        b = ser.read_all()
        print(b)
    ser.write(b'Hello, world\r\n')
    time.sleep(1)

# print('Received', repr(data))

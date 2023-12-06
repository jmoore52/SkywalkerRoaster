import serial
import time

serial_port = 'COM13'

baud_rate = 9600
with serial.Serial(serial_port, baud_rate, timeout=1) as ser:
    ser.write(b'\n')
    time.sleep(.5)
    if ser.in_waiting > 0:
        response = ser.readline().decode('utf-8').strip()
        print(f"{response}")
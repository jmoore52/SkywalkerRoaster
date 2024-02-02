# Skywalker Roaster

This is a first draft at getting the Skywalker Roaster connected to Artisan. In addition to the arduino sketches the data I've collected about the roaster is available here. 

## Arduino Sketches 

### SkyCommand
This is full control of the skywalker roaster via Artisan. The sketch does the bare minimum to emulate TC4. Exercise caution, there are minimal safety features built in. As with every other coffee roaster.. DO NOT LEAVE THIS UNATTENDED. The sketch requires that it see a command from Artisan at least every 10 seconds or it will shut down. The READ commands from artisan will keep it going. I think it's ok to run, but what do I know? I'm just a nerd on the internet. Use this at your own risk. 

#### Hardware
I use an arduino leonardo with a USB cable soldered to the Vin, GND, and Digital Pins 2 and 3. That's it. 
![PXL_20231206_190446798 MP](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/d03d5bd3-de08-4ee2-986c-d3fef3e07c47)

The black cable goes to the USB port on the back of the roaster (that's only USB in form.. it's not actually using USB communications), the white one to my computer running Artisan. 

#### Instructions

1. Install ArduinoIDE

2. Find any old USB cable. Cut it up in half, and prune a bit off the plastic sleeving. You should find four smaller wires. Connect these wires to your Arduino as such either through soldering or just inserting it into the slots:

Blue -> Digital 2
White -> Digital 3
Red -> VIN
Black -> GND

3. Download repository and extract.

Then, for TimerInterrupt.h either comment it out like so:
![image](https://github.com/paoyong/SkywalkerRoaster/assets/6934989/2ec65309-278c-4a41-8d1a-4df7fc7123c7)

or download TimerInterrupt library [here](https://www.arduino.cc/reference/en/libraries/timerinterrupt/) and copy paste it into the SkyCommand folder like so:
![image](https://github.com/paoyong/SkywalkerRoaster/assets/6934989/6d74cc86-e3f8-47c0-bc41-1c0bfb45a20b)

#### Artisan Config
Here's how I have Artisan configured. 
Config -> Device
![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/d4a61e74-070b-4bd8-a6c0-63acc1e8f28d)

Config -> Events -> Config
![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/10172b83-d5c5-4bc0-9dba-fe810b0541ef)

Config -> Events -> Buttons
![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/b899f2df-6f41-4187-93f3-4e7eb4a2fc4c)

Config -> Events -> Sliders
![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/5ddbc13f-aeee-4911-96f2-38bab24269b6)


### SkywalkerSpy
This provides logging only for Artisan. It uses two pins to monitor the Tx lines from both the roaster and the controller. It alternates between reading the values as quickly as possible. When any input comes in on the serial line the sketch will respond with the status of the roaster in the format TEMP,HEAT DUTY,VENT DUTY 

#### Artisan Config
TODO

## Roaster Hardware
This has been super fun to study. Here's what I've learned so far. 

The controller has a [js32t031f5s7 microcontoller](http://www.honor-ic.com/Product/ProScreenDetail?pid=118). 

I am unsure what kind of microcontroller is inside the roaster. It is marked FMD N3hWIKH and I was unable to locate any datasheets for it. 

![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/9667b4ed-4d56-44d6-9c13-1d5d7ac2737e)

The roaster uses a thermistor as a temperature probe which is connected to the microcontroller as shown here. 

![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/b5e678ef-a7f5-44a4-83ef-9cf92c3277f3)

It appears to use a voltage divider with two taps, one passing through a 1k resistor and the other through a 2k resistor. I'm not the strongest hardware guy by any means, but I don't understand why. If this look familiar to you, please let me know what we're looking at here. 


### Normal Operation
The Skywalker roaster is rather simple all logic lives in the controller. The roaster simply interprets the signals sent by the controller and sets things accordingly. The roaster appears to shut down if it does not receive a control message in 1 second. The controller reads the temperature values from the roaster and then sends instructions to the roaster. There does not appear to be any kind of communications protocol, each device appears to send messages to the other device roughly every 10 milliseconds. There are no requests, responses, acknowledgements, etc..  Simply data bit banged on a GPIO line. 

## Messages
The messages sent by each device are rather simple in structure. I've determined what most everything does. To the logic analyzer, the messages appear as follows. This is an example of the message the roaster sends. The controller's messages look identical other than being 8 bits shorter. 

![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/ba62c969-7bf0-4eb5-afd7-8f788b759ce2) 

Interpreting the messages can be done by simply measuring the logic LOW pulses. The beginning preamble is roughly 7.5ms, a binary 1 is represented by a ~1.5ms pulse, a binary 0 is ~600 microseconds, and the logic HIGH between bits is roughly 750 microseconds.

![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/82005860-83ae-40a5-8574-5bc535f85420)

### Controller
The controller transmits a 6 byte message in LSB order. Both messages contain a checksum and will be ignored if the checksum is not correct.

| Position | Purpose | Value |
|----------|---------|-------|
| 0 | Fan Duty Cycle | 0 to 100
| 1 | Filter | Appears to only be values 0,1,2,3,4 |
| 2 | Bean Cooling Fan | 0 or 100 |
| 3 | Drum | 0 or 100 |
| 4 | Heater Duty Cylce | 0 to 100 |
| 5 | Checksum | |

For the Fan and Heater, duty cycles increment by 5. Sending other values will be ignored. 

### Roaster
The roaster transmits a 7 byte message in LSB Order. 
|Position| Name| Value|
|--------|-----|------|
|0,1| Value A | I'm not _really_ sure what this value is. I suspect it is perhaps a voltage as read by an ADC on the microcontroller|
|2,3|Value B|Again, not _really_ sure what the value really represents. I think it is another ADC value.|  
| 4 | Mystery byte | Always a 0.|
| 5 | Mystery byte | Pretty much always a 1. I've not seen much of a correlation to hardware that seems to indicate what this may represent.|
|6 | Checksum||

Value A and Value B are my biggest open question about this roaster. What is it doing?! There is a very strong linear correlation between the two values and the temperature displayed on the controller screen. However I could not determine what I would consider to be an "elegant" solution for converting these values to the temperature. Rather, I fit a 4th degree polynomial to the data (see `data/model3.py` but please keep in mind it's been a long minute since I've run a regression. ChatGPT wrote a lot of that with some guidance.) 

To collect the data I attached a logic probe to the controller and captured a preheat cycle. I extracted all the messages from the capture, wrote an arduino sketch to replay the bytes in the messages back to the controller, and coded up a quick and dirty python script to push each message to the arduino and allow me to enter the value shown on the screen. The results of this process are in `Data/RealLabeledTemperatures.txt` 

# Bug Fixes and Improvements

I'll spend some more time on things when I can, but pull requests are always welcome and appreciated.  



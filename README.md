# Skywalker Roaster

This is a first draft at getting the Skywalker Roaster connected to Artisan. In addition to the arduino sketches the data I've collected about the roaster is available here. 

## Arduino Sketches 

### SkyCommand
This is full control of the skywalker roaster via Artisan. The sketch does the bare minimum to emulate TC4. Exercise caution, there are minimal safety features built in. As with every other coffee roaster.. DO NOT LEAVE THIS UNATTENDED. The sketch requires that it see a command from Artisan at least every 10 seconds or it will shut down. The READ commands from artisan will keep it going. I think it's ok to run, but what do I know? I'm just a nerd on the internet. Use this at your own risk. 

#### Hardware
I use an arduino leonardo with a USB cable soldered to the Vin, GND, and Digital Pins 2 and 3. That's it. 

#### Artisan Config
TODO

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

Interpreting the messages can be done by simply measuring the logic LOW pulses. The beginning preamble is roughly 7.5ms, a binary 1 is represented by a ~1.5ms pulse, a binary 6 is ~600 microseconds, and the logic HIGH between bits is roughly 750 microseconds. 

![image](https://github.com/jmoore52/SkywalkerRoaster/assets/25308608/82005860-83ae-40a5-8574-5bc535f85420)

### Controller
The controller transmits a 6 byte message in LSB order. Both messages contain a checksum and will be ignored if the checksum is not correct.

Byte 0 - Fan Duty Cycle - 0 to 100
Byte 1 - Filter - Appears to only be values 0,1,2,3,4. 
Byte 2 - Bean Cooling Fan - 0 or 100 
Byte 3 - Drum - 0 or 100
Byte 4 - Heater Duty Cylce - 0 to 100
Byte 5 - Checksum

For the Fan and Heater, duty cycles increment by 5. Sending other values will be ignored. 

### Roaster
The roaster transmits a 7 byte message in LSB Order. 

Bytes 0,1 - Value A - I'm not _really_ sure what this value is. I suspect it is perhaps a voltage as read by an ADC on the microcontroller.
Bytes 2,3 - Value B - Again, not _really_ sure what the value really represents. I think it is another ADC value.  
Byte 4 - Mystery byte - Always a 0. 
Byte 5 - Mystery byte - Pretty much always a 1. I've not seen much of a correlation to hardware that seems to indicate what this may represent. 
Byte 6 - Checksum

Value A and Value B are my biggest open question about this roaster. What is it doing?! There is a very strong linear correlation between the two values and the temperature displayed on the controller screen. However I could not determine what I would consider to be an "elegant" solution for converting these values to the temperature. Rather, I fit a 4th degree polynomial to the data (see `data/model3.py` but please keep in mind it's been a long minute since I've run a regression. ChatGPT wrote a lot of that with some guidance.) 

To collect the data I attached a logic probe to the controller and captured a preheat cycle. I extracted all the messages from the capture, wrote an arduino sketch to replay the bytes in the messages back to the controller, and coded up a quick and dirty python script to push each message to the arduino and allow me to enter the value shown on the screen. The results of this process are in `Data/RealLabeledTemperatures.txt` 





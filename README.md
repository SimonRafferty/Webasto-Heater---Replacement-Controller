# Webasto-Heater---Replacement-Controller---V4.0---WIP
New strictly Work-In-Progress version intended to run on V4 Hardware only.
Use this as a source of ideas only.

The Hardware is listed here:  https://oshwlab.com/simonrafferty/webastardo-v4-0_copy

I had V4 working with a Webasto Heater.  Then I tried it with an HCalory - and ran into problems.  Firstly the change in resistance with temperature of the heater plug was too marginal for the flame sensing to work reliably.  HCalory use an Exhaust Thermistor - and I guess that's why!  I bought a thermocouple which screwed into the same hole as the thermistor.

The Fan motor on the HCalory also produced too much noise - enough to fry the flyback diode, leading to the MOSFET dying too.  I replaced the diode (Q5D1) with a higher current version - and it worked OK.  The current version of the PCB has provision for a through-hole, chunky diode in that location rather than the SMD one used on the other channels.

The Hardware & Software are both experimental!  I ran out of time playing with it - so I can't guarantee it works as intended.  I'm not currently using this version to control anything - so I'm not sure about it's long term fitnes for purpose.

I have thought about a V5 (but not actually built it yet).  My plan is to use an ESP32 (probably an XIAO ESP32 S3 as they are super cheap & tiny) combined with an ADS1115 4x 16 Bit ADC to overcome the ESP32 ADC issues I had in V1. It will go back to using thermistors for the water & exhaust temperature as I've found them te most reliable overall. Id's like the board to be much smaller too.  Watch this space as they say! 

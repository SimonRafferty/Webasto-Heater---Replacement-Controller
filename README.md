# Webasto-Heater---Replacement-Controller
An Arduino M0 based controller for Webasto C/E Diesel Water Heaters

PCB here: https://oshwlab.com/SimonRafferty/webasto-controller

This is a replacement controller for a Webasto C or E water heater. The original heater, intended mainly to heat engine blocks, heats at full power until the water reaches 80C, then continues at half power. If it reaches 90C, it shuts down then re-starts when the water temp drops below 50. For an engine, that works just fine.

I wanted to use it to heat water and provide underfloor heating in my Camper Van. The rate of heat transfer to my hot water cylinder and underfloor heating was a lot less than the full 5kW power of the heater. The result was it would start up, almost immediately reach 90C then shut down. Very quickly cool to 50C and start up again. This cycle would repeat every 5 minutes or so. During startup & shutdown, it runs aglow plug (to pre-heat the diesel) which draws 20A. The short-cycling of the heater meant the glow plug was switched on almost constantly - drawing a lot of battery power.

My solution was to make the controller vary the power by changing the amount of fuel (and air) delivered. It tries to achieve a target temperature then regulates the power to attempt to maintain that temperature without shutting down. Eventually the system will reach a shutdown temperature, either because of a problem, or because the system cannot take any more heat - and shut down. Like the original, it will re-start at 50C.

At lowest power, it is developing about 1kW of heat and at maximum, maybe 4kW. In the code, you can change the fueling at different temperatures, the default is just what I found worked for me.

NOTE:
This is a DIY, Experimental project.  It is not intended as a turn-key, ready made solution for everybody.  If you are not confident with Electronics & Coding - please walk away.  

I do not accept responsibility if your Car, Van, House - or you burst into flames!  

At the very least, build a test rig, pumping water in & out of a bucket to tune & test the rig.  

If, once you are happy with it, you want to install it in your van, it is your responsibility to take suitable precautions and protect you and your property from fire.

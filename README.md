# Webasto-Heater---Replacement-Controller - V3
An Arduino M0 WiFi based controller for Webasto C/E Diesel Water Heaters

At the moment, this is just a placeholder for the next version.
It's based on the new Adafruit Feather M0 WiFi - which as you might have guessed, has WiFi on board!

The main difference however, is it includes an ACS711 Current sensor in line with the Glow plug.  This is to measure the current & calculate the resistance of the plug.

It's resistance is proportional to the exhaust temperature - so this can be used in place of an external sensor.  It's how the original controller works.

This version is still experimental and needs tuning - check back later!

The flame sensor works pretty well.  At the moment I'm trying to calculate the temperature from the flame sensor - but it's not very accurate.  Enough though to detect a flame though.

Initially, I set up a simple WiFi Web Server for control, but each time a client requested a page, it interrupted the running of the heater control too much.  Instead, I've set it up using Blynk for control.  This has a few advantages, other than it being much faster.  As Blynk has it's own servers, the heater can be controlled from anywhere either via a web page or a mobile app.
![image](https://user-images.githubusercontent.com/13219057/208894111-aaec3692-a0ec-4785-b11c-fa1f88bb0fd0.png)

I'm going to use the Exhaust Thermistor pin on the 6 pin connector with a 10k Thermistor to measure the room temperature.  My plan is to install the Webasto in my office, connected to a domestic radiator.

Message me if you have any thoughts about the WiFi / Blynk control - what would be useful to add for example?


# Webasto-Heater---Replacement-Controller - V3
An Arduino M0 WiFi based controller for Webasto C/E Diesel Water Heaters

This version has some compile time directives at the start of Webastardo-SAMD21_WiFi.ino
```
#define BLYNK_ENABLE              //Uncomment if you want to send data to Blynk (only applies to M0 WiFi board)
#define FLAME_SENSOR_ENABLE       //Uncomment if using V3.0 board with ACS711 Current Sensor
#define M0_WIFI_ENABLE            //Uncomment if you are using a Feather M0 WiFi microcontroller
```
If you are using the M0 WiFi controller with a V1 or V2 board, just comment out #define FLAME_SENSOR_ENABLE

If you are using a V3 board with a regular Feather M0 (without WiFi) - comment out #define BLYNK_ENABLE and #define M0_WIFI_ENABLE

If you want to use a web interface rather than Blynk - comment out #define BLYNK_ENABLE

You get the idea!

The V3.0 PCB includes an ACS711 Current sensor in line with the Glow plug.  This is to measure the current & calculate the resistance of the plug.

It's resistance is proportional to the exhaust temperature - so this can be used in place of an external exhaust sensor.  It's how the original controller works.

The flame sensor works pretty well.  At the moment I'm trying to calculate the temperature from the flame sensor - but it's not very accurate.  Enough though to detect a flame though.  A constant [flame_theshold] is defined in Webastardo-SAMD21_WiFi.ino  this is the 'temperature' threshold I found worked reliably to detect the presence or absence of a flame.  You may need to tweak this value?

Initially, I set up a simple WiFi Web Server for control, but each time a client requested a page, it interrupted the running of the heater control too much.  Instead, I've set it up using Blynk for control.  This has a few advantages, other than it being much faster.  As Blynk has it's own servers, the heater can be controlled from anywhere either via a web page or a mobile app.
![image](https://user-images.githubusercontent.com/13219057/208894111-aaec3692-a0ec-4785-b11c-fa1f88bb0fd0.png)

I'm going to use the Exhaust Thermistor pin on the 6 pin connector with a 10k Thermistor to measure the room temperature.  My plan is to install the Webasto in my office, connected to a domestic radiator.

Message me if you have any thoughts about the WiFi / Blynk control - what would be useful to add for example?


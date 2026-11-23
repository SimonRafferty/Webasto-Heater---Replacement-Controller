# Webastardo V5 – Replacement controller for Webasto ThermoTop C / E heaters


### **[▶ Open the Webastardo phone app](https://simonrafferty.github.io/Webasto-Heater---Replacement-Controller/)**

Open the link in **Chrome on Android**, then choose **⋮ → Add to Home screen** to install it.
Once installed it works without a mobile signal and talks to the heater over Bluetooth.

1. Press **Connect** and choose **Webastardo**.
2. The first time, Android asks for the pairing PIN: **123456** (change it on the heater's WiFi page).
3. Use the app to turn the heater on and off, set the room temperature, switch the convection fan on and off, and purge the fuel pump.

If anything goes wrong, the app shows the reason in plain English at the top of the screen.

> **iPhone / iPad:** Safari doesn't support Bluetooth web apps. Install the free **Bluefy** browser
> and open the link in that.

---

## What it is

A drop-in replacement controller board for Webasto ThermoTop C and E water heaters, built around an
ESP32-S3. It runs the heater from the original harness and adds:

* **Phone app over Bluetooth** for everyday use, with only the controls a user needs.
* **WiFi web page** for setup and diagnostics: tuning, calibration, live data and error handling.
* **Exhaust temperature from a PT100** for flame detection, so no flame sensor is needed.
* **Water temperature NTC**, plus an **optional room NTC** that is detected automatically.
* **Run to Temp**: the heater switches off when the room reaches the target temperature.
* **Convection (vehicle blower) fan output**: on automatically when the water is hot, or manually.
* **Opto-isolated 12 V Heat ON/OFF input** on the original harness (X1-1).
* **Battery monitoring** with under- and over-voltage lockouts.
* **Safety**:
  * Watchdog.
  * The fuel pump stops by itself if the controller hangs.
  * Water and exhaust overheat shutdowns.
  * A purge-cool runs if it restarts with a hot burner.
  * Hardware thermal fuse on the fuel pump.

## Status LED

| LED | Meaning |
|---|---|
| Off | Heater off |
| On | Glow plug live |
| Flashing 1 Hz | Heater running (starting, running or cooling down) |
| Flashing 4 Hz | Fault – see the phone app or web page for the reason |

## WiFi web page (setup & diagnostics)

If no WiFi network is set, or it can't connect, the controller starts its own access point:

* Network **Webastardo**, password **Webastardo**
* Then browse to **http://192.168.4.1**

Once it's on your own network (enter it in the WiFi box on the page) it's at **http://webastardo.local**
or the IP address shown on the serial port.

The web page has the tuning settings (fuel and fan levels, temperature thresholds, flame detection,
pump size), calibration, the Bluetooth PIN, Clear Errors, System Test, Restart and Factory Reset.
**Changing the tuning settings can stop the heater working properly**, which is why the phone app
leaves them out.

## Building the firmware

> **Work in progress:** the V5 firmware has been tested on a bare ESP32-S3 with simulated sensors,
> not yet on a real V5 board and heater.

The firmware is in `Webastardo_V5_ESP32/`.

* Arduino IDE 2 with **ESP32 core 3.x** (Espressif), board **ESP32S3 Dev Module**
  * Flash Size **16MB**, PSRAM **OPI PSRAM**, Partition Scheme **3MB APP / 9MB FATFS**, USB CDC On Boot **Enabled**
* Libraries (Library Manager):
  * **ESP Async WebServer** (ESP32Async, 3.x)
  * **Async TCP** (ESP32Async, 3.x)
  * **NimBLE-Arduino** (2.x)

To flash the board, connect USB-C. If it won't enter download mode, hold **BOOT** while applying power.

### Testing without a heater

Set `SIMULATE_SENSORS` to `1` at the top of `Webastardo_V5_ESP32.ino`. The board then runs the whole
start / run / shutdown sequence with simulated temperatures, so you can try the web page and phone
app on a bare ESP32-S3. **Never fit a heater with this set to 1.**

## Phone app source

The app is the `docs/` folder of this branch. It is served by GitHub Pages from
**Settings → Pages → Branch `Webastardo-V5`, folder `/docs`**. The Bluetooth protocol is
documented at the top of `Webastardo_V5_ESP32/ble.ino`.

## Credits

Based on a Webasto shower controller by David McLuckie
(https://davidmcluckie.com/arduino-webasto-shower-project/), which was based on the work of
Mael Poureau (https://maelpoureau.com/webasto_shower/).

Simon Rafferty 2022–2026

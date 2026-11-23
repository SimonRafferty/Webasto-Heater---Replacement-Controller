//*********************************************************************************************
// DIY Webasto Controller V5.0
// Board:  ESP32S3 Dev Module  (ESP32-S3-WROOM-1-N16R8)
//   Tools settings: Flash Size 16MB, PSRAM "OPI PSRAM", USB CDC On Boot "Enabled"
//   FQBN: esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc
//   Requires ESP32 Arduino core 3.x and the libraries
//     "ESP Async WebServer" (ESP32Async, 3.x) and "Async TCP" (ESP32Async, 3.x)
//
// Code based on a Webasto Shower Controller by David McLuckie
// https://davidmcluckie.com/arduino-webasto-shower-project/
// His project was based on the work of Mael Poureau
// https://maelpoureau.com/webasto_shower/
//
// V5 is a drop-in replacement controller for Webasto ThermoTop C / E water heaters.
// It includes:
// * Web interface for control, monitoring & settings (http://webastardo.local or the IP shown
//   on the serial port).  If it can't join your WiFi it creates its own Access Point
//   "Webastardo" (password "Webastardo") at http://192.168.4.1
// * 5 x low-side MOSFET outputs: Fuel pump, Combustion air fan, Glow plug, Water pump,
//   Convection (vehicle blower) fan
// * Exhaust temperature from a PT100 (replaces the flame sensor / thermocouple of V4)
// * Water temperature NTC and an optional Room temperature NTC (same type)
// * Opto-isolated 12V Heat ON/OFF input (X1-1)
// * Dedicated battery voltage measurement
// * Bluetooth LE for the phone app (docs/ folder - an installable web app for everyday use).
//   The web page above is for setup & diagnostics; the phone app only has the everyday controls.
//
// See HARDWARE_GUIDE_V5.md for the pin map and analogue scaling.
//
// There is no current sensor on V5, so faults on the outputs can't be detected directly.
// An open thermal fuse (or no fuse fitted on the bench) looks like "fuel pump on, no flame"
// and is reported as an ignition failure.
//
// Simon Rafferty SimonSFX@Outlook.com 2022-2026
//*********************************************************************************************

#define FIRMWARE_VERSION "5.0.0"

#include <math.h>                             // To perform some calculations
#include <WiFi.h>                             // You can probably guess!
#include <ESPmDNS.h>                          // http://webastardo.local
#include <AsyncTCP.h>                         // Part of the Async Web Server
#include <ESPAsyncWebServer.h>                // The Async Web Server
#include <Preferences.h>                      // Non volatile storage for settings
#include <esp_task_wdt.h>                     // Task watchdog - reboots if the main loop hangs
#include <esp_system.h>                       // esp_reset_reason()
#include <driver/gpio.h>                      // gpio_set_level() - used in the fuel pump ISR
#include <NimBLEDevice.h>                     // Bluetooth LE for the phone app (library "NimBLE-Arduino" 2.x)

//*******************************************************************************************
// WiFi
// Default credentials.  These can be changed from the web page (stored in flash).
// Leave blank to always use the built-in Access Point.
//*******************************************************************************************
#define SECRET_SSID ""
#define SECRET_PASS ""
#define AP_SSID     "Webastardo"
#define AP_PASS     "Webastardo"              // Must be at least 8 characters
#define MDNS_NAME   "webastardo"              // http://webastardo.local

//*******************************************************************************************
// Testing aid.  Set to 1 to run the whole heater on a bare ESP32-S3 with nothing connected:
// battery, water & exhaust temperatures are simulated and respond to the fuel pump.
// NEVER flash a real heater with this set to 1.
//*******************************************************************************************
#ifndef SIMULATE_SENSORS          // (can also be set from the build command: -DSIMULATE_SENSORS=1)
#define SIMULATE_SENSORS 0
#endif

AsyncWebServer server(80);
AsyncEventSource events("/events");           // Pushes live data to the browser once a second

Preferences preferences;

//Heater Config
//*********************************************************************************
//**Change these values to suit your application ** (most can also be changed on the web page)
volatile float heater_min = 55;      // degrees C - Increase fuel if below.  The heater only (re)starts below this
volatile float heater_target = 65;   // degrees C Decrease fuel if above, increase if below.
volatile float water_warning = 75;   // degrees C - At this temperature, the heater idles
volatile float water_overheat = 85;  // degrees C - This is the temperature the heater will shut down

//Flame detection is from the exhaust PT100.
volatile float flame_threshold = 80;    // degrees C Exhaust temperature above which we assume it's alight
volatile float flame_rise = 5;          // degrees C During start-up, a rise this much above the pre-ignition exhaust temperature also means alight
volatile float exhaust_overheat = 450;  // degrees C Exhaust temperature that forces a shutdown

//*******************************************************************************************
// Heater running Settings
//*******************************************************************************************

//*** Set the fuel pump size
//Values 0, 22, 33 or 60  (0 is a special case for a 'silent' pump with a 2ms pulse)
volatile int pump_size = 60;

//*******************************************************************************************
//Fuel Mixture  (fuel in Hz for a 60 pump, fan in %)
volatile float throttling_high_fuel = 1.8;
volatile float throttling_high_fan = 95;
volatile float throttling_steady_fuel = 1.3;
volatile float throttling_steady_fan = 70;
volatile float throttling_low_fuel = 0.83;
volatile float throttling_low_fan = 60;
//Just enough to keep it alight at idle
volatile float throttling_idle_fuel = 0.6; //Do not reduce this value
volatile float throttling_idle_fan = 35;

//*******************************************************************************************
//Prime
float prime_low_temp = 20;                // Water Temp, below which fueling increased
float prime_high_temp = 20;
float prime_fan_speed = 15;
float prime_low_temp_fuelrate = 3.5;
float prime_high_temp_fuelrate = 2.0;
int   purge_max_seconds = 120;            // A web fuel purge switches itself off after this long

//*******************************************************************************************
//Initial
float start_fan_speed = 40;
float start_fuel_Threshold = 20; //Water temperature, below which to use start_fuel_Cold
float start_fuel_Cold = 1.2;  //Winter Setting (Below start_fuel_Threshold) (Cold Start Enrichment / Choke)
float start_fuel_Warm = 1.0;  //Summer Setting (Above start_fuel_Threshold)

int full_power_increment_time = 120; //seconds

//*******************************************************************************************
//Start-up timing (seconds into burn_mode 1)
int glow_start_seconds = 60;         // Glow plug on-time during start-up
int flame_check_from = 65;           // Don't look for a flame until the glow plug is off
int ignition_timeout = 180;          // Give up on this attempt after this long
int max_ignition_attempts = 3;       // Latch a failure after this many failed attempts

//*******************************************************************************************
//Glow plug
volatile float glow_volts = 8.0;     // Target RMS voltage on the glow plug (8V is standard for Webasto).  Duty is scaled from VBAT
int glow_ramp_ms = 5000;             // Fade the glow plug up over this long (cold plug inrush)

//*******************************************************************************************
//Combustion fan.  The Webasto fan is a 10V fan.  V4 limited the PWM to 169/255
float burn_fan_max_duty = 66.0;      // % duty at 100% fan_speed

//*******************************************************************************************
//Convection (vehicle blower) fan X1-4
float conv_fan_hysteresis = 3.0;     // degrees C. On above heater_min, off below heater_min - this

//*******************************************************************************************
//Battery.  Lockouts are non-latching - the heater restarts when the voltage recovers
volatile float low_volt_cutoff = 10.5;   // Volts - shut down if below this for low_volt_seconds
volatile float high_volt_cutoff = 16.0;  // Volts - shut down if above this for low_volt_seconds
int   low_volt_seconds = 20;             // Filter glow-plug / cranking dips
float bench_mode_volts = 6.0;            // Below this there's no 12V (USB power only): outputs can't work

//*******************************************************************************************
// Sensor calibration (see HARDWARE_GUIDE_V5.md section 3).  Editable on the web page.
//*******************************************************************************************
volatile float rail_3v3 = 3.300;      // Measured voltage at TP1 (+3V3).  Excitation for all temperature sensors
volatile float vbat_scale = 5.70;     // Battery divider 47k/10k = 1/0.1754
volatile float ntc_r25 = 10000;       // Water & Room NTC resistance at 25C (Ohms)
volatile float ntc_beta = 3977;       // Water & Room NTC Beta
volatile float water_offset = 0;      // degrees C added to the water reading
volatile float room_offset = 0;       // degrees C added to the room reading
volatile float exhaust_offset = 0;    // degrees C added to the exhaust reading (PT100 lead resistance ~1C per 0.385 Ohm)

//*******************************************************************************************
// Room thermostat (optional room NTC - detected automatically: no sensor = input at ~3.3V)
//*******************************************************************************************
volatile float room_temp_target = 21.0;  // desired room temp - degrees C
volatile bool  RunToTemp = false;        // Switch off the heater when the room exceeds room_temp_target
bool room_sensor_fitted = false;         // Detected, not configured
bool Room_Target_Reached = false;        // Heater was switched off by Run to Temp (until switched on again)

const float NTC_PULLUP = 4700.0;      // 4k7 pull-up to 3V3 on the water & room inputs
const float PT100_PULLUP = 1000.0;    // 1k pull-up to 3V3 on the exhaust input

//*******************************************************************************************
//  Pin Connections (Webastardo V5)
//*******************************************************************************************
const uint8_t  vbat_pin           = 1;   // VBAT_ADC   ADC1_CH0
const uint8_t  exhaust_temp_pin   = 2;   // EXH_ADC    ADC1_CH1  PT100
const uint8_t  water_temp_pin     = 9;   // WATER_ADC  ADC1_CH8  NTC
const uint8_t  room_temp_pin      = 10;  // ROOM_ADC   ADC1_CH9  NTC
const uint8_t  fuel_pump_pin      = 48;  // FUEL_PWM   X1-6
const uint8_t  conv_fan_pin       = 47;  // FAN_PWM    X1-4 Convection fan
const uint8_t  glow_plug_pin      = 21;  // GLOW_PWM
const uint8_t  burn_fan_pin       = 14;  // AIR_PWM    Combustion air fan
const uint8_t  water_pump_pin     = 13;  // WPUMP_PWM
const uint8_t  ON_OFF_pin         = 42;  // HEAT_N     Active LOW, opto-isolated, external pull-up
const uint8_t  led_pin            = 38;  // Status LED, active high
const uint8_t  boot_button_pin    = 0;   // BOOT button (low = pressed) - not used yet

//PWM (LEDC) frequencies
const uint32_t FAN_PWM_HZ   = 1000;   // Combustion air fan & water pump
const uint32_t GLOW_PWM_HZ  = 100;    // Glow plug (resistive, large thermal mass)
const uint32_t CONV_PWM_HZ  = 100;    // Convection fan
const uint8_t  PWM_BITS     = 10;     // 0..1023
const uint32_t PWM_MAX      = (1 << PWM_BITS) - 1;

//*******************************************************************************************
//  Temperature Filtering
//*******************************************************************************************
#define filterSamples   7               // filterSamples should  be an odd number, no smaller than 3
#define adcOversample   32              // ADC samples averaged per reading
struct MedianFilter {                   // One per channel (V4 shared its index between channels)
  float buf[filterSamples];
  uint8_t idx;
  bool primed;
};
MedianFilter WaterFilter   = {};
MedianFilter ExhaustFilter = {};
MedianFilter RoomFilter    = {};
MedianFilter VbatFilter    = {};

//*******************************************************************************************
//  User settings table (filled in Prefs.ino)
//*******************************************************************************************
struct Setting {
  const char *key;          // NVS key and web form field name (max 15 characters)
  volatile float *value;
  float min_val;
  float max_val;
};

//*******************************************************************************************
//Variables
//*******************************************************************************************
String ErrorMessage = "";  //Latching faults, in plain English, one per line.  The heater will not start until cleared (web or reboot)
volatile uint32_t Error_Seq = 0;  //Changes whenever ErrorMessage changes (so the phone app knows to fetch it)
String InfoMessage = "";   //What the heater is doing now

bool glow_plug_on = false;
int Ignition_Failures = 0;
float fan_speed;          // percent
float water_pump_speed;   // percent
float fuel_need;          // Hz (for a 60 pump)
int glow_time;            // seconds
int glow_left = 0;
int last_glow_value = 0;
bool burn = false;
bool Heater_fail = false;
int seconds;
float battery_voltage = 0;
bool bench_mode = false;          // No 12V present (board powered from USB only)
bool battery_lockout = false;     // Battery outside low_volt_cutoff..high_volt_cutoff

//Thermo Data
float room_temp = NAN;      // degrees C (NAN if not fitted / faulty)
float water_temp;           // degrees C
float exhaust_temp;         // degrees C
float exhaust_at_ignition;  // Exhaust temperature captured just before the fuel is primed
bool  water_sensor_fault = false;
bool  exhaust_sensor_fault = false;
bool  room_sensor_fault = false;

int Fueling_Level = 0;  //This is the current low, med, high fueling level 0=Off, 1=Idle, 2=Low, 3=Steady, 4=High
int burn_mode = 0;      //0=Off, 1=Starting, 2=Running, 3=Shutting down

bool conv_fan_on = false;

//Requests from the web page.  Set by the web server task, acted on by loop()
volatile bool Web_heater_ON = false;      //Set to true when heater triggered by web interface
volatile bool Web_purge_ON = false;       //Set to true when fuel purge triggered by web interface
volatile bool Web_conv_fan_ON = false;    //Manual override for the convection fan
volatile bool Save_Settings_Pending = false;
volatile bool Clear_Errors_Pending = false;
volatile bool Reboot_Pending = false;
volatile bool Factory_Reset_Pending = false;
volatile bool System_Test_Pending = false;
volatile bool Forget_Phones_Pending = false;
volatile bool BLE_Pin_Changed = false;
volatile bool Errors_Present = false;     //ErrorMessage != "" || Heater_fail  (safe to read from other tasks)

//Bluetooth (ble.ino).  Phones must pair with this 6-digit PIN.  Changeable on the web page.
uint32_t BLE_Pin = 123456;

String WiFi_SSID = SECRET_SSID;
String WiFi_PASS = SECRET_PASS;
bool   AP_mode = false;

String Reset_Reason = "";

//*******************************************************************************************
// Runs before setup() - drive every output low as early as possible.
// (The hardware pull-downs already hold them off; this is belt and braces.)
//*******************************************************************************************
void initVariant() {
  const uint8_t outs[] = {fuel_pump_pin, conv_fan_pin, glow_plug_pin, burn_fan_pin, water_pump_pin};
  for (uint8_t p : outs) {
    gpio_reset_pin((gpio_num_t)p);
    gpio_set_direction((gpio_num_t)p, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)p, 0);
  }
}

//*******************************************************************************************
// Setup
//*******************************************************************************************
void setup() {
  //Outputs off before anything else
  Outputs_Setup();

  Serial.begin(115200);
  //Don't wait for Serial - there's no USB host in a vehicle.  Give a host a moment to attach.
  delay(1500);

  Serial.println();
  Serial.println("Webastardo V" FIRMWARE_VERSION);
  Reset_Reason = Get_Reset_Reason();
  Serial.print("Reset reason: "); Serial.println(Reset_Reason);
#if SIMULATE_SENSORS
  Serial.println("*** SIMULATE_SENSORS is ON - sensors are simulated, not read ***");
#endif

  pinMode(ON_OFF_pin, INPUT_PULLUP);  //External 10k pull-up fitted; internal one too so a floating input reads OFF
  pinMode(boot_button_pin, INPUT_PULLUP);

  //Set up a namespace in Flash memory for settings (Prefs.ino)
  preferences.begin("Webastardo", false);
  Load_Preferences(); //Update variables from Flash memory

  Sensors_Setup();
  //Fill the filters so the first readings are meaningful
  for (int i = 0; i < filterSamples; i++) get_temperatures(true);

  //Heater will refuse to start if there are any error messages
  Serial.println("Running System Test...");
  System_Test();

  //If we've rebooted with a hot burner (brown-out, watchdog...) run a purge-cool first
  Hot_Boot_Check();

  //Establish a WiFi Connection & start the web server (web_server.ino)
  WiFi_setup();
  WebServer_setup();
  BLE_setup();              //Bluetooth for the phone app (ble.ino)

  //Task watchdog.  The loop never blocks, so 8s is generous.
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 8000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  if (esp_task_wdt_status(NULL) == ESP_ERR_INVALID_STATE) {
    esp_task_wdt_init(&wdt_config);
  } else {
    esp_task_wdt_reconfigure(&wdt_config);
  }
  esp_task_wdt_add(NULL);
}

//*******************************************************************************************
// Loop
//*******************************************************************************************
void loop() { // runs over and over again, calling the functions one by one
  esp_task_wdt_reset();

  get_temperatures(false);  //Read Battery, Room, Water & Exhaust (rate limited internally)
  Fuel_Keepalive();         //The fuel pump stops by itself if this isn't called
  control();                //Startup & shut down based on Errors, web & physical inputs
  Heater();                 //Finite state machine to run the heater
  Fuel_Purge_Action();      //Run the fuel purge cycle
  conv_fan();               //Convection / vehicle blower fan
  Status_LED();
  Errors_Present = (ErrorMessage != "") || Heater_fail;
  Web_Requests();           //Act on anything the web page asked for
  WiFi_Maintain();          //AP fallback / reconnect (never blocks)

  if (Seconds_Ticker()) {   //Perform actions once per second
    logging();
    Send_Live_Data();       //Push live data to any open web pages
    BLE_Send_Status();      //...and to a connected phone app
  }
  delay(1);                 //Let the idle task run
}

//*******************************************************************************************
// Tick counter - used for counting seconds
//*******************************************************************************************
bool Seconds_Ticker() {
static unsigned long timer = 0;

  if (millis() - timer >= 1000) { // every second, run this (rollover safe)
    timer += 1000;
    if (millis() - timer >= 1000) timer = millis(); //Catch up if we fell behind
    seconds ++; // increment the seconds counter
    return true;
  }
  return false;
}

//*******************************************************************************************
// Fuel Purge
//*******************************************************************************************
void Fuel_Purge_Action() {
//If it's safe to do so (heater & glow plug switched off), run the fuel pump rapidly to purge air
static bool Purging = false;
static unsigned long Purge_Start = 0;

  if (burn_mode == 0 && !glow_plug_on) {
    if (Web_purge_ON && !bench_mode) {
      if (!Purging) Purge_Start = millis();
      Purging = true;
      InfoMessage = "Fuel Purge";
      Fuel_Rate(pump_size, prime_rate(prime_low_temp));  //Set fuel rate to max
      if (millis() - Purge_Start > (unsigned long)purge_max_seconds * 1000UL) {
        Web_purge_ON = false;  //Don't leave the pump running if the page is forgotten
      }
    } else if (Purging) {
      Purging = false;
      Web_purge_ON = false;
      InfoMessage = "Purge End";
      Fuel_Rate(pump_size, 0.0);
    }
  } else if (Purging) {
    Purging = false;
    Web_purge_ON = false;
  }
}

//*******************************************************************************************
// Test Hardware
// V5 has no current sensor, so only the battery and sensors can be checked.
// Errors are added to ErrorMessage (which stops the heater starting)
//*******************************************************************************************
void System_Test() {
  get_temperatures(true);

  //Battery.  No 12V is "bench mode", not a fault
  if (bench_mode) {
    Serial.println("System Test: no 12V - bench mode (outputs have no supply)");
  } else if (battery_voltage < low_volt_cutoff) {
    Serial.println("System Test: battery low");   //Non-latching - see Battery_Check()
  }

  //Sensors
  if (water_sensor_fault)
    AddError("The water temperature sensor is " + Sensor_Fault_Text(water_temp_pin) + ". Check its wiring and plug.");
  if (exhaust_sensor_fault)
    AddError("The exhaust temperature sensor is " + Sensor_Fault_Text(exhaust_temp_pin) + ". Check its wiring and plug.");
  if (room_sensor_fault) {
    //Only Run to Temp uses the room sensor - report it, don't stop the heater
    Serial.println("System Test: room sensor short circuit (Run to Temp disabled)");
  } else {
    Serial.println(room_sensor_fitted ? "System Test: room sensor fitted" : "System Test: no room sensor");
  }

  //If there are any errors, print them
  if (ErrorMessage != "") {
    Serial.print("Errors: "); Serial.println(ErrorMessage);
  } else {
    Serial.println("System Test: OK");
  }
}

//*******************************************************************************************
// Useful version of map function, for doubles / floats
//*******************************************************************************************
double mapf(double x, double in_min, double in_max, double out_min, double out_max) {
  // the perfect map function, with constraining and double handling
  x = constrain(x, in_min, in_max);
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//*******************************************************************************************
// Add message to ErrorMessage global, unless it's already there.
// Write messages as a plain sentence someone could read out over the phone.
//*******************************************************************************************
void AddError(String Err_Message) {
  //Look to see if this message has already been reported.  If not, don't add it again
  if (ErrorMessage.indexOf(Err_Message) < 0) {
    if (ErrorMessage.length() > 0) ErrorMessage += '\n';
    ErrorMessage += Err_Message;
    Error_Seq = Error_Seq + 1;
    Serial.print("ERROR: "); Serial.println(Err_Message);
  }
}

void ClearErrors() {
  ErrorMessage = "";
  Error_Seq = Error_Seq + 1;
}

//*******************************************************************************************
// Why did we last reset?  (Brown-out is a real possibility when cranking)
//*******************************************************************************************
String Get_Reset_Reason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "Power on";
    case ESP_RST_SW:        return "Software restart";
    case ESP_RST_PANIC:     return "Crash (panic)";
    case ESP_RST_INT_WDT:   return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "Task watchdog";
    case ESP_RST_WDT:       return "Watchdog";
    case ESP_RST_BROWNOUT:  return "Brown-out";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    case ESP_RST_EXT:       return "External";
    case ESP_RST_USB:       return "USB";
    default:                return "Unknown";
  }
}

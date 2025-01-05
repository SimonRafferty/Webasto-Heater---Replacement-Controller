//*********************************************************************************************
// DIY Webasto Controller V4.0 Universal
// Board:  Adafruit ESP Feather (Will not compile if set wrongly)
//
// Add the following board link in preferences: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
// Code based on a Webasto Shower Controller by David McLuckie
// https://davidmcluckie.com/arduino-webasto-shower-project/ 
// His project was based on the work of Mael Poureau
// https://maelpoureau.com/webasto_shower/
//
// V4 is intended to be a universal controller which can be used on any Petrol or Diesel 
// Air or Water heater.
// It includes:
// Web interface for control & monitoring
// Built in Web Server at 192.168.4.1 when it fails to connect to WiFi network
// * 6 x PWM MOSFET outputs, including two auxilliary outputs you can use for whatever you like
// * 3 x K-Type Thermocouple channels linked to SPI
// * ACS711 Current transducer connected to all PWM Outputs - which you can use as a flame sensor 
// * Headers for I2C and SPI for connection to displays or other peripherals
// * 12V Trigger input to start / stop the heater
// * Battery voltage measurement
//
// Simon Rafferty SimonSFX@Outlook.com 2022
//*********************************************************************************************



#include <math.h>                             // To perform some calculations
#include <SPI.h>                              // To read Thermocouple SPI Interface
#include <Adafruit_MAX31855.h>                // To convert Thermocouple data to deg C
#include <ACS712.h>                           // To read Current
#include <ESP32_PWM.h>                        //Slow PWM to drive Fuel Pump in the background
#include <WiFi.h>                             //You can probably guess!
#include <AsyncTCP.h>                         //Part of the Async Web Server
#include <ESPAsyncWebServer.h>                //The Async Web Server
#include <Preferences.h>                      //Non volatile storage for settings

//Comment out if display not fitted
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SECRET_SSID "DanglyBits"  //Leave blank if you want to use built-in webserver
#define SECRET_PASS "stoatgobbler"  //Leave blank if you want to use built-in webserver

//Comment out if display not fitted
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


WiFiClient client;
AsyncWebServer server(80);

Preferences preferences;

// Init ESP32_ISR_PWM
ESP32_PWM Fuel_Pump;
int channelNum = -1;

//Heater Config 
//*********************************************************************************
//**Change these values to suit your application **
volatile int heater_min = 55; // Increase fuel if below
volatile int heater_target = 65; // degrees C Decrease fuel if above, increase if below.
volatile int water_warning = 75;// degrees C - At this temperature, the heater idles
volatile int water_overheat = 85;// degrees C - This is the temperature the heater will shut down

volatile bool use_flame_sensor = false;  //Use Flame Sensor or Exhaust Temperature
volatile int flame_threshold = 200; //Exhaust or Flame temperature above which we assume it's alight

//Webasto Resistances
//double glow_25C   = 0.342; //Glow Plug resistance at 25C
//double glow_1300C = 1.600;  //Glow Plug resistance at 1300C (full glowing temperature)
//HCalory Water Heater Resistances
double glow_25C   = 0.78; //Glow Plug resistance at 25C
double glow_1300C = 0.923;  //Glow Plug resistance at 1300C (full glowing temperature)
//*******************************************************************************************
// Heater running Settings
//*******************************************************************************************

//*** Set the fuel pump size
//Values 22,30 or 60 seem to work in most cases.
volatile int pump_size = 60; //22,30,60 

//*******************************************************************************************
//Fuel Mixture
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
float prime_low_temp = 20; //Water Temp, below which fueling increased
float prime_high_temp = 20;
bool Fuel_Purge = false; //Set by blynk.  Delivers fuel rapidly without running anything else

float prime_fan_speed = 15;
float prime_low_temp_fuelrate = 3.5;
float prime_high_temp_fuelrate = 2.0;

//*******************************************************************************************
//Inital
float start_fan_speed = 40;
float start_fuel = 1;  //Summer setting
float start_fuel_Threshold = 20; //Water temperature, below which to use start_fuel_Cold
float start_fuel_Cold = 1.2;  //Winter Setting (Below start_fuel_Threshold) (Cold Start Enrichment / Choke)
float start_fuel_Warm = 1.0;  //Summer Setting (Above start_fuel_Threshold)

int full_power_increment_time = 120; //seconds




//*******************************************************************************************
//  Pin Connections
//*******************************************************************************************
const uint8_t  SPI_CHIP_SELECT_1  =      14; ///< Room Chip-Select PIN for SPI
const uint8_t  SPI_CHIP_SELECT_2  =      32; ///< Water Chip-Select PIN for SPI
const uint8_t  SPI_CHIP_SELECT_3  =      15; ///< Exhaust Chip-Select PIN for SPI
const uint8_t  SPI_MISO         =        19; ///< Master-In, Slave-Out PIN for SPI
const uint8_t  SPI_SYSTEM_CLOCK =         5; ///< System Clock PIN for SPI


const uint8_t  aux_1_pin          = 21; 
const uint8_t  aux_2_pin          = 17; 
const uint8_t  fuel_pump_pin      = 16; 
const uint8_t  water_pump_pin     = 25; 
const uint8_t  burn_fan_pin       = 26; 
const uint8_t  glow_plug_pin      = 27; 
const uint8_t  batt_voltage_pin   = 33; 
const uint8_t  current_sensor_pin = A2; 
const uint8_t  ON_OFF_pin         = 36; 

// initialize the Thermocouples. Swap around the SPI_CHIP_SELECTs to change which is which
Adafruit_MAX31855    th_room_temp(SPI_SYSTEM_CLOCK, SPI_CHIP_SELECT_1, SPI_MISO);
Adafruit_MAX31855   th_water_temp(SPI_SYSTEM_CLOCK, SPI_CHIP_SELECT_2, SPI_MISO);
Adafruit_MAX31855 th_exhaust_temp(SPI_SYSTEM_CLOCK, SPI_CHIP_SELECT_3, SPI_MISO);

//*******************************************************************************************
//  Hall Effect Current Sensor Used for diagnostics and flame sensor
//*******************************************************************************************
ACS712  ACS(current_sensor_pin, 3.3, 4095, 48);  //Setup Current Monitor (Pin, Supply Voltage, ADC Resolution, mV per Amp)
int Current_Threshold = 100; //Minimum current flow in mA to detect if a peripheral is connected in System_Test()

//*******************************************************************************************
//  Temperature Filtering
//*******************************************************************************************
#define filterSamples   13              // filterSamples should  be an odd number, no smaller than 3
float WaterSmoothArray [filterSamples];   // array for holding raw sensor values for sensor1 
float ExhaustSmoothArray [filterSamples];   // array for holding raw sensor values for sensor2 
float RoomSmoothArray [filterSamples];   // array for holding raw sensor values for sensor3 



//*******************************************************************************************
//Varaiables
//*******************************************************************************************
String ErrorMessage = "";  //Send messages to web server and Serial.  Heater will need a reboot after this set
String InfoMessage = ""; //Sub InfoMessage with info on problem
String inputMessage = ""; //Used to receive messages from web server

bool HeaterOn = false;  //Used internally to switch on / off
bool glow_plug_on = false;
int Ignition_Failures = 0;
float fan_speed; // percent
float water_pump_speed; // percent
float fuel_need; // Hz
int glow_time; // seconds
int glow_left = 0;
int last_glow_value = 0;
bool burn = false;
bool Heater_fail = false;
int seconds;
double battery_voltage = 0; 

//Variables for fuel pump timer Volatile is important!
volatile double pulseFrequency = 0;
volatile int pulseLength = 0; // in milliseconds
volatile bool pulseEnable = false;
volatile bool pulseisHigh = false;

//Thermo Data
float room_temp;  // degrees C
volatile float room_temp_target = 23.0;  // desired room temp - degrees C
bool RunToTemp = false; //Switch off heater when it reaches room_temp_target
float room_temp_hysteresis = 2.0; //Hysteresis is the gap between switching off then back on again.
float water_temp;  // degrees C
float exhaust_temp;  // degrees C

int Fueling_Level = 0;  //This is the current low, med, high fueling level 0=Off, 1=Idle, 2=Low, 3=Steady, 4=High

int delayed_period = 0;
long glowplug_on = 0;
int burn_mode = 0;

//When this is true, setup WiFi / Webserver interface
bool WiFi_active = true; //Set to false if you only want to use the physical on/off switch
bool Web_heater_ON = false; //Set to true when heater triggered by web interface
bool Web_purge_ON = false;  //Set to true when fuel purge triggered bby web interface

TaskHandle_t WebServer;
TaskHandle_t LoggingEv;


//*******************************************************************************************
// Setup
//*******************************************************************************************
void setup() {
  Serial.begin(115200);
  while (!Serial) vTaskDelay(1); // wait for Serial on Leonardo/Zero, etc



  //Set pin definitions
  pinMode(aux_1_pin, OUTPUT);
  pinMode(aux_2_pin, OUTPUT);
  pinMode(fuel_pump_pin, OUTPUT);
  pinMode(water_pump_pin, OUTPUT);
  pinMode(burn_fan_pin, OUTPUT);
  pinMode(glow_plug_pin, OUTPUT);

  analogWrite(aux_1_pin, 0);
  analogWrite(aux_2_pin, 0);
  digitalWrite(fuel_pump_pin, 0);
  analogWrite(water_pump_pin, 0);
  analogWrite(burn_fan_pin, 0);
  analogWrite(glow_plug_pin, 0);

  pinMode(ON_OFF_pin, INPUT);
  pinMode(batt_voltage_pin, INPUT);
  pinMode(current_sensor_pin, INPUT);

  //Heater will refuse to start if there are any error messages
  //Serial.println("Running System Hardware Test...");
  //System_Test();

  //Comment out this block if display not fitted
  //Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.display();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 4);
  display.print("WEBASTARDO 4.2");

  display.setCursor(3, 20); // Center the heater status number
  display.print("Starting up...");
  display.display();


  //Define the value for no (minimum) current flow
  ACS.setMidPoint(ACS.getMinimum(2000)/2 + ACS.getMaximum(2000)/ 2); 
  vTaskDelay(100);

  //Set fault mask on Thermocouple to ignore ground faults
  th_room_temp.setFaultChecks(MAX31855_FAULT_OPEN);
  th_water_temp.setFaultChecks(MAX31855_FAULT_OPEN);
  th_exhaust_temp.setFaultChecks(MAX31855_FAULT_OPEN);

  //Set up a namespace in Flash memory for settings (Prefs.ino)
  preferences.begin("Webastardo", false);

  Load_Preferences(); //Update variables from Flash memory

  //Wifi / Webserver is active.  Look in control.ino, heater_on() for the on/off logic
  if(WiFi_active) 
    WiFi_setup();  //Establish a WiFi Connection
    Serial.print("Main program running on core ");
    Serial.println(xPortGetCoreID());

    //Determine which core is running at the moment and make webserver use the other one 
    BaseType_t MainCore = xPortGetCoreID();
    BaseType_t SubCore = 0;
    if(MainCore == 0) SubCore = 1; 

  //Program uses multi-tasking with tasks running on two different cores.
  //The high priority tasks are on Core 0 - coordinating the heater itself
  //The webserver, logging and OLED Screen updates are on Core 1
  //The Webserver is set to have higher priority than the OLED Updates

    xTaskCreatePinnedToCore(
          WebServerEvents, /* Function to implement the task */
          "WebServer", /* Name of the task */
          40000,  /* Stack size in words */
          NULL,  /* Task input parameter */
          1,  /* Priority of the task */
          &WebServer,  /* Task handle. */
          SubCore); /* Core where the task should run */

    xTaskCreatePinnedToCore(
          LoggingEvents, /* Function to implement the task */
          "Logging", /* Name of the task */
          40000,  /* Stack size in words */
          NULL,  /* Task input parameter */
          0,  /* Priority of the task */
          &LoggingEv,  /* Task handle. */
          SubCore); /* Core where the task should run */
  
     
  //Make sure ADCs are reading Max resolution
  analogReadResolution(12);

}

//*******************************************************************************************
// Loop
//*******************************************************************************************
void loop() { // runs over and over again, calling the functions one by one

  get_temperatures();     //Read Room, Water & Exhaust temperatures
  control();              //Startup & shut down based on Errors, web & physical inputs
  Heater();               //Finite state machine to run the heater
  Fuel_Purge_Action();    //Run the fuel purge cycle
  
//  if(Seconds_Ticker()){   //Perform actions once per second
//    logging();
//    battery_voltage = float(analogRead(batt_voltage_pin)) / 100.0;    
//  }

}

//*******************************************************************************************
// Tick counter - used for counting seconds
//*******************************************************************************************
bool Seconds_Ticker() { // runs over and over again, calling the functions one by one
static unsigned long timer;

  if (millis() < timer) {
    timer = millis();
  }

  if (millis() > timer + 1000) { // every seconds, run this
    timer = millis();
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

  if(burn_mode==0 && !glow_plug_on) {
    if(Web_purge_ON) {
      fuel_need = prime_rate(prime_low_temp);  //Set fuel rate to max
      Purging = true;
      Fuel_Rate(pump_size, fuel_need);
    } else if(Purging){
      fuel_need = 0;
      Purging = false;
      InfoMessage = "Purge End";
      Fuel_Rate(pump_size, fuel_need);
    }
    
  }
}





//*******************************************************************************************
// Test Hardware
//*******************************************************************************************
void System_Test(){
//Test the different peripherals to make sure they're working and return an error String if not
String Error_Message = "";
  //Battery
  battery_voltage = float(analogRead(batt_voltage_pin)) / 100.0;
  if(battery_voltage < 12.4){
    AddError("Battery Low , ");
  }

  //Thermocouples
  double c1 = th_room_temp.readCelsius();
  double c2 = th_water_temp.readCelsius();
  double c3 = th_exhaust_temp.readCelsius();
  uint8_t e;
  //Comment out block if Thermocouple(s) not in use
  if (isnan(c1))  AddError("Therm 1: NC , ");
  if (isnan(c2))  AddError("Therm 2: NC , ");
  if (isnan(c3))  AddError("Therm 3: NC , ");


  //Outputs
  //Apply power briefly to each output, looking for current flow
  //Comment out any unused outputs
 
  
  /*
  analogWrite(aux_1_pin, 255);  
  if(ACS.mA_DC(120) < Current_Threshold)  AddError("AUX 1: NC , ");
  analogWrite(aux_1_pin, 0);
  */
  /*
  analogWrite(aux_2_pin, 255); 
  if(ACS.mA_DC(120) < Current_Threshold)  AddError("AUX 2: NC , ");
  analogWrite(aux_2_pin, 0);
  */
  
  digitalWrite(fuel_pump_pin, 255);
  if(ACS.mA_DC(120) < Current_Threshold)  AddError("FUEL PUMP: NC , ");
  digitalWrite(fuel_pump_pin, 0); 

  analogWrite(water_pump_pin, 255); 
  if(ACS.mA_DC(120) < Current_Threshold)  AddError("WATER PUMP: NC , ");
  analogWrite(water_pump_pin, 0);

  analogWrite(burn_fan_pin, 255); 
  if(ACS.mA_DC(120) < Current_Threshold)  AddError("FAN: NC , ");
  analogWrite(burn_fan_pin, 0);

  analogWrite(glow_plug_pin, 255); 
  if(ACS.mA_DC(120) < Current_Threshold)  AddError("GLOW PLUG: NC , ");
  analogWrite(glow_plug_pin, 0);

  //If there are any errors, print them
  if(Error_Message != ""){
    Serial.print("Errors: "); Serial.println(Error_Message);
  }

}
//*******************************************************************************************
// Useful version of map function, for doubles / floats
//*******************************************************************************************
double mapf(double x, double in_min, double in_max, double out_min, double out_max) {
  // the perfect map fonction, with constraining and double handling
  x = constrain(x, in_min, in_max);
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//*******************************************************************************************
// Similar to map() but is not limited to the range of output values
//*******************************************************************************************
double Interpolate(double x, double in_min, double in_max, double out_min, double out_max) {
    // Calculate the slope and intercept of the line
    double slope = (out_max - out_min) / (in_max - in_min);
    double intercept = out_min - slope * in_min;

    // Apply the linear equation (y = mx + b) to find the new value
    return slope * x + intercept;
}

//*******************************************************************************************
// Add message to Error_Message global, unless it's already there
//*******************************************************************************************
void AddError(String Err_Message){

  //Look to see if this message has already been reported.  If not, don't add it again
  if(ErrorMessage.indexOf(Err_Message) < 0){
    ErrorMessage = ErrorMessage + Err_Message;
  }


}
//*********************************************************************************************
// DIY Webasto Controller
// Board:  Adafruit Feather M0 WiFi
// Add the following board link in preferences: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
// Code based on a Webasto Shower Controller by David McLuckie
// https://davidmcluckie.com/arduino-webasto-shower-project/ 
// His project was based on the work of Mael Poureau
// https://maelpoureau.com/webasto_shower/
//
// I've changed it from a Shower to a general purpose controller which tries to regulate the 
// Temperature to a pre-defined target by adjusting the Fueling and Combustion fan
// I've converted the code to run on an Adafruit Feather M0 SAMD21 and designed a drop-in
// replacement PCB for Webasto ThermoTop C & E Heaters.
//
// The wiring harness is similar to the original, except:
// 6 Pin Connector - Pin:  [Changes indicated by * ]
// 1 - Clock (+12V = Heater On)/(0V = Heater Off)
// 2*- ESP32 Serial Out (Not used)
// 3*- Exhaust Thermistor (100k NTC between pin 3 & Gnd)
// 4*- ESP Analog Input 03 (could be configured as a 3.3V Output)
// 5*- Water Thermistor (100k NTC between pin 3 & Gnd) [See Note]
// 6 - Fuel Dosing Pump
// 
// [Note] The PCB has holes for the original Thermistor which you can salvage from an old unit
// It has a different 25C Resistance around 4.7k.  You will need to change R12 to 4.7k
// and in get_webasto_temp, change "Nominal resistance at 25 ºC" from 100000 to 4700
//
// The latest revision of the PCB (https://oshwlab.com/SimonRafferty/webasto-controller)
// has provision for a Thermal Fuse.  This simply cuts the fueling if the heater really overheats
// to prevent it catching fire.  I used RS Part Number 797-6042 which fuses at 121C
// If you prefer the excitement of waiting for it to catch fire, you can always bridge the 
// contacts with a bit of wire.
//
// Simon Rafferty SimonSFX@Outlook.com 2022
//*********************************************************************************************

//Build options
#define BLYNK_ENABLE              //Uncomment if you want to send data to Blynk (only applies to M0 WiFi board)
#define FLAME_SENSOR_ENABLE       //Uncomment if using V3.0 board with ACS711 Current Sensor
#define M0_WIFI_ENABLE            //Uncomment if you are using a Feather M0 WiFi microcontroller
//With all three commented out, project will compile the same as the main branch

//This bit just avoids the situation of enabling blynk, without Wifi - which would be a bit dumb huh?  Can't imagine anyone would do that?? ;-)
#ifdef BLYNK_ENABLE
  #ifndef M0_WIFI_ENABLE
    #define M0_WIFI_ENABLE
  #endif
#endif


#ifdef BLYNK_ENABLE
  //**BLYNK Defines MUST be before Includes
  #define BLYNK_TEMPLATE_ID "#############"
  #define BLYNK_DEVICE_NAME "###############"
  #define BLYNK_AUTH_TOKEN "##############"
#endif

#include <math.h> // needed to perform some calculations
#ifdef M0_WIFI_ENABLE
  #include <SPI.h>
  #include <WiFi101.h>
  #define SECRET_SSID "############"
  #define SECRET_PASS "############"
#endif

#ifdef BLYNK_ENABLE
  #include <BlynkSimpleWiFiShield101.h>
#else
  #include <WiFiMDNSResponder.h>
#endif

//Heater Config 
//*********************************************************************************
//**Change these values to suit your application **
int heater_min = 55; // Increase fuel if below
int heater_target = 65; // degrees C Decrease fuel if above, increase if below.
int water_warning = 70;// degrees C - At this temperature, the heater idles
int water_overheat = 85;// degrees C - This is the temperature the heater will shut down

int flame_threshold = 75; //Exhaust temperature above which we assume it's alight


//Fuel Mixture
//If you find the exhaust is smokey, increase the fan or reduce the fuel
/*
float throttling_high_fuel = 1.8 * fuel_trim;
//float throttling_high_fuel = 1.6; //In summer, exhaust gets too hot on startup
float throttling_high_fan = 95;
float throttling_steady_fuel = 1.3 * fuel_trim;
float throttling_steady_fan = 70;
float throttling_low_fuel = 0.83 * fuel_trim;  
float throttling_low_fan = 60;
//Just enough to keep it alight at idle
float throttling_idle_fuel = 0.6; //Do not reduce this value
float throttling_idle_fan = 35; 
*/
/*
//David's settings
float throttling_high_fuel = 1.8;
float throttling_high_fan = 90;
float throttling_steady_fuel = 1.5;
float throttling_steady_fan = 65;
float throttling_low_fuel = 0.83;
float throttling_low_fan = 50;
float throttling_idle_fuel = 0.5; //Just enough to keep it alight
float throttling_idle_fan = 30; 
*/
//Low CO Settings
float throttling_high_fuel = 1.8 * 1.3;
float throttling_high_fan = 90 / 1.3;
float throttling_steady_fuel = 1.5 * 1.5;
float throttling_steady_fan = 65 /1.5;
float throttling_low_fuel = 0.83 * 2;
float throttling_low_fan = 50 /2 ;
float throttling_idle_fuel = 0.6; //Just enough to keep it alight
float throttling_idle_fan = 20; 



//Fuel Pump Setting
//Different after-market pumps seem to deliver different amounts of fuel
//If the exhaust is consistently smokey, reduce this number
//If you get no fuel (pump not clicking) increase this number
//Values 22,30 or 60 seem to work in most cases.

int pump_size = 22; //22,30,60 
int fuel_trim = 1000; //used to experimentally adjust the fueling, globally
//**********************************************************************************
 
//Prime
float prime_low_temp = 10; //Water Temp, below which fueling increased
float prime_high_temp = 20;
bool Fuel_Purge = false; //Set by blynk.  Delivers fuel rapidly without running anything else

float prime_fan_speed = 15;
float prime_low_temp_fuelrate = 3.5;
float prime_high_temp_fuelrate = 2.0;

//Inital
float start_fan_speed = 40;
float start_fuel = 1;  //Summer setting
float start_fuel_Threshold = -10; //Exhaust temperature, below which to use start_fuel_Cold
float start_fuel_Cold = 1.2;  //Winter Setting (use below 10C)
float start_fuel_Warm = 1.0;  //Winter Setting (use below 10C)

int full_power_increment_time = 30; //seconds




//Pin Connections
int fuel_pump_pin = 11;
int glow_plug_pin = 5;
int burn_fan_pin = 10;
int water_pump_pin = 9;
int water_temp_pin = A1;
int exhaust_temp_pin = A2;
int lambda_pin = A3;
int push_pin = A0;
int flame_sensor = A4;
//Pin Connections


//Blynk Write Variables
int BlynkHeaterOn = 0;
int BlynkPurgeFuel = 0;


#ifndef BLYNK_ENABLE
  #ifdef M0_WIFI_ENABLE //Setup web server if this is a wifi board and blynk not selected.

    //WiFi Setup
    //#include "Arduino_Secrets" 
    char ssid[] = SECRET_SSID;    // your network SSID (name)
    char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
    int keyIndex = 0;             // your network key Index number (needed only for WEP)
    bool WiFiACTIVE = false;
    WiFiClient client;
    
    char mdnsName[] = "webastardo"; // the MDNS name that the board will respond to
    // Note that the actual MDNS name will have '.local' after
    // the name above, so "webastardo" will be accessible on
    // the MDNS name "webastardo.local".
    
    int status = WL_IDLE_STATUS;
    
    // Create a MDNS responder to listen and respond to MDNS name requests.
    WiFiMDNSResponder mdnsResponder;
    
    WiFiServer server(80);
  #endif
#endif


//Temperature Filtering
#define filterSamples   13              // filterSamples should  be an odd number, no smaller than 3
float rawDataWater, smoothDataWater;  // variables for sensor1 data
float rawDataExhaust, smoothDataExhaust;  // variables for sensor2 data

float WaterSmoothArray [filterSamples];   // array for holding raw sensor values for sensor1 
float ExhaustSmoothArray [filterSamples];   // array for holding raw sensor values for sensor2 
float RoomSmoothArray [filterSamples];   // array for holding raw sensor values for sensor3 

float Last_Exh_T = 0;
float Last_Wat_T = 0;
float Last_Mute_T = 0;
int GWTLast_Sec;
int Last_TSec;
boolean EX_Mute = false;
float Last_Temp = 0;
float Max_Change_Per_Sec_Exh = 4;  //Used to slow down changes in temperature to remove spikes
float Max_Change_Per_Sec_Wat = 2;  //Used to slow down changes in temperature to remove spikes
//Flame Sensor workings
float Flame_Diff = 0; 
float Flame_Threshold = 1.000;
long Flame_Timer = millis(); //prevent flame sensor being called too often
float Flame_Last = 0;


//Serial Settings
String message = "Off";
//bool pushed;
//bool long_press;
bool heater_on;
bool debug_glow_plug_on = false;
int debug_water_percent_map = 999;

//Varaiables
int Ignition_Failures = 0;
float fan_speed; // percent
float water_pump_speed; // percent
float fuel_need; // percent
int glow_time; // seconds
float water_temp; // degrees C
float exhaust_temp; // degrees C
float exhaust_temp_sec[11]; // array of last 10 sec water temp, degres C
int water_temp_sec[181];
int glow_left = 0;
int last_glow_value = 0;
bool burn = false;
bool webasto_fail = false;
int Start_Failures = 0;
int seconds;
int room_temp_set = 20;  // desired room temp - degrees C
float room_temp;  // degrees C
long restart_timer = 0;  //Holder for restart time
long restart_delay = 60; //(minutes) After an overheat, delay the restart 

bool lean_burn;
int delayed_period = 0;
unsigned long water_pump_started_on;
int water_pump_started = 0;
long glowing_on = 0;
int burn_mode = 0;



//PWM properties
//TurboPWM pwm; //Turbo PWM for SAMD21

const int glow_channel = 0;
const int water_channel = 1;
const int air_channel = 2;

void setup() {
  Serial.begin(115200);
  delay(2000);
#ifdef M0_WIFI_ENABLE
  //Config pins for Adafruit M0 Wifi
  WiFi.setPins(8, 7, 4, 2);
#endif
#ifdef BLYNK_ENABLE
  Serial.println("Connect to Blynk");

  Blynk.begin(BLYNK_AUTH_TOKEN, SECRET_SSID, SECRET_PASS);
  delay(1000);
  while(!Blynk.connected()) {
    Serial.print(".");
    Blynk.connect();
    delay(1000);
  }
  Serial.println("Blynk Connected");
#endif
  
  pinMode(glow_plug_pin, OUTPUT);
  pinMode(fuel_pump_pin, OUTPUT);
  pinMode(burn_fan_pin, OUTPUT);
  pinMode(water_pump_pin, OUTPUT);
  pinMode(water_temp_pin, INPUT);
  pinMode(exhaust_temp_pin, INPUT);
  pinMode(push_pin, INPUT);
  pinMode(lambda_pin, INPUT);
  pinMode(flame_sensor, INPUT);

  
  

  
  analogWrite(water_pump_pin, 100); //Run water pump on startup for a few seconds
  //Pulse Burn fan - to test & indicate startup
  fan_speed = 70;
  burn_fan();
  delay(1000);
  fan_speed = 0;
  burn_fan();
  delay(1000);
  fan_speed = 70;
  burn_fan();
  delay(1000);
  fan_speed = 0;
  burn_fan();
  delay(3000);
  

  analogReadResolution(12);



#ifdef M0_WIFI_ENABLE
  #ifndef BLYNK_ENABLE

    // attempt to connect to WiFi network:
    while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);
  
      // wait 5 seconds for connection:
      delay(5000);
    }
    // you're connected now, so print out the status:
    printWiFiStatus();
  
    server.begin();
    
  
    // Setup the MDNS responder to listen to the configured name.
    // NOTE: You _must_ call this _after_ connecting to the WiFi network and
    // being assigned an IP address.
    if (!mdnsResponder.begin(mdnsName)) {
      Serial.println("Failed to start MDNS responder!");
      while(1);
    }
  
    Serial.print("Server listening at http://");
    Serial.print(mdnsName);
    Serial.println(".local/");  
  #endif
#endif


  
}

void loop() { // runs over and over again, calling the functions one by one

  temp_data();
  control();
  webasto();
  
#ifdef M0_WIFI_ENABLE
  #ifdef BLYNK_ENABLE
    Fuel_Purge_Action();
    Blynk.run();
  #else
    
    // Call the update() function on the MDNS responder every loop iteration to
    // make sure it can detect and respond to name requests.
    mdnsResponder.poll();
    // listen for incoming clients
    client = server.available();
    if (client) {
      WiFiACTIVE = true;
      WiFi_Deliver_Content();
    } else {
       WiFiACTIVE = false;  //Suspend serial logging
    }
  #endif
#endif
}

void Fuel_Purge_Action() {
//If it's safe to do so (heater & glow plug switched off), run the fuel pump rapidly to purge air
  if(!heater_on && !debug_glow_plug_on) {
    if(Fuel_Purge) {
      fuel_need = prime_ratio(prime_low_temp);  //Set fuel rate to max
    } else {
      fuel_need = 0;
    }
    fuel_pump();
  }
}

#ifdef BLYNK_ENABLE
//Receive data from Blynk App

  BLYNK_WRITE(V50)
  {
    //When the heater is switched on via the Blynk Console - BlynkHeaterOn changes from 0 to 1
    BlynkHeaterOn = param.asInt(); // assigning incoming value from pin V1 to a variable
  }

  BLYNK_WRITE(V51)
  {
    //When selected from the Blynk Console, deliver fuel rapidly
    Fuel_Purge = param.asInt(); // assigning incoming value from pin V1 to a variable
  }

  BLYNK_WRITE(V52)
  {
    //When selected from the Blynk Console, deliver fuel rapidly
    room_temp_set = param.asInt(); // assigning incoming value from pin V1 to a variable
  }

  BLYNK_WRITE(V53)
  {
    //When selected from the Blynk Console, deliver fuel rapidly
    fuel_trim = param.asInt(); // assigning incoming value from pin V1 to a variable
  }

  

#endif

#ifdef M0_WIFI_ENABLE
  void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
  
    // print your WiFi 101 Shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
  
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
#endif

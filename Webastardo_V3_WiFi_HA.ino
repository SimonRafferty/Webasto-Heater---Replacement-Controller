//*********************************************************************************************
// DIY Webasto Controller
// Board:  Adafruit Feather M0 WiFi
// Add the following board link in preferences: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
// Code based on a Webasto Shower Controller by David McLuckie
// https://davidmcluckie.com/arduino-webasto-shower-project/ 
// His project was based on the work of Mael Poureau
// https://maelpoureau.com/webasto_shower/
//
// This version uses an MQTT Server to send & receive information to Home Assistant
// You need to figure out how to implement MQTT on HA yourself - there are loads of tutorials on line
// It's very easy!
// Once it's working, this will install a number of entities which are defined in Webastardo_V3_WiFi_HA.ino.  Change the Entity names
// to whatever you prefer.  Avoid spaces, underscores, minus signs or punctuation in the names - it doesn't report an error,
// just doesn't work!
//
// Temperature to a pre-defined target by adjusting the Fueling and Combustion fan
// I've converted the code to run on an Adafruit Feather M0 SAMD21 and designed a drop-in
// replacement PCB for Webasto ThermoTop C & E Heaters.
//
// The wiring harness is similar to the original, except:
// 6 Pin Connector - Pin:  [Changes indicated by * ]
// 1 - Clock (+12V = Heater On)/(0V = Heater Off)
// 2*- ESP32 Serial Out (Not used)
// 3*- Room Temp Thermistor (10k NTC between pin 3 & Gnd)
// 4*- ESP Analog Input 03 (could be configured as a 3.3V Output)
// 5*- Water Thermistor (10k NTC between pin 3 & Gnd) [See Note]
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

//This version will use Home Assistant via MQTT on an M0 WiFi and with a flame sensor only! 


#include <math.h> // needed to perform some calculations
//#include <SPI.h>
#include <WiFi101.h>
#define SECRET_SSID "Enter your SSID"
#define SECRET_PASS "Enter your password"
//#include <WiFiMDNSResponder.h>
#include <ArduinoHA.h>



//MQTT Server Credentials
#define BROKER_ADDR         IPAddress(192,168,1,203) //Change this to the IP address or host of your MQTT Server
#define BROKER_USERNAME     "MQTT Username" // replace with your credentials
#define BROKER_PASSWORD     "MQTT Password"
#define PUBLISH_INTERVAL  1000 // how often image should be published to HA (milliseconds)
byte mac[] = {0xcd, 0xe2, 0xb4, 0x10, 0x8a, 0x60};  //Set this to the mac address of the M0


WiFiClient client;

//MQTT / HA Config
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device,12);




//Define Home Assistant controls & sensors.  These will automatically appear as an MQTT Device called Webastardo
HANumber Target_Room_Temp("targetroomtemp");   //This will be set from HA & trigger a callback

//Effectively read-only values
HANumber Tick_Count("tickcount");   //This will be set from HA & trigger a callback
HANumber Water_Temp("watertemp");
HANumber Room_Temp("roomtemp");
HANumber Flame_Temp("flametemp");
HANumber Water_Pump("waterpump");
HANumber Fan("combustionfan");
HANumber Fuel_Pump("fuelpump");
HANumber Glow_Time("glowtime");

// Switches
HASwitch Run_Heater("runheater");  //Run the heater until Room temperature reaches value set later
HASwitch Purge_Fuel("purgefuel");  //When the heater is off (and cool), run the fuel pump to purge fuel through the lines

//Heater Config 
//*********************************************************************************
//**Change these values to suit your application **
int heater_min = 55; // Increase fuel if below
int heater_target = 65; // degrees C Decrease fuel if above, increase if below.
int water_warning = 75;// degrees C - At this temperature, the heater idles
int water_overheat = 85;// degrees C - This is the temperature the heater will shut down

int flame_threshold = 90; //Exhaust temperature above which we assume it's alight

//*******************************************************************************************
// Heater running Settings
//*******************************************************************************************

//*** Set the fuel pump size
//Values 22,30 or 60 seem to work in most cases.
int pump_size = 60; //22,30,60 

//*******************************************************************************************
//Fuel Mixture
float throttling_high_fuel = 1.8;
float throttling_high_fan = 95;
float throttling_steady_fuel = 1.3;
float throttling_steady_fan = 70;
float throttling_low_fuel = 0.83;  
float throttling_low_fan = 60;
//Just enough to keep it alight at idle
float throttling_idle_fuel = 0.6; //Do not reduce this value
float throttling_idle_fan = 35; 

 
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
//float start_fuel_Threshold = -10; //Exhaust temperature, below which to use start_fuel_Cold
float start_fuel_Threshold = 20; //Water temperature, below which to use start_fuel_Cold
float start_fuel_Cold = 1.2;  //Winter Setting (use below 10C)
float start_fuel_Warm = 1.0;  //Winter Setting (use below 10C)

int full_power_increment_time = 120; //seconds




//*******************************************************************************************
//  Pin Connections
//*******************************************************************************************
int fuel_pump_pin = 11;
int glow_plug_pin = 5;
int burn_fan_pin = 10;
int water_pump_pin = 9;
int water_temp_pin = A1;
int Room_Temp_pin = A2;
int lambda_pin = A3;
int push_pin = A0;
int flame_sensor = A4;


//*******************************************************************************************
//  Temperature Filtering
//*******************************************************************************************
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
float Max_Change_Per_Sec_Exh = 2;  //Used to slow down changes in temperature to remove spikes
float Max_Rise_Per_Sec_Exh = 8;  //Used to slow down changes in temperature to remove spikes
float Max_Change_Per_Sec_Wat = 2;  //Used to slow down changes in temperature to remove spikes
//Flame Sensor workings
float Flame_Diff = 0; 
//float Flame_Threshold = 1.000;
long Flame_Timer = millis(); //prevent flame sensor being called too often
float Flame_Last = 0;



//*******************************************************************************************
//Varaiables
//*******************************************************************************************
String message = "Off";
bool HeaterOn;  //Used internally to switch on / off
bool RemoteHeaterOn; //Set by Home Assistant
bool debug_glow_plug_on = false;
int debug_water_percent_map = 999;
int Ignition_Failures = 0;
float fan_speed; // percent
float water_pump_speed; // percent
float fuel_need; // percent
int glow_time; // seconds
float water_temp; // degrees C
float Flame_temp_value; // degrees C
float Flame_temp_sec[11]; // array of last 10 sec water temp, degres C
int water_temp_sec[181];
int glow_left = 0;
int last_glow_value = 0;
bool burn = false;
bool Heater_fail = false;
int Start_Failures = 0;
int seconds;
int room_temp_set = 23;  // desired room temp - degrees C
float room_temp;  // degrees C
long restart_timer = 0;  //Holder for restart time
long restart_delay = 60; //(minutes) After an overheat, delay the restart 

bool lean_burn;
int delayed_period = 0;
unsigned long water_pump_started_on;
int water_pump_started = 0;
long glowing_on = 0;
int burn_mode = 0;



//*******************************************************************************************
// Setup
//*******************************************************************************************
void setup() {
  Serial.begin(115200);
  delay(200);

  //Config pins for Adafruit M0 Wifi
  WiFi.setPins(8, 7, 4, 2);

  //Set pin definitions
  pinMode(glow_plug_pin, OUTPUT);
  pinMode(fuel_pump_pin, OUTPUT);
  pinMode(burn_fan_pin, OUTPUT);
  pinMode(water_pump_pin, OUTPUT);
  pinMode(water_temp_pin, INPUT);
  pinMode(Room_Temp_pin, INPUT);
  pinMode(push_pin, INPUT);
  pinMode(lambda_pin, INPUT);
  pinMode(flame_sensor, INPUT);

  //Little startup sequence to let you know it's running  
  analogWrite(water_pump_pin, 255); //Run water pump on startup for a few seconds
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
  
  //Make sure ADCs are reading Max resolution
  analogReadResolution(12);



  int counter = 0;
  byte mac[6];
  delay(10);
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(SECRET_SSID);

  WiFi.begin(SECRET_SSID, SECRET_PASS);
  
  WiFi.macAddress(mac);
  String g_UniqueId =  "0x" + String(mac[1],HEX) + ", 0x" + String(mac[0],HEX) + ", 0x" + String(mac[2],HEX) + ", 0x" + String(mac[3],HEX) + ", 0x" + String(mac[4],HEX) + ", 0x" + String(mac[5],HEX);

  Serial.print("Local MAC Address: ");
  Serial.println(g_UniqueId);    
  Serial.print("Update the code with this value! ");
  
  while(WiFi.status() != WL_CONNECTED && counter++ < 8) 
  {
      delay(1000);
      Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() == WL_CONNECTED)
  {
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
  } else
  {
      Serial.println("WiFi NOT connected!!!");
  }

  //Set the definitions for the HA Controls
  HASetup();
  if(!mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD)) {
    Serial.println("MQTT NOT CONNECTED !!!");
  } else {
    Serial.println("MQTT Connected OK");
  }

  for(int cnt=0; cnt<100; cnt++){
    mqtt.loop();
    delay(5);
  }

  SendHADummyData(); //Send HA some dummy data to persuade it to initialise all the controls
}

//*******************************************************************************************
// Loop
//*******************************************************************************************
void loop() { // runs over and over again, calling the functions one by one
  //Reconnect WiFi if disconnected
  while(WiFi.status() != WL_CONNECTED) 
  {
      delay(1000);
      Serial.println("Reconnecting WiFi");
      WiFi.begin(SECRET_SSID, SECRET_PASS);
  }

  mqtt.loop();
  temp_data();
  mqtt.loop();
  control();
  mqtt.loop();
  Heater();
  mqtt.loop();
  Fuel_Purge_Action();
  
  

}


//*******************************************************************************************
// Fuel Purge
//*******************************************************************************************
void Fuel_Purge_Action() {
//If it's safe to do so (heater & glow plug switched off), run the fuel pump rapidly to purge air
  if(!HeaterOn && !debug_glow_plug_on) {
    if(Fuel_Purge) {
      fuel_need = prime_ratio(prime_low_temp);  //Set fuel rate to max
    } else {
      fuel_need = 0;
    }
    fuel_pump();
  }
}


//*******************************************************************************************
// HA Setup
//*******************************************************************************************
void HASetup(){
//This defines all the controls which will be used in Home Assistant
//The controls will magically appear in the HA UI (look in Devices for "Webastardo")
  // This is the name of the MQTT Device which will appear in HA (Magically!)
  device.setName("Office Heating");
  device.setSoftwareVersion("3.2");
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));
  device.setManufacturer("PSI-Design Ltd");
  device.setModel("Webastardo V3");


  //Define control switches
  Run_Heater.setName("Run Heater");
  Run_Heater.setIcon("mdi:radiator");
  Run_Heater.onCommand(onSwitchCommand);
  Run_Heater.setCurrentState(0);

  Purge_Fuel.setName("Purge Fuel Line");
  Purge_Fuel.setIcon("mdi:fuel");
  Purge_Fuel.onCommand(onSwitchCommand);    
  Purge_Fuel.setCurrentState(0);

  //Define the Number controls

  Tick_Count.setName("Run Time");
  Tick_Count.setMode(HANumber::ModeBox); //Display in a box
  Tick_Count.setCurrentState(0);
  Tick_Count.setIcon("mdi:clock-outline");
  Tick_Count.setMin(0); // can be float if precision is set via the constructor
  Tick_Count.setMax(48000); // can be float if precision is set via the constructor
  Tick_Count.setStep(1.0f); // minimum step: 0.001f

  //Target Room Temperature
  Target_Room_Temp.setIcon("mdi:thermometer");
  Target_Room_Temp.setName("Target Room Temp");
  Target_Room_Temp.setMin(5); // can be float if precision is set via the constructor
  Target_Room_Temp.setMax(30); // can be float if precision is set via the constructor
  Target_Room_Temp.setStep(1.0f); // minimum step: 0.001f
  // Target_Temp.setMode(HANumber::ModeBox); //Display numeric value in a box
  Target_Room_Temp.setMode(HANumber::ModeSlider);  //Default
  //Target_Temp.setRetain(true);  //HA will retain the last value set
  //Target_Temp.setCurrentState(0);

  //Define SensorNumber controls (read only)
  //Glow Plug time remaining
  Glow_Time.setIcon("mdi:sun-clock-outline");
  Glow_Time.setName("Glow Time");
  Glow_Time.setCurrentState(0);

  //Current Room Temperature
  Room_Temp.setIcon("mdi:home-thermometer-outline");
  Room_Temp.setName("Current Room Temp");
  Room_Temp.setMin(-10); // can be float if precision is set via the constructor
  Room_Temp.setMax(40); // can be float if precision is set via the constructor
  Room_Temp.setStep(1.0f); // minimum step: 0.001f
  // Room_Temp.setMode(HANumber::ModeBox); //Display numeric value in a box
  Room_Temp.setMode(HANumber::ModeSlider);  //Default
  Room_Temp.setRetain(false);  //HA will retain the last value set
  Room_Temp.setCurrentState(0);

  //Water Temperature
  Water_Temp.setIcon("mdi:thermometer-water");
  Water_Temp.setName("Water Temp");
  Water_Temp.setMin(-20); // can be float if precision is set via the constructor
  Water_Temp.setMax(100); // can be float if precision is set via the constructor
  Water_Temp.setStep(1.0f); // minimum step: 0.001f
  // Water_Temp.setMode(HANumber::ModeBox); //Display numeric value in a box
  Water_Temp.setMode(HANumber::ModeSlider);  //Default
  Water_Temp.setRetain(false);  //HA will retain the last value set
  //Water_Temp.setValue(0);
  Water_Temp.setCurrentState(0);

  //Flame Temperature
  Flame_Temp.setIcon("mdi:fire");
  Flame_Temp.setName("Flame Temp");
  Flame_Temp.setMin(-20); // can be float if precision is set via the constructor
  Flame_Temp.setMax(500); // can be float if precision is set via the constructor
  Flame_Temp.setStep(1.0f); // minimum step: 0.001f
  // Flame_Temp.setMode(HANumber::ModeBox); //Display numeric value in a box
  Flame_Temp.setMode(HANumber::ModeSlider);  //Default
  Flame_Temp.setRetain(false);  //HA will retain the last value set
  //Flame_Temp.setValue(0);
  Flame_Temp.setCurrentState(0);

  //Water Pump
  Water_Pump.setIcon("mdi:water-pump");
  Water_Pump.setName("Water Pump");
  Water_Pump.setMin(0); // can be float if precision is set via the constructor
  Water_Pump.setMax(100); // can be float if precision is set via the constructor
  Water_Pump.setStep(1.0f); // minimum step: 0.001f
  // Water_Pump.setMode(HANumber::ModeBox); //Display numeric value in a box
  Water_Pump.setMode(HANumber::ModeSlider);  //Default
  Water_Pump.setRetain(false);  //HA will retain the last value set
  //Water_Pump.setValue(0);
  Water_Pump.setCurrentState(0);

  //Combustion Fan
  Fan.setIcon("mdi:fan");
  Fan.setName("Combustion Fan");
  Fan.setMin(0); // can be float if precision is set via the constructor
  Fan.setMax(100); // can be float if precision is set via the constructor
  Fan.setStep(1.0f); // minimum step: 0.001f
  // Fan.setMode(HANumber::ModeBox); //Display numeric value in a box
  Fan.setMode(HANumber::ModeSlider);  //Default
  Fan.setRetain(false);  //HA will retain the last value set
  //Fan.setValue(0);
  Fan.setCurrentState(0);

  //Fuel Pump
  Fuel_Pump.setIcon("mdi:gas-station");
  Fuel_Pump.setName("Fuel Hz");
  Fuel_Pump.setMin(0); // can be float if precision is set via the constructor
  Fuel_Pump.setMax(4); // can be float if precision is set via the constructor
  Fuel_Pump.setStep(1.0f); // minimum step: 0.001f
  // Fuel_Pump.setMode(HANumber::ModeBox); //Display numeric value in a box
  Fuel_Pump.setMode(HANumber::ModeSlider);  //Default
  Fuel_Pump.setRetain(false);  //HA will retain the last value set
  //Fuel_Pump.setValue(0);
  Fuel_Pump.setCurrentState(0);

  //Setup Callbacks
  Run_Heater.onCommand(onSwitchCommand);
  Purge_Fuel.onCommand(onSwitchCommand);
  Target_Room_Temp.onCommand(onNumberCommand);
  Target_Room_Temp.setCurrentState(1);  //Required as initially set to null


}

void SendHADummyData(){
  //Send a little dummy data to initiate controls in HA
  Water_Temp.setState(0,true); // remember to change precision before using floats
  Room_Temp.setState(0,true); // remember to change precision before using floats
  Flame_Temp.setState(0,true); // remember to change precision before using floats
  Water_Pump.setState(0,true); // remember to change precision before using floats
  Fan.setState(0,true); // remember to change precision before using floats
  Fuel_Pump.setState(0,true); // remember to change precision before using floats

  Target_Room_Temp.setState(room_temp_set,true);

  Run_Heater.setState(0,true);
  Purge_Fuel.setState(0,true);
  mqtt.loop();

}



//*******************************************************************************************
// HA ESPHome Callbacks
//*******************************************************************************************
void onSwitchCommand(bool state, HASwitch* sender)
{
    sender->setState(state); // report state back to the Home Assistant that command received
    delay(10);
    if (sender == &Run_Heater) {
        // the switch1 has been toggled
        // state == true means ON state
        //if(Run_Heater.getCurrentState()) RemoteHeaterOn = true; else RemoteHeaterOn = false;
        //if(!Run_Heater.getCurrentState()) RemoteHeaterOn = false;
        Serial.print("Heater Command: "); Serial.println(RemoteHeaterOn);
    } else if (sender == &Purge_Fuel) {
        // the switch2 has been toggled
        // state == true means ON state
        //if(Purge_Fuel.getCurrentState()) Fuel_Purge = true; else Fuel_Purge = false;
        Serial.print("Purge Command: "); Serial.println(Fuel_Purge);
    }

}

void onNumberCommand(HANumeric number, HANumber* sender){
  if (!number.isSet()) {
    // the reset command was send by Home Assistant
  } else {
    // you can do whatever you want with the number as follows:
    //int8_t numberInt8 = number.toInt8();
    //int16_t numberInt16 = number.toInt16();
    int32_t numberInt32 = number.toInt32();
    //uint8_t numberUInt8 = number.toUInt8();
    //uint16_t numberUInt16 = number.toUInt16();
    //uint32_t numberUInt32 = number.toUInt32();
    //float numberFloat = number.toFloat();

    room_temp_set = numberInt32;
    
  }

  Serial.print("New Number Received = "); Serial.println(number.getBaseValue());
  sender->setState(number); // report the selected option back to the HA panel
}


//*******************************************************************************************
// Wifi Status
//*******************************************************************************************
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


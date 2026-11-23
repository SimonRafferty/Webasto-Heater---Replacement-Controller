//*******************************************************************************************
// Functions to save settings in Non Volatile Storage using the Preferences.h library
//
// Every user setting is listed once in Settings[] below.  The same table is used to load and
// save them, to send them to the web page and to validate what the web page sends back,
// so adding a setting only needs one line here (plus an input on the web page).
//*******************************************************************************************

//(struct Setting is defined in the main .ino so the Arduino builder's prototypes can see it)
Setting Settings[] = {
  //Fuel mixture (Hz for a 60 pump / fan %)
  {"high_fuel",     &throttling_high_fuel,   0.2,  6.0},
  {"high_fan",      &throttling_high_fan,    10,   100},
  {"steady_fuel",   &throttling_steady_fuel, 0.2,  6.0},
  {"steady_fan",    &throttling_steady_fan,  10,   100},
  {"low_fuel",      &throttling_low_fuel,    0.2,  6.0},
  {"low_fan",       &throttling_low_fan,     10,   100},
  {"idle_fuel",     &throttling_idle_fuel,   0.2,  6.0},
  {"idle_fan",      &throttling_idle_fan,    10,   100},
  //Room thermostat
  {"room_target",   &room_temp_target,       5,    35},
  //Water temperature thresholds
  {"heater_min",    &heater_min,             10,   95},
  {"heater_target", &heater_target,          10,   95},
  {"water_warning", &water_warning,          10,   100},
  {"water_overheat",&water_overheat,         10,   105},
  //Exhaust
  {"flame_thresh",  &flame_threshold,        30,   400},
  {"flame_rise",    &flame_rise,             1,    50},
  {"exh_overheat",  &exhaust_overheat,       100,  700},
  //Power
  {"glow_volts",    &glow_volts,             6,    14},
  {"low_volt",      &low_volt_cutoff,        9,    13},
  {"high_volt",     &high_volt_cutoff,       14,   17},
  //Calibration
  {"rail_3v3",      &rail_3v3,               3.1,  3.5},
  {"vbat_scale",    &vbat_scale,             4.0,  7.0},
  {"ntc_r25",       &ntc_r25,                1000, 100000},
  {"ntc_beta",      &ntc_beta,               2000, 5000},
  {"water_offset",  &water_offset,           -20,  20},
  {"room_offset",   &room_offset,            -20,  20},
  {"exh_offset",    &exhaust_offset,         -50,  50},
};
const int Num_Settings = sizeof(Settings) / sizeof(Settings[0]);

//Find a setting by key (NULL if not found)
Setting *Find_Setting(const String &key) {
  for (int i = 0; i < Num_Settings; i++) {
    if (key == Settings[i].key) return &Settings[i];
  }
  return NULL;
}

//*******************************************************************************************
// Get preferences from NVS.  Anything not saved yet keeps the default from the variable definition
//*******************************************************************************************
void Load_Preferences() {
  for (int i = 0; i < Num_Settings; i++) {
    float v = preferences.getFloat(Settings[i].key, *Settings[i].value);
    if (isnan(v)) v = *Settings[i].value;
    *Settings[i].value = constrain(v, Settings[i].min_val, Settings[i].max_val);
  }
  pump_size = preferences.getInt("pump_size", pump_size);
  RunToTemp = preferences.getBool("RunToTemp", RunToTemp);
  BLE_Pin = preferences.getUInt("ble_pin", BLE_Pin);
  if (BLE_Pin < 100000 || BLE_Pin > 999999) BLE_Pin = 123456;
  WiFi_SSID = preferences.getString("wifi_ssid", WiFi_SSID);
  WiFi_PASS = preferences.getString("wifi_pass", WiFi_PASS);

  //Sanity - the thresholds must be in order or the fueling logic misbehaves
  if (!Thresholds_Valid()) {
    Serial.println("Saved temperature thresholds out of order - using defaults");
    heater_min = 55; heater_target = 65; water_warning = 75; water_overheat = 85;
  }
}

//*******************************************************************************************
// Save all the settings in Flash.  Called from loop() - never from the web server task
//*******************************************************************************************
void SavePreferences() {
  for (int i = 0; i < Num_Settings; i++) {
    preferences.putFloat(Settings[i].key, *Settings[i].value);
  }
  preferences.putInt("pump_size", pump_size);
  preferences.putBool("RunToTemp", RunToTemp);
  Serial.println("Settings saved");
}

void SaveWiFiPreferences() {
  preferences.putString("wifi_ssid", WiFi_SSID);
  preferences.putString("wifi_pass", WiFi_PASS);
}

//*******************************************************************************************
// Erase everything - defaults are loaded on the next boot
//*******************************************************************************************
void ResetPreferences() {
  preferences.clear();
  Serial.println("Settings erased - defaults on next boot");
}

bool Thresholds_Valid() {
  return (heater_min < heater_target) && (heater_target < water_warning) && (water_warning < water_overheat);
}

//*******************************************************************************************
// Pump size must be one of 0, 22, 33 or 60
//*******************************************************************************************
int Valid_Pump_Size(float new_pump_size) {
  if (new_pump_size <= 0) return 0;
  if (new_pump_size < 30) return 22;
  if (new_pump_size < 60) return 33;
  return 60;
}

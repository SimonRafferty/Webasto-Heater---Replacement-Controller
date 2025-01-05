//*******************************************************************************************
// Functions to save settings in Non Volatile Storage
// using the Preferences.h library
//*******************************************************************************************

void Load_Preferences(){
//Get preferneces from NVS

  if(preferences.getBool("Saved", false)){
    //Data has previously need saved.  Restore to global variables
    //Set default as variable just in case

    room_temp_target = preferences.getDouble("temp_target", room_temp_target); 
    RunToTemp  = preferences.getBool("RunToTemp", RunToTemp); 
    throttling_high_fuel = preferences.getDouble("high_fuel", throttling_high_fuel); 
    throttling_high_fan = preferences.getDouble("high_fan", throttling_high_fan); 
    throttling_steady_fuel = preferences.getDouble("steady_fuel", throttling_steady_fuel); 
    throttling_steady_fan = preferences.getDouble("steady_fan", throttling_steady_fan); 
    throttling_low_fuel = preferences.getDouble("low_fuel", throttling_low_fuel); 
    throttling_low_fan = preferences.getDouble("low_fan", throttling_low_fan); 
    throttling_idle_fuel = preferences.getDouble("idle_fuel", throttling_idle_fuel); 
    throttling_idle_fan = preferences.getDouble("idle_fan", throttling_idle_fan); 
    heater_min = preferences.getDouble("heater_min", heater_min); 
    heater_target = preferences.getDouble("heater_target", heater_target); 
    water_warning = preferences.getDouble("water_warning", water_warning); 
    water_overheat = preferences.getDouble("water_overheat", water_overheat); 
    flame_threshold = preferences.getDouble("flame_threshold", flame_threshold); 
    pump_size = preferences.getDouble("pump_size", pump_size); 


  } else {
    //No Data - Save from variables
    SavePreferences();
  }

}
void SavePreferences(){
  //Save all the variables in Flash

  preferences.putDouble("temp_target", room_temp_target); 
  preferences.putBool("RunToTemp", RunToTemp); 
  preferences.putDouble("high_fuel", throttling_high_fuel); 
  preferences.putDouble("high_fan", throttling_high_fan); 
  preferences.putDouble("steady_fuel", throttling_steady_fuel); 
  preferences.putDouble("steady_fan", throttling_steady_fan); 
  preferences.putDouble("low_fuel", throttling_low_fuel); 
  preferences.putDouble("idle_fuel", throttling_idle_fuel); 
  preferences.putDouble("idle_fan", throttling_idle_fan); 
  preferences.putDouble("heater_min", heater_min); 
  preferences.putDouble("heater_target", heater_target); 
  preferences.putDouble("water_warning", water_warning); 
  preferences.putDouble("water_overheat", water_overheat); 
  preferences.putDouble("flame_threshold", flame_threshold); 
  preferences.putDouble("pump_size", pump_size); 
  preferences.putBool("Saved", true);  //Flag to say data has been saved

}

void ResetPreferences(){
  //Only reset if heater is off
  if(!heater_on()){
    preferences.putBool("Saved", false);  //Flag to say NO data has been saved
    
    //Need to reboot to load values from original variable definitions

  }
}



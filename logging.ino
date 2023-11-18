void logging(int ignit_fail, float temp_init, int seconds){
    // print all the interesting data
    if(((float)seconds/2.0) == (int)(seconds/2.0)) HA_Send(); //Send data every 2 sec

    //New debug variables 
    Serial.print(" | BTN: ");
    Serial.print(HeaterOn);
    //Serial.print(" | Blk: ");
    //Serial.print(HeaterOn);
    
    //Serial.print(analogRead(push_pin));
    Serial.print(" | Glow: ");
    Serial.print(debug_glow_plug_on);

    
    float water_percentage = (100.00/255.00) * debug_water_percent_map;
    Serial.print(" | WTR_SP: ");
    Serial.print(water_percentage);    
    //Serial.print("/");
    //Serial.print(debug_water_percent_map);    
    
    //Serial.print(" | Fail: ");
    //New debug variables 


    //Serial.print(Heater_fail);
//    Serial.print(" | StartFail#: ");
//    Serial.print(Start_Failures);
//    Serial.print(" | BGo: ");
//    Serial.print(burn);
    Serial.print(" | B Mode: ");
    Serial.print(burn_mode);

//    if(burn_mode == 0)
//      Serial.print("OFF");
//    if(burn_mode == 1)
//      Serial.print("Starting");
//    if(burn_mode == 2)
//      Serial.print("Running");
//    if(burn_mode == 3)
//      Serial.print("Shuting Down");

    //Serial.print(burn_mode);
    Serial.print(" | W_Tmp: ");
    Serial.print(water_temp);
    //Serial.print(" | W_Raw: ");
    //Serial.print(rawDataWater);
    Serial.print(" | E_Tmp: ");
    Serial.print(Flame_temp_value);
    //Serial.print(" | E_Raw: ");
    //Serial.print(rawDataExhaust);
    //Serial.print(" | E_Flame: ");
    //Serial.print(Flame_Temp());
    Serial.print(" | Room T: ");
    Serial.print(room_temp);
    Serial.print(" | Set T: ");
    Serial.print(room_temp_set);
    //Serial.print(" | R.Strt: ");
    //Serial.print(restart_timer*60-seconds);
    
    if(burn_mode == 1)
    {
      //Serial.print("/");
      //Serial.print(temp_init+3);
    }
    Serial.print(" | Fan_Speed_%: ");
    Serial.print(fan_speed);
//    Serial.print(" | Fuel_HZ ");
//    if(delayed_period>0)
//      Serial.print(1000.00/delayed_period);
    Serial.print(" | Fuel: ");
    Serial.print(fuel_need);
    
   // Serial.print(" | Glow For (Sec): ");
   // Serial.print(glow_time);
    //Serial.print(" | Glow Left: ");
    //
    //Serial.print(glow_left);
    
    Serial.print(" | Time: ");


    
    Serial.print(seconds);
    Serial.print(" |  ");
    Serial.println(message); 



}


void HA_Send(){
  //Send data to HA
  Water_Temp.setState(water_temp,true); // remember to change precision before using floats
  Room_Temp.setState(room_temp,true); // remember to change precision before using floats
  Flame_Temp.setState(Flame_temp_value,true); // remember to change precision before using floats
  float water_percentage = (100.00/255.00) * debug_water_percent_map;
  Water_Pump.setState(water_percentage,true); // remember to change precision before using floats
  Fan.setState(fan_speed,true); // remember to change precision before using floats
  Fuel_Pump.setState(fuel_need * 60,true); // Pulses per minute
  Serial.println("Sending HA Data");
  Target_Room_Temp.setState(room_temp_set,true);
  Tick_Count.setState(seconds,true);
  Glow_Time.setState(glow_left,true);

  //Read on/off switches
  //I've found, occasionally they glitch on very briefly fro some reason
  RemoteHeaterOn = (Run_Heater.getCurrentState()) && (Run_Heater.getCurrentState()); //Check twice, just in case it's a glitch
  Fuel_Purge = (Purge_Fuel.getCurrentState()) && (Purge_Fuel.getCurrentState());

}




void logging(int ignit_fail, float temp_init, int seconds){
    // print all the interesting data
#ifdef BLYNK_ENABLE    
    if(((float)seconds/10.0) == (int)(seconds/10.0)) Blynk_Send();
#endif    
    //New debug variables 
    Serial.print(" | BTN: ");
    Serial.print(heater_on);
    Serial.print(" | Blk: ");
    Serial.print(BlynkHeaterOn);
    
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


    //Serial.print(webasto_fail);
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
    Serial.print(exhaust_temp);
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

#ifdef BLYNK_ENABLE
  void Blynk_Send(){
  //Called periodically by Blynk Timer
  
    if(Blynk.connected()) {
      //Serial.println("Blynk Connected");
      Blynk.virtualWrite(V40, burn_mode);
      Blynk.virtualWrite(V41, Start_Failures);
      Blynk.virtualWrite(V42, water_temp);
      Blynk.virtualWrite(V43, exhaust_temp);
      Blynk.virtualWrite(V44, fan_speed);
      Blynk.virtualWrite(V45, fuel_need);
      Blynk.virtualWrite(V46, glow_left);
      Blynk.virtualWrite(V47, seconds);
      float water_percentage = (100.00/255.00) * debug_water_percent_map;
      Blynk.virtualWrite(V48, water_percentage);
      Blynk.virtualWrite(V49, room_temp);
      Blynk.virtualWrite(V52, room_temp_set);
    
      //Serial.println("Blynk Data Sent");
    }  else {
      Serial.println("Blynk FAILED!");
    }
  
  }
#endif

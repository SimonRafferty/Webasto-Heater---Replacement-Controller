void logging(int ignit_fail, float temp_init, int seconds){
    // print all the interesting data

    //New debug variables 
    Serial.print(" | BTN: ");
    Serial.print(heater_on);
    //Serial.print(analogRead(push_pin));
    Serial.print(" | Glow: ");
    Serial.print(debug_glow_plug_on);

    
    float water_percentage = (100.00/255.00) * debug_water_percent_map;
    Serial.print(" | WTR_SP: ");
    Serial.print(water_percentage);    
    //Serial.print("/");
    //Serial.print(debug_water_percent_map);    
    
    Serial.print(" | Fail: ");
    //New debug variables 
    
    Serial.print(webasto_fail);
    Serial.print(" | IgFail#: ");
    Serial.print(Incidents);
//    Serial.print(" | BGo: ");
//    Serial.print(burn);
//    Serial.print(" | B Mode: ");
//    if(burn_mode == 0)
//      Serial.print("OFF");
//    if(burn_mode == 1)
//      Serial.print("Starting");
//    if(burn_mode == 2)
//      Serial.print("Running");
//    if(burn_mode == 3)
//      Serial.print("Shuting Down");

    Serial.print(burn_mode);
    Serial.print(" | W_Tmp: ");
    Serial.print(water_temp);
    //Serial.print(" | W_Raw: ");
    //Serial.print(rawDataWater);
    Serial.print(" | E_Tmp: ");
    Serial.print(exhaust_temp);
    //Serial.print(" | E_Raw: ");
    //Serial.print(rawDataExhaust);
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
    Serial.print(glow_left);
    Serial.print(" | Batt: ");

    Serial.print(Battery_Voltage(),1);    
    
    Serial.print(" | Time: ");


    
    Serial.print(seconds);
    Serial.print(" |  ");
    Serial.println(message); 

    int Running = int(heater_on);

}

float running_ratio(float exhaust_temp) {
  static unsigned long fuel_change_timer;
  static unsigned long fan_change_timer;
  //Not hot enough to start adjusting
  int fuel_target;
  int fan_target;
  int fuel_current;

  if(exhaust_temp > 40)
  {    
    fuel_current = fuel_need*100;
   
    if(water_temp < heater_min)  { 
      fuel_target = throttling_high_fuel*100;
      fan_target = throttling_high_fan;
    } 
    if((water_temp >= heater_min) && (water_temp < heater_target)) {
      fuel_target = throttling_steady_fuel*100;
      fan_target = throttling_steady_fan;
    }

    if((water_temp >= heater_target) && (water_temp < water_warning)) {
      fuel_target = throttling_low_fuel*100;
      fan_target = throttling_low_fan;
    }

    if((water_temp >= water_warning) && (water_temp < water_overheat))   {
      fuel_target = throttling_idle_fuel*100;
      fan_target = throttling_idle_fan;
    }

    
    if(exhaust_temp>200) { //Just in case exhaust temp getting too high
      fuel_target = throttling_idle_fuel*100;
      fan_target = throttling_idle_fan;
    }




    if(fuel_change_timer == 0)
      fuel_change_timer = millis();
     
    if(fan_change_timer == 0)
      fan_change_timer = millis();
     

    //Adjust Fuel
    if(fuel_target != fuel_current)
    {
        
        if(fuel_current<fuel_target) 
        {
          if(millis() - fuel_change_timer >= 1500){  //Increase fuel slowly
            fuel_need += 0.01;
            message = "T Inc Fuel";
            fuel_change_timer = millis();
          }
        }
        else if(fuel_current>fuel_target)
        {
          if(millis() - fuel_change_timer >= 150) {  //Decrease fuel less slowly
            fuel_need -= 0.01;
            message = "T Dec Fuel";
            fuel_change_timer = millis();
          }
        }
        
    }
    else
      message = "T Running";
  

    //Adjust Fan
    EX_Mute = false; //Temperature unstable while fan speed changing.  Mute readings during change
    if(fan_target != fan_speed)
    {
        
        
        if(fan_speed<fan_target)
        {
          if(millis() - fan_change_timer >= 1500) { //Increase fan slowly
            fan_speed += 0.5;
            message = "T Inc Fan";
            EX_Mute = true;
            fan_change_timer = millis();
          }
        }
        else if(fan_speed>fan_target)
        {
          if(millis() - fan_change_timer >= 150) { //Decrease fuel less slowly
            fan_speed -= 0.5;
            message = "T Dec Fan";
            EX_Mute = true;
            fan_change_timer = millis();
          }
        }          
          
    }
    else
      message = "T Running";
  }
  return true;
}

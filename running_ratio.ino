//*******************************************************************************************
// Set the air/fuel ratio when the pump is running
//*******************************************************************************************
void running_ratio() {
  static unsigned long fuel_change_timer;
  static unsigned long fan_change_timer;
  //Not hot enough to start adjusting
  int fuel_target = 0;
  int fan_target = 0;
  int fuel_current;


  fuel_current = fuel_need*100;
  
  if(Is_Flame())
  {    
    
  // Fueling_Level: 0=Off, 1=Idle, 2=Low, 3=Steady, 4=High
 
    if(water_temp < heater_min)  { 
      fuel_target = throttling_high_fuel*100;
      fan_target = throttling_high_fan;
      Fueling_Level = 4;
    } 
    if((water_temp >= heater_min) && (water_temp < heater_target)) {
      fuel_target = throttling_steady_fuel*100;
      fan_target = throttling_steady_fan;
      Fueling_Level = 3;
    }

    if((water_temp >= heater_target) && (water_temp < water_warning)) {
      fuel_target = throttling_low_fuel*100;
      fan_target = throttling_low_fan;
      Fueling_Level = 2;
    }

    if((water_temp >= water_warning) && (water_temp < water_overheat))   {
      fuel_target = throttling_idle_fuel*100;
      fan_target = throttling_idle_fan;
      Fueling_Level = 1;
    }

/*
    //Just in case exhaust temp getting too high
    if(get_flame_temp()>400) { 
      fuel_target = throttling_idle_fuel*100;
      fan_target = throttling_idle_fan;
    }
*/




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
            InfoMessage = "Inc Fuel";
            fuel_change_timer = millis();
          }
        }
        else if(fuel_current>fuel_target)
        {
          if(millis() - fuel_change_timer >= 150) {  //Decrease fuel less slowly
            fuel_need -= 0.01;
            InfoMessage = "Dec Fuel";
            fuel_change_timer = millis();
          }
        }
        
    }
    else
      InfoMessage = "Running";
  

    //Adjust Fan
    if(fan_target != fan_speed)
    {
        
        
        if(fan_speed<fan_target)
        {
          if(millis() - fan_change_timer >= 1500) { //Increase fan slowly
            fan_speed += 0.5;
            //InfoMessage = "T Inc Fan";
            fan_change_timer = millis();
          }
        }
        else if(fan_speed>fan_target)
        {
          if(millis() - fan_change_timer >= 150) { //Decrease fuel less slowly
            fan_speed -= 0.5;
            //InfoMessage = "T Dec Fan";
            fan_change_timer = millis();
          }
        }          
          
    }
  }
  
}

//*******************************************************************************************
// This is the main running control loop
//*******************************************************************************************
void control() {

  if (heater_on()){
    if ((burn_mode<3) && (Ignition_Failures < 3)) //Don't start if shutting down or it has failed to start 3 times
    {
      if(water_temp*100 < (heater_min)*100) //Only turn on heater when water temp below minimum to stop short-cycling
      {
        burn = true;  //Heater ON/OFF - Start controlled startup process
      }      
    }
    water_pump_speed = 100;    
  } else {
    if(burn_mode>0 && burn_mode<3)
    {
      burn = false;    //Heater ON/OFF - Start controlled shutdown process
    }
    //Run the fan for a while to cool down
    if(burn_mode==0 && seconds<30 && (get_flame_temp()>get_water_temp()+10)) {
      fan_speed = 40;
      burn_fan();  // calls the combustion fan function to update speed
      vTaskDelay(1000);
    }  
    if(burn_mode==0 && seconds>=10) {
      fan_speed = 0;
      burn_fan();  // calls the combustion fan function to update speed
    }  

    //Keep the water pump running until heater has cooled a little
    //If you just witch off the heater, the water can boil!  Much badness!
    if(burn_mode==0) {
      if(water_temp > heater_target) {
        water_pump_speed = 100;  //Keep pump running if water hot
      } else {
        water_pump_speed = 0;
      }
    } else {
      water_pump_speed = 100;  //Keep pump running while heater running 
    }
  }
  
  water_pump(); // calls the water_pump function to update speed

}

//*******************************************************************************************
// Should the heater be on or off?
//*******************************************************************************************
bool heater_on(){
  //return a value indicating whether the heater is on or off
  //Use this to change the relative priority of web and hardware on/off inputs

  //Check for problems
  if(ErrorMessage!=""){
    Web_heater_ON = false; //Switch off on web
    return false; //Do not allow heater to start if there's a hardware problem
  }

  //See if fuel purge is active
  if(Web_purge_ON){
    Web_heater_ON = false; //Switch off on web
    return false; //Do not allow heater to start if priming fuel pump
  }

  //Check & try reconnecting to WiFi.  If it fails, revert to manual control
  if(WiFi_active && !Check_WiFi()){
    WiFi_active = false;
  } else {
    WiFi_active = true;
  }

  if(WiFi_active) {
    //Switch on heater if BOTH hardware trigger and web on/off are active
    //This lets you use the hardware as an override or maybe connected to a thermostat?
    if(Web_heater_ON && (Pushbutton())){
      //With the web interface, the heater can either run to temp then switch off, or switch on/off to maintain temperature, like a thermostat
      //Hysteresis is the gap between switching off then back on again.  Say you set target at 20C and hysteresis at 4C, it will switch off at 22C then back on at 18C
      if(room_temp<(room_temp_target-room_temp_hysteresis/2)) {
        return true;
      } else if(room_temp>=(room_temp_target+room_temp_hysteresis/2)) {
        //Room temperature has reached target
        if(RunToTemp){
          //Switch off heater and don't re-start until re-triggered from Web Interface
          Web_heater_ON = false; //Switch off on web
          return false;
        } else {
          return false;
        }
      }
    } else {
      return false;
    }
  } else {
    //WiFi inactive, just use the hardware ON/OFF switch instead
    if(Pushbutton()){
      //Make room temp act like a thermostat, restarting the heater when the room has cooled
      if(room_temp<(room_temp_target-room_temp_hysteresis/2)) {
        return true;
      } else if(room_temp>=(room_temp_target+room_temp_hysteresis/2)) {
        //Room temperature has reached target
        return false;
      }
    } else {
      return false;
    }
  }
  

}

bool Pushbutton(){
//Is the hardware on/off pushbutton pressed?
  return(analogRead(ON_OFF_pin)>250);
}

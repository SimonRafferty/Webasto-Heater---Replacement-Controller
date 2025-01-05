//*******************************************************************************************
// Finite State Machine to start, run & stop heater
//*******************************************************************************************
void Heater() { // this will handle the combustion

  if (!Heater_fail) { // if everything's going fine
    Heater_Start_Stop();
  } else { // if there has been a major failure, stop everything
    burn = 0;
    burn_mode = 3; //Controlled shutdown
    
    AddError("*** Heater Failure - Check Hardware!<br>" + InfoMessage + " , ");
  }

  //Run the heater
  switch (burn_mode) {
    case 0: { // everything is turned off in this mode
      Heater_FSM_Off();
    } break;
    case 1: { // Start Heater
      Heater_FSM_Startup();
    } break;
    case 2: { // Heater running - control fueling dynamically
      Heater_FSM_Running();
    } break;
    case 3: { // Shut down heater
      Heater_FSM_Shutdown();
    } break;
  }

  //Control peripherals
  burn_fan();
  glow_plug();

}


//*******************************************************************************************
// Transition from heater off to on or on to off
//*******************************************************************************************
void Heater_Start_Stop(){
  if ((burn_mode == 0) && burn) {  //Heater has been switched on
    // initiate the start sequence and retry
    burn_mode = 1;
    seconds = 0;
    Ignition_Failures = 0;
    glow_time = 0;
  }

  if ((burn_mode == 1 || burn_mode == 2) && !burn) { //Heater has been switched off
    burn_mode = 3;
    seconds = 0;
    Ignition_Failures = 0;
  }


  if ((Ignition_Failures > 3) && (burn_mode == 1)) { // if there was more than 3 attempts to start fire but all failed
    InfoMessage = ">3 Ign Failures";
    Heater_fail = true;
    burn_mode = 3;
    seconds = 0;
    burn = false;
  }
}


  
//*******************************************************************************************
// Heater is OFF, shut down everything
//*******************************************************************************************
void Heater_FSM_Off(){ // everything is turned off in this mode
  if(!Web_purge_ON) Fuel_Rate(pump_size, 0.0); //Unless fuel purge is active, switch off fuel pump
  fan_speed = 0;
  glow_time = 0;
  //Leave the water pump as is.  It will run until cool
} 

//*******************************************************************************************
// Heater is Startng up
//*******************************************************************************************
void Heater_FSM_Startup(){ // everything is turned off in this mode
static unsigned long Startup_fan_timer = 0;

  //If burn chamber is hot.  Allow to cool before starting
  if(seconds < 5){
    if(!Chamber_Cool())  
      {
        InfoMessage = "Initial Cooling";
        fan_speed = 60;
        Fuel_Rate(pump_size, 0.0);
        seconds = 0;
      }
  }    
  //**** 0 to 5 Seconds - Clear Chamber ****
  if(seconds > 0 && seconds < 5){
    fan_speed = 70; //prime_fan_speed * 2;
   glow_time = 60;  //30 Sec was not long enough in cold weather
    Fuel_Rate(pump_size, 0.0);
    InfoMessage = "Clearing Chamber";
  }


  //**** 6 to 9 Seconds - Prime Fuel ****
  if(seconds >= 6 && seconds <= 9)
  {
    fan_speed = 15;
    Fuel_Rate(pump_size, prime_rate(water_temp));
    InfoMessage = "Prime";
  }

  //**** 9 to 11 Seconds - Stop Fuel ****
  if(seconds > 9 && seconds <= 11)
    Fuel_Rate(pump_size, 0.0);
    

  //**** 11 Seconds - Begin firing up ****
  if (seconds > 17) { 
    //Cold Start Enrichment - makes starting easier in cold weather
    if(water_temp < start_fuel_Threshold) {
      Fuel_Rate(pump_size, start_fuel_Cold);
    } else {
      Fuel_Rate(pump_size, start_fuel_Warm);
    }
    InfoMessage = "Firing Up";

    //Gradually increase fan speed
    if(fan_speed < start_fan_speed)
    {
      if(millis() - Startup_fan_timer >= 333)
      {
        fan_speed += 1;
        Startup_fan_timer = millis();
      }
    } else {
      fan_speed = start_fan_speed; //Startup fan speed reached, hold at that for a while  
    }     
  }


  //**** 50 Seconds - Do we have a flame? ****
  if (Is_Flame() && (seconds >=50)) {
    //Is_Flame() will return false until Glow Plug switched off
    burn_mode = 2;            //Change to the Running state
    seconds = 0;              //Reset clock
    glow_time = 0;            //Make sure Glow Plug is Off (probably not needed)
    Ignition_Failures = 0;    //Successful start, clear failure count
    Startup_fan_timer = millis();
    InfoMessage = "Started";

  }

  //**** 100 Seconds and no flame! - Startup has failed, go to Shutdown mode ****
  if ((seconds > 100) && (burn_mode == 1)) {
    // the fire sequence didn't work, give it an other try
    burn_mode = 3;          //Change to the Shutdown state
    seconds = 0;            //Reset clock
    Ignition_Failures++;     //Increase failure count
    InfoMessage = "Restarting";
  }



} 



//*******************************************************************************************
// Heater is Running - control Fueling
//*******************************************************************************************
void Heater_FSM_Running(){ 
static unsigned long Startup_fan_timer = 0;
  //Gradually increase Fuel & Combustion fan until heater reaches it's minimum operating temperature, then throttle automatically
  if(water_temp <= (heater_min-15)) {
    InfoMessage = "Increasing Burn";
    if(millis() - Startup_fan_timer >= 333) { //Slowly increase fueling
      fan_speed += (throttling_high_fan-35.00)/full_power_increment_time/3;
      if(fan_speed>throttling_high_fan) fan_speed=throttling_high_fan;
      fuel_need += (throttling_high_fuel-1.00)/full_power_increment_time/3;
      if(fuel_need>throttling_high_fuel) fuel_need=throttling_high_fuel;
      Fuel_Rate(pump_size, fuel_need);
      Startup_fan_timer = millis();
    }
  } else {
    //Run automatic throttling 
    running_ratio();  //Sets fuel_need variable
    Fuel_Rate(pump_size, fuel_need);  //Execute change                    
  }

  //Check for an overheat
  if(water_temp > water_overheat){
    fan_speed = 80;                   //Set a relatively high fan speed
    Fuel_Rate(pump_size, 0.0);        //Turn off fueling
    InfoMessage = "Overheating";
    burn = 0;
    burn_mode = 3;                    //Change mode to controlled shutdown
    seconds = 0;                      //Reset Clock
  }

  //Check for a thermocouple failure
  if(isnan(th_water_temp.readCelsius())) {      
    fan_speed = 80;                   //Set a relatively high fan speed
    Fuel_Rate(pump_size, 0.0);        //Turn off fueling
    InfoMessage = "Thermocouple";
    Heater_fail = true;               //Heater will not start until reboot
    burn = 0;
    seconds = 0;                      //Reset Clock
  }

  //If, after 60s running, the exhaust temp drops below the water temp, the flame must have died. 
  if ((!Is_Flame() || get_flame_temp() < water_temp) && seconds >= 60) {
    Fuel_Rate(pump_size, 0.0);        //Turn off fueling
    InfoMessage = "Exh < Wat T";
    Ignition_Failures++;
    burn = 0;
    seconds = 0;                      //Reset Clock
  }
}

//*******************************************************************************************
// Heater is Running - control Fueling
//*******************************************************************************************
void Heater_FSM_Shutdown(){ 
 // snuff out the fire, with just a little air to avoid fumes and backfire

  Fuel_Rate(pump_size, 0.0);
  //Purge-cool for at least 30 sec, and until there's no flame
  if (seconds <= 30 || Is_Flame()) {
    fan_speed = 60;             //Moderate fan speed
    InfoMessage = "Purge-Cool";
    glow_time = 15;             //Glow Plug for 15 seconds
    water_pump_speed = 100;     //Run water pump to aid cooling
  } else {
    fan_speed = 40;             //Low Fan
    InfoMessage = "Final-Cool";
    glow_time = 0;              //Glow Plug off
    water_pump_speed = 100;     //Run water pump to aid cooling
  }
  //Wait until exhaust cools below water temp, or 60 seconds - whichever happens first
  if ((seconds>60) || (get_flame_temp() < water_temp)) {  
    burn_mode = 0;
    InfoMessage = "Off";
    glow_time = 0;
 }
  
}

//*******************************************************************************************
// Is the burn chamber cool enough to start?
//*******************************************************************************************
bool Chamber_Cool(){

  return (!Is_Flame() || (get_flame_temp() < (water_temp+20)));
}


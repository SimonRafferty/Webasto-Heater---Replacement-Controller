//*******************************************************************************************
// Finite State Machine to start, run & stop heater
// burn_mode: 0=Off, 1=Starting, 2=Running, 3=Shutting down
//*******************************************************************************************
void Heater() { // this will handle the combustion
static int Last_Mode = -1;

  if (!Heater_fail) { // if everything's going fine
    Heater_Start_Stop();
  } else { // if there has been a major failure, stop everything
    burn = false;
    if (burn_mode == 1 || burn_mode == 2) {
      burn_mode = 3; //Controlled shutdown
      seconds = 0;
    }
  }

  //Remember if we've just entered a state (used to do things once on entry)
  bool Entered = (burn_mode != Last_Mode);
  Last_Mode = burn_mode;

  //Run the heater
  switch (burn_mode) {
    case 0: { // everything is turned off in this mode
      Heater_FSM_Off();
    } break;
    case 1: { // Start Heater
      Heater_FSM_Startup(Entered);
    } break;
    case 2: { // Heater running - control fueling dynamically
      Heater_FSM_Running(Entered);
    } break;
    case 3: { // Shut down heater
      Heater_FSM_Shutdown(Entered);
    } break;
  }

  //Safety checks that apply whenever the burner might be alight
  if (burn_mode == 1 || burn_mode == 2) {
    //Water sensor failure - can't control the temperature
    if (water_sensor_fault) {
      Heater_Failure("The water temperature sensor failed while the heater was running. Check its wiring and plug.");
    }
    //Exhaust sensor failure - can't see the flame
    else if (exhaust_sensor_fault) {
      Heater_Failure("The exhaust temperature sensor failed while the heater was running. Check its wiring and plug.");
    }
    //Exhaust overheat
    else if (exhaust_temp > exhaust_overheat) {
      Heater_Failure("The exhaust got too hot (over " + String((int)exhaust_overheat) + "C), so the heater shut down. Check the exhaust and air intake are not blocked.");
    }
    //Battery out of range
    else if (battery_lockout) {
      Fuel_Rate(pump_size, 0.0);
      burn = false;
      burn_mode = 3;
      seconds = 0;
      InfoMessage = "Battery lockout";
    }
  }

  //Control peripherals
  burn_fan();
  glow_plug();
}

//*******************************************************************************************
// Major failure - stop the heater and latch an error (cleared from the web page or a reboot)
//*******************************************************************************************
void Heater_Failure(String Why) {
  Fuel_Rate(pump_size, 0.0);        //Turn off fueling
  InfoMessage = "Fault - shut down";
  Heater_fail = true;               //Heater will not start until errors cleared
  burn = false;
  burn_mode = 3;
  seconds = 0;
  AddError(Why);
}

//*******************************************************************************************
// Transition from heater off to on or on to off
//*******************************************************************************************
void Heater_Start_Stop() {
  if ((Ignition_Failures >= max_ignition_attempts) && (burn_mode == 0)) {
    // there were too many attempts to start the fire and all failed
    Heater_fail = true;
    burn = false;
    AddError("The heater failed to light " + String(Ignition_Failures) + " times. Check there is fuel in the tank and the fuel line is clear. The overheat fuse may also have blown.");
    return;
  }

  if ((burn_mode == 0) && burn) {  //Heater has been switched on
    // initiate the start sequence
    burn_mode = 1;
    seconds = 0;
    glow_time = 0;
  }

  if ((burn_mode == 1 || burn_mode == 2) && !burn) { //Heater has been switched off
    burn_mode = 3;
    seconds = 0;
  }
}

//*******************************************************************************************
// Heater is OFF, shut down everything
//*******************************************************************************************
void Heater_FSM_Off() { // everything is turned off in this mode
  if (!Web_purge_ON) Fuel_Rate(pump_size, 0.0); //Unless fuel purge is active, switch off fuel pump
  fan_speed = 0;
  glow_time = 0;
  Fueling_Level = 0;
  //Leave the water pump as is.  It will run until cool
}

//*******************************************************************************************
// Heater is Starting up
//*******************************************************************************************
void Heater_FSM_Startup(bool Entered) {
static unsigned long Startup_fan_timer = 0;
static bool Glow_Started = false;
static unsigned long Cooling_Since = 0;

  if (Entered) {
    Glow_Started = false;
    Fueling_Level = 0;
    Cooling_Since = millis();
  }

  //If burn chamber is hot.  Allow to cool before starting
  if (seconds < 5) {
    if (!Chamber_Cool())
    {
      InfoMessage = "Initial Cooling";
      fan_speed = 60;
      Fuel_Rate(pump_size, 0.0);
      seconds = 0;
      //If it never cools, the exhaust sensor (or its calibration) is probably wrong
      if (millis() - Cooling_Since > 300000UL) {
        Heater_Failure("The burner would not cool down enough to start. The exhaust temperature sensor may be faulty.");
      }
      return;
    }
  }

  //**** 0 to 5 Seconds - Clear Chamber ****
  if (seconds > 0 && seconds < 5) {
    fan_speed = 70; //prime_fan_speed * 2;
    if (!Glow_Started) {
      glow_time = glow_start_seconds;  //30 Sec was not long enough in cold weather
      Glow_Started = true;
    }
    Fuel_Rate(pump_size, 0.0);
    exhaust_at_ignition = exhaust_temp;  //Reference for detecting the flame
    InfoMessage = "Clearing Chamber";
  }

  //**** 6 to 9 Seconds - Prime Fuel ****
  if (seconds >= 6 && seconds <= 9)
  {
    fan_speed = prime_fan_speed;
    Fuel_Rate(pump_size, prime_rate(water_temp));
    InfoMessage = "Prime";
  }

  //**** 9 to 17 Seconds - Stop Fuel ****
  if (seconds > 9 && seconds <= 17) {
    Fuel_Rate(pump_size, 0.0);
    InfoMessage = "Soak";
  }

  //**** 17 Seconds - Begin firing up ****
  if (seconds > 17) {
    //Cold Start Enrichment - makes starting easier in cold weather
    if (water_temp < start_fuel_Threshold) {
      Fuel_Rate(pump_size, start_fuel_Cold);
    } else {
      Fuel_Rate(pump_size, start_fuel_Warm);
    }
    InfoMessage = "Firing Up";

    //Gradually increase fan speed
    if (fan_speed < start_fan_speed)
    {
      if (millis() - Startup_fan_timer >= 333)
      {
        fan_speed += 1;
        Startup_fan_timer = millis();
      }
    } else {
      fan_speed = start_fan_speed; //Startup fan speed reached, hold at that for a while
    }
  }

  //**** Glow plug off - Do we have a flame? ****
  //Either the exhaust is above flame_threshold or it has risen flame_rise above the pre-ignition value
  if ((seconds >= flame_check_from) && Flame_Started()) {
    burn_mode = 2;            //Change to the Running state
    seconds = 0;              //Reset clock
    glow_time = 0;            //Make sure Glow Plug is Off (probably not needed)
    Ignition_Failures = 0;    //Successful start, clear failure count
    InfoMessage = "Started";
    return;
  }

  //**** ignition_timeout and no flame! - Startup has failed, go to Shutdown mode ****
  if (seconds > ignition_timeout) {
    // the fire sequence didn't work, give it another try (control() restarts it after the purge)
    burn_mode = 3;          //Change to the Shutdown state
    seconds = 0;            //Reset clock
    Ignition_Failures++;    //Increase failure count
    InfoMessage = "No flame - Restarting";
    Serial.print("Ignition failure "); Serial.println(Ignition_Failures);
  }
}

//*******************************************************************************************
// Heater is Running - control Fueling
//*******************************************************************************************
void Heater_FSM_Running(bool Entered) {
static unsigned long Startup_fan_timer = 0;

  if (Entered) Startup_fan_timer = millis();

  //Gradually increase Fuel & Combustion fan until heater reaches it's minimum operating temperature, then throttle automatically
  if (water_temp <= (heater_min - 15)) {
    InfoMessage = "Increasing Burn";
    Fueling_Level = 4;
    if (millis() - Startup_fan_timer >= 333) { //Slowly increase fueling
      fan_speed += (throttling_high_fan - 35.00) / full_power_increment_time / 3;
      if (fan_speed > throttling_high_fan) fan_speed = throttling_high_fan;
      float f = fuel_need + (throttling_high_fuel - 1.00) / full_power_increment_time / 3;
      if (f > throttling_high_fuel) f = throttling_high_fuel;
      Fuel_Rate(pump_size, f);
      Startup_fan_timer = millis();
    }
  } else {
    //Run automatic throttling
    running_ratio();  //Sets fuel_need variable
    Fuel_Rate(pump_size, fuel_need);  //Execute change
  }

  //Check for an overheat.  Shut down; control() restarts it once the water drops below heater_min
  if (water_temp > water_overheat) {
    fan_speed = 80;                   //Set a relatively high fan speed
    Fuel_Rate(pump_size, 0.0);        //Turn off fueling
    InfoMessage = "Overheating";
    burn = false;
    burn_mode = 3;                    //Change mode to controlled shutdown
    seconds = 0;                      //Reset Clock
    return;
  }

  //If, after 60s running, the exhaust temp drops below the water temp (or the flame threshold), the flame must have died.
  if ((!Is_Flame() || exhaust_temp < water_temp) && seconds >= 60) {
    Fuel_Rate(pump_size, 0.0);        //Turn off fueling
    InfoMessage = "Flame out";
    Ignition_Failures++;
    Serial.println("Flame out");
    burn_mode = 3;                    //Purge, then control() restarts it
    seconds = 0;                      //Reset Clock
  }
}

//*******************************************************************************************
// Shut down heater
//*******************************************************************************************
void Heater_FSM_Shutdown(bool Entered) {
 // snuff out the fire, with just a little air to avoid fumes and backfire

  Fuel_Rate(pump_size, 0.0);
  Fueling_Level = 0;
  if (Entered) {
    glow_time = 15;             //Glow Plug for 15 seconds to burn off residue (once - V4 re-armed it every loop)
  }

  //Purge-cool for at least 30 sec, and until there's no flame
  if (seconds <= 30 || Is_Flame()) {
    fan_speed = 60;             //Moderate fan speed
    if (InfoMessage.indexOf("Purge") < 0) InfoMessage = "Purge-Cool";
  } else {
    fan_speed = 40;             //Low Fan
    InfoMessage = "Final-Cool";
    glow_time = 0;              //Glow Plug off
  }
  water_pump_speed = 100;       //Run water pump to aid cooling
  water_pump();

  //Wait until exhaust cools close to the water temp, or 180 seconds - whichever happens first (60s minimum)
  if ((seconds > 60 && exhaust_temp < water_temp + 20) || seconds > 180) {
    burn_mode = 0;
    burn = false;               //control() decides whether to start again
    InfoMessage = "Off";
    glow_time = 0;
    fan_speed = 0;
  }
}

//*******************************************************************************************
// Is the burn chamber cool enough to start?
//*******************************************************************************************
bool Chamber_Cool() {
  return (!Is_Flame() && (exhaust_temp < (water_temp + 20)));
}

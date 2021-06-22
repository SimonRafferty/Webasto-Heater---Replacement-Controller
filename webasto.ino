void webasto() { // this will handle the combustion
static unsigned long timer;
static unsigned long fan_timer;
static String ShutdownReason = "";
static float temp_init;
static int cooled_off = 0;
bool Ret_Val = false;


  if (millis() < timer) {
    timer = millis();
  }

  if (millis() > timer + 1000) { // every seconds, run this
    timer = millis();
    seconds ++; // increment the seconds counter
    logging(Ignition_Failures, temp_init, seconds);
  }
  if (!webasto_fail) { // if everything's going fine

    if ((burn_mode == 0) && burn) {  //Heater switched on
      // initiate the start sequence and retry
      burn_mode = 1;
      seconds = 0;
      Ignition_Failures = 0;
      Incidents = 0;
      glow_time = 0;
      //temp_init = exhaust_temp; // store the exhaust temperature before trying to start the fire


      cooled_off = 1;
    }

    if ((burn_mode == 1 || burn_mode == 2) && !burn) { //Heater switched off
      ShutdownReason = "Sw OFF ";
      burn_mode = 3;
      seconds = 0;
      Ignition_Failures = 0;
    }

/*
    if ((Ignition_Failures > 3) && (burn_mode == 1)) { // if there was more than 3 attempts to start fire but all failed
      webasto_fail = 1;
      ShutdownReason = "i_fail > 3 ";
      burn_mode = 3;
      seconds = 0;
      burn = 0;
    }
*/
    if ((exhaust_temp > 250) && (burn_mode != 3)) { // if overheating. Temp sometimes spikes as heater shuts down. This is not a fail.
      message = "Exhaust Overheat";
      Serial.println("############## OVERHEAT ####################");
      ShutdownReason = "Overheat ";
      //webasto_fail = 1;
      burn_mode = 3;
      burn = 0;
     }

  } else { // if there has been a major failure, stop everything
    burn = 0;
    burn_mode = 3;
    Ignition_Failures = 0;
    message = "Major Fail";
    if(seconds > 600) webasto_fail = 0; //Reset failure after 10 mins switched off
  }

  
  switch (burn_mode) {
    case 0: { // everything is turned off in this mode
        fan_speed = 0;
        fuel_need = 0;
        glow_time = 0;
        lean_burn = 0;
        Ignition_Failures = 0;
      } break;

    case 1: { // the fire starting sequence
        if(webasto_fail)
          fuel_need = 0;
          
          
        if(exhaust_temp > 150 && (cooled_off == 0 || seconds < 5))
        {
          message = "Cooling < 150";
          fan_speed = 80;
          fuel_need = 0;
          seconds = 0;
          cooled_off = 0;
        }
        else if(exhaust_temp <= 150)
          cooled_off = 1;
        
        if(seconds > 0 && seconds < 5)
        {
          Ret_Val = Flame_Sensor(true); //Initialise Flame Sensor
          fan_speed = 70; //prime_fan_speed * 2;
          temp_init = exhaust_temp; // store the exhaust temperature before trying to start the fire
          glow_time = 105;
          fuel_need = 0;
          message = "Clearing Chamber";
        }


        if(seconds >= 6 && seconds <= 9)
        {
          fan_speed = 15;
          fuel_need = prime_ratio(exhaust_temp);
          message = "Prime";
        }

        if(seconds > 9 && seconds <= 11)
          fuel_need = 0;

        if (seconds >=40 && seconds <= 41)
        {
          temp_init = exhaust_temp;
          fan_timer = millis();
        }




        if (seconds > 17) { // the glow plug has just been turn of (7+12=19)
          fuel_need = start_fuel;
          message = "Firing Up";

          if(fan_speed < start_fan_speed)
          {
            if(millis() - fan_timer >= 333)
            {
              fan_speed += 1;
              fan_timer = millis();
            }
          }
          else
            fan_speed = start_fan_speed; // get some more air and restart pumping fuel slowly        
        }

        if (((exhaust_temp - temp_init) > 10) && (seconds >=50)) { // exhaust temp raised a bit meaning fire has started //Debug this value of 0.5c is way too low maybe change it to 5c
          burn_mode = 2; // go to main burning mode and initialize variables
          seconds = 0;
          glow_time = 0;
          Ignition_Failures = 0;
          Incidents = 0;
          temp_init = exhaust_temp; 
          fan_timer = millis();
          message = "Started";

        }

        if ((seconds > 120) && (burn_mode == 1)) {
          // the fire sequence didn't work, give it an other try
          burn_mode = 3;
          seconds = 0;
          Ignition_Failures ++;
          Incidents ++;
          glow_time = 5;
          cooled_off = 0;
          message = "Restarting";
        }
        if(!debug_glow_plug_on) {
          Ret_Val = Flame_Sensor(false);
        }
        
        if ((exhaust_temp < (temp_init-5.0)) && (seconds >= 60) && (burn_mode == 1)) { // if flame died during burn
          burn_mode = 3;
          seconds = 0;
          Ignition_Failures ++;
          Incidents ++;
          cooled_off = 1;
          message = "Flameout";
        }

      } break;

    case 2: { // a really simple flame managment here, just get at full power

        //if ((exhaust_temp < 400 || water_temp < 50) && !webasto_fail) {
        if ( water_temp < water_overheat && !webasto_fail) {
         
          //Slowly speed up
          //if (fan_speed<(final_fan_speed-1)) { 
          if(exhaust_temp <= 40) {
          //if(water_temp <= 40) {

            if(millis() - fan_timer >= 333)
            {
              message = "Increasing Burn";
              fan_speed += (throttling_high_fan-35.00)/full_power_increment_time/3;
              if(fan_speed>throttling_high_fan) fan_speed=throttling_high_fan;
              fuel_need += (throttling_high_fuel-1.00)/full_power_increment_time/3;
              if(fuel_need>throttling_high_fuel) fuel_need=throttling_high_fuel;
              
              fan_timer = millis();
            }
          } else {
            //Run automatic throttling 
            running_ratio(exhaust_temp);                     
          }
        }

        if(webasto_fail || water_temp > water_overheat)
        {
          //fan_speed = 100;
          EX_Mute = false;
          fan_speed = 80;
          fuel_need = 0;
          cooled_off = 1;
          message = "Overheating";
          burn = 0;
          burn_mode = 3;
          seconds = 0;
        }
        
        if(water_temp < 0)  //Probably a thermistor failure
        {
          EX_Mute = false;
          fan_speed = 80;
          fuel_need = 0;
          cooled_off = 1;
          message = "Therm Fail";
          burn = 0;
          burn_mode = 3;
          seconds = 0;
        }

        if (exhaust_temp > 0 && exhaust_temp < 40 && exhaust_temp < exhaust_temp_sec[9]-5 && seconds >= 90) { // if flame died
          burn = 0;
          seconds = 0;
          burn_mode = 3;
          EX_Mute = false;
          message = "Running Flameout";
        }
        
        if (exhaust_temp  < water_temp && seconds >= 240) { // flame has died
          burn = 0;
          seconds = 0;
          burn_mode = 3;
          EX_Mute = false;
          message = "Flame Died";
        }
        
        if (seconds >= 3600) { // Timeout
          //Most often the flame seems to die around an hour after startup.  This just ensures it never gets there
          burn = 0;
          seconds = 0;
          burn_mode = 3;
          EX_Mute = false;
          message = "Timeout";
        }
 
      } break;

    case 3: { // snuff out the fire, with just a little air to avoid fumes and backfire

        fuel_need = 0;
        EX_Mute = false;
        message = "Shut Down" + ShutdownReason;
        if (seconds > 60 || exhaust_temp < 100) {
          fan_speed = 40;
          message = "Final-Cool";
          glow_time = 0;
          water_pump_speed = 100;

        } else {
          fan_speed = 60;
          message = "Purge-Cool";
          glow_time = 60;
          water_pump_speed = 100;          
        }
        if (seconds > 120) {
          burn_mode = 0;
          message = "Off";
          glow_time = 0;
          ShutdownReason = "";
          
          //Clear the sensor smoothing variables.  Sometimes, during shutdown they are poisoned by outlying values
          for ( int i = 9; i >= 0 ; i-- ) { // Reset the exhaust temperature history
            exhaust_temp_sec[i] = temp_init; 
          }
    
          //Reset Smoothing arrays
          for (int j=0; j<filterSamples; j++){    
            ExhaustSmoothArray[j] = 0.0;
            WaterSmoothArray[j] = 0.0;        
          }
        }
      } break;
  }
  fuel_pump();
  burn_fan();
  glow_plug();
}

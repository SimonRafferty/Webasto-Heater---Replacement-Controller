//*******************************************************************************************
// This is the main running control loop
//*******************************************************************************************
void control() {
static bool Was_Requested = false;

  Battery_Check();
  Run_To_Temp_Check();

  bool requested = heater_on();

  //A fresh ON request (off -> on) gives the heater a fresh set of ignition attempts
  if (requested && !Was_Requested) {
    Ignition_Failures = 0;
  }
  Was_Requested = requested;

  if (requested) {
    if ((burn_mode < 3) && (Ignition_Failures < max_ignition_attempts)) //Don't start if shutting down or it has failed to start
    {
      if (burn_mode != 0 || water_temp < heater_min) //Only (re)start when water temp below minimum to stop short-cycling
      {
        burn = true;  //Heater ON/OFF - Start controlled startup process
      }
    }
  } else {
    if (burn_mode > 0 && burn_mode < 3)
    {
      burn = false;    //Heater ON/OFF - Start controlled shutdown process
    }
  }

  //Keep the water pump running until heater has cooled a little
  //If you just switch off the heater, the water can boil!  Much badness!
  if (burn_mode == 0) {
    if (!water_sensor_fault && water_temp > heater_target) {
      water_pump_speed = 100;  //Keep pump running if water hot
    } else if (requested && !water_sensor_fault) {
      water_pump_speed = 100;  //Waiting to restart - keep the water circulating
    } else {
      water_pump_speed = 0;
    }
  } else {
    water_pump_speed = 100;  //Keep pump running while heater running
  }

  //Run to Temp: while the convection fan is on, keep the water circulating through the heater matrix
  if (RunToTemp && conv_fan_on && !water_sensor_fault) {
    water_pump_speed = 100;
  }

  water_pump(); // calls the water_pump function to update speed
}

//*******************************************************************************************
// Should the heater be on or off?
// Either the hardware HEAT input (X1-1) OR the web page can switch the heater on.
// The web OFF button only cancels a web ON - it can't override the hardware input.
//*******************************************************************************************
bool heater_on() {
  //Check for problems
  if (ErrorMessage != "" || Heater_fail) {
    Web_heater_ON = false; //Switch off on web
    return false; //Do not allow heater to start if there's a hardware problem
  }

  //See if fuel purge is active
  if (Web_purge_ON) {
    return false; //Do not allow heater to start if priming fuel pump
  }

  //No 12V, or battery out of range
  if (bench_mode || battery_lockout) {
    return false;
  }

  //Run to Temp has switched it off - stays off until switched on again
  if (Room_Target_Reached) {
    return false;
  }

  return (Pushbutton() || Web_heater_ON);
}

//*******************************************************************************************
// Run to Temp: if the switch on the web page is ON and the room exceeds the target,
// shut the heater down and switch it off on the web page.  It stays off until it's switched
// on again (web ON, or the HEAT input going off then on).
// Does nothing if there's no room sensor.
//*******************************************************************************************
void Run_To_Temp_Check() {
static bool Last_Web = false;
static bool Last_Button = false;

  bool web = Web_heater_ON;
  bool button = Pushbutton();

  //A new ON from either source clears a previous Run to Temp switch-off
  if ((web && !Last_Web) || (button && !Last_Button) || !RunToTemp) {
    Room_Target_Reached = false;
  }
  Last_Web = web;
  Last_Button = button;

  if (RunToTemp && !Room_Target_Reached && room_sensor_fitted && !room_sensor_fault && !isnan(room_temp) &&
      (web || button) && room_temp > room_temp_target) {
    Room_Target_Reached = true;
    Web_heater_ON = false;     //Switch off on web
    Last_Web = false;
    InfoMessage = "Room at target - Off";
    Serial.print("Run to Temp: room "); Serial.print(room_temp, 1);
    Serial.print("C > target "); Serial.println(room_temp_target, 1);
  }
}

//*******************************************************************************************
// Is the hardware on/off input active?  (HEAT_N is active LOW, debounced)
//*******************************************************************************************
bool Pushbutton() {
static bool state = false;
static bool last_raw = false;
static unsigned long changed = 0;

  bool raw = (digitalRead(ON_OFF_pin) == LOW);
  if (raw != last_raw) {
    last_raw = raw;
    changed = millis();
  }
  if (millis() - changed >= 50) state = raw;  //Must be stable for 50ms - it's a vehicle signal
  return state;
}

//*******************************************************************************************
// Battery under / over voltage lockout (non-latching)
// The voltage must be out of range for low_volt_seconds to filter glow plug & cranking dips
//*******************************************************************************************
void Battery_Check() {
static unsigned long in_range_at = 0;

  if (bench_mode) {
    battery_lockout = false;
    in_range_at = millis();
    return;
  }

  bool in_range = (battery_voltage >= low_volt_cutoff) && (battery_voltage <= high_volt_cutoff);
  if (in_range) {
    in_range_at = millis();
    if (battery_lockout) {
      //Recovered - add a little hysteresis before allowing a restart
      if (battery_voltage >= low_volt_cutoff + 0.5 && battery_voltage <= high_volt_cutoff - 0.5) {
        battery_lockout = false;
        Serial.println("Battery back in range");
      }
    }
  } else if (millis() - in_range_at > (unsigned long)low_volt_seconds * 1000UL) {
    if (!battery_lockout) {
      battery_lockout = true;
      InfoMessage = (battery_voltage < low_volt_cutoff) ? "Battery Low" : "Battery High";
      Serial.print("Battery lockout: "); Serial.println(battery_voltage);
    }
  }
}

//*******************************************************************************************
// After an unexpected reset (brown-out while cranking, watchdog...) the burner may still be hot
// or even alight.  Run the shutdown (purge-cool) sequence before anything else.
//*******************************************************************************************
void Hot_Boot_Check() {
  if (!exhaust_sensor_fault && !bench_mode &&
      (Is_Flame() || exhaust_temp > water_temp + 20)) {
    Serial.println("Hot on boot - running purge-cool");
    InfoMessage = "Hot on boot - Purge-Cool";
    burn = false;
    burn_mode = 3;
    seconds = 0;
  }
}

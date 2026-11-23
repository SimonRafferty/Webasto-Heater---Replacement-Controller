//*******************************************************************************************
// Set the air/fuel ratio when the heater is running
// Fueling_Level: 0=Off, 1=Idle, 2=Low, 3=Steady, 4=High
//*******************************************************************************************
void running_ratio() {
  static unsigned long fuel_change_timer = 0;
  static unsigned long fan_change_timer = 0;
  float fuel_target;
  float fan_target;

  if (!Is_Flame()) return;   //Not hot enough to start adjusting

  if (water_temp < heater_min) {
    fuel_target = throttling_high_fuel;
    fan_target = throttling_high_fan;
    Fueling_Level = 4;
  } else if (water_temp < heater_target) {
    fuel_target = throttling_steady_fuel;
    fan_target = throttling_steady_fan;
    Fueling_Level = 3;
  } else if (water_temp < water_warning) {
    fuel_target = throttling_low_fuel;
    fan_target = throttling_low_fan;
    Fueling_Level = 2;
  } else {
    //Above water_warning (water_overheat is handled in Heater_FSM_Running)
    fuel_target = throttling_idle_fuel;
    fan_target = throttling_idle_fan;
    Fueling_Level = 1;
  }

  //Work in 1/100 Hz steps so rounding can't make it hunt
  long fuel_current = lround(fuel_need * 100);
  long fuel_goal = lround(fuel_target * 100);

  //Adjust Fuel
  if (fuel_current < fuel_goal) {
    if (millis() - fuel_change_timer >= 1500) {  //Increase fuel slowly
      fuel_need = (fuel_current + 1) / 100.0;
      InfoMessage = "Inc Fuel";
      fuel_change_timer = millis();
    }
  } else if (fuel_current > fuel_goal) {
    if (millis() - fuel_change_timer >= 150) {   //Decrease fuel less slowly
      fuel_need = (fuel_current - 1) / 100.0;
      InfoMessage = "Dec Fuel";
      fuel_change_timer = millis();
    }
  } else {
    InfoMessage = "Running";
  }

  //Adjust Fan (0.5% steps)
  if (fan_speed < fan_target - 0.25) {
    if (millis() - fan_change_timer >= 1500) {   //Increase fan slowly
      fan_speed += 0.5;
      fan_change_timer = millis();
    }
  } else if (fan_speed > fan_target + 0.25) {
    if (millis() - fan_change_timer >= 150) {    //Decrease fan less slowly
      fan_speed -= 0.5;
      fan_change_timer = millis();
    }
  }
}

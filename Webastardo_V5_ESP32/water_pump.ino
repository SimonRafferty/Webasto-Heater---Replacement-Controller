//*******************************************************************************************
// Set speed of the water pump.
// Water pumps tend not to start at less than 100%, so it's run at 0% or 100%
//*******************************************************************************************
void water_pump() {
  if (water_pump_speed > 0) {
    Set_PWM_Percent(water_pump_pin, 100);  //Always run pump at 0% or 100%
  } else {
    Set_PWM_Percent(water_pump_pin, 0);
  }
}

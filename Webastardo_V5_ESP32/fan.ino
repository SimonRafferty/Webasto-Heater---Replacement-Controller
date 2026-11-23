//*******************************************************************************************
// Set the Combustion fan speed
//*******************************************************************************************
void burn_fan() {
  // the webasto fan runs on 10v so the duty is limited to burn_fan_max_duty at 100%
  //(Scaling by battery voltage was tried in V4 but made the fan speed even more variable)
  float duty = mapf(fan_speed, 0, 100, 0, burn_fan_max_duty);
  if (fan_speed <= 0) duty = 0;
  Set_PWM_Percent(burn_fan_pin, duty);
}

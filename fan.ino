//*******************************************************************************************
// Set the Combustion fan speed
//*******************************************************************************************
void burn_fan() {
int Adjusted_Fan = 0;
  // the webasto fan runs on 10v so we need to adjust if battery is that into account

  //The following was a good idea but makes the fan speed even more variable!
/*
  if(battery_voltage > 10) {
    int Max_Fan = 10.0 / battery_voltage * 255.0;
    if(Max_Fan > 255) Max_Fan = 255; //Shouldn't happen but just in case
    Adjusted_Fan = mapf(fan_speed, 0, 100, 0, Max_Fan); // Assuming a 13V Input, 769 gives 10V
  }
*/
  Adjusted_Fan = mapf(fan_speed, 0, 100, 0, 169);
  analogWrite(burn_fan_pin, Adjusted_Fan);
}

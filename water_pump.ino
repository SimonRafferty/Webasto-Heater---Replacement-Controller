//*******************************************************************************************
// Set speed of the water pump. 
// Water pumps tend not to start at less than 100%, decrease if needed after
//*******************************************************************************************
void water_pump() {
  //Maps speed
  int water_pump_255 = mapf(water_pump_speed, 0, 100, 0, 255);

  if(water_pump_255>0) water_pump_255 = 255; //Always run pump at 0% or 100%
  analogWrite(water_pump_pin, water_pump_255);
}

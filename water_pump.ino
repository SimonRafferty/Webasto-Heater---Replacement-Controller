void water_pump() {
  //Maps speed
  int percent_map = mapf(water_pump_speed, 0, 100, 0, 255);

  if(percent_map>0) percent_map = 255; //Always run pump at 0% or 100%
  debug_water_percent_map = percent_map;
  analogWrite(water_pump_pin, percent_map);
  //ledcWrite(water_channel, percent_map); 
}

void burn_fan() {
  // the webasto fan runs on 10v so we need to that into account
  int percent_map = mapf(fan_speed, 0, 100, 0, 239); // Assuming a 13V Input, 769 gives 10V

  analogWrite(burn_fan_pin, percent_map);
  //ledcWrite(air_channel, percent_map);
  //pwm.analogWrite(burn_fan_pin, percent_map);
}

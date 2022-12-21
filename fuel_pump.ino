void fuel_pump() {
  static unsigned long next_pulse_timer;
  static unsigned long pulse_started_off_at;
  
  if(fuel_need == 0)
  {
    next_pulse_timer = millis()+50;
    pulse_started_off_at = millis()-50;
  }


  int pulse_length = 9;

  if(pump_size == 0)
    pulse_length = 2;  //This is a special case for James's silent fuel pump jamesbrowningsmith@yahoo.co.uk
  else if(pump_size == 22)
    pulse_length = 9;
  else if(pump_size == 33)
    pulse_length = 18;
  else if(pump_size == 60)
    pulse_length = 36;
    
  double pump_division = 60.00/pump_size;

  delayed_period = 1000/fuel_need/pump_division; //Changing this to HZ fuel need, adding a pump volume switch also to adjust for different pumps

  if(pulse_started_off_at<=millis())
    digitalWrite(fuel_pump_pin, LOW);
  
  if(next_pulse_timer<=millis())
  {
      next_pulse_timer = delayed_period + pulse_length + millis();
      digitalWrite(fuel_pump_pin, HIGH);
      pulse_started_off_at = millis()+pulse_length;
  }
      
}

void control() {
static unsigned long timer;
bool Ret_Val = false;

  //Latching switch connected to heating timer.
  //When On, attempt to regulate temperature to target
  if (analogRead(push_pin)>1000) {
//  if (analogRead(push_pin)>250) {
//    if (digitalRead(push_pin)) {

    if(burn_mode<3) //Don't start if shutting down
    {
      if(water_temp*100 < (heater_min)*100)
      {
        burn = 1;
        heater_on = 1;
      }
      
      
    }
    water_pump_speed = 100;
    
    //webasto_fail = 0;
  } else {
    heater_on = 0;
    if(burn_mode>0 && burn_mode<3)
    {
      burn = 0;  //Shut down nicely
    }
    if(burn_mode==0 && seconds<600) {
      fan_speed = 40;
      burn_fan();
      delay(1000);
    }  
    if(burn_mode==0 && seconds>=600) {
      fan_speed = 0;
      burn_fan();
    }  
    if(burn_mode==0) water_pump_speed = 0; //Switch off pump if heater has shut down
    if(water_temp > heater_target) {
      water_pump_speed = 100;  //Keep pump running if water hot
    } else {
      water_pump_speed = 0;
    }
  }
  
  
  water_pump(); // calls the water_pump function

}

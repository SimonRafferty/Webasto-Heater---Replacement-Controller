void control() {

  //Latching switch connected to heating timer.
  //When On, attempt to regulate temperature to target
  //if (analogRead(push_pin)>1000) {
  if (BlynkHeaterOn==1) { //Disable for now
//  if (analogRead(push_pin)>250) {
//    if (digitalRead(push_pin)) {

    if ((burn_mode<3) && (Start_Failures < 3)) //Don't start if shutting down or it has failed to start 3 times
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

    //Run the fan for a couple on minutes to cool down
    if(burn_mode==0 && seconds<10) {
      fan_speed = 40;
      burn_fan();
      delay(1000);
    }  
    if(burn_mode==0 && seconds>=10) {
      fan_speed = 0;
      burn_fan();
      Start_Failures = 0;  //Cycle the Ignition line to reset start failures
    }  
    //if(burn_mode==0) water_pump_speed = 0; //Switch off pump if heater has shut down
    if(water_temp > heater_target) {
      water_pump_speed = 100;  //Keep pump running if water hot
    } else {
      water_pump_speed = 0;
    }
  }
  
  
  water_pump(); // calls the water_pump function

}

#ifdef BLYNK_ENABLE
  BLYNK_WRITE(V50)
  {
    //When the heater is switched on via the Blynk Console - BlynkHeaterOn changes from 0 to 1
    BlynkHeaterOn = param.asInt(); // assigning incoming value from pin V1 to a variable
  }
#endif

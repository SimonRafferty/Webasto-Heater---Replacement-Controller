void control() {

  //Latching switch connected to heating timer.
  //When On, attempt to regulate temperature to target
  if((room_temp>=room_temp_set) && (HeaterOn==1)){
    Run_Heater.setState(false); //Might need to be set to "off" or something like that

    HeaterOn = 0;
  }
  if(water_temp*100 > (heater_min - 10)*100){
    water_pump_speed = 100;
  } else {
    water_pump_speed = 0;
  }

  if ((RemoteHeaterOn==1) && (room_temp<room_temp_set)) { 
  //if (analogRead(push_pin)>250) {

    if ((burn_mode<3) && (Start_Failures < 3)) //Don't start if shutting down or it has failed to start 3 times
    {
      if(water_temp*100 < (heater_min)*100) 
      {
        burn = 1;
        HeaterOn = 1;
        restart_timer = 0;
      }
      
      
    }
    water_pump_speed = 100;
    
    //Heater_fail = 0;
  } else {
    HeaterOn = 0;
    //Run_Heater.setState(0,true); //Share heater off state with HA


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
/*
    if(water_temp > heater_target) {
      water_pump_speed = 100;  //Keep pump running if water hot
    } else {
      water_pump_speed = 0;
    }
*/
  }
  
  
  water_pump(); // calls the water_pump function

}

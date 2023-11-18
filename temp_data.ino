void temp_data() { // keeps the temp variables updated
  static unsigned long timer;
  
  if (millis() < timer) {
    timer = millis();
  }
  // call the get_temp function and smoothen the result
  if(glow_time==0){
    water_temp = get_wabasto_temp(water_temp_pin);
    Flame_temp_value = get_wabasto_temp(Room_Temp_pin);
    
    if (millis() > timer + 1000) { // every sec
      timer = millis();
      //Array of historic exhaust temperatures  
      for ( int i = 9; i >= 1 ; i-- ) { // updating the exhaust temperature history
        Flame_temp_sec[i] = Flame_temp_sec[i - 1]; // shift array values, deleting the older one
      }
      Flame_temp_sec[0] = Flame_temp_value; // add new temp value
    } 
  }
}

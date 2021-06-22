void temp_data() { // keeps the temp variables updated
  static unsigned long timer;
  int temp_Water_temp;
  
  if (millis() < timer) {
    timer = millis();
  }
  // call the get_temp function and smoothen the result
  //water_temp = (9 * water_temp + get_wabasto_temp(water_temp_pin)) / 10; 
  water_temp = get_wabasto_temp(water_temp_pin);
  //exhaust_temp = (9 * exhaust_temp + get_wabasto_temp(exhaust_temp_pin)) / 10;
  exhaust_temp = get_wabasto_temp(exhaust_temp_pin);
  //if(exhaust_temp<0) exhaust_temp=0;
  
  if (millis() > timer + 1000) { // every sec
    timer = millis();
    //Array of historic exhaust temperatures  
    for ( int i = 9; i >= 1 ; i-- ) { // updating the exhaust temperature history
      exhaust_temp_sec[i] = exhaust_temp_sec[i - 1]; // shift array values, deleting the older one
    }
    exhaust_temp_sec[0] = exhaust_temp; // add new temp value
 
    //Array of historic water temperatures for web server graph
    //for ( int i = 179; i >= 1 ; i-- ) { // updating the exhaust temperature history
    //  water_temp_sec[i] = water_temp_sec[i - 1]; // shift array values, deleting the older one
    //}
    //temp_Water_temp = int(water_temp);
    //if(temp_Water_temp>130) temp_Water_temp=130;
    //if(temp_Water_temp<0) temp_Water_temp=0;
    //water_temp_sec[0] = temp_Water_temp; // add new temp value


  
  
  }
}

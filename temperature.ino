//*******************************************************************************************
// Set globals from readings
//*******************************************************************************************
void get_temperatures(){
  room_temp = get_room_temp();
  water_temp = get_water_temp();
  //get_flame_temp() for Exhaust is called elsewhere.  Doesn't need to be called here
}




//*******************************************************************************************
// Read the Room Tepm
//*******************************************************************************************
double get_room_temp(){
  double c1 = th_room_temp.readCelsius();
  //Comment out block if Thermocouple(s) not in use
//  if (isnan(c1)) {
//    AddError("Therm 1: NC , ");
//    return 9999; //Unlikely value to cause shutdown
//  }
  return digitalSmooth(c1, RoomSmoothArray);
}
//*******************************************************************************************
// Read the Water Tepm
//*******************************************************************************************
double get_water_temp(){
  double c1 = th_water_temp.readCelsius();
  //Comment out block if Thermocouple(s) not in use
//  if (isnan(c1)) {
//    AddError("Therm 1: NC , ");
//    return 9999; //Unlikely value to cause shutdown
//  }
  return digitalSmooth(c1, WaterSmoothArray);
    
}

//*******************************************************************************************
// Read the Exhaust Temp
//*******************************************************************************************
double get_exhaust_temp(){
  double c1 = th_exhaust_temp.readCelsius();
  //Comment out block if Thermocouple(s) not in use
//  if (isnan(c1)) {
//    AddError("Therm 1: NC , ");
//    return 9999; //Unlikely value to cause shutdown
//  }
  //DigitalSmooth is good at removing transient, unexpected values
  return digitalSmooth(c1, ExhaustSmoothArray);
}

//*******************************************************************************************
// What is the flame temperature value
//*******************************************************************************************
double get_flame_temp(){
static long Flame_Timer = 0;
static double Flame_Last = 0;
double battery_local = 0;

  //if Flame sensor not used, return Exhaust Temperature instead
  if(!use_flame_sensor){
    return get_exhaust_temp();
  }

  //Make sure there's sufficient battery before continuing
  battery_voltage = float(analogRead(batt_voltage_pin)) / 100.0;
  if(battery_voltage < 12.4){
    InfoMessage = "Battery Low: " + String(battery_voltage) + "V<br>";
    return 9999; //Value high enough to cause shutdown.
  } else {
    InfoMessage = "";
  }


  //If the value has been read recently, just return the old value
  if((millis()-Flame_Timer)<1000){
    analogWrite(glow_plug_pin, 0);
    return Flame_Last;   
  }
  Flame_Timer = millis();
  if(glowplug_on!= 0) return Flame_Last; //Cannot take a reading if the glow plug is switched on
  //The Current is measured by switching on the glow plug briefly
  int Initial_Reading = 0;

  Initial_Reading = ACS.mA_DC(120);  //The hall sensor has an offset based on other things operating.  Sample 120 times = 10mS
  if(Initial_Reading < 5000){ //Glowplug bust - return error
    AddError("GLOW PLUG: NC , ");
    return 9999;
  } 
  analogWrite(glow_plug_pin, 255);  //Hopefully this will be brief enough not to allow the glow plug to heat up on it's own too much
  //Measure the battery voltage:
  battery_local = float(analogReadMilliVolts(batt_voltage_pin)*1.1) / 100.0;
  double Glow_Amps = float(ACS.mA_DC(120) - Initial_Reading) / 1000.0; //Sample 120 times = 10mS
  analogWrite(glow_plug_pin, 0); //Glow plug off
  vTaskDelay(10);
  analogWrite(glow_plug_pin, 0); //Glow plug off

  double Glow_Ohms = battery_local / Glow_Amps;


  //For genuine Webasto, 0.342 Ohm = 25C,  1.600 Ohm = 1300C Assuming this is linear, convert Ohms to Deg C
  Flame_Last = Interpolate(Glow_Ohms, glow_25C, glow_1300C, 25, 1300);
  Flame_Last = digitalSmooth(Flame_Last, ExhaustSmoothArray);
  //Serial.print("Batt V="); Serial.print(battery_voltage);
  //Serial.print("| Glow A="); Serial.print(Glow_Amps);
  //Serial.print("| Glow R="); Serial.print(Glow_Ohms);
  //Serial.print("| Glow T="); Serial.println((1300.0 - 25.0) / (1.6 - 0.342) * (Glow_Ohms - 0.40)/2);
  return Flame_Last;
  
 
  
}

//*******************************************************************************************
// Is the flame alight?
//*******************************************************************************************
bool Is_Flame(){
double FLTemp = get_flame_temp();
  if(FLTemp == 9999){
    return false;
  }else{
    return (FLTemp > flame_threshold);
  }
}



//*******************************************************************************************
// Median filter for temperature values
//*******************************************************************************************
float digitalSmooth(float rawIn, float *sensSmoothArray){     // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
//This is what's known as a Median Filter.  It's good for values with noise spikes where they are centred on the correct one, but there are 
//widely fluctuating exceptional values (spikes).  The filter sorts the last few results then discards the top & bottom 15% and returns the middle / median value of what remains
  
  int j, k, temp, top, bottom;
  long total;
  static int i;
  static float sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;    // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;                 // input new data into the oldest slot

  for (j=0; j<filterSamples; j++){     // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;                // flag to know when we're done sorting              
  while(done != 1){        // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++){
      if (sorted[j] > sorted[j + 1]){     // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted [j+1] =  sorted[j] ;
        sorted [j] = temp;
        done = 0;
      }
    }
  }


  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  // total remaining indices
    k++; 
  }

  return sorted[int(filterSamples/2)]; //Return Median value
}
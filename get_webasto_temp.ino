#include "thermistor.h"

THERMISTOR thermistor(exhaust_temp_pin,        // Analog pin
                      100000,          // Nominal resistance at 25 ºC
                      //4050,           // thermistor's beta coefficient
                      3950,           // thermistor's beta coefficient
                      4700);         // Value of the series resistor
THERMISTOR thermistor2(water_temp_pin,        // Analog pin
                      10000,          // Nominal resistance at 25 ºC
                      3977,           // thermistor's beta coefficient
                      2200);         // Value of the series resistor

float get_wabasto_temp(int temp_pin) { // read a sensor value, smoothen it a bit and convert it to C degrees
float temp_temp = 0;
analogReadResolution(12);
  if(temp_pin == exhaust_temp_pin) {
    rawDataExhaust = thermistor.read();
    rawDataExhaust = rawDataExhaust/10;
    temp_temp = digitalSmooth(rawDataExhaust, ExhaustSmoothArray);
    if(millis() - GWTLast_Sec > 1000 ) {
      GWTLast_Sec = millis();

      //When the fan speed is changing, it sometimes affects the temp readings.  Mute changes when
      if((EX_Mute == false) && (temp_temp > 0)) {
        Last_Mute_T = temp_temp;
      } else {
        temp_temp = Last_Mute_T;
      }

      //Limit rate of change to Max_Change_Per_Sec
       if(abs(temp_temp-Last_Exh_T) > Max_Change_Per_Sec) {
        if((temp_temp-Last_Exh_T) > 0) { 
          Last_Exh_T += Max_Change_Per_Sec;
        } else {
          Last_Exh_T -= Max_Change_Per_Sec;
        }
      } else {
        Last_Exh_T = temp_temp;
      }

      
    }
    
    return Last_Exh_T;
  } else {
    rawDataWater = thermistor2.read();
    rawDataWater = rawDataWater/10;
    //if(debug_glow_plug_on) rawDataWater = rawDataWater -14; //Glow plug causes the temp to misread by 14C
    
    if(temp_temp < 0 || temp_temp > 100) temp_temp = 101; //Out of range, make sure heater does not start
    if(((millis() - Last_TSec > 1000) && burn_mode==2) || ((millis() - Last_TSec > 300) && burn_mode!=2)) {
      Last_TSec = millis();
      //Limit rate of change to Max_Change_Per_Sec
      temp_temp = digitalSmooth(rawDataWater, WaterSmoothArray);
      temp_temp = rawDataWater;
      /*
      if(abs(temp_temp-Last_Wat_T) > Max_Change_Per_Sec) {
        
        if((temp_temp-Last_Wat_T) > 0) { 
          Last_Wat_T += Max_Change_Per_Sec;
        } else {
          Last_Wat_T -= Max_Change_Per_Sec;
        }
      } else {
        Last_Wat_T = temp_temp;
      }
      */
      //Last_Wat_T = SVWFilter(temp_temp);  //Only feed data to filter once every 5 sec
      Last_Wat_T = temp_temp;  //Only feed data to filter once every 5 sec
    }
    return Last_Wat_T; 
  }
}
/*
float rawDataWater, smoothDataWater;  // variables for sensor1 data
float rawDataExhaust, smoothDataExhaust;  // variables for sensor2 data

float WaterSmoothArray [filterSamples];   // array for holding raw sensor values for sensor1 
float ExhaustSmoothArray [filterSamples];   // array for holding raw sensor values for sensor2 
*/

float digitalSmooth(float rawIn, float *sensSmoothArray){     // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
  int j, k, temp, top, bottom;
  long total;
  static int i;
 // static int raw[filterSamples];
  static float sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;    // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;                 // input new data into the oldest slot

  // Serial.print("raw = ");

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

/*
  for (j = 0; j < (filterSamples); j++){    // print the array to debug
    Serial.print(sorted[j]); 
    Serial.print("   "); 
  }
  Serial.println();
*/

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  // total remaining indices
    k++; 
    // Serial.print(sorted[j]); 
    // Serial.print("   "); 
  }

//  Serial.println();
//  Serial.print("average = ");
//  Serial.println(total/k);
  //return total / k;    // divide by number of samples
  return sorted[int(filterSamples/2)]; //Return Median value
}

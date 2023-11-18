#include "thermistor.h"

//When flame sensor enabled, use Exhaust pin as a room temp sensor (I'm using a 10k thermistor with 10k bias resistor)
THERMISTOR thermistor(Room_Temp_pin,        // Analog pin
                      10000,          // Nominal resistance at 25 ºC
                      3950,           // thermistor's beta coefficient
                      9800);         // Value of the series resistor
THERMISTOR thermistor2(water_temp_pin,        // Analog pin
                      10000,          // Nominal resistance at 25 ºC
                      3977,           // thermistor's beta coefficient
                      4400);         // Value of the series resistor

float get_wabasto_temp(int temp_pin) { // read a sensor value, smoothen it a bit and convert it to C degrees
float temp_temp = 0;
analogReadResolution(12);

    if(temp_pin == Room_Temp_pin) {

      temp_temp = digitalSmooth(Flame_Temp_Read(), ExhaustSmoothArray);
      //temp_temp = Flame_Temp();
      
      if(millis() - GWTLast_Sec > 1000 ) {
        GWTLast_Sec = millis();
    
        //Limit rate of change to Max_Change_Per_Sec
        if(temp_temp<-120) {
          //At low temperatures rise fast, fall slow damping
          if((Last_Exh_T-temp_temp) > Max_Change_Per_Sec_Exh) {
            if((temp_temp-Last_Exh_T) > 0) { 
              Last_Exh_T += Max_Rise_Per_Sec_Exh;
              //Last_Exh_T = temp_temp;
            } else {
              Last_Exh_T -= Max_Change_Per_Sec_Exh;
            }
          } else {
            Last_Exh_T = temp_temp;
          }

        } else {
          //At higher temperatures damp temp rise & fall
          if(abs(Last_Exh_T-temp_temp) > Max_Change_Per_Sec_Exh) {
            if((temp_temp-Last_Exh_T) > 0) { 
              Last_Exh_T += Max_Rise_Per_Sec_Exh;
              //Last_Exh_T = temp_temp;
            } else {
              Last_Exh_T -= Max_Change_Per_Sec_Exh;
            }
          } else {
            Last_Exh_T = temp_temp;
          }
        }
      }
      //Get room temperature data
      room_temp = digitalSmooth(thermistor.read(), RoomSmoothArray);

      //room_temp = analogRead(Room_Temp_pin);
      room_temp = room_temp / 10;

      return Last_Exh_T;



    } else {
      rawDataWater = thermistor2.read();
      rawDataWater = rawDataWater/10;
      
          if(temp_temp < 0 || temp_temp > (water_overheat+15)) temp_temp = 999; //Out of range, make sure heater does not start


      if(((millis() - Last_TSec > 1000) && burn_mode==2) || ((millis() - Last_TSec > 300) && burn_mode!=2)) {
        Last_TSec = millis();
        //Limit rate of change to Max_Change_Per_Sec
        temp_temp = digitalSmooth(rawDataWater, WaterSmoothArray);
        temp_temp = rawDataWater;
    
    
        //Limit the rate the rate temperature is allowed to fall
        if((Last_Wat_T - temp_temp) > Max_Change_Per_Sec_Wat) {
          
          if((temp_temp-Last_Wat_T) > 0) { 
            Last_Wat_T += Max_Change_Per_Sec_Wat;
          } else {
            Last_Wat_T -= Max_Change_Per_Sec_Wat;
          }
        } else {
          Last_Wat_T = temp_temp;
        }
        
    
        
      }
      return Last_Wat_T; 
    }
  
}

float digitalSmooth(float rawIn, float *sensSmoothArray){     // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
//This is what's known as a Median Filter.  It's good for values with noise spikes where they are centred on the correct one, but there are 
//widely fluctuating exceptional values (spikes).  The filter sorts the last few results then discards the top & bottom 15% and returns the middle / median value of what remains
  
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

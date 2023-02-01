//This version is equipped with an ACS711 Current sensor with a sensitivity of 110mV per Amp
//Connected to A4
//This function uses the resistance of the glow plug, which increases with temperature
//to detect if there is a flame present, 
// Resistance at 25C is 0.324 ... 0.360 Ohms
// Resistance when Glow Plug Opererating is 1.6 Ohms
// Resistance when there is a flame is around 0.6 Ohms
//
// In order to measure the resistance, we need the Current (from the ACS711) and the battery voltage from push_pin

float Flame_Temp() {
  if((millis()-Flame_Timer)<1000){
    analogWrite(glow_plug_pin, 0);
    return Flame_Last;   
  }
  Flame_Timer = millis();
  if(glowing_on!= 0) return -99.0; //Cannot take a reading if the glow plug is switched on
  
  //Calculate the battery voltage:
  int Batt_ADC = analogRead(push_pin);
  //This is a 12 bit value - 0 to 4095.
  //The divider values are 6.8k & 1.8k
  //  Vin = Vdiv(6.8+1.8)/1.8
  //  3.3V = 4095 Therefore 1241 Counts per Volt (Vdiv)
  //  Vin = Batt_ADC * (6.8+1.8) / 1.8 * 1241
  //  Simplifies to Vin = Batt_ADC * 000385
  
  // Max measurable voltage = 15.76V
  
  //The Current is measured by switching on the glow plug briefly
  int Initial_Reading = 0;

  //if(glowing_on= 0) { //Only turn on the glowplug is it's currently off
    Initial_Reading = analogRead(flame_sensor);  //The hall sensor has a (random) offset.  Remove this from the calc
    analogWrite(glow_plug_pin, 255);  //Hopefully this will be brief enough not to allow the glow plug to heat up on it's own too much
    delay(2);
  //}
  int Glow_Reading = analogRead(flame_sensor);
  float Battery_Voltage = (Batt_ADC * 0.003965927); //Measure the battery voltage while the glow plug is on

  //if(glowing_on= 0) { //Only turn off the glowplug is it's currently off
    analogWrite(glow_plug_pin, 0);
  //}

  //This is a 12 bit value - 0 to 4095 for the range 0 to 3.3V or 8.05664mV per count
  float Glow_Amps = abs(Initial_Reading - Glow_Reading) * 8.05664 / 330;

  float Glow_Ohms = Battery_Voltage / Glow_Amps;


  //Serial.print("Batt V="); Serial.print(Battery_Voltage);
  //Serial.print("| Glow A="); Serial.print(Glow_Amps);
  //Serial.print("| Glow R="); Serial.print(Glow_Ohms);
  //Serial.print("| Glow T="); Serial.println((1300.0 - 25.0) / (1.6 - 0.342) * (Glow_Ohms - 0.40)/2);
  //Glow plugs heat to about 1300C apparently.  Using this as the temperature at 1.6 Ohm and assuming it's linear
  //return ((1300.0 - 25.0) / (1.6 - 0.342) * (Glow_Ohms - 0.317)/2);
  Flame_Last = ((1300.0 - 25.0) / (1.6 - 0.342) * (Glow_Ohms - 0.40)/2);
  //Unfortunately, it seems the reading from the ACS711 current sensor is temperature dependent.  However, this calculation should correct it mostly
  Flame_Last = water_temp / 1.48 + Flame_Last;
  return Flame_Last;
  
  //In practice, it doesn't have to be spot on, just enough to tell if we ave a flame or not
  //If 0.6 Ohm is accurate for the heater running normally, this equates to 286C - probably not a million miles off
 
  
}

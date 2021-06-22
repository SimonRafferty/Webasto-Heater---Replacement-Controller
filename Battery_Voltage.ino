//Pin 1 is used as the on/off switch.  However, it connects to an Analog pin via a 
//potential divider.  While the heater is on, this can be used to read battery voltage
//The purpose of this is to shut down if the voltage dips below a certain value - say 11V

float Battery_Voltage() {
int Batt_ADC = analogRead(push_pin);

//This is a 12 bit value - 0 to 4095.
//The divider values are 6.8k & 1.8k
//  Vin = Vdiv(6.8+1.8)/1.8
//  3.3V = 4095 Therefore 1241 Counts per Volt (Vdiv)
//  Vin = Batt_ADC * (6.8+1.8) / 1.8 * 1241
//  Simplifies to Vin = Batt_ADC * 000385

// Max measurable voltage = 15.76V
  return (Batt_ADC * 0.00385);
}

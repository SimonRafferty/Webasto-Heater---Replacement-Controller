//*******************************************************************************************
// Set the fuel rate when the pump is priming
//*******************************************************************************************
float prime_rate(float ambient_temperature) {
  if (ambient_temperature <= prime_low_temp)
    return prime_low_temp_fuelrate;

  if (ambient_temperature >= prime_high_temp)
    return prime_high_temp_fuelrate;

  //Between the two temperatures - interpolate
  return mapf(ambient_temperature, prime_low_temp, prime_high_temp, prime_low_temp_fuelrate, prime_high_temp_fuelrate);
}

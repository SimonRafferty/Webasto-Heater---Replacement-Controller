float prime_ratio(float ambient_temperature) {
  if(ambient_temperature<= prime_low_temp)
    return prime_low_temp_fuelrate;

  if(ambient_temperature> prime_low_temp)
    return prime_high_temp_fuelrate;

  float steps = prime_high_temp-prime_low_temp;
  float fuel_steps = (prime_low_temp_fuelrate-prime_high_temp_fuelrate)/steps;  
  return prime_low_temp_fuelrate-((exhaust_temp-prime_low_temp)*fuel_steps);
}

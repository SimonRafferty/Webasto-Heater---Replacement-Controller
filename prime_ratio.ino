float prime_ratio(float exhaust_temp) {
  if(exhaust_temp<= prime_low_temp)
    return prime_low_temp_fuelrate;

  if(exhaust_temp>= prime_high_temp)
    return prime_high_temp_fuelrate;

  float steps = prime_high_temp-prime_low_temp;
  float fuel_steps = (prime_low_temp_fuelrate-prime_high_temp_fuelrate)/steps;  
  return prime_low_temp_fuelrate-((exhaust_temp-prime_low_temp)*fuel_steps);
}

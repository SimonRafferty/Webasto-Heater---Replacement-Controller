//*******************************************************************************************
// Output setup.  All five outputs are low-side MOSFETs via TC4427A drivers: HIGH = load on.
// PWM uses the LEDC peripheral.  The fuel pump is pulsed from a hardware timer (fuel_pump.ino)
//*******************************************************************************************
void Outputs_Setup() {
  //Fuel pump - plain digital output, driven by the timer ISR
  pinMode(fuel_pump_pin, OUTPUT);
  digitalWrite(fuel_pump_pin, LOW);

  //PWM outputs - start at 0% duty
  ledcAttach(burn_fan_pin, FAN_PWM_HZ, PWM_BITS);
  ledcWrite(burn_fan_pin, 0);
  ledcAttach(water_pump_pin, FAN_PWM_HZ, PWM_BITS);
  ledcWrite(water_pump_pin, 0);
  ledcAttach(glow_plug_pin, GLOW_PWM_HZ, PWM_BITS);
  ledcWrite(glow_plug_pin, 0);
  ledcAttach(conv_fan_pin, CONV_PWM_HZ, PWM_BITS);
  ledcWrite(conv_fan_pin, 0);

  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  Fuel_Pump_Setup();
}

//*******************************************************************************************
// Set a PWM output as a percentage
//*******************************************************************************************
void Set_PWM_Percent(uint8_t pin, float percent) {
  percent = constrain(percent, 0.0, 100.0);
  ledcWrite(pin, (uint32_t)(percent * PWM_MAX / 100.0 + 0.5));
}

//*******************************************************************************************
// Status LED (in priority order)
//  Error / failure          : flashes at 4Hz
//  Glow plug live           : on
//  Heater running           : flashes at 1Hz  (starting, running or shutting down, glow plug off)
//  Off                      : off
//*******************************************************************************************
void Status_LED() {
  unsigned long t = millis();
  bool on = false;

  if (ErrorMessage != "" || Heater_fail) {
    on = (t % 250) < 125;          //4Hz
  } else if (glow_plug_on) {
    on = true;
  } else if (burn_mode != 0) {
    on = (t % 1000) < 500;         //1Hz
  }
  digitalWrite(led_pin, on);
}

//*******************************************************************************************
// Turn the plug on if glow_time > 0, and decrement glow_time every second
//*******************************************************************************************
void glow_plug() {
  static unsigned long timer;
  static unsigned long glowplug_on = 0;   //millis() when the plug was switched on (0 = off)

  if (glow_left < 0)
    glow_time = 0;

  if (last_glow_value != glow_time)
  {
    glow_left = glow_time;
    last_glow_value = glow_time;

    if (glow_time != 0)
      timer = millis();
  }

  if (glow_time == 0)
    timer = millis();

  if (millis() - timer >= 1000)
  {
    timer = millis();
    glow_left -= 1;
  }

  if (glow_left > 0 && !bench_mode)
  {
    if (glowplug_on == 0)
      glowplug_on = millis();

    //Full power duty for the target RMS voltage: P is proportional to V^2 x duty
    float full_duty = 100.0;
    if (battery_voltage > glow_volts) {
      full_duty = 100.0 * (glow_volts / battery_voltage) * (glow_volts / battery_voltage);
    }

    //Fade up over glow_ramp_ms - a cold plug has a low resistance and a big inrush
    unsigned long on_for = millis() - glowplug_on;
    if (on_for < (unsigned long)glow_ramp_ms)
      Set_PWM_Percent(glow_plug_pin, full_duty * on_for / glow_ramp_ms);
    else
      Set_PWM_Percent(glow_plug_pin, full_duty);

    glow_plug_on = true;
  }
  else
  {
    glowplug_on = 0;
    Set_PWM_Percent(glow_plug_pin, 0);
    glow_plug_on = false;
    if (glow_left <= 0) glow_time = 0;
  }
}

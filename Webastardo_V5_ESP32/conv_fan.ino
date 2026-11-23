//*******************************************************************************************
// Convection (vehicle blower) fan on X1-4
// On when the water is above heater_min (with hysteresis), OR when switched on from the web page
//*******************************************************************************************
void conv_fan() {
static bool hot = false;

  if (water_sensor_fault) {
    hot = false;
  } else if (water_temp > heater_min) {
    hot = true;
  } else if (water_temp < heater_min - conv_fan_hysteresis) {
    hot = false;
  }

  conv_fan_on = (hot || Web_conv_fan_ON) && !bench_mode;
  Set_PWM_Percent(conv_fan_pin, conv_fan_on ? 100 : 0);
}

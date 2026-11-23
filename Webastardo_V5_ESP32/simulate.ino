//*******************************************************************************************
// Sensor simulation for testing on a bare ESP32-S3 (set SIMULATE_SENSORS 1 in the main tab)
//
// A very rough thermal model so the whole start / run / throttle / shutdown sequence and the
// web page can be exercised with nothing connected:
//  * The fuel "ignites" if it's pumped while the chamber is hot or the glow plug is on
//  * Exhaust temperature heads towards a value set by the fuel rate while burning
//  * Water heats from the burner (if the pump runs) and loses heat to ambient
// Time runs at real speed.  Room temperature follows the water loosely.
//*******************************************************************************************
#if SIMULATE_SENSORS

void Simulate_Sensors() {
  static bool started = false;
  static float sim_water = 15, sim_exhaust = 15, sim_room = 12;
  static unsigned long last = 0;
  const float ambient = 10;

  unsigned long now = millis();
  float dt = started ? (now - last) / 1000.0 : 0;
  last = now;
  started = true;

  float pump_hz = Fuel_Pump_Hz();
  bool burning = (pump_hz > 0.1) && (sim_exhaust > 60 || glow_plug_on);

  //Exhaust: heads for ~60C per Hz of fuel while burning, cools towards the water otherwise
  float exh_target = burning ? (sim_water + 60.0 * fuel_need + 40.0) : sim_water;
  float exh_rate = burning ? 0.08 : (fan_speed > 0 ? 0.05 : 0.01);
  if (glow_plug_on && !burning) exh_target = sim_water + 8;   //The glow plug warms it a little
  sim_exhaust += (exh_target - sim_exhaust) * exh_rate * dt;

  //Water: ~0.12C/s per Hz of fuel while burning & pumping, losses to ambient
  float heat = (burning && water_pump_speed > 0) ? 0.12 * fuel_need : (burning ? 0.3 * fuel_need : 0);
  float loss = (sim_water - ambient) * (conv_fan_on ? 0.004 : 0.0015);
  sim_water += (heat - loss) * dt;

  sim_room += ((sim_water > 30 ? 22 : ambient) - sim_room) * 0.01 * dt;

  battery_voltage = glow_plug_on ? 12.1 : 12.9;
  bench_mode = false;
  water_sensor_fault = false;
  exhaust_sensor_fault = false;
  room_sensor_fault = false;
  water_temp = sim_water;
  exhaust_temp = sim_exhaust;
  room_sensor_fitted = true;
  room_temp = sim_room;
}

#endif

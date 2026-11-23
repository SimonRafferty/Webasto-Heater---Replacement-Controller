//*******************************************************************************************
// Sensor reading: Battery voltage, Water NTC, Room NTC & Exhaust PT100
// All four inputs are on ADC1 so they work with WiFi running.
// analogReadMilliVolts() uses the factory ADC calibration (curve fitting on the S3).
//*******************************************************************************************

//Fault limits (mV at the ADC pin)
const int NTC_OPEN_MV    = 3050;   // 11dB range saturates around 3.1V.  Open circuit = 3.3V
const int NTC_SHORT_MV   = 50;
const int PT100_OPEN_MV  = 900;    // 0dB range is ~0-950mV.  Open circuit = 3.3V (saturated).  900mV = ~725C
const int PT100_SHORT_MV = 150;    // 150mV = ~48 Ohm = below -100C

int water_mv = 0, room_mv = 0, exhaust_mv = 0, vbat_mv = 0;   //Last raw readings (shown on the web page)

//*******************************************************************************************
// Configure the ADC inputs
//*******************************************************************************************
void Sensors_Setup() {
  analogReadResolution(12);
  analogSetPinAttenuation(vbat_pin, ADC_11db);
  analogSetPinAttenuation(water_temp_pin, ADC_11db);
  analogSetPinAttenuation(room_temp_pin, ADC_11db);
  analogSetPinAttenuation(exhaust_temp_pin, ADC_0db);   // ~0-950mV for PT100 resolution
}

//*******************************************************************************************
// Set globals from readings.  Called every loop; reads at 10Hz unless force is set
//*******************************************************************************************
void get_temperatures(bool force) {
static unsigned long timer = 0;

  if (!force && (millis() - timer < 100)) return;
  timer = millis();

#if SIMULATE_SENSORS
  Simulate_Sensors();   //simulate.ino
  return;
#endif

  battery_voltage = get_battery_voltage();
  water_temp = get_water_temp();
  exhaust_temp = get_exhaust_temp();
  room_temp = get_room_temp();
}

//*******************************************************************************************
// Oversampled, calibrated reading in mV
//*******************************************************************************************
int Read_mV(uint8_t pin) {
  uint32_t total = 0;
  for (int i = 0; i < adcOversample; i++) {
    total += analogReadMilliVolts(pin);
  }
  return total / adcOversample;
}

//*******************************************************************************************
// Battery voltage.  47k/10k divider taken before the reverse-polarity diode
//*******************************************************************************************
float get_battery_voltage() {
  vbat_mv = Read_mV(vbat_pin);
  float v = digitalSmooth(vbat_mv / 1000.0 * vbat_scale, &VbatFilter);
  bench_mode = (v < bench_mode_volts);
  return v;
}

//*******************************************************************************************
// NTC: 4k7 pull-up to 3V3, sensor to ground.  Beta equation.
// Returns NAN and sets fault if open or short circuit
//*******************************************************************************************
float NTC_Temp(int mv, bool &fault) {
  if (mv >= NTC_OPEN_MV || mv <= NTC_SHORT_MV) {
    fault = true;
    return NAN;
  }
  fault = false;
  float v = mv / 1000.0;
  float r = NTC_PULLUP * v / (rail_3v3 - v);
  float t = 1.0 / (1.0 / 298.15 + log(r / ntc_r25) / ntc_beta) - 273.15;
  return t;
}

//*******************************************************************************************
// Read the Water Temp
//*******************************************************************************************
float get_water_temp() {
  water_mv = Read_mV(water_temp_pin);
  float t = NTC_Temp(water_mv, water_sensor_fault);
  if (water_sensor_fault) return 999;   //Unlikely value - stops the heater starting / forces overheat shutdown
  return digitalSmooth(t + water_offset, &WaterFilter);
}

//*******************************************************************************************
// Read the Room Temp (optional sensor, used by Run to Temp)
// No sensor fitted = pulled up to 3.3V, which is "not fitted", not a fault.
// (A very cold sensor, below about -15C, also reads as not fitted - the 4k7 bias saturates the ADC)
//*******************************************************************************************
float get_room_temp() {
  room_mv = Read_mV(room_temp_pin);
  bool fault;
  float t = NTC_Temp(room_mv, fault);
  bool fitted = (room_mv < NTC_OPEN_MV);
  if (fitted != room_sensor_fitted) {
    room_sensor_fitted = fitted;
    RoomFilter.primed = false;   //Start the filter afresh
    Serial.println(fitted ? "Room sensor detected" : "Room sensor not fitted");
  }
  room_sensor_fault = fitted && fault;   //Only a short circuit counts as a fault
  if (!fitted || fault) return NAN;
  return digitalSmooth(t + room_offset, &RoomFilter);
}

//*******************************************************************************************
// Read the Exhaust Temp from the PT100
// 1k pull-up to 3V3, PT100 to ground.  Callendar-Van Dusen for T >= 0C
//*******************************************************************************************
float get_exhaust_temp() {
  exhaust_mv = Read_mV(exhaust_temp_pin);
  if (exhaust_mv >= PT100_OPEN_MV || exhaust_mv <= PT100_SHORT_MV) {
    exhaust_sensor_fault = true;
    return 999;   //Looks hot - the heater won't start on it
  }
  exhaust_sensor_fault = false;
  float t = PT100_Temp(Rpt_From_mV(exhaust_mv));
  //DigitalSmooth is good at removing transient, unexpected values
  return digitalSmooth(t + exhaust_offset, &ExhaustFilter);
}

float Rpt_From_mV(int mv) {
  float v = mv / 1000.0;
  return PT100_PULLUP * v / (rail_3v3 - v);
}

//*******************************************************************************************
// PT100 resistance to temperature (IEC 60751)
//*******************************************************************************************
float PT100_Temp(float r) {
  const float R0 = 100.0;
  const float A = 3.9083e-3;
  const float B = -5.775e-7;
  if (r < R0) {
    //Below 0C - linear is plenty for an exhaust (error < 0.2C to -30C)
    return (r / R0 - 1.0) / A;
  }
  //Solve R = R0(1 + A.T + B.T^2) for T
  return (-A + sqrt(A * A - 4.0 * B * (1.0 - r / R0))) / (2.0 * B);
}

//*******************************************************************************************
// Short description of a sensor fault for error messages
//*******************************************************************************************
String Sensor_Fault_Text(uint8_t pin) {
  int mv = (pin == exhaust_temp_pin) ? exhaust_mv : (pin == water_temp_pin) ? water_mv : room_mv;
  int short_mv = (pin == exhaust_temp_pin) ? PT100_SHORT_MV : NTC_SHORT_MV;
  if (mv <= short_mv) return "short-circuited";
  return "disconnected (open circuit)";
}

//*******************************************************************************************
// Is the flame alight?  (Exhaust temperature above flame_threshold)
// The start-up sequence also accepts a rise of flame_rise above exhaust_at_ignition
//*******************************************************************************************
bool Is_Flame() {
  if (exhaust_sensor_fault) return false;
  return (exhaust_temp > flame_threshold);
}

bool Flame_Started() {
  if (exhaust_sensor_fault) return false;
  return Is_Flame() || (exhaust_temp > exhaust_at_ignition + flame_rise);
}

//*******************************************************************************************
// Median filter for sensor values
//*******************************************************************************************
float digitalSmooth(float rawIn, MedianFilter *f) {
//This is what's known as a Median Filter.  It's good for values with noise spikes where they are centred
//on the correct one, but there are widely fluctuating exceptional values (spikes).
//The filter sorts the last few results and returns the middle / median value
  float sorted[filterSamples];

  if (!f->primed) {
    //First reading - fill the buffer so we don't return zeros
    for (int j = 0; j < filterSamples; j++) f->buf[j] = rawIn;
    f->primed = true;
  }
  f->idx = (f->idx + 1) % filterSamples;
  f->buf[f->idx] = rawIn;                       // input new data into the oldest slot

  for (int j = 0; j < filterSamples; j++) sorted[j] = f->buf[j];

  //Insertion sort, lowest to highest
  for (int j = 1; j < filterSamples; j++) {
    float key = sorted[j];
    int k = j - 1;
    while (k >= 0 && sorted[k] > key) {
      sorted[k + 1] = sorted[k];
      k--;
    }
    sorted[k + 1] = key;
  }
  return sorted[filterSamples / 2]; //Return Median value
}

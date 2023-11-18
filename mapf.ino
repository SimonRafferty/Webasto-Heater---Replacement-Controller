float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  // the perfect map fonction, with constraining and float handling
  x = constrain(x, in_min, in_max);
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

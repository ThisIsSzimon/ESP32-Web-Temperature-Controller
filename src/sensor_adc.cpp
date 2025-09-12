#include "sensor_adc.hpp"

TMP36Sensor::TMP36Sensor(adc1_channel_t channel, int samples) : channel_(channel), samples_(samples) {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(channel_, ADC_ATTEN_DB_11);
}

void TMP36Sensor::calibrateKnownVout(float vout_V) {
  uint32_t acc = 0;
  for (int i = 0; i < samples_; ++i)
    acc += adc1_get_raw(channel_);
  uint32_t raw = acc / (uint32_t)samples_;
  if (raw > 0) v_per_count_ = vout_V / (float)raw;
}

bool TMP36Sensor::readVoltage(float &vout_V) {
  uint32_t acc = 0;
  for (int i = 0; i < samples_; ++i)
    acc += adc1_get_raw(channel_);
  uint32_t raw = acc / (uint32_t)samples_;
  vout_V = raw * v_per_count_; // skalowanie po kalibracji
  return true;
}

bool TMP36Sensor::readCelsius(float &tC) {
  float v;
  if (!readVoltage(v)) return false;
  tC = 100.0f * (v - 0.500f);
  return true;
}

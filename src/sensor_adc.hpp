#pragma once
#include "driver/adc.h"
#include "esp_adc_cal.h"

class SensorADC {
public:
  SensorADC(adc1_channel_t channel, adc_atten_t atten, adc_bits_width_t width);
  float readTemperatureC(); // zwraca temperaturę w °C (TMP36)

private:
  adc1_channel_t channel_;
  esp_adc_cal_characteristics_t adc_chars_;
};
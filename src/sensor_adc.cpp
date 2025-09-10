#include "sensor_adc.hpp"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SensorADC";

SensorADC::SensorADC(adc1_channel_t channel, adc_atten_t atten, adc_bits_width_t width) : channel_(channel) {
  // konfiguracja ADC
  adc1_config_width(width);
  adc1_config_channel_atten(channel_, atten);
  // kalibracja (eFuse Vref albo 1100 mV jako domyślne)
  esp_adc_cal_characterize(ADC_UNIT_1, atten, width, 1100, &adc_chars_);
}

float SensorADC::readTemperatureC() {
  // uśrednianie dla stabilności
  const int N = 32;
  uint32_t acc = 0;
  for (int i = 0; i < N; i++) {
    acc += adc1_get_raw(channel_);
  }
  uint32_t raw = acc / N;

  // na mV
  uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars_);
  // TMP36: 10 mV/°C, offset 500 mV
  float tempC = (mv - 500.0f) / 10.0f;

  ESP_LOGI(TAG, "ADC raw=%" PRIu32 " mv=%" PRIu32 " -> T=%.2f C", raw, mv,
           tempC);

  return tempC;
}
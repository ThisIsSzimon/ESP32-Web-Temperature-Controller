#pragma once
#include <cstdint>

extern "C" {
#include "driver/adc.h"
}

class TMP36Sensor {
public:
  TMP36Sensor(adc1_channel_t channel, int samples = 16);

  void calibrateKnownVout(float vout_V);
  bool readCelsius(float &tC);

  bool readVoltage(float &vout_V);

private:
  adc1_channel_t channel_;
  int samples_;
  float v_per_count_ = 3.10f / 4095.0f;
};

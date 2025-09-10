#pragma once
#include "driver/gpio.h"

class DS18B20 {
public:
  explicit DS18B20(gpio_num_t pin);
  bool readTemperature(float &tempC); // glowny funkcja czytająca temperaturę

private:
  gpio_num_t pin_;
  bool resetPulse();
  void writeBit(int bit);
  int readBit();
  void writeByte(uint8_t v);
  uint8_t readByte();
};
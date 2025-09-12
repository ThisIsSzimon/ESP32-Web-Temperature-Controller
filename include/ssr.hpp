#pragma once
#include "driver/gpio.h"

class SSR {
public:
  explicit SSR(gpio_num_t pin);
  void set(bool on);
  bool state() const { return state_; }

private:
  gpio_num_t pin_;
  bool state_;
};
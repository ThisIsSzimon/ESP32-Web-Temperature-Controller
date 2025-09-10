#include "ssr.hpp"
#include "driver/gpio.h"

SSR::SSR(gpio_num_t pin) : pin_(pin), state_(false) {
  gpio_config_t io{};
  io.mode = GPIO_MODE_OUTPUT;
  io.pin_bit_mask = 1ULL << pin_;
  gpio_config(&io);
  gpio_set_level(pin_, 0);
}

void SSR::set(bool on) {
  state_ = on;
  gpio_set_level(pin_, on ? 1 : 0);
}
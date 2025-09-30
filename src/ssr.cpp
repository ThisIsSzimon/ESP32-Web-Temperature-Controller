#include "ssr.hpp"

void relay_init() {
  gpio_config_t io{};
  io.mode = GPIO_MODE_OUTPUT;
  io.pin_bit_mask = 1ULL << RELAY_PIN;
  io.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io.pull_up_en = GPIO_PULLUP_DISABLE;
  io.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io);

  // stan spoczynkowy = WYŁĄCZONY
  int off = RELAY_ACTIVE_LOW ? 1 : 0;
  gpio_set_level(RELAY_PIN, off);
}

void relay_set(bool on) {
  int level = RELAY_ACTIVE_LOW ? (on ? 0 : 1) : (on ? 1 : 0);
  gpio_set_level(RELAY_PIN, level);
}

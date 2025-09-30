#pragma once
#include "driver/gpio.h"

// Ustaw tu swój pin (np. D26) i logikę modułu:
static constexpr gpio_num_t RELAY_PIN = GPIO_NUM_26;
static constexpr bool RELAY_ACTIVE_LOW = true; // większość modułów: stan niski = ZAŁĄCZONY

void relay_init();
void relay_set(bool on); // on=true -> ZAŁĄCZ, on=false -> WYŁĄCZ

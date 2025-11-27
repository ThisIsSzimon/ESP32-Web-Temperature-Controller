#pragma once
#include "driver/gpio.h"

static constexpr gpio_num_t RELAY_PIN = GPIO_NUM_26;
static constexpr bool RELAY_ACTIVE_LOW = true;

void relay_init();
void relay_set(bool on);

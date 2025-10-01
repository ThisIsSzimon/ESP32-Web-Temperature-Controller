#pragma once
#include <cstdint>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// Callbacki do funkcji OLED:
struct UiCallbacks {
  void (*oled_clear)();
  void (*draw_text)(uint8_t col, uint8_t page, const char *s);
};

// Funkcja pobierająca temperaturę z API (Ateny)
using WeatherFunc = bool (*)(float &out_tempC);

void start_weather_task(const UiCallbacks &ui, WeatherFunc get_weather, UBaseType_t prio = 4, uint32_t stack = 6144);
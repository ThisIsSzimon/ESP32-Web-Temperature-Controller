#pragma once
#include <cstdint>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// Funkcje wyświetlacza przekazywane jako callbacki:
struct UiCallbacks {
  void (*oled_clear)();                                        // np. Twoje oled_clear()
  void (*draw_text)(uint8_t col, uint8_t page, const char *s); // np. Twoje draw_text(...)
};

// Sygnatura funkcji pobierającej temperaturę z API:
using WeatherFunc = bool (*)(float &out_tempC);

// Startuje task 60 s: pobiera API i odświeża OLED (API + ostatni lokalny odczyt)
void start_weather_task(const UiCallbacks &ui, WeatherFunc get_weather, UBaseType_t prio = 4, uint32_t stack = 6144);

#include "tasks/weather_task.hpp"
#include "shared_state.hpp"

#include <cmath>
#include <cstdio>

extern "C" {
#include "esp_log.h"
}

static const char *TAG_W = "WEATHER_TASK";

static void weather_task(void *arg) {
  auto *pack = static_cast<std::pair<UiCallbacks, WeatherFunc> *>(arg);
  UiCallbacks ui = pack->first;
  WeatherFunc get_weather = pack->second;
  delete pack; // już niepotrzebne

  for (;;) {
    float t_api;
    bool ok = get_weather ? get_weather(t_api) : false;

    float local_copy = NAN;
    if (g_data_mtx && xSemaphoreTake(g_data_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
      local_copy = g_local_tempC;
      xSemaphoreGive(g_data_mtx);
    }

    if (ui.oled_clear) ui.oled_clear();
    if (ui.draw_text) {
      ui.draw_text(0, 0, "Pogoda: Ateny");
      if (ok) {
        char line[32];
        snprintf(line, sizeof(line), "API: %.1f C", t_api);
        ui.draw_text(0, 2, line);
        ESP_LOGI(TAG_W, "Ateny: %.1f C", t_api);
      } else {
        ui.draw_text(0, 2, "API: blad");
      }

      if (!std::isnan(local_copy)) {
        char line2[32];
        snprintf(line2, sizeof(line2), "Lokal: %.1f C", local_copy);
        ui.draw_text(0, 4, line2);
      } else {
        ui.draw_text(0, 4, "Lokal: --.- C");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(60000)); // 60 s
  }
}

void start_weather_task(const UiCallbacks &ui, WeatherFunc get_weather, UBaseType_t prio, uint32_t stack) {
  // spakuj parametry do heapu, żeby przekazać do taska
  auto *pack = new std::pair<UiCallbacks, WeatherFunc>(ui, get_weather);
  xTaskCreate(weather_task, "weather_oled_task", stack, pack, prio, nullptr);
}

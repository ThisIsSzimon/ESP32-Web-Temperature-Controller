#include "tasks/weather_task.hpp"
#include "shared_state.hpp"
#include "ssr.hpp"

#include <cmath>
#include <cstdio>

extern "C" {
#include "esp_log.h"
}

static const char *TAG = "WEATHER_TASK";
static constexpr float HYST = 0.3f; // histereza 0.3 C

static void weather_task(void *arg) {
  auto *pack = static_cast<std::pair<UiCallbacks, WeatherFunc> *>(arg);
  UiCallbacks ui = pack->first;
  WeatherFunc get_weather = pack->second;
  delete pack;

  bool relay_state = false; // bieżący stan przekaźnika

  for (;;) {
    // 1) Pobierz temperaturę z API
    float t_api;
    bool ok = get_weather ? get_weather(t_api) : false;

    // 2) Zaktualizuj globalny stan
    float t_local = NAN;
    if (g_data_mtx && xSemaphoreTake(g_data_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (ok) g_api_tempC = t_api;
      t_local = g_local_tempC;
      xSemaphoreGive(g_data_mtx);
    }

    // 3) OLED
    if (ui.oled_clear) ui.oled_clear();
    if (ui.draw_text) {
      ui.draw_text(0, 0, "Pogoda: Ateny");
      if (ok) {
        char l1[32];
        std::snprintf(l1, sizeof l1, "API:   %.1f C", t_api);
        ui.draw_text(0, 2, l1);
        ESP_LOGI(TAG, "Athens: %.1f C", t_api);
      } else {
        ui.draw_text(0, 2, "API:   blad");
      }
      if (!std::isnan(t_local)) {
        char l2[32];
        std::snprintf(l2, sizeof l2, "Lokal: %.1f C", t_local);
        ui.draw_text(0, 4, l2);
      } else {
        ui.draw_text(0, 4, "Lokal: --.- C");
      }
    }

    // 4) Decyzja: local < api ? ON : OFF (z histerezą)
    if (!std::isnan(t_local) && !std::isnan(g_api_tempC)) {
      // histereza wokół progu t_api
      if (t_local < g_api_tempC - HYST) relay_state = true;       // załącz niżej
      else if (t_local > g_api_tempC + HYST) relay_state = false; // wyłącz wyżej
      relay_set(relay_state);
    }

    vTaskDelay(pdMS_TO_TICKS(10000)); // co 60 s
  }
}

void start_weather_task(const UiCallbacks &ui, WeatherFunc get_weather, UBaseType_t prio, uint32_t stack) {
  auto *pack = new std::pair<UiCallbacks, WeatherFunc>(ui, get_weather);
  xTaskCreate(weather_task, "weather_task", stack, pack, prio, nullptr);
}
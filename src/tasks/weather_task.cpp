#include "tasks/weather_task.hpp"
#include "shared_state.hpp"
#include "ssr.hpp"
#include "temperature_source.hpp" // Provider temperatury (API/manual)

#include <cmath>
#include <cstdio>

extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

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
    float t_api = NAN;
    bool ok = get_weather ? get_weather(t_api) : false;

    // 2) Zaktualizuj globalny stan + provider
    float t_local = NAN;
    if (g_data_mtx && xSemaphoreTake(g_data_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (ok) {
        g_api_tempC = t_api;                     // surowe API do globali
        temperature_source_set_api_value(t_api); // udostępniamy przez provider (API/manual)
      }
      t_local = g_local_tempC; // ostatnia lokalna z TMP36
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

    // 4) Docelowa temperatura (prog) do sterowania z providera:
    //    - gdy tryb "API": ostatnia z API
    //    - gdy tryb "manual": wartość z NVS ustawiona w web-UI
    const float t_set = temperature_source_get_c();

    // 5) Decyzja: local < t_set ? ON : OFF (z histerezą)
    if (!std::isnan(t_local) && !std::isnan(t_set)) {
      // histereza wokół progu t_set
      if (t_local < t_set - HYST) relay_state = true;       // załącz niżej
      else if (t_local > t_set + HYST) relay_state = false; // wyłącz wyżej
      relay_set(relay_state);
    }

    vTaskDelay(pdMS_TO_TICKS(10000)); // co 10 s
  }
}

void start_weather_task(const UiCallbacks &ui, WeatherFunc get_weather, UBaseType_t prio, uint32_t stack) {
  auto *pack = new std::pair<UiCallbacks, WeatherFunc>(ui, get_weather);
  xTaskCreate(weather_task, "weather_task", stack, pack, prio, nullptr);
}

#include "temperature_source.hpp"
#include "settings.hpp"
#include "shared_state.hpp"
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
}
#include <cmath>

void temperature_source_set_api_value(float c) {
  if (g_data_mtx && xSemaphoreTake(g_data_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
    g_api_tempC = c;
    xSemaphoreGive(g_data_mtx);
  }
}

float temperature_source_get_c() {
  AppSettings s = settings_get();
  if (s.sourceMode == TempSourceMode::Manual) {
    return s.manualTempC;
  } else {
    float v = NAN;
    if (g_data_mtx && xSemaphoreTake(g_data_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
      v = g_api_tempC;
      xSemaphoreGive(g_data_mtx);
    }
    return v;
  }
}

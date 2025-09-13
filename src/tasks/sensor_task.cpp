#include "tasks/sensor_task.hpp"
#include "shared_state.hpp"

extern "C" {
#include "esp_log.h"
}

static const char *TAG_ADC_TASK = "SENSOR_TASK";

static void sensor_task(void *arg) {
  TMP36Sensor *sensor = static_cast<TMP36Sensor *>(arg);

  for (;;) {
    float tC;
    if (sensor->readCelsius(tC)) {
      ESP_LOGI(TAG_ADC_TASK, "TMP36: %.2f C", tC);
      if (g_data_mtx && xSemaphoreTake(g_data_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_local_tempC = tC;
        xSemaphoreGive(g_data_mtx);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz
  }
}

void start_sensor_task(TMP36Sensor *sensor, UBaseType_t prio, uint32_t stack) {
  xTaskCreate(sensor_task, "sensor_task", stack, sensor, prio, nullptr);
}

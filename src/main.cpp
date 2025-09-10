extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

extern "C" void app_main(void) {
  ESP_LOGI("MAIN", "Hello from C++");
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
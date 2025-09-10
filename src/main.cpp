#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_adc.hpp"
#include "ssr.hpp"

static const char *TAG = "MAIN";

// konfiguracja progów histerezy
constexpr float T_ON_C = 15.0f;
constexpr float T_OFF_C = 20.0f;

extern "C" void app_main(void) {
  SensorADC sensor(ADC1_CHANNEL_6, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12);
  SSR ssr(GPIO_NUM_18);

  bool heat_on = false;

  while (true) {
    float tempC = sensor.readTemperatureC();

    if (tempC <= T_ON_C && !heat_on) {
      heat_on = true;
      ESP_LOGI(TAG, "Histereza: T<=%.1f -> GRZANIE ON", T_ON_C);
    } else if (tempC >= T_OFF_C && heat_on) {
      heat_on = false;
      ESP_LOGI(TAG, "Histereza: T>=%.1f -> GRZANIE OFF", T_OFF_C);
    }

    ssr.set(heat_on);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
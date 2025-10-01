#include "settings.hpp"
extern "C" {
#include "nvs.h"
#include "nvs_flash.h"
}
#include <cstring>

static const char *NS = "app";
static AppSettings g_settings;

const char *settings_mode_to_cstr(TempSourceMode m) { return (m == TempSourceMode::Api) ? "api" : "manual"; }

TempSourceMode settings_mode_from_cstr(const char *s) {
  if (!s) return TempSourceMode::Api;
  if (strcasecmp(s, "manual") == 0) return TempSourceMode::Manual;
  return TempSourceMode::Api;
}

bool settings_init() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) return false;

  nvs_handle_t h;
  if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
    uint8_t m = 0;
    if (nvs_get_u8(h, "src_mode", &m) == ESP_OK)
      g_settings.sourceMode = (m == 1) ? TempSourceMode::Manual : TempSourceMode::Api;

    size_t len = sizeof(float);
    float tmp = 21.0f;
    if (nvs_get_blob(h, "manual_temp", &tmp, &len) == ESP_OK && len == sizeof(float)) g_settings.manualTempC = tmp;

    nvs_close(h);
  }
  return true;
}

AppSettings settings_get() { return g_settings; }

bool settings_set(const AppSettings &s) {
  g_settings = s;
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;
  uint8_t m = (s.sourceMode == TempSourceMode::Manual) ? 1 : 0;
  esp_err_t e1 = nvs_set_u8(h, "src_mode", m);
  esp_err_t e2 = nvs_set_blob(h, "manual_temp", &s.manualTempC, sizeof(float));
  esp_err_t e3 = nvs_commit(h);
  nvs_close(h);
  return (e1 == ESP_OK && e2 == ESP_OK && e3 == ESP_OK);
}

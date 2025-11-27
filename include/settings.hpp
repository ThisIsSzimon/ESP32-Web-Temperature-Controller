#pragma once
#include <cstdint>

enum class TempSourceMode : uint8_t { Api = 0, Manual = 1 };

struct AppSettings {
  TempSourceMode sourceMode = TempSourceMode::Api;
  float manualTempC = 21.0f;
};

bool settings_init();
AppSettings settings_get();
bool settings_set(const AppSettings &s);
const char *settings_mode_to_cstr(TempSourceMode m);
TempSourceMode settings_mode_from_cstr(const char *s);

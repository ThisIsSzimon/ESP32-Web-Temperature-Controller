#pragma once
#include <cstdint>

enum class TempSourceMode : uint8_t { Api = 0, Manual = 1 };

struct AppSettings {
  TempSourceMode sourceMode = TempSourceMode::Api;
  float manualTempC = 21.0f;
};

bool settings_init();                    // inicjalizacja NVS + odczyt
AppSettings settings_get();              // kopia w RAM
bool settings_set(const AppSettings &s); // zapis do NVS
const char *settings_mode_to_cstr(TempSourceMode m);
TempSourceMode settings_mode_from_cstr(const char *s);

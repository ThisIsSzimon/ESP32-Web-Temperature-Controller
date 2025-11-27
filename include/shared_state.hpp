#pragma once

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
}

extern float g_local_tempC;          // ostatni odczyt z TMP36
extern float g_api_tempC;            // ostatni odczyt z API pogody
extern SemaphoreHandle_t g_data_mtx;
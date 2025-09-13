#pragma once
#include "sensor_adc.hpp"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// Startuje task 1 Hz: odczyt TMP36, log na Serial, zapis do g_local_tempC
void start_sensor_task(TMP36Sensor *sensor, UBaseType_t prio = 5, uint32_t stack = 4096);

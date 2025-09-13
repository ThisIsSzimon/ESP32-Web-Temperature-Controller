#include "shared_state.hpp"
#include <cmath>

float g_local_tempC = NAN;
SemaphoreHandle_t g_data_mtx = nullptr;

#pragma once

// Zwraca temperaturę do sterowania (wg ustawień: API lub manual)
float temperature_source_get_c();

// Ustawiana przez task pogody po udanym odczycie
void temperature_source_set_api_value(float c);

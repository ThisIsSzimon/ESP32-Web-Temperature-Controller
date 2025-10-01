# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS "main.cpp" "sensor_adc.cpp" "tasks/sensor_task.cpp" "tasks/weather_task.cpp" "shared_state.cpp" "ssr.cpp" "settings.cpp" "temperature_source.cpp" "web/http_server.cpp" "web/index_html.cpp" ${app_sources}
    INCLUDE_DIRS "." "../include" "../include/tasks"
    REQUIRES esp_http_server esp_event esp_wifi esp_netif nvs_flash driver freertos
)

#include "ds18b20.hpp"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DS18B20";

DS18B20::DS18B20(gpio_num_t pin) : pin_(pin) {
  gpio_config_t io{};
  io.mode = GPIO_MODE_INPUT;
  io.pin_bit_mask = (1ULL << pin_);
  io.pull_up_en = GPIO_PULLUP_DISABLE; // off bo zew rezystor jest przy czujniku
  io.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io);
}

static inline void line_low(gpio_num_t pin) {
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
  gpio_set_level(pin, 0);
}

static inline void line_release(gpio_num_t pin) {
  gpio_set_direction(pin, GPIO_MODE_INPUT);
}

static inline int line_read(gpio_num_t pin) {
  return gpio_get_level(pin);
}

bool DS18B20::resetPulse() {
  line_low(pin_);
  esp_rom_delay_us(500);
  line_release(pin_);
  esp_rom_delay_us(70);
  bool presence = (line_read(pin_) == 0);
  esp_rom_delay_us(430);
  return presence;
}

void DS18B20::writeBit(int bit) {
  if (bit) {
    line_low(pin_);
    esp_rom_delay_us(6);
    line_release(pin_);
    esp_rom_delay_us(64);
  } else {
    line_low(pin_);
    esp_rom_delay_us(60);
    line_release(pin_);
    esp_rom_delay_us(10);
  }
}

int DS18B20::readBit() {
  int b;
  line_low(pin_);
  esp_rom_delay_us(6);
  line_release(pin_);
  esp_rom_delay_us(9);
  b = line_read(pin_);
  esp_rom_delay_us(55);
  return b;
}

// Główna metoda do odczytu temperatury
bool DS18B20::readTemperature(float &tempC) {
  if (!resetPulse()) {
    ESP_LOGW(TAG, "Brak presence");
    return false;
  }

  writeByte(0xCC); // SKIP ROM
  writeByte(0x44); // CONVERT T

  vTaskDelay(pdMS_TO_TICKS(750)); // czas konwersji (12-bit)

  if (!resetPulse())
    return false;
  writeByte(0xCC); // SKIP ROM
  writeByte(0xBE); // READ SCRATCHPAD

  uint8_t data[9];
  for (int i = 0; i < 9; i++) {
    data[i] = readByte();
  }

  int16_t raw = (data[1] << 8) | data[0];
  tempC = raw / 16.0f;
  return true;
}
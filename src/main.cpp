#include "http_server.hpp"
#include "sensor_adc.hpp"
#include "settings.hpp"
#include "shared_state.hpp"
#include "ssr.hpp"
#include "tasks/sensor_task.hpp"
#include "tasks/weather_task.hpp"
#include "temperature_source.hpp"
#include "wifi_credentials.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "cJSON.h"
#include "driver/i2c.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
}

// KONFIG
static const char *TAG = "SSD1306_WEATHER";

// Open-Meteo: Ateny (aktualna pogoda)
static const char *WEATHER_URL =
    "https://api.open-meteo.com/v1/forecast?latitude=37.98&longitude=23.72&current_weather=true";

// I2C / SSD1306 128x64
constexpr int I2C_PORT = I2C_NUM_0;
constexpr gpio_num_t SDA_PIN = GPIO_NUM_21;
constexpr gpio_num_t SCL_PIN = GPIO_NUM_22;
constexpr uint32_t I2C_FREQ = 400000;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr int OLED_W = 128;
constexpr int OLED_H = 64;

static const uint8_t FONT5x7[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x3E, 0x41, 0x5D, 0x59, 0x4E}, // '@'
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x40, 0x3D, 0x00}, // 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x14, 0x7C}, // 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x10, 0x08, 0x08, 0x10, 0x08}, // '~'
};

// SSD1306
static void i2c_init() {
  i2c_config_t c{};
  c.mode = I2C_MODE_MASTER;
  c.sda_io_num = SDA_PIN;
  c.scl_io_num = SCL_PIN;
  c.sda_pullup_en = GPIO_PULLUP_ENABLE;
  c.scl_pullup_en = GPIO_PULLUP_ENABLE;
  c.master.clk_speed = I2C_FREQ;
  ESP_ERROR_CHECK(i2c_param_config((i2c_port_t)I2C_PORT, &c));
  ESP_ERROR_CHECK(i2c_driver_install((i2c_port_t)I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));
}

static esp_err_t oled_cmd(uint8_t cmd) {
  i2c_cmd_handle_t h = i2c_cmd_link_create();
  i2c_master_start(h);
  i2c_master_write_byte(h, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(h, 0x00, true); // command
  i2c_master_write_byte(h, cmd, true);
  i2c_master_stop(h);
  esp_err_t r = i2c_master_cmd_begin((i2c_port_t)I2C_PORT, h, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(h);
  return r;
}

static esp_err_t oled_data(const uint8_t *d, size_t n) {
  i2c_cmd_handle_t h = i2c_cmd_link_create();
  i2c_master_start(h);
  i2c_master_write_byte(h, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(h, 0x40, true); // data
  i2c_master_write(h, (uint8_t *)d, n, true);
  i2c_master_stop(h);
  esp_err_t r = i2c_master_cmd_begin((i2c_port_t)I2C_PORT, h, pdMS_TO_TICKS(200));
  i2c_cmd_link_delete(h);
  return r;
}

static void ssd1306_init_128x64() {
  oled_cmd(0xAE);
  oled_cmd(0xD5);
  oled_cmd(0x80);
  oled_cmd(0xA8);
  oled_cmd(0x3F);
  oled_cmd(0xD3);
  oled_cmd(0x00);
  oled_cmd(0x40);
  oled_cmd(0x8D);
  oled_cmd(0x14);
  oled_cmd(0x20);
  oled_cmd(0x00);
  oled_cmd(0xA1);
  oled_cmd(0xC8);
  oled_cmd(0xDA);
  oled_cmd(0x12);
  oled_cmd(0x81);
  oled_cmd(0xCF);
  oled_cmd(0xD9);
  oled_cmd(0xF1);
  oled_cmd(0xDB);
  oled_cmd(0x40);
  oled_cmd(0xA4);
  oled_cmd(0xA6);
  oled_cmd(0x2E);
  oled_cmd(0xAF);
}

static void oled_set_window(uint8_t x0, uint8_t x1, uint8_t page0, uint8_t page1) {
  oled_cmd(0x21);
  oled_cmd(x0);
  oled_cmd(x1);
  oled_cmd(0x22);
  oled_cmd(page0);
  oled_cmd(page1);
}

static void oled_clear() {
  oled_set_window(0, OLED_W - 1, 0, (OLED_H / 8) - 1);
  uint8_t z[16];
  memset(z, 0x00, sizeof z);
  for (int i = 0; i < (OLED_W * OLED_H / 8) / 16; ++i)
    oled_data(z, sizeof z);
}

static void draw_char_at(uint8_t x, uint8_t page, char c) {
  if (c < 32 || c > 127) c = '?';
  oled_set_window(x, x + 5, page, page);
  uint8_t buf[6];
  memcpy(buf, FONT5x7[c - 32], 5);
  buf[5] = 0x00;
  oled_data(buf, sizeof buf);
}

static void draw_text(uint8_t col, uint8_t page, const char *s) {
  uint8_t x = col;
  while (*s && x + 6 <= OLED_W) {
    draw_char_at(x, page, *s++);
    x += 6;
  }
}

// SPIFFS
static const char *TAG_FS = "FS";

static void init_spiffs() {
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs", .partition_label = "webui", .max_files = 8, .format_if_mount_failed = true};
  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_FS, "SPIFFS mount failed (%s)", esp_err_to_name(ret));
  } else {
    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);
    ESP_LOGI(TAG_FS, "SPIFFS mounted. Total=%u, Used=%u", (unsigned)total, (unsigned)used);
  }
}

// Wi-Fi
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_handler(void *, esp_event_base_t base, int32_t id, void *) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    esp_wifi_connect();
  }
}

static void wifi_init_connect() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  wifi_config_t wc{};
  strncpy((char *)wc.sta.ssid, WIFI_SSID, sizeof(wc.sta.ssid));
  strncpy((char *)wc.sta.password, WIFI_PASS, sizeof(wc.sta.password));
  wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
  ESP_ERROR_CHECK(esp_wifi_start());
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Wi-Fi connected");
}

// HTTP: Open-Meteo
static bool http_get_temp_c(float &out_temp) {
  std::string body;

  esp_http_client_config_t cfg{};
  cfg.url = WEATHER_URL;
  cfg.timeout_ms = 10000;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  if (!client) return false;

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "esp_http_client_open err=%d", err);
    esp_http_client_cleanup(client);
    return false;
  }

  (void)esp_http_client_fetch_headers(client);
  char buf[512];
  while (true) {
    int r = esp_http_client_read(client, buf, sizeof(buf));
    if (r <= 0) break;
    body.append(buf, r);
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  cJSON *root = cJSON_Parse(body.c_str());
  if (!root) return false;
  cJSON *cw = cJSON_GetObjectItemCaseSensitive(root, "current_weather");
  cJSON *t = cw ? cJSON_GetObjectItemCaseSensitive(cw, "temperature") : nullptr;

  bool ok = false;
  if (cJSON_IsNumber(t)) {
    out_temp = (float)t->valuedouble;
    ok = true;
  }
  cJSON_Delete(root);
  return ok;
}

// MAIN
extern "C" void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  settings_init();
  wifi_init_connect();

  i2c_init();
  ssd1306_init_128x64();

  oled_clear();
  draw_text(0, 0, "Pogoda: Ateny");
  draw_text(0, 2, "Laczenie...");

  g_data_mtx = xSemaphoreCreateMutex();
  relay_init();

  static TMP36Sensor sensor(ADC1_CHANNEL_6, 16);
  sensor.calibrateKnownVout(0.71f);

  oled_clear();
  draw_text(0, 0, "Pogoda: Ateny");

  // Start 1 Hz task (TMP36 + log + zapis do globalnej)
  start_sensor_task(&sensor, /*prio=*/5, /*stack=*/4096);

  UiCallbacks ui{
      .oled_clear = []() { oled_clear(); },
      .draw_text = [](uint8_t c, uint8_t p, const char *s) { draw_text(c, p, s); },
  };

  // Start 60 s task (API + OLED)
  start_weather_task(ui, /*get_weather:*/ &http_get_temp_c, /*prio=*/4, /*stack=*/6144);

  init_spiffs();

  http_server_start();
}
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "esp_http_server.h"

#define WIFI_SSID   "Szymon Komputer"
#define WIFI_PASS   "70wf-lrta-dfna"
#define LED_GPIO    GPIO_NUM_16

static const char *TAG = "LED_HTTP_STA";
static EventGroupHandle_t s_wifi_event_group;
static const int GOT_IP_BIT = BIT0;

static bool led_on = false;
static void led_apply(void) { gpio_set_level(LED_GPIO, led_on ? 1 : 0); }

/* --------- Prosta strona HTML --------- */
static const char INDEX_HTML[] =
"<!doctype html><html><head><meta charset='utf-8'/>"
"<meta name='viewport' content='width=device-width,initial-scale=1'/>"
"<title>ESP32 LED</title>"
"<style>body{font-family:system-ui;margin:2rem}button{font-size:1.2rem;padding:.6rem 1rem}</style>"
"</head><body>"
"<h1>ESP32 LED (GPIO16)</h1>"
"<p>Status: <b id='st'>...</b></p>"
"<p><button onclick='setLed(true)'>Włącz</button> "
"<button onclick='setLed(false)'>Wyłącz</button></p>"
"<script>"
"async function refresh(){let r=await fetch('/status'); if(r.ok){let t=await r.text(); document.getElementById('st').textContent=t;}}"
"async function setLed(on){await fetch('/led?state='+(on?'on':'off')); refresh();}"
"refresh();"
"</script>"
"</body></html>";

/* ---------- HTTP handlers ---------- */
static esp_err_t index_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, led_on ? "on" : "off");
}

static esp_err_t led_get_handler(httpd_req_t *req){
    char buf[32]; size_t qlen = httpd_req_get_url_query_len(req) + 1;
    if (qlen > sizeof(buf)) qlen = sizeof(buf);
    if (qlen > 1 && httpd_req_get_url_query_str(req, buf, qlen) == ESP_OK){
        char state[8];
        if (httpd_query_key_value(buf, "state", state, sizeof(state)) == ESP_OK){
            if (strcasecmp(state, "on")==0)  { led_on = true;  led_apply(); }
            if (strcasecmp(state, "off")==0) { led_on = false; led_apply(); }
        }
    }
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "OK");
}

static httpd_handle_t http_start(void){
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    httpd_handle_t srv = NULL;
    if (httpd_start(&srv, &cfg) == ESP_OK){
        static const httpd_uri_t uri_index  = { .uri="/",      .method=HTTP_GET, .handler=index_get_handler };
        static const httpd_uri_t uri_status = { .uri="/status",.method=HTTP_GET, .handler=status_get_handler };
        static const httpd_uri_t uri_led    = { .uri="/led",   .method=HTTP_GET, .handler=led_get_handler };
        httpd_register_uri_handler(srv, &uri_index);
        httpd_register_uri_handler(srv, &uri_status);
        httpd_register_uri_handler(srv, &uri_led);
        ESP_LOGI(TAG, "HTTP server started");
    } else {
        ESP_LOGE(TAG, "HTTP start failed");
    }
    return srv;
}


/* ---------- Wi-Fi STA ---------- */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* e = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, GOT_IP_BIT);
    }
}

static void wifi_init_sta(void){
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cfg = { 0 };
    strncpy((char*)wifi_cfg.sta.ssid, WIFI_SSID, sizeof(wifi_cfg.sta.ssid)-1);
    strncpy((char*)wifi_cfg.sta.password, WIFI_PASS, sizeof(wifi_cfg.sta.password)-1);
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void){
    // LED GPIO
    gpio_config_t io = { .pin_bit_mask = 1ULL<<LED_GPIO, .mode = GPIO_MODE_OUTPUT,
                         .pull_up_en = 0, .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE };
    gpio_config(&io);
    led_on = false; led_apply();

    // NVS (wymagane przez Wi-Fi)
    ESP_ERROR_CHECK(nvs_flash_init());

    // Wi-Fi STA
    wifi_init_sta();
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s ...", WIFI_SSID);

    // Czekaj na IP
    xEventGroupWaitBits(s_wifi_event_group, GOT_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    // mDNS (opcjonalnie)

    // HTTP
    http_start();

    ESP_LOGI(TAG, "Open http://esp32-led.local/  lub użyj adresu IP z logów.");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}

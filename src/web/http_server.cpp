#include "settings.hpp"
#include "temperature_source.hpp"
#include "web/index_html.hpp"
#include <cmath>

extern "C" {
#include "esp_http_server.h"
}
#include <cstdlib>
#include <cstring>

static esp_err_t handle_root(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, INDEX_HTML, INDEX_HTML_LEN);
}

static esp_err_t handle_get_settings(httpd_req_t *req) {
  AppSettings s = settings_get();
  char json[128];
  snprintf(json,
           sizeof(json),
           "{\"sourceMode\":\"%s\",\"manualTemp\":%.2f}",
           settings_mode_to_cstr(s.sourceMode),
           s.manualTempC);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, json);
}

static esp_err_t handle_post_settings(httpd_req_t *req) {
  char buf[256];
  int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (len <= 0) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
  buf[len] = 0;

  AppSettings s = settings_get();
  if (strstr(buf, "\"sourceMode\":\"manual\"")) s.sourceMode = TempSourceMode::Manual;
  else if (strstr(buf, "\"sourceMode\":\"api\"")) s.sourceMode = TempSourceMode::Api;

  const char *p = strstr(buf, "\"manualTemp\":");
  if (p) s.manualTempC = strtof(p + 13, nullptr);

  bool ok = settings_set(s);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, ok ? "{\"status\":\"ok\"}" : "{\"status\":\"error\"}");
}

static esp_err_t handle_get_current(httpd_req_t *req) {
  float t = temperature_source_get_c();
  AppSettings s = settings_get();
  char json[128];
  snprintf(json,
           sizeof(json),
           "{\"temp\":%.2f,\"source\":\"%s\"}",
           std::isnan(t) ? 0.f : t,
           settings_mode_to_cstr(s.sourceMode));
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, json);
}

static httpd_handle_t s_server = nullptr;

bool http_server_start() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 80;
  if (httpd_start(&s_server, &cfg) != ESP_OK) return false;

  httpd_uri_t root{.uri = "/", .method = HTTP_GET, .handler = handle_root, .user_ctx = nullptr};
  httpd_uri_t gs{.uri = "/api/settings", .method = HTTP_GET, .handler = handle_get_settings, .user_ctx = nullptr};
  httpd_uri_t ps{.uri = "/api/settings", .method = HTTP_POST, .handler = handle_post_settings, .user_ctx = nullptr};
  httpd_uri_t ct{.uri = "/api/current-temp", .method = HTTP_GET, .handler = handle_get_current, .user_ctx = nullptr};

  httpd_register_uri_handler(s_server, &root);
  httpd_register_uri_handler(s_server, &gs);
  httpd_register_uri_handler(s_server, &ps);
  httpd_register_uri_handler(s_server, &ct);
  return true;
}

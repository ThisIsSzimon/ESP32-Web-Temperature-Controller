#include "http_server.hpp"
#include "settings.hpp"
#include "temperature_source.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include "esp_http_server.h"
#include "esp_log.h"
}

static const char *TAG = "HTTP_SERVER";
static httpd_handle_t s_server = nullptr;

// MIME types
static const char *mime_from_path(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext) return "text/plain";
  if (strcmp(ext, ".html") == 0) return "text/html";
  if (strcmp(ext, ".js") == 0) return "application/javascript";
  if (strcmp(ext, ".css") == 0) return "text/css";
  if (strcmp(ext, ".json") == 0) return "application/json";
  if (strcmp(ext, ".png") == 0) return "image/png";
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
  if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
  if (strcmp(ext, ".ico") == 0) return "image/x-icon";
  return "application/octet-stream";
}

static esp_err_t serve_static(httpd_req_t *req, const char *uri) {
  std::string path = std::string("/spiffs") + (strcmp(uri, "/") == 0 ? "/index.html" : uri);
  FILE *f = fopen(path.c_str(), "rb");

  if (!f) {
    ESP_LOGW(TAG, "Static not found: %s", path.c_str());
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, mime_from_path(path.c_str()));

  if (strstr(path.c_str(), ".js") || strstr(path.c_str(), ".css") || strstr(path.c_str(), ".png") ||
      strstr(path.c_str(), ".jpg") || strstr(path.c_str(), ".jpeg") || strstr(path.c_str(), ".svg") ||
      strstr(path.c_str(), ".ico")) {
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=604800");
  }

  char buf[1024];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
      fclose(f);
      httpd_resp_sendstr_chunk(req, NULL);
      return ESP_FAIL;
    }
  }
  fclose(f);
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
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

// Rejestracja endpointów API
static void register_api_handlers(httpd_handle_t server) {
  // GET /api/settings
  httpd_uri_t gs = {.uri = "/api/settings", .method = HTTP_GET, .handler = handle_get_settings, .user_ctx = nullptr};
  httpd_register_uri_handler(server, &gs);

  // POST /api/settings
  httpd_uri_t ps = {.uri = "/api/settings", .method = HTTP_POST, .handler = handle_post_settings, .user_ctx = nullptr};
  httpd_register_uri_handler(server, &ps);

  // GET /api/current-temp
  httpd_uri_t ct = {.uri = "/api/current-temp", .method = HTTP_GET, .handler = handle_get_current, .user_ctx = nullptr};
  httpd_register_uri_handler(server, &ct);
}

// Root i statyki
static esp_err_t root_handler(httpd_req_t *req) { return serve_static(req, "/"); }
static esp_err_t static_handler(httpd_req_t *req) { return serve_static(req, req->uri); }

// Start serwera HTTP
bool http_server_start() {
  if (s_server) {
    ESP_LOGW(TAG, "Server already running");
    return true;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.lru_purge_enable = true;

  esp_err_t err = httpd_start(&s_server, &config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
    s_server = nullptr;
    return false;
  }
  ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);

  // API
  register_api_handlers(s_server);

  // Strona główna i pozostałe statyki
  httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_handler, .user_ctx = nullptr};
  httpd_uri_t any = {.uri = "/*", .method = HTTP_GET, .handler = static_handler, .user_ctx = nullptr};

  httpd_register_uri_handler(s_server, &root);
  httpd_register_uri_handler(s_server, &any);

  return true;
}

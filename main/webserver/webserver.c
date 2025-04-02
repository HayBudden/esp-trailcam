#include "webserver.h"
#include "esp_log.h"

#define MAX_HANDLERS 10

static const char *TAG = "webserver";
static httpd_handle_t server = NULL;
static const httpd_uri_t *handlers[MAX_HANDLERS];
static size_t num_handlers = 0;

esp_err_t webserver_add_handler(const httpd_uri_t *uri_handler) {
    if (num_handlers >= MAX_HANDLERS) {
        ESP_LOGE(TAG, "Maximum number of handlers reached");
        return ESP_ERR_NO_MEM;
    }
    handlers[num_handlers++] = uri_handler;
    return ESP_OK;
}

esp_err_t webserver_start(void) {
    if (server != NULL) {
        ESP_LOGW(TAG, "Webserver already started");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = MAX_HANDLERS;

    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start webserver: %s", esp_err_to_name(err));
        return err;
    }

    for (size_t i = 0; i < num_handlers; i++) {
        err = httpd_register_uri_handler(server, handlers[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register handler for %s: %s",
                     handlers[i]->uri, esp_err_to_name(err));
        }
    }
    ESP_LOGI(TAG, "Webserver started");
    return ESP_OK;
}

esp_err_t webserver_stop(void) {
    if (server == NULL) {
        return ESP_OK;
    }

    esp_err_t err = httpd_stop(server);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop webserver: %s", esp_err_to_name(err));
    } else {
        server = NULL;
        ESP_LOGI(TAG, "Webserver stopped");
    }
    return err;
}

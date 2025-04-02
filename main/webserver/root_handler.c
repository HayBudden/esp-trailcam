#include "root_handler.h"
#include "esp_log.h"
#include "webserver/webserver.h"

static const char *TAG = "webserver_root_handler";

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const char *html = "<html><body>"
                       "<h1>Trailcam Web Interface</h1>"
                       "<p><a href=\"/files\">File Browser</a></p>"
                       "<p><a href=\"/config\">Configuration</a></p>"
                       "</body></html>";
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t root_uri = {.uri = "/",
                                     .method = HTTP_GET,
                                     .handler = root_get_handler,
                                     .user_ctx = NULL};

esp_err_t root_handler_init(void) {
    esp_err_t err = webserver_add_handler(&root_uri);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Root handler registered");
    }
    return err;
}

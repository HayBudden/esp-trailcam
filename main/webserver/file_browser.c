#include "file_browser.h"
#include "dirent.h"
#include "esp_log.h"
#include "webserver/webserver.h"
#include <string.h>

static const char *TAG = "webserver_file_browser";
static const char *mount_point = "/sdcard";

static esp_err_t file_list_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr_chunk(req, "<html><body><h1>File Browser</h1><ul>");

    DIR *dir = opendir(mount_point);
    if (!dir) {
        httpd_resp_sendstr_chunk(req, "<li>Error: Could not open SD card</li>");
        httpd_resp_sendstr_chunk(req, "</ul></body></html>");
        httpd_resp_sendstr_chunk(req, NULL);
        ESP_LOGE(TAG, "Failed to open directory %s", mount_point);
        return ESP_OK;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char link[1028];
            snprintf(link, sizeof(link),
                     "<li><a href=\"/files/download?file=%s\">%s</a></li>",
                     entry->d_name, entry->d_name);
            httpd_resp_sendstr_chunk(req, link);
        }
    }
    closedir(dir);

    httpd_resp_sendstr_chunk(req, "</ul></body></html>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t file_download_handler(httpd_req_t *req) {
    char file_param[64];
    if (httpd_query_key_value(req->uri, "file", file_param,
                              sizeof(file_param)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    char file_path[128];
    snprintf(file_path, sizeof(file_path), "%s/%s", mount_point, file_param);

    FILE *f = fopen(file_path, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    char disp[128];
    snprintf(disp, sizeof(disp), "attachment; filename=\"%s\"", file_param);
    httpd_resp_set_hdr(req, "Content-Disposition", disp);

    char buf[1024];
    size_t len;
    while ((len = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, len);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

static const httpd_uri_t list_uri = {.uri = "/files",
                                     .method = HTTP_GET,
                                     .handler = file_list_handler,
                                     .user_ctx = NULL};

static const httpd_uri_t download_uri = {.uri = "/files/download",
                                         .method = HTTP_GET,
                                         .handler = file_download_handler,
                                         .user_ctx = NULL};

esp_err_t file_browser_init(void) {
    esp_err_t err = webserver_add_handler(&list_uri);
    if (err != ESP_OK)
        return err;
    err = webserver_add_handler(&download_uri);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "File browser handlers registered");
    }
    return err;
}

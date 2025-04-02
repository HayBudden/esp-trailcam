#include "config_manager.h"
#include "camera.h"
#include "esp_log.h"
#include "webserver/webserver.h"
#include "wifi.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "webserver_config";

static const char *pixformat_str[] = {
    [PIXFORMAT_RGB565] = "RGB565",       [PIXFORMAT_YUV422] = "YUV422",
    [PIXFORMAT_GRAYSCALE] = "GRAYSCALE", [PIXFORMAT_JPEG] = "JPEG",
    [PIXFORMAT_RGB888] = "RGB888",       [PIXFORMAT_RAW] = "RAW",
    [PIXFORMAT_RGB444] = "RGB444",       [PIXFORMAT_RGB555] = "RGB555"};

static const char *framesize_str[] = {
    [FRAMESIZE_QQVGA] = "QQVGA", [FRAMESIZE_QVGA] = "QVGA",
    [FRAMESIZE_CIF] = "CIF",     [FRAMESIZE_VGA] = "VGA",
    [FRAMESIZE_SVGA] = "SVGA",   [FRAMESIZE_XGA] = "XGA",
    [FRAMESIZE_SXGA] = "SXGA",   [FRAMESIZE_UXGA] = "UXGA"};

static esp_err_t config_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");

    camera_settings_t cam_settings;
    esp_err_t err = camera_load_settings(&cam_settings);
    if (err != ESP_OK)
        cam_settings = (camera_settings_t){.pixel_format = DEFAULT_PIXEL_FORMAT,
                                           .frame_size = DEFAULT_FRAME_SIZE,
                                           .jpeg_quality = DEFAULT_JPEG_QUALITY,
                                           .fb_count = DEFAULT_FB_COUNT};

    wifi_credentials_t wifi_creds = {0};
    wifi_load_credentials(
        &wifi_creds); // Ignore errors, use empty strings if not found

    httpd_resp_sendstr_chunk(req, "<html><body><h1>Configuration</h1>"
                                  "<form action=\"/config\" method=\"post\">"
                                  "<h2>Camera Settings</h2>");

    // Pixel Format
    httpd_resp_sendstr_chunk(
        req, "<label>Pixel Format: <select name=\"pixel_format\">");
    for (int i = 0; i < sizeof(pixformat_str) / sizeof(pixformat_str[0]); i++) {
        if (pixformat_str[i]) {
            char option[128];
            snprintf(option, sizeof(option),
                     "<option value=\"%s\" %s>%s</option>", pixformat_str[i],
                     (i == cam_settings.pixel_format) ? "selected" : "",
                     pixformat_str[i]);
            httpd_resp_sendstr_chunk(req, option);
        }
    }
    httpd_resp_sendstr_chunk(req, "</select></label><br>");

    // Frame Size
    httpd_resp_sendstr_chunk(req,
                             "<label>Frame Size: <select name=\"frame_size\">");
    for (int i = 0; i < sizeof(framesize_str) / sizeof(framesize_str[0]); i++) {
        if (framesize_str[i]) {
            char option[128];
            snprintf(option, sizeof(option),
                     "<option value=\"%s\" %s>%s</option>", framesize_str[i],
                     (i == cam_settings.frame_size) ? "selected" : "",
                     framesize_str[i]);
            httpd_resp_sendstr_chunk(req, option);
        }
    }
    httpd_resp_sendstr_chunk(req, "</select></label><br>");

    // JPEG Quality
    char quality_str[128];
    snprintf(
        quality_str, sizeof(quality_str),
        "<label>JPEG Quality: <input type=\"number\" name=\"jpeg_quality\" "
        "value=\"%d\" min=\"0\" max=\"63\"></label><br>",
        cam_settings.jpeg_quality);
    httpd_resp_sendstr_chunk(req, quality_str);

    // FB Count
    char fb_count_str[128];
    snprintf(fb_count_str, sizeof(fb_count_str),
             "<label>FB Count: <input type=\"number\" name=\"fb_count\" "
             "value=\"%d\" min=\"1\" max=\"2\"></label><br>",
             cam_settings.fb_count);
    httpd_resp_sendstr_chunk(req, fb_count_str);

    // WiFi Credentials
    httpd_resp_sendstr_chunk(req, "<h2>WiFi Credentials</h2>");
    char ssid_str[128];
    snprintf(ssid_str, sizeof(ssid_str),
             "<label>SSID: <input type=\"text\" name=\"ssid\" value=\"%s\" "
             "maxlength=\"31\"></label><br>",
             wifi_creds.ssid);
    httpd_resp_sendstr_chunk(req, ssid_str);

    char pass_str[256];
    snprintf(pass_str, sizeof(pass_str),
             "<label>Password: <input type=\"password\" name=\"password\" "
             "value=\"%s\" maxlength=\"63\"></label><br>",
             wifi_creds.password);
    httpd_resp_sendstr_chunk(req, pass_str);

    httpd_resp_sendstr_chunk(req, "<input type=\"submit\" value=\"Save\">"
                                  "</form></body></html>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t parse_form_field(const char *buf, size_t len, const char *key,
                                  char *value, size_t value_len) {
    char *pos = strstr(buf, key);
    if (!pos)
        return ESP_ERR_NOT_FOUND;

    pos += strlen(key) + 1; // Skip key and "="
    size_t i = 0;
    while (i < value_len - 1 && pos < buf + len && *pos != '&') {
        value[i++] = *pos++;
    }
    value[i] = '\0';
    return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req) {
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    camera_settings_t cam_settings;
    esp_err_t err = camera_load_settings(&cam_settings);
    if (err != ESP_OK)
        cam_settings = (camera_settings_t){.pixel_format = DEFAULT_PIXEL_FORMAT,
                                           .frame_size = DEFAULT_FRAME_SIZE,
                                           .jpeg_quality = DEFAULT_JPEG_QUALITY,
                                           .fb_count = DEFAULT_FB_COUNT};

    wifi_credentials_t wifi_creds = {0};
    wifi_load_credentials(&wifi_creds);

    char value[64];
    if (parse_form_field(buf, ret, "pixel_format", value, sizeof(value)) ==
        ESP_OK) {
        for (int i = 0; i < sizeof(pixformat_str) / sizeof(pixformat_str[0]);
             i++) {
            if (pixformat_str[i] && strcmp(value, pixformat_str[i]) == 0) {
                cam_settings.pixel_format = i;
                break;
            }
        }
    }
    if (parse_form_field(buf, ret, "frame_size", value, sizeof(value)) ==
        ESP_OK) {
        for (int i = 0; i < sizeof(framesize_str) / sizeof(framesize_str[0]);
             i++) {
            if (framesize_str[i] && strcmp(value, framesize_str[i]) == 0) {
                cam_settings.frame_size = i;
                break;
            }
        }
    }
    if (parse_form_field(buf, ret, "jpeg_quality", value, sizeof(value)) ==
        ESP_OK) {
        cam_settings.jpeg_quality = atoi(value);
    }
    if (parse_form_field(buf, ret, "fb_count", value, sizeof(value)) ==
        ESP_OK) {
        cam_settings.fb_count = atoi(value);
    }
    if (parse_form_field(buf, ret, "ssid", wifi_creds.ssid,
                         sizeof(wifi_creds.ssid)) != ESP_OK) {
        wifi_creds.ssid[0] = '\0'; // Keep old value if not provided
    }
    if (parse_form_field(buf, ret, "password", wifi_creds.password,
                         sizeof(wifi_creds.password)) != ESP_OK) {
        wifi_creds.password[0] = '\0'; // Keep old value if not provided
    }

    camera_save_settings(&cam_settings);
    wifi_save_credentials(&wifi_creds);

    free(buf);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/config");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t config_get_uri = {.uri = "/config",
                                           .method = HTTP_GET,
                                           .handler = config_get_handler,
                                           .user_ctx = NULL};

static const httpd_uri_t config_post_uri = {.uri = "/config",
                                            .method = HTTP_POST,
                                            .handler = config_post_handler,
                                            .user_ctx = NULL};

esp_err_t config_manager_init(void) {
    esp_err_t err = webserver_add_handler(&config_get_uri);
    if (err != ESP_OK)
        return err;
    err = webserver_add_handler(&config_post_uri);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuration manager handlers registered");
    }
    return err;
}

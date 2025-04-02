#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"

#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_PASS_LEN 64
#define WIFI_MAX_CONNECT_RETRIES 5

typedef struct {
    char ssid[WIFI_MAX_SSID_LEN];
    char password[WIFI_MAX_PASS_LEN];
} wifi_credentials_t;

esp_err_t wifi_initialize(void);
esp_err_t wifi_connect(void);
void wifi_deinitialize(void);
esp_err_t wifi_save_credentials(const wifi_credentials_t *credentials);
esp_err_t wifi_load_credentials(wifi_credentials_t *credentials);
bool wifi_is_connected(void);

#endif

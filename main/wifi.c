#include "wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_storage.h"
#include "string.h"

static const char *TAG = "wifi";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int retry_count = 0;
static bool is_initialized = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi STA started, connecting...");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            wifi_event_sta_disconnected_t *event =
                (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGI(TAG, "Disconnected from AP, reason: %d", event->reason);

            // Explanations for common disconnection reasons
            switch (event->reason) {
            case 2:
                ESP_LOGW(TAG, "Reason 2: Authentication expired");
                break;
            case 4:
                ESP_LOGW(TAG, "Reason 4: Disassociated due to inactivity");
                break;
            case 15:
                ESP_LOGW(TAG, "Reason 15: 4-way handshake timeout");
                break;
            case 201:
                ESP_LOGW(TAG, "Reason 201: AP rejected connection");
                break;
            case 202:
                ESP_LOGW(TAG,
                         "Reason 202: Too many stations connected to the AP");
                break;
            case 203:
                ESP_LOGW(TAG,
                         "Reason 203: AP is in sleep mode or not available");
                break;
            case 204:
                ESP_LOGW(TAG, "Reason 204: Disconnected due to AP restart");
                break;
            default:
                ESP_LOGW(TAG, "Unknown disconnection reason: %d",
                         event->reason);
                break;
            }

            if (retry_count < WIFI_MAX_CONNECT_RETRIES) {
                ESP_LOGI(TAG, "Reconnecting to AP... Attempt %d/%d",
                         retry_count + 1, WIFI_MAX_CONNECT_RETRIES);
                esp_wifi_connect();
                retry_count++;
            } else {
                ESP_LOGE(TAG, "Failed to connect after %d attempts",
                         WIFI_MAX_CONNECT_RETRIES);
                xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            }
            break;
        }
    }

    else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            retry_count = 0;
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

esp_err_t wifi_initialize(void) {
    if (is_initialized) {
        ESP_LOGW(TAG, "WiFi already initialized");
        return ESP_OK;
    }

    // Initialize event group
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default STA interface
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Set WiFi mode to STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    is_initialized = true;
    ESP_LOGI(TAG, "WiFi initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_connect(void) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_credentials_t credentials = {0};
    esp_err_t err = wifi_load_credentials(&credentials);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load credentials: %s", esp_err_to_name(err));
        return err;
    }

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = {0},
                .password = {0},
                .scan_method = WIFI_FAST_SCAN,
                .failure_retry_cnt = WIFI_MAX_CONNECT_RETRIES,
            },
    };

    strncpy((char *)wifi_config.sta.ssid, credentials.ssid, WIFI_MAX_SSID_LEN);
    strncpy((char *)wifi_config.sta.password, credentials.password,
            WIFI_MAX_PASS_LEN);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait for connection result
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
        pdMS_TO_TICKS(10000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP successfully");
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP");
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "Connection timeout");
        return ESP_ERR_TIMEOUT;
    }
}

void wifi_deinitialize(void) {
    if (!is_initialized) {
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    esp_netif_destroy_default_wifi(
        esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"));

    if (wifi_event_group != NULL) {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = NULL;
    }

    is_initialized = false;
    retry_count = 0;
    ESP_LOGI(TAG, "WiFi deinitialized");
}

esp_err_t wifi_save_credentials(const wifi_credentials_t *credentials) {
    esp_err_t err;

    err = nvs_storage_write_string(NVS_WIFI_NAMESPACE, NVS_KEY_SSID,
                                   credentials->ssid);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_storage_write_string(NVS_WIFI_NAMESPACE, NVS_KEY_PASSWORD,
                                   credentials->password);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "WiFi credentials saved to NVS");
    return ESP_OK;
}

esp_err_t wifi_load_credentials(wifi_credentials_t *credentials) {
    esp_err_t err;
    size_t ssid_len = WIFI_MAX_SSID_LEN;
    size_t pass_len = WIFI_MAX_PASS_LEN;
    bool found = false;

    err = nvs_storage_read_string(NVS_WIFI_NAMESPACE, NVS_KEY_SSID,
                                  credentials->ssid, &ssid_len);
    if (err == ESP_OK) {
        found = true;
    } else if (err != ESP_ERR_NOT_FOUND) {
        return err;
    }

    err = nvs_storage_read_string(NVS_WIFI_NAMESPACE, NVS_KEY_PASSWORD,
                                  credentials->password, &pass_len);
    if (err == ESP_OK) {
        found = true;
    } else if (err != ESP_ERR_NOT_FOUND) {
        return err;
    }

    if (!found) {
        ESP_LOGW(TAG, "No WiFi credentials found in NVS");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "WiFi credentials loaded from NVS");
    return ESP_OK;
}

bool wifi_is_connected(void) {
    if (!is_initialized) {
        return false;
    }

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    return (mode == WIFI_MODE_STA &&
            (xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT));
}

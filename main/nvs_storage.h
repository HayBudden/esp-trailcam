#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include <stdint.h>

#define NVS_CAMERA_NAMESPACE "camera"
#define NVS_WIFI_NAMESPACE "wifi"

#define NVS_KEY_PIXEL_FORMAT "pixel_format"
#define NVS_KEY_FRAME_SIZE "frame_size"
#define NVS_KEY_JPEG_QUALITY "jpeg_quality"
#define NVS_KEY_FB_COUNT "fb_count"

#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASSWORD "password"

esp_err_t nvs_storage_init(void);
esp_err_t nvs_storage_write_string(const char *namespace, const char *key,
                                   const char *value);
esp_err_t nvs_storage_read_string(const char *namespace, const char *key,
                                  char *value, size_t *length);
esp_err_t nvs_storage_write_u32(const char *namespace, const char *key,
                                uint32_t value);
esp_err_t nvs_storage_read_u32(const char *namespace, const char *key,
                               uint32_t *value);
esp_err_t nvs_storage_erase_key(const char *namespace, const char *key);
esp_err_t nvs_storage_erase_all(const char *namespace);

#endif

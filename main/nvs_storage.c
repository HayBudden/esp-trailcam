#include "nvs_storage.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <inttypes.h>

static const char *TAG = "nvs_storage";
static bool is_initialized = false;

esp_err_t nvs_storage_init(void) {
    if (is_initialized) {
        ESP_LOGI(TAG, "NVS already initialized");
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS partition due to error");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        return err;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "NVS initialized");
    return ESP_OK;
}

esp_err_t nvs_storage_write_string(const char *namespace, const char *key,
                                   const char *value) {
    if (!is_initialized)
        return ESP_ERR_INVALID_STATE;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace %s: %s", namespace,
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write string %s in %s: %s", key, namespace,
                 esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Wrote string %s = %s in %s", key, value, namespace);
        }
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_read_string(const char *namespace, const char *key,
                                  char *value, size_t *length) {
    if (!is_initialized)
        return ESP_ERR_INVALID_STATE;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace %s: %s", namespace,
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(handle, key, value, length);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Read string %s = %s from %s", key, value, namespace);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Key %s not found in %s", key, namespace);
    } else {
        ESP_LOGE(TAG, "Failed to read string %s from %s: %s", key, namespace,
                 esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_write_u32(const char *namespace, const char *key,
                                uint32_t value) {
    if (!is_initialized)
        return ESP_ERR_INVALID_STATE;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace %s: %s", namespace,
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u32(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write u32 %s in %s: %s", key, namespace,
                 esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Wrote u32 %s = %" PRIu32 " in %s", key, value,
                     namespace);
        }
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_read_u32(const char *namespace, const char *key,
                               uint32_t *value) {
    if (!is_initialized)
        return ESP_ERR_INVALID_STATE;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace %s: %s", namespace,
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_get_u32(handle, key, value);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Read u32 %s = %" PRIu32 " from %s", key, *value,
                 namespace);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Key %s not found in %s", key, namespace);
    } else {
        ESP_LOGE(TAG, "Failed to read u32 %s from %s: %s", key, namespace,
                 esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_erase_key(const char *namespace, const char *key) {
    if (!is_initialized)
        return ESP_ERR_INVALID_STATE;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace %s: %s", namespace,
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Erased key %s from %s", key, namespace);
        }
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to erase key %s from %s: %s", key, namespace,
                 esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_storage_erase_all(const char *namespace) {
    if (!is_initialized)
        return ESP_ERR_INVALID_STATE;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace %s: %s", namespace,
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Erased all keys in namespace %s", namespace);
        }
    } else {
        ESP_LOGE(TAG, "Failed to erase all in %s: %s", namespace,
                 esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

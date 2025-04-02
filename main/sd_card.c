#include "sd_card.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <dirent.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "sd_card";
static SemaphoreHandle_t sd_mutex = NULL;
static bool is_mounted = false;
static const char *mount_point = "/sdcard";
static sdmmc_card_t *card = NULL;
static uint32_t image_counter = 0;

esp_err_t sd_card_init(const sd_card_config_t *config) {
    esp_err_t err;

    if (sd_mutex == NULL) {
        sd_mutex = xSemaphoreCreateMutex();
        if (sd_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create semaphore");
            return ESP_ERR_NO_MEM;
        }
    }

    if (is_mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return ESP_OK;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = config->clk_gpio;
    slot_config.cmd = config->cmd_gpio;
    slot_config.d0 = config->d0_gpio;
    slot_config.width = 1;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config,
                                  &mount_config, &card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(err));
        return err;
    }

    is_mounted = true;
    ESP_LOGI(TAG, "SD card mounted successfully");
    return ESP_OK;
}

esp_err_t sd_card_scan_last_image_number(uint32_t *last_number) {
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take semaphore");
        return ESP_ERR_TIMEOUT;
    }

    DIR *dir = opendir(mount_point);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory %s", mount_point);
        xSemaphoreGive(sd_mutex);
        return ESP_FAIL;
    }

    uint32_t max_number = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *name = entry->d_name;
            if (strstr(name, ".JPG")) {
                uint32_t num = 0;
                if (sscanf(name, "%" PRIu32 ".JPG", &num) == 1) {
                    if (num > max_number) {
                        max_number = num;
                    }
                }
            }
        }
    }

    closedir(dir);
    *last_number = max_number;
    image_counter = max_number + 1;
    ESP_LOGI(TAG, "Last image number found: %" PRIu32 ", next will be %" PRIu32,
             max_number, image_counter);

    xSemaphoreGive(sd_mutex);
    return ESP_OK;
}

esp_err_t sd_card_save_image(const uint8_t *data, size_t len) {
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take semaphore");
        return ESP_ERR_TIMEOUT;
    }

    char filename[32];
    snprintf(filename, sizeof(filename), "%s/%" PRIu32 ".JPG", mount_point,
             image_counter);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s for writing", filename);
        xSemaphoreGive(sd_mutex);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG,
                 "Failed to write full image to %s (wrote %zu of %zu bytes)",
                 filename, written, len);
        xSemaphoreGive(sd_mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saved image to %s, size: %zu bytes", filename, len);
    image_counter++;

    xSemaphoreGive(sd_mutex);
    return ESP_OK;
}

void sd_card_deinit(void) {
    if (!is_mounted)
        return;

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take semaphore for deinit");
        return;
    }

    esp_vfs_fat_sdcard_unmount(mount_point, card);
    is_mounted = false;
    card = NULL;
    ESP_LOGI(TAG, "SD card unmounted");

    xSemaphoreGive(sd_mutex);
}

#include "camera.h"
#include "esp_log.h"

static const char *TAG = "camera";
static camera_config_t camera_config = {0};
static bool is_initialized = false;

static const camera_settings_t default_settings = {
    .pixel_format = DEFAULT_PIXEL_FORMAT,
    .frame_size = DEFAULT_FRAME_SIZE,
    .jpeg_quality = DEFAULT_JPEG_QUALITY,
    .fb_count = DEFAULT_FB_COUNT};

esp_err_t camera_init(void) {
    if (is_initialized) {
        ESP_LOGW(TAG, "Camera already initialized");
        return ESP_OK;
    }

    ESP_ERROR_CHECK(nvs_storage_init());

    camera_settings_t settings = default_settings;
    esp_err_t err = camera_load_settings(&settings);
    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "No camera settings found in NVS, using defaults");
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load settings from NVS (0x%x), using defaults",
                 err);
    }

    camera_config_t config = {.pin_pwdn = CAM_PWDN_GPIO,
                              .pin_reset = CAM_RESET_GPIO,
                              .pin_xclk = CAM_XCLK_GPIO,
                              .pin_sccb_sda = CAM_SIOD_GPIO,
                              .pin_sccb_scl = CAM_SIOC_GPIO,
                              .pin_d7 = CAM_Y9_GPIO,
                              .pin_d6 = CAM_Y8_GPIO,
                              .pin_d5 = CAM_Y7_GPIO,
                              .pin_d4 = CAM_Y6_GPIO,
                              .pin_d3 = CAM_Y5_GPIO,
                              .pin_d2 = CAM_Y4_GPIO,
                              .pin_d1 = CAM_Y3_GPIO,
                              .pin_d0 = CAM_Y2_GPIO,
                              .pin_vsync = CAM_VSYNC_GPIO,
                              .pin_href = CAM_HREF_GPIO,
                              .pin_pclk = CAM_PCLK_GPIO,
                              .xclk_freq_hz = 20000000,
                              .ledc_timer = LEDC_TIMER_0,
                              .ledc_channel = LEDC_CHANNEL_0,
                              .pixel_format = settings.pixel_format,
                              .frame_size = settings.frame_size,
                              .jpeg_quality = settings.jpeg_quality,
                              .fb_count = settings.fb_count,
                              .fb_location = CAMERA_FB_IN_PSRAM,
                              .grab_mode = CAMERA_GRAB_WHEN_EMPTY};

    camera_config = config;

    err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

esp_err_t camera_capture(camera_fb_t **fb) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "Camera not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    *fb = esp_camera_fb_get();
    if (!*fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Picture taken, size: %zu bytes", (*fb)->len);
    return ESP_OK;
}

void camera_deinit(void) {
    if (is_initialized) {
        esp_camera_deinit();
        is_initialized = false;
        ESP_LOGI(TAG, "Camera deinitialized");
    }
}

esp_err_t camera_save_settings(const camera_settings_t *settings) {
    esp_err_t err;

    err = nvs_storage_write_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_PIXEL_FORMAT,
                                (uint32_t)settings->pixel_format);
    if (err != ESP_OK)
        return err;

    err = nvs_storage_write_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_FRAME_SIZE,
                                (uint32_t)settings->frame_size);
    if (err != ESP_OK)
        return err;

    err = nvs_storage_write_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_JPEG_QUALITY,
                                (uint32_t)settings->jpeg_quality);
    if (err != ESP_OK)
        return err;

    err = nvs_storage_write_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_FB_COUNT,
                                (uint32_t)settings->fb_count);
    if (err != ESP_OK)
        return err;

    ESP_LOGI(TAG, "Camera settings saved to NVS");
    return ESP_OK;
}

esp_err_t camera_load_settings(camera_settings_t *settings) {
    esp_err_t err;
    uint32_t temp;
    bool any_found = false;

    err =
        nvs_storage_read_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_PIXEL_FORMAT, &temp);
    if (err == ESP_OK) {
        settings->pixel_format = (pixformat_t)temp;
        any_found = true;
    }

    err = nvs_storage_read_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_FRAME_SIZE, &temp);
    if (err == ESP_OK) {
        settings->frame_size = (framesize_t)temp;
        any_found = true;
    }

    err =
        nvs_storage_read_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_JPEG_QUALITY, &temp);
    if (err == ESP_OK) {
        settings->jpeg_quality = (int)temp;
        any_found = true;
    }

    err = nvs_storage_read_u32(NVS_CAMERA_NAMESPACE, NVS_KEY_FB_COUNT, &temp);
    if (err == ESP_OK) {
        settings->fb_count = (int)temp;
        any_found = true;
    }

    if (!any_found) {
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

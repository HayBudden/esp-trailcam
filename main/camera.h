#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "nvs_storage.h"

#define CAMERA_NVS_NAMESPACE "camera"

#define DEFAULT_PIXEL_FORMAT PIXFORMAT_JPEG
#define DEFAULT_FRAME_SIZE FRAMESIZE_VGA
#define DEFAULT_JPEG_QUALITY 10
#define DEFAULT_FB_COUNT 1

#define CAM_PWDN_GPIO -1
#define CAM_RESET_GPIO -1
#define CAM_XCLK_GPIO 15
#define CAM_SIOD_GPIO 4
#define CAM_SIOC_GPIO 5
#define CAM_Y9_GPIO 16
#define CAM_Y8_GPIO 17
#define CAM_Y7_GPIO 18
#define CAM_Y6_GPIO 12
#define CAM_Y5_GPIO 10
#define CAM_Y4_GPIO 8
#define CAM_Y3_GPIO 9
#define CAM_Y2_GPIO 11
#define CAM_VSYNC_GPIO 6
#define CAM_HREF_GPIO 7
#define CAM_PCLK_GPIO 13

typedef struct {
    pixformat_t pixel_format;
    framesize_t frame_size;
    int jpeg_quality;
    int fb_count;
} camera_settings_t;

esp_err_t camera_init(void);
esp_err_t camera_capture(camera_fb_t **fb);
void camera_deinit(void);
esp_err_t camera_save_settings(const camera_settings_t *settings);
esp_err_t camera_load_settings(camera_settings_t *settings);

#endif

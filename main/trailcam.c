#include "camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_storage.h"
#include "sd_card.h"
#include "webserver/config_manager.h"
#include "webserver/file_browser.h"
#include "webserver/root_handler.h"
#include "webserver/webserver.h"
#include "wifi.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "APP MAIN START");
    ESP_ERROR_CHECK(nvs_storage_init());

    sd_card_config_t sd_config = {.clk_gpio = SDMMC_CLK_GPIO,
                                  .cmd_gpio = SDMMC_CMD_GPIO,
                                  .d0_gpio = SDMMC_D0_GPIO};
    ESP_ERROR_CHECK(sd_card_init(&sd_config));

    ESP_ERROR_CHECK(wifi_initialize());
    ESP_ERROR_CHECK(wifi_connect());

    // Initialize webserver modules
    ESP_ERROR_CHECK(root_handler_init());
    ESP_ERROR_CHECK(file_browser_init());
    ESP_ERROR_CHECK(config_manager_init());
    ESP_ERROR_CHECK(webserver_start());

    ESP_LOGI(TAG, "Webserver running, keeping WiFi active");
}

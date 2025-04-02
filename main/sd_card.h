#ifndef SD_CARD_H
#define SD_CARD_H

#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include <stdint.h>

#define SDMMC_CLK_GPIO 39
#define SDMMC_CMD_GPIO 38
#define SDMMC_D0_GPIO 40

typedef struct {
    uint32_t clk_gpio;
    uint32_t cmd_gpio;
    uint32_t d0_gpio;
} sd_card_config_t;

esp_err_t sd_card_init(const sd_card_config_t *config);
esp_err_t sd_card_scan_last_image_number(uint32_t *last_number);
esp_err_t sd_card_save_image(const uint8_t *data, size_t len);
void sd_card_deinit(void);

#endif

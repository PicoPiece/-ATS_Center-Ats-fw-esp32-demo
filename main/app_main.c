#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "ats-fw-demo";

/* TFT 160x128 SPI pins - wire hardware to these GPIOs */
#if defined(SPI2_HOST)
#define TFT_SPI_HOST         SPI2_HOST
#else
#define TFT_SPI_HOST         HSPI_HOST   /* IDF 4.x */
#endif
#define TFT_SCK_GPIO         18
#define TFT_MOSI_GPIO        23   /* SDA on display module */
#define TFT_CS_GPIO          4
#define TFT_RST_GPIO         15  /* Res */
#define TFT_DC_GPIO          32  /* Rs (DC/register select) */
#define TFT_SPI_CLOCK_HZ     (10 * 1000 * 1000)

static spi_device_handle_t tft_spi_handle;

static esp_err_t tft_spi_init(void)
{
    /* Control pins: RST, DC */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TFT_RST_GPIO) | (1ULL << TFT_DC_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) return err;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = TFT_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = TFT_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    err = spi_bus_initialize(TFT_SPI_HOST, &bus_cfg, 0);  /* dma_chan 0 for IDF 4.x/5.x */
    if (err != ESP_OK) return err;

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = TFT_SPI_CLOCK_HZ,
        .spics_io_num = TFT_CS_GPIO,
        .queue_size = 7,
    };
    return spi_bus_add_device(TFT_SPI_HOST, &dev_cfg, &tft_spi_handle);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello RKTech");   /* v1.1 - used for test pass/fail */
    ESP_LOGI(TAG, "ATS ESP32 Firmware Demo");
    ESP_LOGI(TAG, "Build successful!");

    esp_err_t err = tft_spi_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "TFT SPI init OK (SCK=%d, SDA/MOSI=%d, CS=%d, Res=%d, Rs=%d)",
                 TFT_SCK_GPIO, TFT_MOSI_GPIO, TFT_CS_GPIO, TFT_RST_GPIO, TFT_DC_GPIO);
    } else {
        ESP_LOGW(TAG, "TFT SPI init failed: %s", esp_err_to_name(err));
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Running...");
    }
}


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ats-fw-demo";

void app_main(void)
{
    ESP_LOGI(TAG, "ATS ESP32 Firmware Demo");
    ESP_LOGI(TAG, "Build successful!");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Running...");
    }
}


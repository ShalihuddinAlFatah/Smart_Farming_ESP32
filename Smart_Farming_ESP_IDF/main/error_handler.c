#include "error_handler.h"

void my_error_handler(const char* error_source)
{
    ESP_LOGI(error_source, "Restarting device...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}
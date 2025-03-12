#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "sensor_interface_task.h"
#include "network_connection.h"
#include "My_MQTT_task.h"


static const char TAG[] = "main";

void network_connected_events(void)
{
	ESP_LOGI(TAG, "Network Connected!!");
    My_MQTT_task_start();
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "Main start...");
    
    sensor_interface_start();
    network_start();

    // Set connected event callback
	network_connection_set_callback(&network_connected_events);

    // xTaskCreate(sleep_task, "sleep_task", 2048, NULL, 2, NULL);
}

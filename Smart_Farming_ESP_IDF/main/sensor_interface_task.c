#include "sensor_interface_task.h"

static const char TAG[] = "sensor_interface";

static bool sensor_interface_started = false;

float get_temperature(void)
{
    return getTemperature();
}

float get_humidity(void)
{
    return getHumidity();
}

void sensor_interface_task(void *pvParameter)
{
    setDHTgpio(GPIO_NUM_25);

    while (1)
    {
        ESP_LOGI(TAG, "=== Reading DHT ===");
        int ret = readDHT();

        errorHandler(ret);

        ESP_LOGI(TAG, "Hum: %.1f Tmp: %.1f", get_humidity(), get_temperature());

        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !!
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void sensor_interface_start(void)
{
    if (sensor_interface_started) {
        ESP_LOGW(TAG, "Sensor interface task already started, skipping.");
        return;
    }
    sensor_interface_started = true;

    xTaskCreatePinnedToCore(&sensor_interface_task, "sensor_interface_task", SENSOR_INTERFACE_TASK_STACK_SIZE, NULL, SENSOR_INTERFACE_TASK_PRIORITY, NULL, SENSOR_INTERFACE_TASK_CORE_ID);
}
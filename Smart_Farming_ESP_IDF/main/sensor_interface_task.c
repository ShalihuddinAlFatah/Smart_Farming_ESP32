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

float get_soil_moisture(void)
{
    return getSoilMoisture();
}

// ADS111x config structure
ads111x_cfg_t my_ads111x_cfg;

/**
 * @brief Configure ADC for soil moisture sensor
 * @return ESP_OK if success, ESP_FAIL otherwise
 * @note Helper function for sensor_interface_task
 */
static esp_err_t ADC_config(void)
{
    // I2C master bus configuration
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = ADC_I2C,
        .scl_io_num = ADC_SCL,
        .sda_io_num = ADC_SDA,
        .glitch_ignore_cnt = 7,
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
        return ESP_FAIL;
    }

    // Reset ADS111x config structure to default
    // ALWAYS CALL THIS TO MAKE SURE THE LIBRARY HAS THE SAME DEFAULT CONFIG AS THE DEVICE
    ads111x_reset_config_reg(&my_ads111x_cfg);

    // Set device address
    err = ads111x_configure_address(ADDRPIN_TO_GND, &my_ads111x_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
        return ESP_FAIL;
    }

    // ADS111x I2C configuration
    i2c_device_config_t i2C_device_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = (uint16_t) my_ads111x_cfg.device_addr,
        .scl_speed_hz = I2C_SPEED_HZ,
    };

    i2c_master_dev_handle_t device_handle;
    err = i2c_master_bus_add_device(bus_handle, &i2C_device_cfg, &device_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
        return ESP_FAIL;
    }

    // Connection test to the ADS111x
    err = i2c_master_probe(bus_handle, (uint16_t) my_ads111x_cfg.device_addr, 100);
    if (err != ESP_OK)
    {    
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
        return ESP_FAIL;
    }

    // ADS111x device configuration
    my_ads111x_cfg.mux_config = ADS111x_MUX_SNGL_AIN0_GND;
    my_ads111x_cfg.gain_amp = ADS111x_FSR_4V096;
    my_ads111x_cfg.data_rate = ADS111x_DR_8SPS;
    my_ads111x_cfg.operating_mode = ADS111x_SINGLE_SHOT;

    // Initialize device 
    err = initialize_ads111x(device_handle, &my_ads111x_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ADC ready");

    return ESP_OK;
}

/**
 * @brief Sensor interface task to run
 * @param pvParameters
 */
void sensor_interface_task(void *pvParameter)
{
    setDHTgpio(DHT_GPIO);

    esp_err_t err = ADC_config();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC configuration error");
        my_error_handler(TAG);
    }

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

    BaseType_t err = xTaskCreatePinnedToCore(&sensor_interface_task, "sensor_interface_task", SENSOR_INTERFACE_TASK_STACK_SIZE, NULL, SENSOR_INTERFACE_TASK_PRIORITY, NULL, SENSOR_INTERFACE_TASK_CORE_ID);
    if (err != pdPASS)
    {
        ESP_LOGE(TAG, "Sensor interface task create fail...");
        my_error_handler(TAG);
    }
}
#include "soil_moisture.h"

static const char TAG[] = "soil_moisture";

float getSoilMoisture(void)
{
    uint16_t adc_raw = 0;
    
    esp_err_t err = ads111x_measure_raw(&my_ads111x_cfg, &adc_raw);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
    
    return (float) adc_raw;
}